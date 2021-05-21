#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>

#include "common.h"
#include "log.h"


typedef struct current_info{
	uint8_t *atdevice;
	uint8_t *sim;
	uint8_t *imei;			//
	uint8_t *ccid;			//
	uint8_t *pincode_stat;
	uint8_t *sig_lev;		//
	uint8_t *reg_stat;
	uint8_t *base_st_id;
	uint8_t *base_st_bw;
	uint8_t *net_type;
	uint8_t *gprs_reg_stat;
	uint8_t *pack_type;
}current_info_t;

current_info_t siminfo;   

int ReadConfiguration(current_info_t *set)
{
	char * p;
	if ((p = GetUCIParam("simman.info.atdevice")) == NULL)
	{
		fprintf(stderr,"Error reading atdevice\n");
		return -1;
	}
	siminfo.atdevice = p;
	return 0;
}

int ModemStarted(char *atdevice)
{
	// 0 - OK, -1 - not found
	return access(atdevice, F_OK);
}

int GetSimInfo(char *device)
{
	LOG("Looking for SIM \n");
	siminfo.sim = 			GetModemInfo(INFO_SIM, device);
	LOG("%s \n",siminfo.sim);
	LOG("Reading SIM CCID\n");
	siminfo.ccid = 			GetModemInfo(INFO_CCID, device);
	LOG("%s \n",siminfo.ccid);
	LOG("Reading PIN Status\n");
	siminfo.pincode_stat = 	GetModemInfo(INFO_PINSTAT, device);
	LOG("%s \n",siminfo.pincode_stat);
	LOG("Reading Signal Strength\n");
	siminfo.sig_lev = 		GetModemInfo(INFO_SIGLEV, device);
	LOG("%s \n",siminfo.sig_lev);
	LOG("Reading Registration Status\n");
	siminfo.reg_stat = 		GetModemInfo(INFO_REGSTAT, device);
	LOG("%s \n",siminfo.reg_stat);
	LOG("Reading Base Station ID\n");
	siminfo.base_st_id = 	GetModemInfo(INFO_BASESTID, device);
	LOG("%s \n",siminfo.base_st_id);
	LOG("Reading Band\n");
	siminfo.base_st_bw = 	GetModemInfo(INFO_BASESTBW, device);
	LOG("%s \n",siminfo.base_st_bw);
	LOG("Reading Network Type\n");
	siminfo.net_type = 		GetModemInfo(INFO_NETTYPE, device);
	LOG("%s \n",siminfo.net_type);
	LOG("Reading GPRS Status\n");
	siminfo.gprs_reg_stat = GetModemInfo(INFO_GPRSSTAT, device);
	LOG("%s \n",siminfo.gprs_reg_stat);
	LOG("Reading Package Type\n");
	siminfo.pack_type = 	GetModemInfo(INFO_PACKTYPE, device);
	LOG("%s \n",siminfo.pack_type);
	LOG("Reading Modem IMEI\n");
	siminfo.imei = 			GetModemInfo(INFO_IMEI, device);
	LOG("%s \n",siminfo.imei);
	return 0;
}

int main(int argc, char **argv)
{
	if ( ReadConfiguration(&siminfo) == 0 )
	{
		if ( ModemStarted(siminfo.atdevice) < 0 )
		{
			LOG("modem not found\n");
			return 1;
		}
		LOG("Reading SIM info...\n");
		if(GetSimInfo(siminfo.atdevice))
			LOG("Error while reading SIM info\n");
		LOG("OK\n");
	}
	return 0;
}
