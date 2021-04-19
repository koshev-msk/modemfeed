#include <sys/stat.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <inttypes.h>

#include "uci.h"
#include "log.h"

//MODBUS
#include "modbus.h"
#include "crc16.h"

#include "client_list.h"


#define MODE_DISABLED       0
#define MODE_SERVER         1
#define MODE_CLIENT         2

#define PARITY_NONE         0
#define PARITY_EVEN         1
#define PARITY_ODD          2

#define FC_NONE         0
#define FC_XONXOFF      1
#define FC_RTSCTS       2

#define EPOLL_RUN_TIMEOUT       -1
#define MAX_CLIENTS_IN_QUEUE    64

#define MAX_EPOLL_EVENTS        MAX_CLIENTS_IN_QUEUE + 4  // + timer + listen socket + com-port
#define NUM_OF_DEVICES          7  // num of services (e.g. RS232, RS485 means 2)

#define MAX_TCP_BUF_SIZE        1024
#define MAX_SERIAL_BUF_SIZE     4096

#define MAX_DIGITS_IN_DEV_NUM   4 // 2+2 nonprinted symbols
#define MAX_CHARS_IN_UCIPATH    128
#define MAX_INOTIFY_BUF_SIZE    1024

#define TMP_PATH_LENGTH         20
#define DOMAIN_NAME_LENGTH      256

#define IMEI_LENGTH             15

#define FIRST_RECONN_TIMEOUT    10

//MODBUS
#define MAX_PACK_SIZE			300

#define MB_TCP_HEADER_SIZE		6
#define MB_TCP_HEADER_LEN		5

#define MB_ASCII_ON				1

typedef struct 
{
    int mode;
    char deviceName[TMP_PATH_LENGTH];
    int baudRate;
    int byteSize;
    int parity;
    int flowcontrol;
    int stopBits;
    int serverPort;
    int connection_mode;
    int holdConnTime;
    char clientHost[DOMAIN_NAME_LENGTH];
    int clientPort;
    int clientAuth;
    int clientTimeout;
    long long int teleofisID;
    int modbus_gateway;
    int quiet;
    int timeout_pack;
    int size_pack;
} device_config_t;


typedef struct{
	int		state;
	char 	header[MB_TCP_HEADER_SIZE];
	char	packet[MAX_PACK_SIZE];
	int		offset;
	int 	len;
	uint8_t crc;
}t_rtu;

typedef struct{
	char	packet[MAX_PACK_SIZE];
	int		offset;
	int		timer;
	int		state;
	int		sock;
	struct itimerspec newValue;
}t_mb_tcp;

struct fdStructType 
{
    int 		mainSocket;
    int 		serialPort;
    int 		TCPtimer;
    int 		epollFD;
    t_rtu 		state_rtu;
    t_mb_tcp	mb_tcp;
};

void CleanThread(struct fdStructType *threadFD);
void *ServerThreadFunc(void *args);
void *ClientThreadFunc(void *args);
device_config_t GetFullDeviceConfig(int deviceID);
int FormAuthAnswer(char *dataBuffer, long long int teleofisID);
uint16_t Crc16Block(uint8_t* block, uint16_t len);
//Modbus
int SendModbusRTU(struct fdStructType *threadFD,char *pBuf, int len, int ascii_rtu, int quiet);
void SendModbusTCP(struct fdStructType *threadFD,char *pBuf, int len, int quiet);
void SendModbusASCIItoTCP(struct fdStructType *threadFD,char *pBuf, int len, uint8_t server, uint8_t connection_mode, int quiet);
void EndTimeoutModbusTCP(struct fdStructType *threadFD, uint8_t server, int ascii_rtu, uint8_t connection_mode, int quiet);
void ClearModbus(struct fdStructType *threadFD);
void send_all(const void *dataBuffer, size_t numOfReadBytes, int flags);

/*****************************************/
/*************** MAIN FUNC ***************/
/*****************************************/
int main(int argc, char **argv)
{
    pthread_t thread[NUM_OF_DEVICES] = {0};
    pthread_attr_t threadAttr; 
    pthread_attr_init(&threadAttr); 
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED); 

    device_config_t deviceConfig[NUM_OF_DEVICES];
    device_config_t newDeviceConfig;

    int i;
    int result;



    /* Check devices' settings */
    for(i = 0; i < NUM_OF_DEVICES; i++)
    { 
        // READ ALL UCI CONFIGS -> deviceConfig[i]
        deviceConfig[i] = GetFullDeviceConfig(i);

        // ACCODING TO MODE START CLIENT OR SERVER THREAD
        if(deviceConfig[i].mode == MODE_SERVER)
        {
            result = pthread_create(&thread[i], &threadAttr, ServerThreadFunc, (void*) &deviceConfig[i]);
            if(result != 0) 
                LOG("Creating server thread for dev %s false. Error: %d\n", deviceConfig[i].deviceName, result);
            else
                LOG("Server thread is started for dev %s\n", deviceConfig[i].deviceName);
        }
        else if (deviceConfig[i].mode == MODE_CLIENT)
        {
            result = pthread_create(&thread[i], &threadAttr, ClientThreadFunc, (void*) &deviceConfig[i]);
            if(result != 0) 
                LOG("Creating client thread for dev %s false. Error: %d\n", deviceConfig[i].deviceName, result);
            else
                LOG("Client thread is started for dev %s\n", deviceConfig[i].deviceName);
        }
    }

    // inotify for parsing changed in /etc/config/pollmydevice
    int inotifyFD = inotify_init();
    if(inotifyFD < 0)
    {
        LOG("Couldn't initialize inotify");
    }
    int inotifyWatch = inotify_add_watch(inotifyFD, "/overlay/upper/etc/config/pollmydevice", IN_MODIFY); 
    if (inotifyWatch < 0)
    {
        printf("Couldn't add watch to /etc/config/pollmydevice\n");
    }
    char inotifyBuf[1024];

    // epoll for detecting inotify
    struct epoll_event epollConfig;
    struct epoll_event epollEvent[1];
    int epollFD = epoll_create(sizeof(inotifyFD));

    epollConfig.events = EPOLLIN | EPOLLET;
    epollConfig.data.fd = inotifyFD;
    // add our inotify into epoll
    result = epoll_ctl(epollFD, EPOLL_CTL_ADD, inotifyFD, &epollConfig);
    if(result < 0)
    {
        LOG("Error while inotify epoll regisration\n");
    }


    while (1) 
    {
        result = epoll_wait(epollFD, epollEvent, MAX_EPOLL_EVENTS, EPOLL_RUN_TIMEOUT);
        
        read(inotifyFD, &inotifyBuf, MAX_INOTIFY_BUF_SIZE);
        inotifyWatch = inotify_add_watch(inotifyFD, "/overlay/upper/etc/config/pollmydevice", IN_MODIFY); // not working with luci witout it (mystic)

        for(i = 0; i < NUM_OF_DEVICES; i++)
        { 
            // GET SETTINGS
            newDeviceConfig = GetFullDeviceConfig(i);
            // IF SETTINGS WAS CHANGED
            if(memcmp(&newDeviceConfig, deviceConfig+i, sizeof(newDeviceConfig)) != 0)
            {

                // IF ANY THREAD IS WORKING ON THIS DERVICE, STOP IT
                if(thread[i] != 0)
                {  
                    result = pthread_cancel(thread[i]);
                    thread[i] = 0;
                    if(result != 0) 
                    {
                        LOG("Stopping service thread for dev ID %d false. Error: %d\n", i, result);
                    } else
                    {
                        LOG("Service thread is stopped for dev ID %d\n", i);
                    }
                }
                
                // WRITE NEW CONFIG INTO STRUCT deviceConfig[i]
                memcpy(deviceConfig+i, &newDeviceConfig, sizeof(newDeviceConfig));

                // ACCODING TO MODE START CLIENT OR SERVER THREAD
                if(deviceConfig[i].mode == MODE_SERVER)
                {
                    result = pthread_create(&thread[i], &threadAttr, ServerThreadFunc, (void*) &deviceConfig[i]);

                    if(result != 0) 
                        LOG("Creating server thread for dev ID %s false. Error: %d\n", deviceConfig[i].deviceName, result);
                    else
                        LOG("Server thread is started for dev ID %s\n", deviceConfig[i].deviceName);
                }
                else if (deviceConfig[i].mode == MODE_CLIENT)
                {
                    result = pthread_create(&thread[i], &threadAttr, ClientThreadFunc, (void*) &deviceConfig[i]);
                    if(result != 0) 
                        LOG("Creating client thread for dev %s false. Error: %d\n", deviceConfig[i].deviceName, result);
                    else
                        LOG("Client thread is started for dev %s\n", deviceConfig[i].deviceName);
                }
            }
        }
    }
    return 0;
}


// When thread close we must close all it's file descriptors
void CleanThread(struct fdStructType *threadFD)
{
    close(threadFD->mainSocket);
    close(threadFD->serialPort);
    close(threadFD->TCPtimer);
    close(threadFD->mb_tcp.timer);
    close(threadFD->epollFD);
    free(threadFD);
    LOG("Descriptors closed\n");
}
/*****************************************/
/************* SERVER FUNC ***************/
/*****************************************/
void *ServerThreadFunc(void *args)
{ 
    struct fdStructType *threadFD = malloc(sizeof(struct fdStructType));
    
    pthread_cleanup_push(CleanThread, threadFD);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    device_config_t deviceConfig = {0};
    memcpy(&deviceConfig, args, sizeof(device_config_t));

    int dataBufferSize = MAX_SERIAL_BUF_SIZE;
    int listeningServerPort = deviceConfig.serverPort;

    int lastActiveConnSocket = -1, eventSource;
    struct sockaddr_in networkAddr;
    char dataBuffer[dataBufferSize];
    int numOfReadBytes;

    int blockOther = 0;
    int blockSource = -1;

    struct epoll_event epollConfig;
    struct epoll_event epollEventArray[MAX_EPOLL_EVENTS];

    int numOfNewEvents;
    int i, result;
    uint32_t currentEvents = 0;

    threadFD->epollFD = epoll_create(MAX_EPOLL_EVENTS);
    if (threadFD->epollFD < 0)
    {
        LOG("Error while creating epoll on dev %s \n", deviceConfig.deviceName);
        pthread_exit(NULL);
    }


    /////////////////////////
    /**** Listen socket ****/
    /////////////////////////

    threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);     // 0 - default protocol (TCP in this case)
    if(threadFD->mainSocket < 0)
    {
        LOG("Error while creating a listening socket\n");
        pthread_exit(NULL);
    }

    networkAddr.sin_family = AF_INET;
    networkAddr.sin_port = htons(listeningServerPort);    
    networkAddr.sin_addr.s_addr = htonl(INADDR_ANY);    // receive data from any client address
    fcntl(threadFD->mainSocket, F_SETFL, O_NONBLOCK);   // set listening socket as nonblocking

    // sometimes in case of programm restart socket can hang, so we have to do this:
    int yes = 1;
    if (setsockopt(threadFD->mainSocket, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &yes, sizeof(yes)) == -1)
    {
        LOG("Error while setting socket options \n");
        pthread_exit(NULL);
    }

    // bind listening socket with network interface(port,...)
    result = bind(threadFD->mainSocket, (struct sockaddr *)&networkAddr, sizeof(networkAddr));
    if(result < 0)
    {
        LOG("Error while listening socket binding: %d\n", result);
        pthread_exit(NULL);
    }

    epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    epollConfig.data.fd = threadFD->mainSocket;
    // add our listening socket into epoll
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
    if(result < 0)
    {
        LOG("Error while listening socket epoll regisration\n");
        pthread_exit(NULL);
    }


    /////////////////////////
    /***** Serial Port *****/
    /////////////////////////

    struct termios serialPortConfig;

    /// here we must forbid access to serial from other apps

    ///// open serial
    threadFD->serialPort = open(deviceConfig.deviceName, O_RDWR | O_NOCTTY);
    if (threadFD->serialPort < 0)
    {
        LOG("Error %d while opening dev %s \n", threadFD->serialPort, deviceConfig.deviceName);
        pthread_exit(NULL);
    }

    // read config
    if (tcgetattr(threadFD->serialPort, &serialPortConfig) != 0)
    {
        LOG("Error while reading port config %s", deviceConfig.deviceName);
        pthread_exit(NULL);
    }
    bzero(&serialPortConfig, sizeof(serialPortConfig)); // clear struct for new port settings

    // set config
    speed_t newBaudRate;
    switch (deviceConfig.baudRate) 
    {
        case 300:       newBaudRate = B300      ; break;
        case 600:       newBaudRate = B600      ; break;
        case 1200:      newBaudRate = B1200     ; break;
        case 1800:      newBaudRate = B1800     ; break;
        case 2400:      newBaudRate = B2400     ; break;
        case 4800:      newBaudRate = B4800     ; break;
        case 9600:      newBaudRate = B9600     ; break;
        case 19200:     newBaudRate = B19200    ; break;
        case 38400:     newBaudRate = B38400    ; break;
        case 57600:     newBaudRate = B57600    ; break;
        case 115200:    newBaudRate = B115200   ; break;
        case 230400:    newBaudRate = B230400   ; break;
        case 460800:    newBaudRate = B460800   ; break;
        case 921600:    newBaudRate = B921600   ; break;
        default:        newBaudRate = B9600     ; break;
    }
    
    serialPortConfig.c_cflag = newBaudRate | CLOCAL | CREAD;   // Enable the receiver and set local mode, 8n1 ???

    // set parity
    if(deviceConfig.parity == PARITY_EVEN)
    {
        serialPortConfig.c_cflag |= PARENB;
    }
    else if(deviceConfig.parity == PARITY_ODD)
    {
        serialPortConfig.c_cflag |= PARENB;
        serialPortConfig.c_cflag |= PARODD;
    }

    // set flow control
    if(deviceConfig.flowcontrol == FC_RTSCTS)
    {
        serialPortConfig.c_cflag |= CRTSCTS;
    }
    else if(deviceConfig.flowcontrol == FC_XONXOFF)
    {
        serialPortConfig.c_iflag |= (IXON | IXOFF | IXANY);
    }

    if(deviceConfig.stopBits == 2)
        serialPortConfig.c_cflag |= CSTOPB;

    // set bytesize
    if(deviceConfig.byteSize == 8) serialPortConfig.c_cflag |= CS8;
    else if(deviceConfig.byteSize == 7) serialPortConfig.c_cflag |= CS7;
    else if(deviceConfig.byteSize == 6) serialPortConfig.c_cflag |= CS6;
    else if(deviceConfig.byteSize == 5) serialPortConfig.c_cflag |= CS5;
    else serialPortConfig.c_cflag |= CS8;

    //serialPortConfig.c_cc[VMIN]     = 0;                // blocking read until 1 character arrives (0 - no, 1 - yes)
    //serialPortConfig.c_cc[VTIME]    = 0;                // inter-character timer

    unsigned char val = (uint8_t)((deviceConfig.timeout_pack)%256);
    serialPortConfig.c_line &= ~ICANON;					//DISABLE CANONCIAL
    serialPortConfig.c_cc[VTIME]    = val;            	// inter-character timer
    LOG("Set VTIME=%d\n", val);
    val = (uint8_t)(deviceConfig.size_pack%256);
    serialPortConfig.c_cc[VMIN]     = val;              // blocking read until 1 character arrives (0 - no, 1 - yes)
    LOG("Set VMIN=%d\n", val);

    // write config
    tcflush(threadFD->serialPort, TCIFLUSH);                      // clean the line
    tcsetattr(threadFD->serialPort, TCSANOW, &serialPortConfig);  // change config right now (may be TCSADRAIN,TCSAFLUSH)

    // add serial port fd to epoll
    epollConfig.data.fd = threadFD->serialPort;
    epollConfig.events = EPOLLIN | EPOLLET; // message, edge trigger
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->serialPort, &epollConfig);
    if(result < 0)
    {
        LOG("Error while serial port epoll regisration\n");
        pthread_exit(NULL);
    }

    /////////////////////////
    /**** Timer for TCP ****/
    /////////////////////////

    int holdConnTime = deviceConfig.holdConnTime;
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue,sizeof(newValue));  
    bzero(&oldValue,sizeof(oldValue));

    struct timespec ts;
    ts.tv_sec = holdConnTime;
    ts.tv_nsec = 0;
    newValue.it_value = ts;

    threadFD->TCPtimer = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    if(threadFD->TCPtimer < 0)
    {
        LOG("Error while timerfd create \n");
        pthread_exit(NULL);
    }

    // add timer fd to epoll
    epollConfig.events = EPOLLIN | EPOLLET;
    epollConfig.data.fd = threadFD->TCPtimer;
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->TCPtimer, &epollConfig);
    if(result < 0)
    {
        LOG("Epoll timer addition error\n");
        pthread_exit(NULL);
    }
    ////////////////////////////
    /** Timer for TCP MODBUS **/
    ////////////////////////////
    ClearModbus(threadFD);
    int pack_timeout;
    if(deviceConfig.baudRate > 19200){		//calculating timeout between package for MODBUS protocol
    	pack_timeout = 1750;
    }else{
    	pack_timeout = 35000000/deviceConfig.baudRate;
    }
    bzero(&threadFD->mb_tcp.newValue,sizeof(struct itimerspec));
    threadFD->mb_tcp.newValue.it_value.tv_sec = 0;
    threadFD->mb_tcp.newValue.it_value.tv_nsec = pack_timeout*1000*10;

    threadFD->mb_tcp.timer = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    if(threadFD->mb_tcp.timer < 0){
    	LOG("Error while timerfd for TCP MODBUS create \n");
    	pthread_exit(NULL);
    }
    // add modbus tcp timer fd to epoll
    epollConfig.events = EPOLLIN | EPOLLET;
    epollConfig.data.fd = threadFD->mb_tcp.timer;
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mb_tcp.timer, &epollConfig);
    if(result < 0)
    {
    	LOG("Error while timer epoll regisration\n");
    	pthread_exit(NULL);
    }

    // begin waiting for clients
    listen(threadFD->mainSocket, MAX_CLIENTS_IN_QUEUE);
    LOG("Listening on dev %s port %d\n", deviceConfig.deviceName, listeningServerPort);

    while(1)
    {
        numOfNewEvents = epoll_wait(threadFD->epollFD, epollEventArray, MAX_EPOLL_EVENTS, EPOLL_RUN_TIMEOUT);

        for(i = 0; i < numOfNewEvents; i++) 
        {
            eventSource = epollEventArray[i].data.fd;
            currentEvents = epollEventArray[i].events;

            // if we have a new connection
            if(eventSource == threadFD->mainSocket)   
            {
                result = accept(threadFD->mainSocket, NULL, NULL);  // we don't care about a client address
                if(result < 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Error while accepting connection\n");
                }
                else
                    lastActiveConnSocket = result;

                fcntl(lastActiveConnSocket, F_SETFL, O_NONBLOCK);   // set listening socket as nonblocking
                if (!deviceConfig.quiet)
                    LOG("New incoming connection\n");

                epollConfig.data.fd = lastActiveConnSocket;
                epollConfig.events = EPOLLIN | EPOLLET;             // message, edge trigger
                result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, lastActiveConnSocket, &epollConfig);
                if(result < 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Epoll connection addition error %d\n", result);
                } else {
                    if(deviceConfig.connection_mode == 1)
                    {
                        client_insert(lastActiveConnSocket);
                    }
                }
            }

            // data from serial
            else if(eventSource == threadFD->serialPort)
            {
                numOfReadBytes = read(threadFD->serialPort, dataBuffer, dataBufferSize);
                if(deviceConfig.modbus_gateway){
                	if(deviceConfig.modbus_gateway == 2){
                        if (!deviceConfig.quiet)
                		    LOG("Receive modbus ASCII data %d\n", numOfReadBytes);
                		threadFD->mb_tcp.sock = lastActiveConnSocket;
                		SendModbusASCIItoTCP(threadFD, dataBuffer, numOfReadBytes, 1, deviceConfig.connection_mode, deviceConfig.quiet);
                	}else{
                        if (!deviceConfig.quiet)
                		    LOG("Receive modbus RTU data %d\n", numOfReadBytes);
                		SendModbusTCP(threadFD, dataBuffer, numOfReadBytes, deviceConfig.quiet);
                	}
                }else{
                    if(deviceConfig.connection_mode == 0)
                    {
                        send(lastActiveConnSocket, dataBuffer, numOfReadBytes, 0);
                    } else {
                        send_all(dataBuffer, numOfReadBytes, 0);
                    }
                }
            }else
            // timer modbus tcp for forming packets
			if(eventSource == threadFD->mb_tcp.timer){
				threadFD->mb_tcp.sock = lastActiveConnSocket;
				EndTimeoutModbusTCP(threadFD, 1, deviceConfig.modbus_gateway, deviceConfig.connection_mode, deviceConfig.quiet);
			}

            else if(eventSource == threadFD->TCPtimer)
            {
                if(deviceConfig.connection_mode == 0)
                {
                    blockOther = 0; // free access for all clients
                    LOG("Connection hold time is up. Free access for all clients\n");
                }
            }

            // data from TCP on existing connection
            else if(currentEvents & EPOLLIN) 
            {
                numOfReadBytes = recv(eventSource, dataBuffer, dataBufferSize, 0);
                if(numOfReadBytes > 0)
                {
                    // if block source sent data or block is disabled, we handle it
                    if(blockOther == 0 || blockSource == eventSource)
                    {
                        // close access for other untill (last activity timer will signalize) or (our TCP will close)
                        
                        blockSource = eventSource;
                        if(deviceConfig.connection_mode == 0){
                            blockOther = 1;
                            result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                            if(result < 0)
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Error while timer setup \n");
                            }
                        }
                        dataBuffer[numOfReadBytes] = 0;
                        if(!deviceConfig.modbus_gateway){
                        	write(threadFD->serialPort, dataBuffer, numOfReadBytes);
                        	lastActiveConnSocket = eventSource;
                        }else
                        if(!SendModbusRTU(threadFD, dataBuffer, numOfReadBytes, deviceConfig.modbus_gateway, deviceConfig.quiet)){
                        	close(eventSource);
                        	epollConfig.data.fd = eventSource;
                        	epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                        	// if block source wanted to close connection, we free access for all
                        	if(blockSource == eventSource)
                        	{
                        		blockOther = 0; // free access for all clients
                        	}
                            if (!deviceConfig.quiet)
                        	    LOG("Connection closed\n");
                            lastActiveConnSocket = -1;
                        }else  lastActiveConnSocket = eventSource;
                    }
                    else
                        if (!deviceConfig.quiet)
                            LOG("Data refused\n");  
                        //lastActiveConnSocket = -1;
                }
                else // if someone want to close connection
                {
                    close(eventSource);
                    epollConfig.data.fd = eventSource;
                    epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                    if(deviceConfig.connection_mode == 1)
                    {
                        remove_descr(eventSource);
                    }
                    // if block source wanted to close connection, we free access for all
                    if(blockSource == eventSource) 
                    {
                        blockOther = 0; // free access for all clients
                    }
                    if (!deviceConfig.quiet)
                        LOG("Connection closed\n");
                    lastActiveConnSocket = -1;
                }
            }

            else
            {
                if (!deviceConfig.quiet)
                    LOG("Unknown epoll event\n");
                //lastActiveConnSocket = -1;
            }
        }      
    }

    pthread_cleanup_pop(1);
    return (void *) 0;
} 



/*****************************************/
/************* CLIENT FUNC ***************/
/*****************************************/
void *ClientThreadFunc(void *args)
{ 
    struct fdStructType *threadFD = malloc(sizeof(struct fdStructType));
    
    pthread_cleanup_push(CleanThread, threadFD);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    device_config_t deviceConfig = {0};
    memcpy(&deviceConfig, args, sizeof(device_config_t));

    int dataBufferSize = MAX_SERIAL_BUF_SIZE;

    int eventSource;

    char dataBuffer[dataBufferSize];
    const char authRequest[28] = "\xC0\x00\x06\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xE7\x48\xC2";
    const char authAcknow[28] = "\xC0\x00\x06\x01\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3F\x25\xC2";
    int numOfReadBytes;

    int serverAvailable = 0, autorized = 0;
    int stat = 0; 

    struct epoll_event epollConfig;
    struct epoll_event epollEventArray[MAX_EPOLL_EVENTS];

    int numOfNewEvents;
    int i, result;

    /////////////////////////
    /***** Serial Port *****/
    /////////////////////////
    struct termios serialPortConfig;
    /// here we must forbid access to serial from other apps

    // open serial
    threadFD->serialPort = open(deviceConfig.deviceName, O_RDWR | O_NOCTTY);
    if (threadFD->serialPort < 0)
    {
        LOG("Error %d while opening dev %s \n", threadFD->serialPort, deviceConfig.deviceName);
        pthread_exit(NULL);
    }

    // read config
    if (tcgetattr(threadFD->serialPort, &serialPortConfig) != 0)
    {
        LOG("Error while reading port config %s", deviceConfig.deviceName);
        pthread_exit(NULL);
    }
    bzero(&serialPortConfig, sizeof(serialPortConfig)); // clear struct for new port settings

    // set config
    speed_t newBaudRate;
    switch (deviceConfig.baudRate) 
    {
        case 300:       newBaudRate = B300      ; break;
        case 600:       newBaudRate = B600      ; break;
        case 1200:      newBaudRate = B1200     ; break;
        case 1800:      newBaudRate = B1800     ; break;
        case 2400:      newBaudRate = B2400     ; break;
        case 4800:      newBaudRate = B4800     ; break;
        case 9600:      newBaudRate = B9600     ; break;
        case 19200:     newBaudRate = B19200    ; break;
        case 38400:     newBaudRate = B38400    ; break;
        case 57600:     newBaudRate = B57600    ; break;
        case 115200:    newBaudRate = B115200   ; break;
        case 230400:    newBaudRate = B230400   ; break;
        case 460800:    newBaudRate = B460800   ; break;
        case 921600:    newBaudRate = B921600   ; break;
        default:        newBaudRate = B9600     ; break;
    }
    
    serialPortConfig.c_cflag = newBaudRate | CLOCAL | CREAD;   // Enable the receiver and set local mode, 8n1 ???

    // set parity
    if(deviceConfig.parity == PARITY_EVEN)
    {
        serialPortConfig.c_cflag |= PARENB;
    }
    else if(deviceConfig.parity == PARITY_ODD)
    {
        serialPortConfig.c_cflag |= PARENB;
        serialPortConfig.c_cflag |= PARODD;
    }

    // set flow control
    if(deviceConfig.flowcontrol == FC_RTSCTS)
    {
        serialPortConfig.c_cflag |= CRTSCTS;
    }
    else if(deviceConfig.flowcontrol == FC_XONXOFF)
    {
        serialPortConfig.c_iflag |= (IXON | IXOFF | IXANY);
    }

    if(deviceConfig.stopBits == 2)
        serialPortConfig.c_cflag |= CSTOPB;

    // set bytesize
    if(deviceConfig.byteSize == 8) serialPortConfig.c_cflag |= CS8;
    else if(deviceConfig.byteSize == 7) serialPortConfig.c_cflag |= CS7;
    else if(deviceConfig.byteSize == 6) serialPortConfig.c_cflag |= CS6;
    else if(deviceConfig.byteSize == 5) serialPortConfig.c_cflag |= CS5;
    else serialPortConfig.c_cflag |= CS8;

//    serialPortConfig.c_cc[VMIN]     = 0;                // blocking read until 1 character arrives (0 - no, 1 - yes)
//    serialPortConfig.c_cc[VTIME]    = 0;                // inter-character timer

    unsigned char val = (uint8_t)((deviceConfig.timeout_pack)%256);
    serialPortConfig.c_line &= ~ICANON;					//DISABLE CANONCIAL
    serialPortConfig.c_cc[VTIME]    = val;            	// inter-character timer
    val = (uint8_t)(deviceConfig.size_pack%256);
    serialPortConfig.c_cc[VMIN]     = val;              // blocking read until 1 character arrives (0 - no, 1 - yes)

    // write config
    tcflush(threadFD->serialPort, TCIFLUSH);                      // clean the line
    tcsetattr(threadFD->serialPort, TCSANOW, &serialPortConfig);  // change config right now (may be TCSADRAIN,TCSAFLUSH)


    /////////////////////////
    /**** Timer for TCP ****/
    /////////////////////////
    int clientTimeout = deviceConfig.clientTimeout;
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue,sizeof(newValue));  
    bzero(&oldValue,sizeof(oldValue));

    struct timespec ts;
    ts.tv_sec = clientTimeout;
    ts.tv_nsec = 0;
    newValue.it_value = ts;

    threadFD->TCPtimer = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    if(threadFD->TCPtimer < 0)
    {
        LOG("Error while timerfd create \n");
        pthread_exit(NULL);
    }

    ////////////////////////////
    /** Timer for TCP MODBUS **/
    ////////////////////////////
    ClearModbus(threadFD);
    int pack_timeout;
    if(deviceConfig.baudRate > 19200){		//calculating timeout between package for MODBUS protocol
    	pack_timeout = 1750;
    }else{
    	pack_timeout = 35000000/deviceConfig.baudRate;
    }
    bzero(&threadFD->mb_tcp.newValue,sizeof(struct itimerspec));
    threadFD->mb_tcp.newValue.it_value.tv_sec = 0;
    threadFD->mb_tcp.newValue.it_value.tv_nsec = pack_timeout*1000*10;
    LOG("Timer set = %lu \n",threadFD->mb_tcp.newValue.it_value.tv_nsec);

    threadFD->mb_tcp.timer = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    if(threadFD->mb_tcp.timer < 0){
    	LOG("Error while timerfd for TCP MODBUS create \n");
    	pthread_exit(NULL);
    }



    /////////////////////////
    /** Connect To Server **/
    /////////////////////////
    struct sockaddr_in addr;
    struct hostent *server;

    LOG("Trying to connect... \n");
    while(1)
    {
        server = gethostbyname(deviceConfig.clientHost);
        if (server == NULL) 
        {
            if (!deviceConfig.quiet)
                LOG("Error while resolving %s\n", deviceConfig.clientHost);
            sleep(FIRST_RECONN_TIMEOUT);
            continue;
        }

        // server's internet address
        bzero((char *) &addr, sizeof(addr));
        addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);
        addr.sin_port = htons(deviceConfig.clientPort);

        threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
        if(threadFD->mainSocket < 0)
        {
            if (!deviceConfig.quiet)
                LOG("Error while creating client socket \n");
            sleep(FIRST_RECONN_TIMEOUT);
            continue;
        }

        result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));
        if(result == 0)
        {
            if (!deviceConfig.quiet)
                LOG("Client connected successfully \n");
            serverAvailable = 1;
            if(timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue) < 0)
            {
                if (!deviceConfig.quiet)
                    LOG("Error while timer setup \n");
            }
            break;
        }
        else
        {
            if (!deviceConfig.quiet)
                LOG("Couldn't connect. Retry after %d sec \n", FIRST_RECONN_TIMEOUT);
            sleep(FIRST_RECONN_TIMEOUT);
        }
    }


    /////////////////////////
    /** Add All To Epoll  **/
    /////////////////////////
    threadFD->epollFD = epoll_create(MAX_EPOLL_EVENTS);
    if (threadFD->epollFD < 0)
    {
        LOG("Error while creating epoll on dev %s \n", deviceConfig.deviceName);
        pthread_exit(NULL);
    }
    // add timer fd to epoll
    epollConfig.events = EPOLLIN | EPOLLET;
    epollConfig.data.fd = threadFD->TCPtimer;
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->TCPtimer, &epollConfig);
    if(result < 0)
    {
        LOG("Error while timer epoll regisration\n");
        pthread_exit(NULL);
    }
    // add modbus tcp timer fd to epoll
    epollConfig.events = EPOLLIN | EPOLLET;
    epollConfig.data.fd = threadFD->mb_tcp.timer;
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mb_tcp.timer, &epollConfig);
    if(result < 0)
    {
    	LOG("Error while timer epoll regisration\n");
    	pthread_exit(NULL);
    }
    // add serial port fd to epoll
    epollConfig.events = EPOLLIN | EPOLLET; // message, edge trigger
    epollConfig.data.fd = threadFD->serialPort;
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->serialPort, &epollConfig);
    if(result < 0)
    {
        LOG("Error while serial port epoll regisration\n");
        pthread_exit(NULL);
    }
    // add socket fd to epoll
    epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    epollConfig.data.fd = threadFD->mainSocket;
    // add socket into epoll
    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
        if(result < 0)
    {
        LOG("Error while socket epoll regisration\n");
        pthread_exit(NULL);
    }


    while(1)
    {
        numOfNewEvents = epoll_wait(threadFD->epollFD, epollEventArray, MAX_EPOLL_EVENTS, EPOLL_RUN_TIMEOUT);

        for(i = 0; i < numOfNewEvents; i++) 
        {
            eventSource = epollEventArray[i].data.fd;

            // data from TCP connection
            if(eventSource == threadFD->mainSocket)   
            {
                numOfReadBytes = recv(eventSource, dataBuffer, dataBufferSize, 0);
                //LOG("incoming %d bytes\n", numOfReadBytes);
                if(numOfReadBytes > 0)  // in case of normal packet
                {
                    // if we are autorized already OR we don't need autorization
                    if(autorized == 1 || deviceConfig.clientAuth == 0)
                    {
                    	//TODO SETTING MODBUS
                    	if(!deviceConfig.modbus_gateway){
                    		write(threadFD->serialPort, dataBuffer, numOfReadBytes);
                    	}else
                        if(!SendModbusRTU(threadFD, dataBuffer, numOfReadBytes, deviceConfig.modbus_gateway, deviceConfig.quiet)){
                        	// close existing connection
                        	close(eventSource);
                        	// remove from epoll
                        	epollConfig.data.fd = eventSource;
                        	epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                            if (!deviceConfig.quiet)
                        	    LOG("Connection closed\n");

                        	serverAvailable = 0;
                        	autorized = 0;
                        	// re-create socket
                        	threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
                        	if(threadFD->mainSocket < 0)
                        	{
                                if (!deviceConfig.quiet)
                        		    LOG("Error while re-opening client socket \n");
                        	}

                        	// add to epoll again
                        	epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
                        	epollConfig.data.fd = threadFD->mainSocket;
                        	result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
                        	if(result < 0)
                        	{
                                if (!deviceConfig.quiet)
                        		    LOG("Error while socket epoll regisration\n");
                        		pthread_exit(NULL);
                        	}

                        	result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));

                        	// if connection is not successfull, restart timer
                        	if(result != 0)
                        	{
                        		result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                        		if(result < 0)
                        		{
                                    if (!deviceConfig.quiet)
                        			    LOG("Error while timer setup \n");
                        		}
                        	}
                        	else
                        	{
                                if (!deviceConfig.quiet)
                        		    LOG("Client re-connected successfully \n");
                        		serverAvailable = 1;
                        	}
                        }
                        //LOG("TCP -> Serial: %d bytes\n", numOfReadBytes);
                        result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                        if(result < 0)
                        {
                            if (!deviceConfig.quiet)
                                LOG("Error while timer setup \n");
                        }
                    }
                    else
                    {
                        // if it is an autorization request from server
                        if (numOfReadBytes == 28)
                        {
                            if(memcmp(dataBuffer, authRequest, 28) == 0)
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Autorization request from server\n");
                                numOfReadBytes = FormAuthAnswer(dataBuffer, deviceConfig.teleofisID);
                                send(threadFD->mainSocket, dataBuffer, numOfReadBytes, 0); // maybe add some check???
                                dataBuffer[numOfReadBytes] = 0;
                            }
                            else if(memcmp(dataBuffer, authAcknow, 28) == 0)
                            {
                                autorized = 1;
                                if (!deviceConfig.quiet)
                                    LOG("Autorization OK \n");
                            }
                            else // error, reconnect
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Autorization ERROR. Reconnect... \n");

                                // close existing connection
                                close(eventSource);
                                // remove from epoll
                                epollConfig.data.fd = eventSource;
                                epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                                if (!deviceConfig.quiet)
                                    LOG("Connection closed by server\n");

                                serverAvailable = 0;
                                autorized = 0;
                                // re-create socket
                                threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
                                if(threadFD->mainSocket < 0)
                                {
                                    if (!deviceConfig.quiet)
                                        LOG("Error while re-opening client socket \n");
                                }

                                // add to epoll again
                                epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
                                epollConfig.data.fd = threadFD->mainSocket;
                                result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
                                if(result < 0)
                                {
                                    if (!deviceConfig.quiet)
                                        LOG("Error while socket epoll regisration\n");
                                    pthread_exit(NULL);
                                }

                                result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));

                                // if connection is not successfull, restart timer
                                if(result != 0)
                                {
                                    result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                                    if(result < 0)
                                    {
                                        if (!deviceConfig.quiet)
                                            LOG("Error while timer setup \n");
                                    }
                                }
                                else
                                {
                                    if (!deviceConfig.quiet)
                                        LOG("Client re-connected successfully \n");
                                    serverAvailable = 1;
                                }
                            }
                        }
                        else // error, reconnect
                        {
                            if (!deviceConfig.quiet)
                                LOG("Autorization ERROR. Reconnect... \n");

                            // close existing connection
                            close(eventSource);
                            // remove from epoll
                            epollConfig.data.fd = eventSource;
                            epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                            if (!deviceConfig.quiet)
                                LOG("Connection closed by server\n");

                            serverAvailable = 0;
                            autorized = 0;
                            // re-create socket
                            threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
                            if(threadFD->mainSocket < 0)
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Error while re-opening client socket \n");
                            }

                            // add to epoll again
                            epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
                            epollConfig.data.fd = threadFD->mainSocket;
                            result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
                            if(result < 0)
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Error while socket epoll regisration\n");
                                pthread_exit(NULL);
                            }

                            result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));

                            // if connection is not successfull, restart timer
                            if(result != 0)
                            {
                                result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                                if(result < 0)
                                {
                                    if (!deviceConfig.quiet)
                                        LOG("Error while timer setup \n");
                                }
                            }
                            else
                            {
                                if (!deviceConfig.quiet)
                                    LOG("Client re-connected successfully \n");
                                serverAvailable = 1;
                            }
                        }
                    }
                }
                else // if server wants to close connection
                {  
                    // close existing connection
                    close(eventSource);
                    // remove from epoll
                    epollConfig.data.fd = eventSource;
                    epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, eventSource, &epollConfig);
                    if (stat == 0)
                        if (!deviceConfig.quiet)
                            LOG("Connection closed by server\n");
                    stat = 1;

                    serverAvailable = 0;
                    autorized = 0;
                    // re-create socket
                    threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
                    if(threadFD->mainSocket < 0)
                    {
                        if (!deviceConfig.quiet)
                            LOG("Error while re-opening client socket \n");
                    }

                    // add to epoll again
                    epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
                    epollConfig.data.fd = threadFD->mainSocket;
                    result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
                    if(result < 0)
                    {
                        if (!deviceConfig.quiet)
                            LOG("Error while socket epoll regisration\n");
                        pthread_exit(NULL);
                    }

                    result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));

                    // if connection is not successfull, restart timer
                    if(result != 0)
                    {
                        result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                        if(result < 0)
                        {
                            if (!deviceConfig.quiet)
                                LOG("Error while timer setup \n");
                        }
                    }
                    else
                    {
                        if (!deviceConfig.quiet)
                            LOG("Client re-connected successfully \n");
                        serverAvailable = 1;
                        stat = 0;
                    }
                    
                }
            }

            // data from serial
            else if(eventSource == threadFD->serialPort)
            {
                numOfReadBytes = read(threadFD->serialPort, dataBuffer, dataBufferSize);
                if(serverAvailable == 1){
                    //TODO SETTING MODBUS
                    if(deviceConfig.modbus_gateway == 1){
                    	SendModbusTCP(threadFD, dataBuffer, numOfReadBytes, deviceConfig.quiet);
                    }else
                    if(deviceConfig.modbus_gateway == 2){
                    	threadFD->mb_tcp.sock = threadFD->mainSocket;
                    	SendModbusASCIItoTCP(threadFD, dataBuffer, numOfReadBytes, 0, deviceConfig.connection_mode, deviceConfig.quiet);
                    }else {
                    	send(threadFD->mainSocket, dataBuffer, numOfReadBytes, 0);
                    }
                }
                //LOG("Serial -> TCP: %d bytes\n", numOfReadBytes);
            }

            // it's time to reconnect
            else if(eventSource == threadFD->TCPtimer) 
            {
                if (!deviceConfig.quiet)
                    LOG("Time to reconnect\n"); // !!!

                // close existing connection
                close(threadFD->mainSocket);
                // remove from epoll
                epollConfig.data.fd = threadFD->mainSocket;
                epoll_ctl(threadFD->epollFD, EPOLL_CTL_DEL, threadFD->mainSocket, &epollConfig);
                serverAvailable = 0;
				autorized = 0;

                // reconnect
                threadFD->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
                if(threadFD->mainSocket < 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Error while re-opening client socket \n");
                }

                // add to epoll again
                epollConfig.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
                epollConfig.data.fd = threadFD->mainSocket;
                result = epoll_ctl(threadFD->epollFD, EPOLL_CTL_ADD, threadFD->mainSocket, &epollConfig);
                if(result < 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Error while socket epoll regisration\n");
                    pthread_exit(NULL);
                }

                result = connect(threadFD->mainSocket, (struct sockaddr *)&addr, sizeof(addr));
                if(result != 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Client re-connection failure\n"); // !!!
                }
                else
                {
                    if (!deviceConfig.quiet)
                        LOG("Client re-connected successfully \n"); // !!!
                    serverAvailable = 1;
                }

                // restart timer
                result = timerfd_settime(threadFD->TCPtimer, 0, &newValue, &oldValue);
                if(result < 0)
                {
                    if (!deviceConfig.quiet)
                        LOG("Error while timer setup \n");
                }
            } else
            // timer modbus tcp for forming packets
            if(eventSource == threadFD->mb_tcp.timer){
            	threadFD->mb_tcp.sock = threadFD->mainSocket;
            	EndTimeoutModbusTCP(threadFD, 0, deviceConfig.modbus_gateway, deviceConfig.connection_mode, deviceConfig.quiet);
            }

            else
            {
                if (!deviceConfig.quiet)
                    LOG("Unknown epoll event\n");
            }
        }      
    }

    pthread_cleanup_pop(1);
    return (void *) 0;
} 

char ByteToASCII(char x) {
	x &= 0x0F;
	if (x <= 9) return x + '0';
	return x - 10 + 'A';
}


int SendModbusRTU(struct fdStructType *threadFD,char *pBuf, int len, int ascii_rtu, int quiet){

	int count=0;
	if(threadFD == NULL || pBuf == NULL || !len)return 0;
	if(len > MAX_PACK_SIZE) return 0;

	while(count < len){
		switch(threadFD->state_rtu.state){
		case 0:		//Init;
			threadFD->state_rtu.len = 0;
			threadFD->state_rtu.offset = 0;
			memset(threadFD->state_rtu.packet, 0, MAX_PACK_SIZE);
			memset(threadFD->state_rtu.header, 0, MB_TCP_HEADER_SIZE);
			threadFD->state_rtu.state = 1;
			break;
		case 1:		//receive MBAP Header
			threadFD->state_rtu.header[threadFD->state_rtu.offset++] = pBuf[count++];
			if(threadFD->state_rtu.offset >= MB_TCP_HEADER_SIZE){
				threadFD->state_rtu.len = threadFD->state_rtu.header[5];
				if (!quiet)
                    LOG("Receive TCP modbus header len = %d\n", threadFD->state_rtu.len);
				if(threadFD->state_rtu.len > MAX_PACK_SIZE || modbus_check_header(threadFD->state_rtu.header)){
					if (!quiet)
                        LOG("Header is damaged, drop connection\n");
					threadFD->state_rtu.state = 0;
					return 0;
				}

				if(ascii_rtu == 2){
					threadFD->state_rtu.offset = 1;
					threadFD->state_rtu.packet[0] = ':';
					threadFD->state_rtu.crc = 0;
					threadFD->state_rtu.state = 3;
				}else{
					threadFD->state_rtu.offset = 0;
					threadFD->state_rtu.state = 2;
				}
			}
			break;
		case 2:		//forming RTU packet
			threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = pBuf[count++];
			if(threadFD->state_rtu.offset >= threadFD->state_rtu.len){
				modbus_crc_write((unsigned char *)threadFD->state_rtu.packet, threadFD->state_rtu.offset);
				if (!quiet)
                    LOG("Forming RTU modbus packet CRC\n", threadFD->state_rtu.len);
				threadFD->state_rtu.offset += 2;
				write(threadFD->serialPort, threadFD->state_rtu.packet, threadFD->state_rtu.offset);
				if (!quiet)    
                    LOG("Send RTU modbus packet len = %d\n", threadFD->state_rtu.offset);
				threadFD->state_rtu.state = 0;
			}
			break;
		case 3:	//forming ASCII packet
			threadFD->state_rtu.crc += pBuf[count];
			threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = ByteToASCII(pBuf[count]>> 4);
			threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = ByteToASCII(pBuf[count++]);
			if(threadFD->state_rtu.offset >= ((threadFD->state_rtu.len*2)+1)){
                if (!quiet)
                    LOG("Forming ASCII modbus packet CRC\n");
				threadFD->state_rtu.crc = (255 - threadFD->state_rtu.crc) + 1;
				threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = ByteToASCII(threadFD->state_rtu.crc>> 4);
				threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = ByteToASCII(threadFD->state_rtu.crc);
				threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = 0x0D;
				threadFD->state_rtu.packet[threadFD->state_rtu.offset++] = 0x0A;
				write(threadFD->serialPort, threadFD->state_rtu.packet, threadFD->state_rtu.offset);
				if (!quiet)
                    LOG("Send modbus ASCII packet len = %d\n", threadFD->state_rtu.offset);
				threadFD->state_rtu.state = 0;

			}
			break;
		}
	}
	return 1;
}
char ASCIItoOctet(char byte){
	if((byte >= '0' && byte <= '9')){
		return (byte-'0');
	}else
	if(byte >= 'A' && byte <= 'F'){
		return (byte-'A'+10);
	}
	return 0;
}
void ClearModbus(struct fdStructType *threadFD){

	if(threadFD == NULL)return;

	bzero(&threadFD->state_rtu,sizeof(t_rtu));
	bzero(&threadFD->mb_tcp, sizeof(t_mb_tcp));
}
void InsertByteModbusTCP(t_mb_tcp *pTCP, char symbole){
	int count;
	if(pTCP == NULL)return;
	count = pTCP->offset + MB_TCP_HEADER_SIZE;
	pTCP->packet[count] = symbole;
	pTCP->offset++;
	return;
}
void SendModbusASCIItoTCP(struct fdStructType *threadFD,char *pBuf, int len, uint8_t server, uint8_t connection_mode, int quiet){
	int count=0;
	uint32_t i;
	uint8_t calculate_crc=0;
	struct itimerspec oldValue;
	int result;

	if(threadFD == NULL || pBuf == NULL || !len)return;
	if(len > MAX_PACK_SIZE) return;

	t_mb_tcp *pTCP= &threadFD->mb_tcp;

	while(count < len){
		switch(pTCP->state){
		case 0:
			if(pBuf[count] == ':'){
				pTCP->state = 1;
				pTCP->offset = 0;
				bzero( pTCP->packet, MAX_PACK_SIZE);
				if (!quiet)
                    LOG("Start receive modbus ASCII packet");
			}
			break;
		case 1:
			if((pBuf[count] >= '0' && pBuf[count] <= '9') || (pBuf[count] >= 'A' && pBuf[count] <= 'F') ){
				pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE] = ASCIItoOctet(pBuf[count]);
				pTCP->state = 2;
			}else
			if(pBuf[count] == 0x0D){
				if(pTCP->offset < 2)pTCP->state = 0;
				pTCP->state = 3;
			}else{
				pTCP->state = 0;
			}
			break;
		case 2:
			if((pBuf[count] >= '0' && pBuf[count] <= '9') || (pBuf[count] >= 'A' && pBuf[count] <= 'F') ){
				pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE] = (pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE]<<4) | ASCIItoOctet(pBuf[count]);
				pTCP->offset++;
				if(pTCP->offset >= MAX_PACK_SIZE){
					pTCP->state = 0;
				}else{
					pTCP->state = 1;
				}
			}else{
				pTCP->state = 0;
			}
			break;
		case 3:
			if(pBuf[count] == 0x0A){
				pTCP->state = 0;
				calculate_crc = 0;
				for(i=0;i < (pTCP->offset+MB_TCP_HEADER_SIZE-1);i++){
					calculate_crc += (uint8_t)pTCP->packet[i];
				}
				calculate_crc = (255 - calculate_crc) + 1;
				if (!quiet)
                    LOG("Check CRC pack/ Device CRC= %d, calculating CRC=%d\n",(uint8_t)pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE-1], calculate_crc);
				if(calculate_crc == (uint8_t)pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE-1]){
					pTCP->offset--;
					memcpy(pTCP->packet,threadFD->state_rtu.header, MB_TCP_HEADER_SIZE);
					pTCP->packet[MB_TCP_HEADER_LEN] = (uint8_t)pTCP->offset;
					if(server)
                    {
                        if(connection_mode == 0)
                        {
                            send(pTCP->sock, pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
                        } else {
                            send_all(pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
                        }
                    }
					else
                    {
						pTCP->packet[1]++;		//Increment ID packet
						send(pTCP->sock, pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
					}
				if (!quiet)
                    LOG("Send TCP modbus data len=%d, mode=%d, sock=%d\n",pTCP->offset+MB_TCP_HEADER_SIZE, server, pTCP->sock);
				}else{

				}
			}else{
				pTCP->state = 0;
			}
			break;
		}
		count++;

	}
//	result = timerfd_settime(pTCP->timer, 0, &pTCP->newValue, &oldValue);
//	if(result < 0){
//		LOG("Error while timer setup \n");
//	}

}
void SendModbusTCP(struct fdStructType *threadFD,char *pBuf, int len, int quiet){
	int count=0;
	int result;
	struct itimerspec oldValue;

	if(threadFD == NULL || pBuf == NULL || !len)return;
	if(len > MAX_PACK_SIZE) return;

	t_mb_tcp *pTCP= &threadFD->mb_tcp;

	while(count < len){
		InsertByteModbusTCP(pTCP, pBuf[count++]);
		result = timerfd_settime(pTCP->timer, 0, &pTCP->newValue, &oldValue);
		if(pTCP->offset >= MAX_PACK_SIZE){
			pTCP->offset=0;
			bzero(pTCP->packet,MAX_PACK_SIZE);
			break;
		}
	}
    if (!quiet)
	    LOG("Start timeout RTU pack\n");
	//restart package timer
	result = timerfd_settime(pTCP->timer, 0, &pTCP->newValue, &oldValue);
	if(result < 0)
	{
        if (!quiet)
		    LOG("Error while timer setup \n");
	}
	;
}

void EndTimeoutModbusTCP(struct fdStructType *threadFD, uint8_t server, int ascii_rtu, uint8_t connection_mode, int quiet){
	uint16_t crc1, crc2;
	uint8_t low_byte, high_byte;
	if(threadFD == NULL)return;

	t_mb_tcp *pTCP= &threadFD->mb_tcp;

	if(ascii_rtu == 2){
		pTCP->state = 0;
        if (!quiet)
		    LOG("End timeout receive modbus ASCII packet");
		return;
	}
    if (!quiet)
	    LOG("End timeout RTU pack\n");
	crc1 = crc16((uint8_t *)&pTCP->packet[MB_TCP_HEADER_SIZE], pTCP->offset-2);
	high_byte = pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE-1];
	low_byte = pTCP->packet[pTCP->offset+MB_TCP_HEADER_SIZE-2];
	crc2 = (uint16_t)high_byte<<8 | (uint16_t)low_byte;
	if (!quiet)
        LOG("Check CRC pack/ Device CRC= %d, calculating CRC=%d\n",crc2,crc1);
	if(crc1 == crc2){
		pTCP->offset -= 2;
		memcpy(pTCP->packet,threadFD->state_rtu.header, MB_TCP_HEADER_SIZE);
		pTCP->packet[MB_TCP_HEADER_LEN] = (uint8_t)pTCP->offset;
		if(server)
        {
            if(connection_mode == 0)
            {
                send(pTCP->sock, pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
            } else {
                send_all(pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
            }
        } else {
			pTCP->packet[1]++;		//Increment ID packet
			send(pTCP->sock, pTCP->packet, pTCP->offset + MB_TCP_HEADER_SIZE, 0);
		}
	    if (!quiet)	    
        	LOG("Send TCP modbus data len=%d, mode=%d, sock=%d\n",pTCP->offset+MB_TCP_HEADER_SIZE, server, pTCP->sock);
	}
	pTCP->offset = 0;
}

// Read all config for current device from UCI
// WARNING: indian code detected!!!
device_config_t GetFullDeviceConfig(int deviceID)
{
    LOG("Reading settings of ID %d \n", deviceID);

    char *STRING_DISABLED = "disabled";
    char *STRING_SERVER = "server";
    char *STRING_CLIENT = "client";

    device_config_t deviceConfig = {0};

    // maybe use malloc? what about overhead?
    char UCIpathBegin[MAX_CHARS_IN_UCIPATH] = "pollmydevice.";

    char UCIpathMode[MAX_CHARS_IN_UCIPATH]          = ".mode";     
    char UCIpathDeviceName[MAX_CHARS_IN_UCIPATH]    = ".devicename";
    char UCIpathBaudRate[MAX_CHARS_IN_UCIPATH]      = ".baudrate";
    char UCIpathBytesize[MAX_CHARS_IN_UCIPATH]      = ".bytesize";
    char UCIpathParity[MAX_CHARS_IN_UCIPATH]        = ".parity";
    char UCIpathFlowControl[MAX_CHARS_IN_UCIPATH]   = ".flowcontrol";
    char UCIpathStopBits[MAX_CHARS_IN_UCIPATH]      = ".stopbits";
    char UCIpathServerPort[MAX_CHARS_IN_UCIPATH]    = ".server_port";
    char UCIpathConnectionMode[MAX_CHARS_IN_UCIPATH]  = ".connection_mode";
    char UCIpathHoldConnTime[MAX_CHARS_IN_UCIPATH]  = ".holdconntime";
    char UCIpathClientHost[MAX_CHARS_IN_UCIPATH]    = ".client_host";
    char UCIpathClientPort[MAX_CHARS_IN_UCIPATH]    = ".client_port";
    char UCIpathClientAuth[MAX_CHARS_IN_UCIPATH]    = ".client_auth";
    char UCIpathClientTimeout[MAX_CHARS_IN_UCIPATH] = ".client_timeout";
    char UCIpathModbusGateway[MAX_CHARS_IN_UCIPATH] = ".modbus_gateway";
    char UCIpathQuiet[MAX_CHARS_IN_UCIPATH]         = ".quiet";
    char UCIpathPackTimeout[MAX_CHARS_IN_UCIPATH]   = ".pack_timeout";
    char UCIpathPackSize[MAX_CHARS_IN_UCIPATH]      = ".pack_size";
    // -------------------------------------------- max = 15 symbols, use TMP_PATH_LENGTH

    char UCIpathNumber[MAX_DIGITS_IN_DEV_NUM];
    char UCIpath[MAX_CHARS_IN_UCIPATH];
    snprintf(UCIpathNumber, MAX_DIGITS_IN_DEV_NUM, "%d", deviceID); // ConvertToString(deviceID)

    struct uci_ptr UCIptr;
    struct uci_context *UCIcontext = uci_alloc_context();
    if(!UCIcontext) 
        return deviceConfig;

    // mode
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);           // UCIpath = UCIpathBegin = "pollmydevice."
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);       // UCIpath + (char)i
    strncat(UCIpath, UCIpathMode, TMP_PATH_LENGTH);          // UCIpath + ".mode"
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathMode);
        return deviceConfig;
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    {
        if(strcmp(UCIptr.o->v.string, STRING_DISABLED) == 0)
            deviceConfig.mode = MODE_DISABLED;
        else if(strcmp(UCIptr.o->v.string, STRING_SERVER) == 0)
            deviceConfig.mode = MODE_SERVER;
        else if(strcmp(UCIptr.o->v.string, STRING_CLIENT) == 0)
            deviceConfig.mode = MODE_CLIENT;
        else
            deviceConfig.mode = MODE_DISABLED;
    }

    // devicename
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathDeviceName, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathDeviceName);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        memcpy(deviceConfig.deviceName, UCIptr.o->v.string, sizeof(deviceConfig.deviceName));

    // baudrate
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathBaudRate, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathBaudRate);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.baudRate = atoi(UCIptr.o->v.string);

    // bytesize
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathBytesize, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathBytesize);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.byteSize = atoi(UCIptr.o->v.string);

    // parity
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathParity, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathParity);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    {
        if(memcmp(UCIptr.o->v.string, "even", 4) == 0)
            deviceConfig.parity = PARITY_EVEN;
        else if(memcmp(UCIptr.o->v.string, "odd", 3) == 0)
            deviceConfig.parity = PARITY_ODD;
        else
            deviceConfig.parity = PARITY_NONE;
    }

    // flow control
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathFlowControl, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathFlowControl);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    {
        if(memcmp(UCIptr.o->v.string, "XON/XOFF", 3) == 0)
            deviceConfig.flowcontrol = FC_XONXOFF;
        else if(memcmp(UCIptr.o->v.string, "RTS/CTS", 3) == 0)
            deviceConfig.flowcontrol = FC_RTSCTS;
        else
            deviceConfig.flowcontrol = FC_NONE;
    }

    // stopbits
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathStopBits, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathStopBits);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.stopBits = atoi(UCIptr.o->v.string);

    // server_port
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathServerPort, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathServerPort);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.serverPort = atoi(UCIptr.o->v.string);

    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathConnectionMode, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathConnectionMode);
        deviceConfig.connection_mode = 0;
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.connection_mode = atoi(UCIptr.o->v.string);

    // holdconntime
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathHoldConnTime, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathHoldConnTime);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.holdConnTime = atoi(UCIptr.o->v.string);
    if(deviceConfig.holdConnTime==0){
        LOG("Connection hold time not set. Set value to 60 sec\n");
        deviceConfig.holdConnTime = 60;
    }
    // client_host
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathClientHost, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathClientHost);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        memcpy(deviceConfig.clientHost, UCIptr.o->v.string, sizeof(deviceConfig.clientHost));

    // client_port
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathClientPort, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathClientPort);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.clientPort = atoi(UCIptr.o->v.string);

    // client_auth
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathClientAuth, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathClientAuth);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.clientAuth = atoi(UCIptr.o->v.string);

    // client_timeout
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathClientTimeout, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathClientTimeout);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.clientTimeout = atoi(UCIptr.o->v.string);

    //Modbus Gateway
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathModbusGateway, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
    		(UCIptr.o==NULL || UCIptr.o->v.string==NULL))
    {
    	LOG("No UCI field %s \n", UCIpathModbusGateway);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    	deviceConfig.modbus_gateway = atoi(UCIptr.o->v.string);

    // quiet mode
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathQuiet, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
        (UCIptr.o==NULL || UCIptr.o->v.string==NULL)) 
    {
        LOG("No UCI field %s \n", UCIpathQuiet);
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
        deviceConfig.quiet = atoi(UCIptr.o->v.string);

    // pack_timeout
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathPackTimeout, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
    		(UCIptr.o==NULL || UCIptr.o->v.string==NULL))
    {
    	LOG("No UCI field %s \n", UCIpathPackTimeout);
        deviceConfig.timeout_pack==0;
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    	deviceConfig.timeout_pack = atoi(UCIptr.o->v.string);
    
    // pack_size
    memcpy(UCIpath , UCIpathBegin, MAX_CHARS_IN_UCIPATH);
    strncat(UCIpath, UCIpathNumber, MAX_DIGITS_IN_DEV_NUM-2);
    strncat(UCIpath, UCIpathPackSize, TMP_PATH_LENGTH);
    if ((uci_lookup_ptr(UCIcontext, &UCIptr, UCIpath, true) != UCI_OK)||
    		(UCIptr.o==NULL || UCIptr.o->v.string==NULL))
    {
    	LOG("No UCI field %s \n", UCIpathPackSize);
        deviceConfig.size_pack==0;
    }
    if(UCIptr.flags & UCI_LOOKUP_COMPLETE)
    	deviceConfig.size_pack = atoi(UCIptr.o->v.string);

    // READ S/N AND CONVERT IT TO INT64
    FILE * pFuseFile;
    char * hexSerialNum = malloc(IMEI_LENGTH);

    pFuseFile = fopen("/sys/devices/soc0/80000000.apb/80000000.apbh/8002c000.ocotp/fuses/HW_OCOTP_CUST0", "r");
    if (pFuseFile == NULL)
    {
        deviceConfig.teleofisID = 0;
    }
    else
    {
        if (fgets(hexSerialNum, IMEI_LENGTH, pFuseFile) == NULL)
        {
            deviceConfig.teleofisID = 0;
        }
        else
        {
            // max - 10 digits in serial; +00000 - for looks like IMEI 
            deviceConfig.teleofisID = strtoll(hexSerialNum, NULL, 16) * 100000 + deviceID; 
            LOG("S/N dev %d = %lld\n", deviceID, deviceConfig.teleofisID);
            
        }
    }

    fclose(pFuseFile);
    if (hexSerialNum)
        free(hexSerialNum);
    
    // for web view
    char cmd[100];
    sprintf(cmd,"uci set pollmydevice.%d.teleofisid=%lld && uci commit pollmydevice",deviceID, deviceConfig.teleofisID);
    FILE *fp;
    fp = popen(cmd,"r");
    if (popen(cmd,"r") != NULL) 
        pclose(fp);


    uci_free_context(UCIcontext);
    return deviceConfig;
}

// from Evgeniy Korovin
int FormAuthAnswer(char *dataBuffer, long long int teleofisID)
{
    int i, len_out;
    uint16_t crc;

    memset(dataBuffer,0, 80);

    char stringTeleofisID[15];
    sprintf(stringTeleofisID, "%lld", teleofisID); // to string

    dataBuffer[0] = 0xC0;
    dataBuffer[1] = 0x00;
    dataBuffer[2] = 0x07;
    dataBuffer[3] = 0x00;
    dataBuffer[4] = 0x3F;
    dataBuffer[5] = 0x01; // auth counter

    memcpy(&dataBuffer[6], &stringTeleofisID[0], 15);   // copy imei to buffer
	
	dataBuffer[67] = 0x01;	//Аворизация по основному каналу

    crc = Crc16Block((uint8_t *)&dataBuffer[1], 68);
    len_out = 69;

    for(i = 0; i < 2; i++)
    {
        uint8_t temp = (uint8_t)(crc>>(i*8));
        if(temp == 0xC0 || temp == 0xC2 || temp == 0xC4)
        {
            dataBuffer[len_out++] = 0xC4;
            if(temp != 0xC4)
                dataBuffer[len_out++] = temp+1;
            else 
                dataBuffer[len_out++] = 0xC4;
        }
        else 
            dataBuffer[len_out++] = temp;
    }
    dataBuffer[len_out++] = 0xC2;

    return len_out;
}

static const uint16_t  Crc16Table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t Crc16Block(uint8_t* block, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--)
    {
        crc = (crc << 8) ^ Crc16Table[(crc >> 8) ^ *block++];
    }
    return crc;
}

void send_all(const void *dataBuffer, size_t numOfReadBytes, int flags){
    int descr=-1;
    start_request_descr();
    while(get_next_descr(&descr)==0) {
        send(descr, dataBuffer, numOfReadBytes, flags);
    }
}
