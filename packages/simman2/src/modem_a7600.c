#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"

int a7600_name(char *receive, char *device){
	strcpy(receive,"SIMCOM A7602E-H");
	return 0;
}

int a7600_probe(char *device){
	char receive[256]={0};
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rATI\r","OK")!=0){
		return -1;
	}
	if(strstr(receive,"A7600E-HNVD")==NULL && strstr(receive,"A7600E-HNVW")==NULL && strstr(receive,"A7602E-H")==NULL){
		return -1;
	}

	return 0;
}

int a7600_init(struct settings_entry *settings){
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

	memset(buf,0,sizeof(buf));

	if(modem_send_command(buf,settings->atdevice,"\rAT+DIALMODE?\r","OK")!=0)
	{
		return -1;
	}

	if(strstr(buf,"+DIALMODE: 0")==NULL)
	{
		modem_send_command(buf,settings->atdevice,"\rAT+DIALMODE=0\r","OK");
		reset=1;
	}

	if(reset)
	{
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPON=1\r","OK");
		modem_send_command(buf,settings->atdevice,"\rAT+UIMHOTSWAPLEVEL=1\r","OK");
		a7600_power_down(settings);
		a7600_power_up(settings);
	}

	return 0;
}

int a7600_version(char *receive, char *device){
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

int a7600_ccid(char *receive, char *device){
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
	receive[strlen(receive)-2]='\0';
	return 0;
}

int a7600_bs_info(char *receive, char *device){
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

int a7600_network_type(char *receive, char *device){
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

int a7600_band_info(char *receive, char *device){
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


int a7600_data_type(char *receive, char *device){
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

int a7600_imei(char *receive, char *device){
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

int a7600_power_down(struct settings_entry *settings){
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

int a7600_power_up(struct settings_entry *settings){
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

int a7600_set_mode(struct settings_entry *settings,char *mode){
	char receive[256]={0};
	if(modem_common_send_at(settings->atdevice)!=0){
		return -1;
	}
	
	if(mode == NULL){
		if(modem_send_command(receive,settings->atdevice,"\rAT+CNMP=2\r","OK")!=0)
			return -1;
	} else {
		switch(mode[0]){
			case 'l': //LTE
				if(modem_send_command(receive,settings->atdevice,"\rAT+CNMP=38\r","OK")!=0)
					return -1;
				break;
			case 'u': //UMTS
				if(modem_send_command(receive,settings->atdevice,"\rAT+CNMP=14\r","OK")!=0)
					return -1;
				break;
			case 'g': //GSM
				if(modem_send_command(receive,settings->atdevice,"\rAT+CNMP=13\r","OK")!=0)
					return -1;
				break;
			default: //ALL MODES
				if(modem_send_command(receive,settings->atdevice,"\rAT+CNMP=2\r","OK")!=0)
					return -1;
				break;
		}
	}
	return 0;
}

int a7600_set_apn(struct settings_entry *settings,char *apn){
	char receive[256]={0},buf[256]={0};
	
	if(apn != NULL){
		if(modem_common_send_at(settings->atdevice)!=0){
			return -1;
		}
		sprintf(buf,"\rAT+CGDCONT=1,\"IP\",\"%s\"\r",apn);
		if(modem_send_command(receive,settings->atdevice,buf,"OK")!=0)
			return -1;
	}
	return 0;
}

int a7600_set_pin(struct settings_entry *settings,char *pin){
	char receive[256]={0},buf[256]={0};
	int count=0;
	
	if(pin != NULL){
		if(modem_common_send_at(settings->atdevice)!=0){
			return -1;
		}

		while(strstr(receive,"ISIMAID:")==NULL){
			memset(receive,0,sizeof(receive));
			modem_send_command(receive,settings->atdevice,"\rAT\r","OK");
			if(count++>10){
				break;
			}
			sleep(1);
		}
		count=0;
		while(strstr(receive,"+CPIN:")==NULL){
			memset(receive,0,sizeof(receive));
			modem_send_command(receive,settings->atdevice,"\rAT+CPIN?\r","OK");
			if(count++>10){
				break;
			}
			usleep(500);
		}

		if(strstr(receive,"+CPIN: SIM PIN")!=NULL)
		{
			sprintf(buf,"\rAT+CPIN=%s\r",pin);
			if(modem_send_command(receive,settings->atdevice,buf,"+CPIN: READY")!=0)
				return -1;
		}
	}
	return 0;
}

int a7600_set_auth(struct settings_entry *settings,char *user,char *pass){
	char receive[256]={0},buf[256]={0};
	
	if(user != NULL && pass != NULL){
		if(modem_common_send_at(settings->atdevice)!=0){
			return -1;
		}
		sprintf(buf,"\rAT+CGAUTH=1,2,%s,%s\r",pass,user);
		if(modem_send_command(receive,settings->atdevice,buf,"OK")!=0)
			return -1;
	}
	return 0;
}

struct modems_ops a7600_ops = {
		.name				= a7600_name,
		.init				= a7600_init,
		.probe				= a7600_probe,
		.version			= a7600_version,
		.imei				= a7600_imei,
		.ccid				= a7600_ccid,
		.imsi				= modem_common_imsi,
		.pin_state			= modem_common_pin_state,
		.csq				= modem_common_csq,
		.bs_info			= a7600_bs_info,
		.registration		= modem_common_registration,
		.band_info			= a7600_band_info,
		.network_type		= a7600_network_type,
		.data_registration	= modem_common_data_registration,
		.data_type			= a7600_data_type,
		.sim_pullout		= modem_common_sim_pullout,
		.sim_pullup			= modem_common_sim_pullup,
		.power_down			= a7600_power_down,
		.power_up			= a7600_power_up,
		.set_mode			= a7600_set_mode,
		.set_apn			= a7600_set_apn,
		.set_pin			= a7600_set_pin,
		.set_auth			= a7600_set_auth
};
