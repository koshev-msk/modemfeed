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


typedef struct current_info{
	uint8_t *atdevice;
	uint8_t *sim;
	uint8_t *imei;
	uint8_t *ccid;
	uint8_t *pincode_stat;
	uint8_t *sig_lev;
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
	if ((p = uci_get_value("simman2.info.atdevice")) == NULL)
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

int GetSimInfo(struct settings_entry *settings)
{
	char cmd[256];
	struct modems_ops *modem = NULL;
	modem = modems_backend(settings->atdevice);
	if(modem!=NULL){
		modem->init(settings);
		LOG("Looking for SIM \n");
		siminfo.sim = 			modem_summary(modem,INFO_SIM, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.sim='%s'",siminfo.sim);
		system(cmd);
		LOG("%s \n",siminfo.sim);

		LOG("Reading SIM CCID\n");
		siminfo.ccid = 			modem_summary(modem,INFO_CCID, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.ccid='%s'",siminfo.ccid);
		system(cmd);
		LOG("%s \n",siminfo.ccid);

		LOG("Reading PIN Status\n");
		siminfo.pincode_stat = 	modem_summary(modem,INFO_PINSTAT, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.pincode_stat='%s'",siminfo.pincode_stat);
		system(cmd);
		LOG("%s \n",siminfo.pincode_stat);

		LOG("Reading Signal Strength\n");
		siminfo.sig_lev = 		modem_summary(modem,INFO_SIGLEV, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.sig_lev='%s'",siminfo.sig_lev);
		system(cmd);
		LOG("%s \n",siminfo.sig_lev);

		LOG("Reading Registration Status\n");
		siminfo.reg_stat = 		modem_summary(modem,INFO_REGSTAT, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.reg_stat='%s'",siminfo.reg_stat);
		system(cmd);
		LOG("%s \n",siminfo.reg_stat);

		LOG("Reading Base Station ID\n");
		siminfo.base_st_id = 	modem_summary(modem,INFO_BASESTID, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.base_st_id='%s'",siminfo.base_st_id);
		system(cmd);
		LOG("%s \n",siminfo.base_st_id);

		LOG("Reading Band\n");
		siminfo.base_st_bw = 	modem_summary(modem,INFO_BASESTBW, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.base_st_bw='%s'",siminfo.base_st_bw);
		system(cmd);
		LOG("%s \n",siminfo.base_st_bw);

		LOG("Reading Network Type\n");
		siminfo.net_type = 		modem_summary(modem,INFO_NETTYPE, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.net_type='%s'",siminfo.net_type);
		system(cmd);
		LOG("%s \n",siminfo.net_type);

		LOG("Reading GPRS Status\n");
		siminfo.gprs_reg_stat = modem_summary(modem,INFO_GPRSSTAT, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.gprs_reg_stat='%s'",siminfo.gprs_reg_stat);
		system(cmd);
		LOG("%s \n",siminfo.gprs_reg_stat);

		LOG("Reading Package Type\n");
		siminfo.pack_type = 	modem_summary(modem,INFO_PACKTYPE, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.pack_type='%s'",siminfo.pack_type);
		system(cmd);
		LOG("%s \n",siminfo.pack_type);

		LOG("Reading Modem IMEI\n");
		siminfo.imei = 			modem_summary(modem,INFO_IMEI, settings->atdevice);
		sprintf(cmd,"uci set simman2.info.imei='%s'",siminfo.imei);
		system(cmd);
		LOG("%s \n",siminfo.imei);
		system("uci commit simman2");
	} else
	{
		LOG("modem does not respond to AT-commands\n");
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct settings_entry settings;
	if ( uci_read_configuration(&settings) == 0 )
	{
		if ( ModemStarted(settings.atdevice) < 0 )
		{
			LOG("modem not found\n");
			return 1;
		}
		LOG("Reading SIM info...\n");
		if(GetSimInfo(&settings))
			LOG("Error while reading SIM info\n");
		//LOG("OK\n");
	}
	return 0;
}
