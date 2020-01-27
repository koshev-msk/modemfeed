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

#include "log.h"

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

int GetFileSize(char *file)
{
	struct stat st;
	if (stat(file, &st) == 0){
    		return st.st_size;
	}
	return 0;
}

float GetCpuUsage()
{
	FILE *fp;
    char b[32];

	fp = popen("grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage}'","r");

	if (fp == NULL)
	{
		return 0;
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return 0;
	}
	
	pclose(fp);

	return atof(b);	
}

float GetRamUsage()
{
	FILE *fp;
    char b[32];
	
	fp = popen("free -m | awk \'NR==2{printf \"%.2f\\n\", $3*100/$2 }\'","r");

	if (fp == NULL)
	{
		return 0;
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return 0;
	}
	
	pclose(fp);

	return atof(b);	
}

int GetDirSize(char *dir)
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	long long dir_size = 0;
	char fn[256];

	if ((d = opendir(dir)) == NULL )
	{
		return 0;
	}

	while((de = readdir(d)) != NULL)
	{
		if (de->d_type == DT_REG)
		{
			sprintf(fn,"%s/%s",dir,de->d_name);

			if (stat(fn,&st) == -1)
				continue;
			
			dir_size += st.st_size;
		}
	}
    closedir(d);
	
	// convert to megabytes
	return (int)(dir_size>>20); 
}

char *GetOldestFileInDir(char *dir)
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	char fn[256];
	char *res = NULL;
	time_t t = 0;

	if ((d = opendir(dir)) == NULL)
	{
		return NULL;
	}
	
	while((de = readdir(d)) != NULL)
	{
		if (de->d_type == DT_REG)
		{
			sprintf(fn,"%s/%s",dir,de->d_name);

			if (stat(fn,&st) == -1)
				continue;
			
			if ((!t)
			   || (t > st.st_mtime))
			{
				t = st.st_mtime;
				res = strdup(fn);
			}
		}
	}
	closedir(d);

	return res;
}

int RemoveFiles(char *path)
{
	if ( access(path, F_OK) == -1 )
	{
		// file not exists
		return -1;
	}

	return remove(path);	
}

char *GetIMEI()
{
	FILE *fp;
    char b[32] = {0};
	char *path = "/etc/simman/getimei.sh";
	char *defval = "NONE";

	if (access(path, F_OK) == -1)
	// script not found
		return NULL;
	
	fp = popen("uci -q get simman.core.imei","r");

	if (fp == NULL)
	{
		return strdup(defval);
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return strdup(defval);
	}
	pclose(fp);

	*(char*)(b+15) = '\0';

	if (strstr(b,"NONE") == NULL)
		// return current CCID
		return strdup(b);

	// need update from GSM
	fp = popen(path,"r");

	if (fp == NULL)
	{
		return strdup(defval);
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return strdup(defval);
	}
	
	pclose(fp);

	*(char*)(b+15) = '\0';

	if ((strstr(b,"NONE") == NULL)
	    && strlen(b))
	{
	 // update uci configuration
	 char cmd[128];
	 sprintf(cmd,"uci set simman.core.imei=%s && uci commit simman",b);
	 //fprintf(stderr,"diag: %s\n", cmd);

	 fp = popen(cmd,"r");
	
	 if (fp != NULL)
		 pclose(fp);
	 }

	return strdup(b);	

}

char *GetCCID()
{
	FILE *fp;
    char b[32] = {0};
	char *path = "/etc/simman/getccid.sh";
	char *defval = "NONE";
	uint8_t i;

	if (access(path, F_OK) == -1)
	{
 	   // script not found
	   fprintf(stderr,"diag: script %s not found\n",path);
	   return strdup(defval);
	}
	
	// need update from GSM
	fp = popen(path,"r");

	if (fp == NULL)
	{
		return strdup(defval);
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return strdup(defval);
	}
    pclose(fp);
	
	for (i =0; i < 20; i++)
	{
	 if (b[i]=='\n')
	 	break;
	}
	for ( ; i <= 20; i++)	
		b[i] = '\0';
	
	if ((strstr(b,"NONE") == NULL)
	    && (strlen(b)))
	{
	 // update uci configuration
	  char cmd[128];
	  sprintf(cmd,"uci set simman.core.ccid=%s && uci commit simman",b);
	  //fprintf(stderr,"diag: %s\n", cmd);

	  fp = popen(cmd,"r");

   	  if (fp != NULL)	
		  pclose(fp);
	 }

	return strdup(b);	
}

int GetSIG()
{
	FILE *fp;
    char b[32];
	char path[] = "/etc/simman/getsig.sh";

	if ( access(path, F_OK) == -1 )
		return 0;

	fp = popen(path,"r");

	if (fp == NULL)
	{
		return 0;
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return 0;
	}
	pclose(fp);

	return atoi(b);	
}

char *GetUCIParam(char *uci_path)
{
   char path[128]= {0};
   char buffer[80] = { 0 };
   struct  uci_ptr ptr;
   struct  uci_context *c = uci_alloc_context();

   if(!c) return NULL;

   strcpy(path, uci_path);

  // fprintf(stderr,"%s\n",path);

   if ((uci_lookup_ptr(c, &ptr, path, true) != UCI_OK) ||
         (ptr.o==NULL || ptr.o->v.string==NULL)) 
   { 
     uci_free_context(c);
     return NULL;
   }

   if(ptr.flags & UCI_LOOKUP_COMPLETE)
      strcpy(buffer, ptr.o->v.string);

   uci_free_context(c);

   return strdup(buffer);
}

int ping(char *ip, char *iface)
//int ping(char *ip)
{
 	FILE *fp;
    char b[128];
	char path[128] = {0};

	if(iface == NULL)
		sprintf(path,"/bin/ping -w10 -c2 -s 8 %s | grep 'rec' | awk -F'[ ]' '{print $4}'",ip);
	else
		sprintf(path,"/bin/ping -w10 -c2 -s 8 -I %s %s | grep 'rec' | awk -F'[ ]' '{print $4}'", iface, ip);

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

int gpioRead(int gpio)
{
	char buf[64];
	int fd, ret;
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

    if ((fd = open(buf, O_RDONLY)) < 0)
		return -1;

	if ((ret = read(fd, buf, 1)) > 0)
	{
		if (*buf == '0') ret=0;
		else
		 if (*buf == '1') ret=1;
	}
	close(fd);

	return ret;
}

int gpioSet(int gpio, int value)
{
	char buf[64];
	int fd, ret;
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    
	if ((fd = open(buf, O_WRONLY)) < 0)
		return -1;

    sprintf(buf, "%d", value);
    ret = write(fd, buf, 1);
	close(fd);

	return ret;
}

char *GetModemInfo(int InfoParam , char *dev)
{
	FILE *fp;
    char b[64] = {0};
    char *defval = "error";
    char *path;
    char *keypath;
    switch(InfoParam)
    {
    	case INFO_SIM:
    		path = "/etc/simman/getsimcheck.sh";
			break;
    	case INFO_CCID:
			path = "/etc/simman/getccid.sh";
			break;
		case INFO_PINSTAT:
			path = "/etc/simman/getpinstat.sh";
			break;
		case INFO_SIGLEV:
			path = "/etc/simman/getsiglev.sh";
			break;
		case INFO_REGSTAT:
			path = "/etc/simman/getreg.sh";
			break;
		case INFO_BASESTID:
			path = "/etc/simman/getbasestid.sh";
			break;
		case INFO_BASESTBW:
			path = "/etc/simman/getband.sh";
			break;
		case INFO_NETTYPE:
			path = "/etc/simman/getnettype.sh";
			break;
		case INFO_GPRSSTAT:
			path = "/etc/simman/getgprsreg.sh";
			break;
		case INFO_PACKTYPE:
			path = "/etc/simman/getpackinfo.sh";
			break;
		case INFO_IMEI:
			path = "/etc/simman/getimei.sh";
			break;
		default:
			return strdup(defval);
    }
	
	uint8_t i;

	if (access(path, F_OK) == -1)
	{
 	   // script not found
	   fprintf(stderr,"diag: script %s not found\n",path);
	   return strdup(defval);
	}

	// need update from GSM
	keypath = malloc(strlen(path) + strlen(dev) + 5);	// should it be free ???
	if (keypath) {
		strcpy(keypath, path);
		strcat(keypath, " -d ");
		strcat(keypath, dev);
	}

	fp = popen(keypath,"r");

	if (fp == NULL)
	{
		return strdup(defval);
	}

	if ( fgets(b,sizeof(b)-1, fp) == NULL )
	{
		return strdup(defval);
	}
    pclose(fp);
	
	for (i =0; i < 64; i++)
	{
	 if (b[i]=='\n')
	 	break;
	}
	for ( ; i <= 64; i++)	
		b[i] = '\0';
	if (/*(strstr(b,"NONE") == NULL) &&*/ ( strlen(b) ))
	{
	 // update uci configuration
		char cmd[256];
		switch(InfoParam)
    	{
    		case INFO_SIM:
				sprintf(cmd,"uci set simman.info.sim=%s && uci commit simman",b);
				break;
    		case INFO_CCID:
				sprintf(cmd,"uci set simman.info.ccid=%s && uci commit simman",b);
				break;
			case INFO_PINSTAT:
				sprintf(cmd,"uci set simman.info.pincode_stat=%s && uci commit simman",b);
				break;
    		case INFO_SIGLEV:
				sprintf(cmd,"uci set simman.info.sig_lev=%s && uci commit simman",b);
				break;
			case INFO_REGSTAT:
				sprintf(cmd,"uci set simman.info.reg_stat=%s && uci commit simman",b);
				break;
			case INFO_BASESTID:
				sprintf(cmd,"uci set simman.info.base_st_id=%s && uci commit simman",b);
				break;
			case INFO_BASESTBW:
				sprintf(cmd,"uci set simman.info.base_st_bw=%s && uci commit simman",b);
				break;
			case INFO_NETTYPE:
				sprintf(cmd,"uci set simman.info.net_type=%s && uci commit simman",b);
				break;
			case INFO_GPRSSTAT:
				sprintf(cmd,"uci set simman.info.gprs_reg_stat=%s && uci commit simman",b);
				break;
			case INFO_PACKTYPE:
				sprintf(cmd,"uci set simman.info.pack_type=%s && uci commit simman",b);
				break;
    		case INFO_IMEI:
				sprintf(cmd,"uci set simman.info.imei=%s && uci commit simman",b);
				break;
			default:
				return strdup(defval);
    	}
	   fp = popen(cmd,"r");

   		if (fp != NULL)	
			pclose(fp);
	}

	return strdup(b);	
}
