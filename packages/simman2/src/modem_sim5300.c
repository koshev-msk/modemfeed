#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"


int sim5300_probe(char *device){
	char receive[256]={0};
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rATI\r","OK")!=0){
		return -1;
	}
	if(strstr(receive,"SIM5300E")==NULL){
		return -1;
	}

	return 0;
}

int sim5300_init(struct settings_entry *settings){
	char receive[256]={0};
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}

	if(modem_send_command(receive,settings->atdevice,"ATE0\r","OK")!=0){
		return -1;
	}

	return 0;
}

int sim5300_version(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGMR\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "Revision:", "\r")!=0){
		return -1;
	}
	return 0;
}

int sim5300_ccid(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CCID\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "\r\n", "FF\r")!=0){
		strcpy(receive,"ERROR");
	}
	return 0;
}

int sim5300_bs_info(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"AT+CENG=4\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	memset(receive, 0, sizeof(receive));

	if(modem_send_command(receive,device,"AT+CENG?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(strstr(receive,"UMTS")!=NULL)
	{
		if(cut_string(receive, "+CENG: 0,\"", "\"")!=0){
			strcpy(receive,"ERROR");
			return -1;
		}

		if(common_awk_f(receive,",",5)!=0){
			strcpy(receive,"ERROR");
		}
	} else 
	{
		if(cut_string(receive, "+CENG: 0,\"", "\"")!=0){
			strcpy(receive,"ERROR");
			return -1;
		}

		if(common_awk_f(receive,",",7)!=0){
			strcpy(receive,"ERROR");
		}
	}

	return 0;
}

int sim5300_network_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"AT+CENG=4\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	memset(receive, 0, sizeof(receive));

	if(modem_send_command(receive,device,"AT+CENG?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(strstr(receive,"UMTS")!=NULL)
	{
		strcpy(receive,"UMTS");

	} else 
	{
		strcpy(receive,"GSM");

	}

	return 0;
}

int sim5300_band_info(char *receive, char *device){
	char buf[256]={0};
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(buf,device,"AT+CENG=4\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	memset(buf, 0, sizeof(buf));

	if(modem_send_command(buf,device,"AT+CENG?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(strstr(buf,"UMTS")!=NULL)
	{
		strcpy(receive,"UARFCN ");
	} else
	{
		strcpy(receive,"ARFCN ");
	}

	if(cut_string(buf, "+CENG: 0,\"", "\"")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(common_awk_f(buf,",",1)!=0){
		strcpy(buf,"ERROR");
	}

	strcat(receive,buf);
	return 0;
}


int sim5300_data_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CSACT?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "+CSACT: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(common_awk_f(receive,",",1)!=0){
		strcpy(receive,"ERROR");
	}

	switch(receive[0]){
		case '0': strcpy(receive,"GSM"); break;
		case '1': strcpy(receive,"GPRS"); break;
		case '2': strcpy(receive,"WCDMA"); break;
		case '3': strcpy(receive,"EGPRS (EDGE)"); break;
		case '4': strcpy(receive,"HSDPA only(WCDMA)"); break;
		case '5': strcpy(receive,"HSUPA only(WCDMA)"); break;
		case '6': strcpy(receive,"HSPA (HSDPA and HSUPA, WCDMA)"); break;
		case '7': strcpy(receive,"LTE"); break;
		default: strcpy(receive,"UNKNOWN"); break;
	}

	return 0;
}

int sim5300_imei(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGSN\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "\r\n", "\r\n")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	return 0;
}

int sim5300_sim_pullout(struct settings_entry *settings){
	char buf[256]={0};
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}

	if(modem_send_command(buf,settings->atdevice,"\rAT+CFUN=0\r","OK")!=0){
		return -1;
	}

	return 0;
}

int sim5300_sim_pullup(struct settings_entry *settings){
	char buf[256]={0};
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}

	if(modem_send_command(buf,settings->atdevice,"\rAT+CFUN=1\r","OK")!=0){
		return -1;
	}

	return 0;
}

int sim5300_power_down(struct settings_entry *settings){
	return 0;
}

int sim5300_power_up(struct settings_entry *settings){
	return 0;
}

struct modems_ops sim5300_ops = {
		.name				= "sim5300",
		.init				= sim5300_init,
		.probe				= sim5300_probe,
		.version			= sim5300_version,
		.imei				= sim5300_imei,
		.ccid				= sim5300_ccid,
		.pin_state			= modem_common_pin_state,
		.csq				= modem_common_csq,
		.bs_info			= sim5300_bs_info,
		.registration		= modem_common_registration,
		.band_info			= sim5300_band_info,
		.network_type		= sim5300_network_type,
		.data_registration	= modem_common_data_registration,
		.data_type			= sim5300_data_type,
		.sim_pullout		= sim5300_sim_pullout,
		.sim_pullup			= sim5300_sim_pullup,
		.power_down			= sim5300_power_down,
		.power_up			= sim5300_power_up
};
