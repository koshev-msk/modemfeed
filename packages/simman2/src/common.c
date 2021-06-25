#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "dirent.h"
#include "uci.h"
#include <fcntl.h>

#include <errno.h>
#include <termios.h>
#include <stdint.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

//---------------------------------------------------------OLD----------------------

void execCommandNoWait(char **cmd) {
	//  Create a child process from current (?)
	pid_t varProcess = fork();
	
	//  if fork successfully created
	if (!varProcess) {
		//  Execute command
		execvp(*cmd, cmd);
	}
	
	//  Wait for command closing
	waitpid(varProcess, NULL, WNOHANG);
}


void execCommand(char **cmd) {
	//  Create a child process from current (?)
	pid_t varProcess = fork();
	
	//  if fork successfully created
	if (!varProcess) {
		//  Execute command
		execvp(*cmd, cmd);
	}
	
	//  Wait for command closing
	waitpid(varProcess, NULL, 0);
}

int ping(char *ip, char *iface)
//int ping(char *ip)
{
 	FILE *fp;
    char b[128];
	char path[128] = {0};

	char ph_iface[128] = {0};

	if(iface == NULL)
		sprintf(path,"/bin/ping -w10 -c2 -s 8 %s | grep 'rec' | awk -F'[ ]' '{print $4}'",ip);
	else
	{
		sprintf(path,"ubus call network.interface.%s status | grep l3_device", iface);
		fp = popen(path,"r");
		if (fp == NULL)
		{
			pclose(fp);
			return 0;
		}
		if ( fgets(ph_iface,sizeof(ph_iface)-1, fp) == NULL )
		{
			pclose(fp);
			return 0;
		}
		pclose(fp);
		if(common_awk_f(ph_iface,"\"", 4)!=0)
			sprintf(path,"/bin/ping -w3 -c2 -s 8 -I %s %s | grep 'rec' | awk -F'[ ]' '{print $4}'", iface, ip);
		else
			sprintf(path,"/bin/ping -w3 -c2 -s 8 -I %s %s | grep 'rec' | awk -F'[ ]' '{print $4}'", ph_iface, ip);
	}

	fp = popen(path,"r");

	if (fp == NULL)
	{
		pclose(fp);
		return 0;
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		pclose(fp);
		return 0;
	}

	pclose(fp);

   return atoi(b);
}



//---------------------------------------------------------OLD----------------------

int gpio_export(int16_t gpio)
{
	char buf[64];
	int fd, ret;
	uint16_t igpio;
	if(gpio<0){
		igpio = 0-gpio;
		sprintf(buf, "/sys/class/gpio/gpio%d/value", igpio);
	} else {
		sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	}

	if(access(buf, F_OK)==0)
	{
		return 0;
	}
	if((fd = open("/sys/class/gpio/export", O_WRONLY)) < 0)
	{
		return -1;
	}
	if(gpio<0){
		sprintf(buf, "%d", igpio);
	} else {
		sprintf(buf, "%d", gpio);
	}
	ret = write(fd, buf, strlen(buf));

	close(fd);
	return ret;
}

int gpio_read(int16_t gpio)
{
	char buf[64];
	int fd, ret;
	uint16_t igpio;
	if(gpio<0){
		igpio = (uint16_t)(0-gpio);
		sprintf(buf, "/sys/class/gpio/gpio%d/value", igpio);
	} else {
		sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	}

    if((fd = open(buf, O_RDONLY)) < 0)
	{
		gpio_export(gpio);
		if((fd = open(buf, O_RDONLY)) < 0)
			return -1;
	}
	if((ret = read(fd, buf, 1)) > 0)
	{
		if(gpio<0)
		{
			if (*buf == '0') ret=1;
			else
				if (*buf == '1') ret=0;
		} else {
			if (*buf == '0') ret=0;
			else
				if (*buf == '1') ret=1;
		}
	}

	close(fd);
	return ret;
}

int gpio_set_direction(int16_t gpio, int dir)
{
	static const char s_directions_str[]  = "in\0out";
	char buf[64];
	int fd;
	if(gpio<0){
		uint16_t igpio = 0-gpio;
		sprintf(buf, "/sys/class/gpio/gpio%d/direction", igpio);
	} else {
		sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	}

 	if((fd = open(buf, O_WRONLY)) < 0)
	{
		gpio_export(gpio);
		if((fd = open(buf, O_RDONLY)) < 0)
			return -1;
	}

	if(write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3) == -1) {
		return -1;
	}

	close(fd);
	return 0;
}

int gpio_set_value(int16_t gpio, uint8_t value)
{
	char buf[64];
	int fd, ret;
	if(gpio<0){
		uint16_t igpio = 0-gpio;
		sprintf(buf, "/sys/class/gpio/gpio%d/value", igpio);
	} else {
		sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	}

	if((fd = open(buf, O_WRONLY)) < 0)
	{
		gpio_export(gpio);
		if((fd = open(buf, O_RDONLY)) < 0)
			return -1;
	}
	gpio_set_direction(gpio, OUT);
	if(gpio<0){
		if(value==0)
			value=1;
		else
			value=0;
	}
    sprintf(buf, "%d", value);
    ret = write(fd, buf, 1);

	close(fd);
	return ret;
}

static const struct modems_ops *modems[] = {
	&ehs5_ops,
	&sim7600_ops,
	&sim5360_ops,
	&sim5300_ops,
};

const struct modems_ops * modems_backend(char *device)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(modems); i++)
		if (modems[i]->probe(device)==0)
			return modems[i];

	return NULL;
}


int write_to_com(int fd, char *text) {
	int res;
	unsigned int a;
	char ch;
	for(a=0;a<strlen(text);a++) {
		ch=text[a];
		res=write(fd,&ch,1);
		if(res!=1) {
			printf("Could not write to COM device\n");
			return -1;
		}
	}
	return 0;
}


int get_one_byte(int fd) {
	int res;
	char ch;
	res=read(fd,&ch,1);
	if(res==1) {
		return ch;
	}
	else {
		return -1;
	}
	return 0;
}


char * wait_for(int fd, char *text, char *receive) {
	time_t timeout,now;
	int b,c;
	time(&now);
	timeout=time(&now)+2;
	b=0;
	while(now<=timeout) {
		time(&now);
		c=get_one_byte(fd);
		if(c!= -1) {
				//fixme: size of receive > b
				receive[b++]=c;
				if(strstr(receive,text)!=NULL) break;
				if(strstr(receive,"ERROR")!=NULL) break;
		}
	}
	return receive;
}


int set_interface(int fd, int speed)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf ("error %d from tcgetattr", errno);
		return -1;
	 }

	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag &= ~CRTSCTS;
	tty.c_cflag |= CREAD | CLOCAL;
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;
	tty.c_lflag &= ~ECHOE;
	tty.c_lflag &= ~ECHONL;
	tty.c_lflag &= ~ISIG;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	tty.c_oflag &= ~OPOST;
	tty.c_oflag &= ~ONLCR;
	tty.c_cc[VTIME] = 10;
	tty.c_cc[VMIN] = 0;

	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
			printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
			return 1;
	}
	return 0;
}

int cut_string(char *stringSource, char *stringStart, char *stringEnd){
	char *pStr;

	if(stringStart[0]!='\0'){
		pStr=strstr(stringSource,stringStart);
		if(!pStr){
			return -1;
		}
		pStr+=(int)strlen(stringStart);
		strcpy(stringSource,pStr);
	}

	if(stringEnd[0]!='\0'){
		pStr=strstr(stringSource,stringEnd);
		if(!pStr){
			return -1;
		}
		*pStr='\0';
	}
	return 0;
}

int modem_send_command(char *receive, char *device ,char *at_command, char *wait_output){
	int fd=open(device,O_RDWR|O_NOCTTY|O_SYNC);
	if(set_interface(fd,115200)!=0){
		close(fd);
		return -1;
	}
	if(write_to_com(fd,at_command)!=0){
		close(fd);
		return -1;
	}
	wait_for(fd,wait_output,receive);
	close(fd);
	return 0;
}

int modem_sim_state(char *receive, struct settings_entry *settings){
	int sim0_stat, sim1_stat, active_sim, res=0;

	res=gpio_set_direction(settings->simdet0_pin,0);
	if (res!=0)
	{
		printf("sim0_detect direction error: %d\n",res);
		return 1;
	}

	res=gpio_set_direction(settings->simdet1_pin,0);
	if (res!=0)
	{
		printf("sim1_detect direction error: %d\n",res);
	    return 1;
	}

	sim0_stat=gpio_read(settings->simdet0_pin);
	if (sim0_stat<0)
	{
		printf("sim0_detect read error: %d\n",sim0_stat);
		return 1;
	}
	//printf("SIM0 status: %d\n",sim0_stat);

	sim1_stat=gpio_read(settings->simdet1_pin);
	if (sim1_stat<0)
	{
		printf("sim1_detect read error: %d\n",sim1_stat);
	    return 1;
	}
	//printf("SIM1 status: %d\n",sim1_stat);

	active_sim=gpio_read(settings->simaddr_pin);
	if (active_sim<0)
	{
		printf("active_sim read error: %d\n",active_sim);
	    return 1;
	}
	//printf("Active SIM: %d\n",active_sim);

	sprintf(receive,"1 %sINSERTED%s  |  2 %sINSERTED%s",sim0_stat?"NOT ":"",active_sim?"":" (ACT)",sim1_stat?"NOT ":"",active_sim?" (ACT)":"");


	return 0;
}

int common_awk_f(char *source_str, char *delim, uint8_t num){
	uint16_t i=0;
	char *ptr = strtok(source_str, delim);
	while(ptr!=NULL)
	{
		if(++i==num){
			break;
		}
		ptr=strtok(NULL, delim);
	}
	//printf("'%s'\n", ptr);
	if(i!=num){
		return -1;
	}
	strcpy(source_str,ptr);
	return 0;
}

char *uci_get_value(char *uci_path)
{
   char path[256]= {0};
   char buffer[256] = {0};
   struct  uci_ptr ptr;
   struct  uci_context *c = uci_alloc_context();
   struct  uci_element *e;

   if(!c) return NULL;

   strcpy(path, uci_path);
   if ((uci_lookup_ptr(c, &ptr, path, true) != UCI_OK) ||
         (ptr.o==NULL || ptr.o->v.string==NULL))
   {
     uci_free_context(c);
     return NULL;
   }

   switch(ptr.o->type) {
       case UCI_TYPE_STRING:
    	   strcpy(buffer, ptr.o->v.string);
           break;
       case UCI_TYPE_LIST:
           uci_foreach_element(&ptr.o->v.list, e){
        	   strcat(buffer,e->name);
        	   strcat(buffer," ");
           }
           break;
       default:
    	   return NULL;
           break;
       }

   uci_free_context(c);

   return strdup(buffer);
}

int uci_set_value(char *section_name, char *option, char *value){

	printf("Set %s.%s to %s\n",section_name,option,value);

//	char path[256]= {0};
//	struct  uci_ptr ptr;
//	struct  uci_context *c = uci_alloc_context();
//
//	if(!c) return -1;
//
//	strcpy(path, section_name);
//   if (uci_lookup_ptr(c, &ptr, path, true) != UCI_OK) {
//       uci_free_context(c);
//       return -1;
//   }
//
//   ptr.option = option;
//   ptr.value = (value)?(value):("");
//
//	if (uci_set(c, &ptr) != UCI_OK)
//		uci_perror(c, "uci_set error");
//
//   //fixme "Child terminated with signal = 0xb (SIGSEGV)"
//   if (uci_commit(c, &ptr.p, false) != UCI_OK) {
// 		uci_free_context(c);
//		return -1;
//   }
//
//	uci_free_context(c);

	 char cmd[256];
	 if(value == NULL){
		 sprintf(cmd,"uci set %s.%s='' && uci commit",section_name,option);
	 } else {
		 sprintf(cmd,"uci set %s.%s='%s' && uci commit",section_name,option,value);
	 }
	 system(cmd);

	return 0;
}

int uci_read_configuration(struct settings_entry *set)
{
	char * p, path[128];
	char i,j;

	if ((p = uci_get_value("simman2.core.pwrkey_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading gsmpow_gpio_pin\n");
		return -1;
	}
	set->pwrkey_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.gsmpow_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading gsmpow_gpio_pin\n");
		return -1;
	}
	set->gsmpow_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet_gpio_pin\n");
		return -1;
	}
	set->simdet_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simaddr_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simaddr_gpio_pin\n");
		return -1;
	}
	set->simaddr_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet0_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet0_gpio_pin\n");
		return -1;
	}
	set->simdet0_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet1_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet1_gpio_pin\n");
		return -1;
	}
	set->simdet1_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.iface")) == NULL)
	{
		fprintf(stderr,"Error reading interface\n");
		return -1;
	}
	set->iface = p;

	if ((p = uci_get_value("simman2.core.only_first_sim")) == NULL)
	{
		fprintf(stderr,"Error reading only_first_sim\n");
		return -1;
	}
	set->only_first_sim = atoi(p);

	if ((p = uci_get_value("simman2.core.retry_num")) == NULL)
	{
		fprintf(stderr,"Error reading retry_num\n");
		return -1;
	}
	set->retry_num = atoi(p);

	if ((p = uci_get_value("simman2.core.check_period")) == NULL)
	{
		fprintf(stderr,"Error reading check_period\n");
		return -1;
	}
	set->check_period = atoi(p);

	if ((p = uci_get_value("simman2.core.delay")) == NULL)
	{
		fprintf(stderr,"Error reading delay\n");
		return -1;
	}
	set->delay = atoi(p);

	if ((p = uci_get_value("simman2.core.csq_level")) == NULL)
	{
		set->csq_level = 0;
	}
	set->csq_level = atoi(p);

	if ((p = uci_get_value("simman2.core.sw_before_modres")) == NULL)
	{
		fprintf(stderr,"Error reading sw_before_modres\n");
		set->sw_before_modres = 0;
		return -1;
	}
	set->sw_before_modres = atoi(p);

	if(set->sw_before_modres > 100)
		set->sw_before_modres = 100;
	else if(set->sw_before_modres < 0)
		set->sw_before_modres = 0;

	if ((p = uci_get_value("simman2.core.sw_before_sysres")) == NULL)
	{
		fprintf(stderr,"Error reading sw_before_sysres\n");
		set->sw_before_sysres = 0;
		return -1;
	}

	set->sw_before_sysres = atoi(p);
	if(set->sw_before_sysres > 100)
		set->sw_before_sysres = 100;
	else if(set->sw_before_sysres < 0)
		set->sw_before_sysres = 0;

	if ((p = uci_get_value("simman2.core.testip")) == NULL)
	{
		fprintf(stderr,"Error reading testip\n");
		return -1;
	}

	char *tok = strtok(p," ");

	for (i = 0; i < sizeof(set->serv)/sizeof(set->serv[0]); i++)
	{
		set->serv[i].ip = NULL;
		set->serv[i].retry_check = 0;
		set->serv[i].sim_num = NULL;
	}

	i = 0;
	while(tok && i < sizeof(set->serv)/sizeof(set->serv[0]))
	{
		set->serv[i].sim_num = 2;
		set->serv[i++].ip = tok;
		tok	= strtok(NULL," ");
	}
	j = i;
	for (i = 0; i < sizeof(set->sim)/sizeof(set->sim[0]); i++)
	{
		set->sim[i].init = 0;

		sprintf(path,"simman2.core.sim%d_priority",i);
		if ((p = uci_get_value(path)) == NULL)
		{
			fprintf(stderr,"Error reading sim%d_priority\n",i);
			continue;
		}
		set->sim[i].prio = atoi(p);
		sprintf(path,"simman2.core.sim%d_testip",i);
		if ((p = uci_get_value(path)) != NULL)
		{
			char *tok = strtok(p," ");
			while(tok && j < sizeof(set->serv)/sizeof(set->serv[0]))
			{
				set->serv[j].sim_num = i;
				set->serv[j++].ip = tok;
				tok	= strtok(NULL," ");
			}
		}
		sprintf(path,"simman2.core.sim%d_apn",i);
		if ((p = uci_get_value(path)) == NULL)
		{
			//fprintf(stderr,"Error reading sim%d_apn\n",i);
			set->sim[i].apn = NULL;
		} else
			set->sim[i].apn = p;

		sprintf(path,"simman2.core.sim%d_username",i);
		if ((p = uci_get_value(path)) == NULL)
		{
			//fprintf(stderr,"Error reading sim%d_username\n",i);
			set->sim[i].user = NULL;
		} else
			set->sim[i].user = p;

		sprintf(path,"simman2.core.sim%d_password",i);
		if ((p = uci_get_value(path)) == NULL)
		{
			//fprintf(stderr,"Error reading sim%d_password\n",i);
			set->sim[i].pass = NULL;
		} else
			set->sim[i].pass = p;

		sprintf(path,"simman2.core.sim%d_pincode",i);
		if ((p = uci_get_value(path)) == NULL)
		{
			//fprintf(stderr,"Error reading sim%d_pincode\n",i);
			set->sim[i].pin = NULL;
		} else
			set->sim[i].pin = p;

		set->sim[i].init = 1;
	}

	if (!(set->sim[0].init || set->sim[1].init))
	{
		fprintf(stderr,"No find SIM card info\n");
		return -1;
	}

	if ((p = uci_get_value("simman2.core.atdevice")) == NULL)
	{
		fprintf(stderr,"Error reading atdevice\n");
		return -1;
	}
	set->atdevice = p;

	if ((p = uci_get_value("simman2.core.iface")) == NULL)
	{
		fprintf(stderr,"Error reading interface\n");
		return -1;
	}
	set->iface = p;

	if ((p = uci_get_value("simman2.core.gsmpow_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading gsmpow_gpio_pin\n");
		return -1;
	}
	set->gsmpow_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet_gpio_pin\n");
		return -1;
	}
	set->simdet_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simaddr_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simaddr_gpio_pin\n");
		return -1;
	}
	set->simaddr_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet0_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet0_gpio_pin\n");
		return -1;
	}
	set->simdet0_pin = atoi(p);

	if ((p = uci_get_value("simman2.core.simdet1_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet1_gpio_pin\n");
		return -1;
	}
	set->simdet1_pin = atoi(p);

//	fprintf(stderr,"retry_num=%d, check_period=%d, delay=%d, atdevice=%s, gsmpow_pin=%d, simdet_pin=%d, simaddr_pin=%d, simdet0_pin=%d, simdet1_pin=%d, ",
//		set->retry_num,
//		set->check_period,
//		set->delay,
//		set->atdevice,
//		set->gsmpow_pin, set->simdet_pin, set->simaddr_pin, set->simdet0_pin, set->simdet1_pin );
	for (i = 0; i < sizeof(set->serv)/sizeof(set->serv[0]);i++)
		if (set->serv[i].ip)
//			fprintf(stderr,"%s ",set->serv[i].ip);

//	fprintf(stderr,"\n");
	return 0;
}

char *modem_summary(struct modems_ops *modem, uint8_t InfoParam, char *dev)
{
	FILE *fp;
    char b[64] = {0};
	char cmd[256] = {0};
    char *defval = "error";

	struct settings_entry settings;


    switch(InfoParam)
    {
		case INFO_MODEM:
			strcpy(b,modem->name);
			break;
		case INFO_FW:
			modem->version(b,dev);
			break;
    	case INFO_SIM:
			uci_read_configuration(&settings);
			modem_sim_state(b,&settings);
			break;
    	case INFO_CCID:
			if(modem->ccid(b,dev)){
				strcpy(b,"NONE");
			};
			sprintf(cmd,"echo %s > /tmp/simman2/ccid",b);
			system(cmd);
			break;
		case INFO_IMSI:
			if(modem->imsi(b,dev)){
				strcpy(b,"NONE");
			};
			break;
		case INFO_PINSTAT:
			modem->pin_state(b,dev);
			break;
		case INFO_SIGLEV:
			modem->csq(b,dev);
			break;
		case INFO_REGSTAT:
			modem->registration(b,dev);
			break;
		case INFO_BASESTID:
			modem->bs_info(b,dev);
			break;
		case INFO_BASESTBW:
			modem->band_info(b,dev);
			break;
		case INFO_NETTYPE:
			modem->network_type(b,dev);
			break;
		case INFO_GPRSSTAT:
			modem->data_registration(b,dev);
			break;
		case INFO_PACKTYPE:
			modem->data_type(b,dev);
			break;
		case INFO_IMEI:
			if(modem->imei(b,dev)){
				strcpy(b,"NONE");
			};
			sprintf(cmd,"echo %s > /tmp/simman2/imei",b);
			system(cmd);
			break;
		default:
			return strdup(defval);
    }

	return strdup(b);	
}

int modem_gpio_export(struct settings_entry *settings){
	int res=0;

	res=gpio_export(settings->pwrkey_pin);
	if (res<0)
	{
		printf("pwrkey_pin export error: %d\n",res);
		return 1;
	}

	res=gpio_export(settings->gsmpow_pin);
	if (res<0)
	{
		printf("gsmpow_pin export error: %d\n",res);
		return 1;
	}

	res=gpio_export(settings->simdet_pin);
	if (res<0)
	{
		printf("simdet_pin export error: %d\n",res);
		return 1;
	}

	res=gpio_export(settings->simaddr_pin);
	if (res<0)
	{
		printf("simaddr_pin export error: %d\n",res);
		return 1;
	}

	res=gpio_export(settings->simdet0_pin);
	if (res<0)
	{
		printf("sim0_detect export error: %d\n",res);
		return 1;
	}

	res=gpio_export(settings->simdet1_pin);
	if (res<0)
	{
		printf("simdet1_pin export error: %d\n",res);
		return 1;
	}

	return 0;
}

int ubus_interface_up(uint8_t *iface){
	char cmd[256];
	sprintf(cmd,"ubus call network.interface.%s up",iface);
	system(cmd);
	printf("Interface %s up\n",iface);
	return 0;
}

int ubus_interface_down(uint8_t *iface){
	char cmd[256];
	sprintf(cmd,"ubus call network.interface.%s down",iface);
	system(cmd);
	printf("Interface %s down\n",iface);
	return 0;
}

int ubus_network_reload(void){
	system("ubus call network reload");
	return 0;
}

int services_stop(uint8_t *iface){
	//fixme
	printf("Services %s stop\n",iface);
	return 0;
}

int services_start(uint8_t *iface){
	//fixme
	printf("Services %s start\n",iface);
	return 0;
}

int modem_sim_up(uint8_t *iface){
	//fixme
	printf("Modem specific command start %s \n",iface);
	return 0;
}

int modem_sim_down(uint8_t *iface){
	//fixme
	printf("Modem specific command stop %s \n",iface);
	return 0;
}

int switch_sim(struct settings_entry *settings, struct modems_ops *modem, uint8_t sim_n, uint8_t first_start){
	uint8_t sim0_stat, sim1_stat, active_sim, res=0;
	char buf[256]={0};
	char buf2[256]={0};
	char cmd[256]={0};

	printf("attempt to switch to SIM%d\n", sim_n+1);
	res=gpio_set_direction(settings->simdet0_pin,0);
	if (res!=0)
	{
		printf("sim0_detect direction error: %d\n",res);
		return 1;
	}

	res=gpio_set_direction(settings->simdet1_pin,0);
	if (res!=0)
	{
		printf("sim1_detect direction error: %d\n",res);
	    return 1;
	}

	sim0_stat=gpio_read(settings->simdet0_pin);
	if (sim0_stat<0)
	{
		printf("sim0_detect read error: %d\n",sim0_stat);
		return 1;
	}
	printf("SIM0 status: %d\n",sim0_stat);

	sim1_stat=gpio_read(settings->simdet1_pin);
	if (sim1_stat<0)
	{
		printf("sim1_detect read error: %d\n",sim1_stat);
	    return 1;
	}
	printf("SIM1 status: %d\n",sim1_stat);

	active_sim=gpio_read(settings->simaddr_pin);
	if (active_sim<0)
	{
		printf("active_sim read error: %d\n",active_sim);
	    return 1;
	}
	printf("Active SIM: %d\n",active_sim);

	if (sim0_stat==1 && sim1_stat==1){
		//fixme log
		printf("Both SIM cards are not inserted\n");
		gpio_set_value(settings->simdet_pin,0);
		ubus_interface_down(settings->iface);
		return 0;
	}

	if ((sim_n==0 && sim0_stat==1) || (sim_n==1 && sim1_stat==1)){
		//fixme log
		printf("Not inserted SIM %d, switch to inserted sim\n",sim_n+1);
		if(sim_n==0){
			sim_n=1;
		} else {
			sim_n=0;
		}		
	}

	ubus_interface_down(settings->iface);
	sleep(1);
	//fixme
	sprintf(buf,"network.%s",settings->iface);

	sprintf(buf2,"simman2.core.sim%d_apn",sim_n);
	uci_set_value(buf,"apn",uci_get_value(buf2));


	sprintf(buf2,"simman2.core.sim%d_pincode",sim_n);
	uci_set_value(buf,"pincode",uci_get_value(buf2));


	sprintf(buf2,"simman2.core.sim%d_username",sim_n);
	uci_set_value(buf,"username",uci_get_value(buf2));

	sprintf(buf2,"simman2.core.sim%d_password",sim_n);
	uci_set_value(buf,"password",uci_get_value(buf2));

	system("uci commit network");

	if (sim_n==active_sim && first_start == 0){
		//fixme log
		printf("SIM %d is already active\n",sim_n+1);
		sprintf(cmd,"echo %d > /tmp/simman2/sim",sim_n);
		system(cmd);
		ubus_network_reload();
		ubus_interface_up(settings->iface);
		return 0;
	}
	//fixme
	services_stop(settings->iface);
	//fixme
	modem->sim_pullout(settings);

	gpio_set_value(settings->simdet_pin,0);
	usleep(500);
	gpio_set_value(settings->simaddr_pin,sim_n);
	sprintf(cmd,"echo %d > /tmp/simman2/sim",sim_n);
	system(cmd);
	usleep(100);
	gpio_set_value(settings->simdet_pin,1);
	usleep(500);
	//fixme
	modem->sim_pullup(settings);
	services_start(settings->iface);
	ubus_network_reload();
	ubus_interface_up(settings->iface);
	return 0;
}
