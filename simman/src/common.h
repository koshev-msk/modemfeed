#ifndef _common_h
#define _common_h

#define	INFO_SIM		0
#define	INFO_CCID		1
#define	INFO_PINSTAT	2
#define	INFO_SIGLEV		3
#define	INFO_REGSTAT	4
#define	INFO_BASESTID	5
#define	INFO_BASESTBW	6
#define	INFO_NETTYPE	7
#define	INFO_GPRSSTAT	8
#define	INFO_PACKTYPE	9
#define	INFO_IMEI		10


void execCommandNoWait(char **cmd);
void execCommand(char **cmd); 
int GetFileSize(char *file);
float GetCpuUsage(void);
float GetRamUsage(void);
int GetDirSize(char *dir);              // return value in Megabytes   
char *GetOldestFileInDir(char *dir);
int RemoveFiles(char *path);
char *GetIMEI(void);
char *GetCCID(void);
int GetSIG(void);

/**
* @brief Read parameter fro UCI
* @param path UCI string
* @return Return string parameter value or null if parameter not exists
*/
char *GetUCIParam(char *path);
int ping(char *ip, char *iface);
//int ping(char *ip);
int gpioRead(int gpio);
int gpioSet(int gpio, int value);
#endif

