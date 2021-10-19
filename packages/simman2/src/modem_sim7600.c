#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"


int sim7600_probe(char *device){
	char receive[256]={0};
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rATI\r","OK")!=0){
		return -1;
	}
	if(strstr(receive,"SIMCOM_SIM7100E")==NULL && strstr(receive,"SIMCOM_SIM7600E")==NULL){
		return -1;
	}

	return 0;
}

int sim7600_init(struct settings_entry *settings){
	char buf[256]={0};
	uint8_t reset=0;
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}
	
	if(modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPON?\r","OK")!=0)
	{
		return -1;
	}

	if(strstr(buf,"+UIMHOTSWAPON: 1")==NULL)
	{
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPON=1\r","OK");
		reset=1;
	}

	memset(buf,0,sizeof(buf));

	modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPLEVEL?\r","OK");
	if(strstr(buf,"+UIMHOTSWAPLEVEL: 1")==NULL)
	{
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPLEVEL=1\r","OK");
		reset=1;
	}

	if(reset)
	{
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPON=1\r","OK");
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPLEVEL=1\r","OK");
		sim7600_power_down(settings);
		sim7600_power_up(settings);
	}

	return 0;
}

int sim7600_version(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGMR\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "+CGMR: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	return 0;
}

int sim7600_ccid(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CICCID\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "+ICCID: ", "\r")!=0){
		strcpy(receive,"ERROR");
	}
	return 0;
}

int sim7600_bs_info(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CPSI?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "CPSI: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(common_awk_f(receive,",",5)!=0){
		strcpy(receive,"ERROR");
	}

	return 0;
}

int sim7600_network_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CPSI?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "CPSI: ", ",")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	return 0;
}

int sim7600_band_info(char *receive, char *device){
	char buf[256]={0};
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(buf,device,"\rAT+CPSI?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(buf, "CPSI: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	switch(buf[0]){
		case 'L': //LTE
			if(common_awk_f(buf,",", 8)!=0){
				strcpy(receive,"ERROR");
				return 0;
			}
			strcpy(receive,"EARFCN ");
			break;
		case 'W': //WCDMA
			if(common_awk_f(buf,",", 8)!=0){
				strcpy(receive,"ERROR");
				return 0;
			}
			strcpy(receive,"UARFCN ");
			break;
		case 'G': //GSM
			if(common_awk_f(buf,",", 6)!=0){
				strcpy(receive,"ERROR");
				return 0;
			}
			strcpy(receive,"ARFCN ");
			break;
		default:
			strcpy(receive,"ERROR");
			return 0;
	}

	strcat(receive,buf);
	return 0;
}


int sim7600_data_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CNSMOD?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "+CNSMOD: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, ",","")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	switch(receive[0]){
		case '0': strcpy(receive,"No service"); break;
		case '1': strcpy(receive,"GSM"); break;
		case '2': strcpy(receive,"GPRS"); break;
		case '3': strcpy(receive,"EGPRS (EDGE)"); break;
		case '4': strcpy(receive,"WCDMA"); break;
		case '5': strcpy(receive,"HSDPA only (WCDMA)"); break;
		case '6': strcpy(receive,"HSUPA only (WCDMA)"); break;
		case '7': strcpy(receive,"HSPA (HSDPA and HSUPA, WCDMA)"); break;
		case '8': strcpy(receive,"LTE"); break;
		default: strcpy(receive,"UNKNOWN"); break;
	}

	return 0;
}

int sim7600_imei(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGSN\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "AT+CGSN\r\r\n", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	return 0;
}

int sim7600_power_down(struct settings_entry *settings){
	char buf[256]={0};
	char count=0;

	ubus_interface_down(settings->iface);
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}
	modem_send_command(buf,settings->atdevice,"\rAT+CPOF\r","OK");

	while(count<=30)
	{
		if(modem_common_exist(settings->atdevice)==-1){
			break;
		}
		count++;
		sleep(1);
	}

	if(count==30){
		return -1;
	}
	gpio_set_value(settings->gsmpow_pin,HIGH);
	sleep(1);
	
	return 0;
}

int sim7600_power_up(struct settings_entry *settings){
	uint8_t count=0;
	gpio_set_value(settings->pwrkey_pin,HIGH);
	usleep(500);
	gpio_set_value(settings->gsmpow_pin,LOW);
	while(count<=45){
		count++;
		if(modem_common_exist(settings->atdevice)==0){
			return 0;
		}
		sleep(1);
	}
	if(count==45){
		return -1;
	}

	return 0;
}

struct modems_ops sim7600_ops = {
		.name				= "SIMCOM SIM7600E-H",
		.init				= sim7600_init,
		.probe				= sim7600_probe,
		.version			= sim7600_version,
		.imei				= sim7600_imei,
		.ccid				= sim7600_ccid,
		.imsi				= modem_common_imsi,
		.pin_state			= modem_common_pin_state,
		.csq				= modem_common_csq,
		.bs_info			= sim7600_bs_info,
		.registration		= modem_common_registration,
		.band_info			= sim7600_band_info,
		.network_type		= sim7600_network_type,
		.data_registration	= modem_common_data_registration,
		.data_type			= sim7600_data_type,
		.sim_pullout		= modem_common_sim_pullout,
		.sim_pullup			= modem_common_sim_pullup,
		.power_down			= sim7600_power_down,
		.power_up			= sim7600_power_up,
		.set_mode			= modem_common_set_mode,
		.set_apn			= modem_common_set_apn,
		.set_pin			= modem_common_set_pin,
		.set_auth			= modem_common_set_auth
};
