#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"

int modem_common_send_at(char *device){
	char buf[256]={0};
	uint8_t i=0;

	while(i<5){
		if(modem_send_command(buf,device,"AT\r","OK")==0){
			return 0;
		};
		i++;
		usleep(500);
	}
	return -1;
}

int modem_common_pin_state(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"NOT READY");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CPIN?\r","OK")!=0){
		strcpy(receive,"NOT READY");
		return -1;
	}
	if(cut_string(receive, "CPIN: ", "\r")!=0){
		strcpy(receive,"NOT READY");
	}
	return 0;
}

int modem_common_csq(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"99");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CSQ\r","OK")!=0){
		strcpy(receive,"99");
		return -1;
	}
	if(cut_string(receive, "CSQ: ", ",")!=0){
		strcpy(receive,"99");
		return -1;
	}
	return 0;
}

int modem_common_registration(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CREG?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "CREG: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, ",","")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	switch(receive[0]){
		case '0': strcpy(receive,"NOT REGISTERED"); break;
		case '1': strcpy(receive,"REGISTERED, HOME"); break;
		case '2': strcpy(receive,"NOT REGISTERED, OPERATOR SEARCH"); break;
		case '3': strcpy(receive,"REGISTRATION DENIED"); break;
		case '5': strcpy(receive,"REGISTERED, ROAMING!"); break;
		case '4':
		default: strcpy(receive,"UNKNOWN"); break;
	}
	return 0;
}

int modem_common_data_registration(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGREG?\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "CGREG: ", "\r")!=0){
		strcpy(receive,"ERROR");
		return 0;
	}
	if(cut_string(receive, ",","")!=0){
		strcpy(receive,"ERROR");
		return 0;
	}

	switch(receive[0]){
		case '0': strcpy(receive,"NOT REGISTERED"); break;
		case '1': strcpy(receive,"REGISTERED, HOME"); break;
		case '2': strcpy(receive,"NOT REGISTERED, OPERATOR SEARCH"); break;
		case '3': strcpy(receive,"REGISTRATION DENIED"); break;
		case '5': strcpy(receive,"REGISTERED, ROAMING!"); break;
		case '4':
		default: strcpy(receive,"UNKNOWN"); break;
	}

	return 0;
}

int modem_common_imsi(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		strcpy(receive,"ERROR");
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CIMI\r","OK")!=0){
		strcpy(receive,"ERROR");
		return -1;
	}
	if(cut_string(receive, "\r\n", "\r\n")!=0){
		strcpy(receive,"ERROR");
	}
	return 0;
}

int modem_common_exist(char *device){
	// 0 - OK, -1 - not found
	return access(device, F_OK);
}

int modem_common_power_down(struct settings_entry *settings, struct modems_ops *modem){
	uint8_t count=0;

	if(modem!=NULL)
	{
		modem->power_down(settings);
	}

	gpio_set_value(settings->gsmpow_pin,HIGH);
	while(count<=30)
	{
		if(modem_common_exist(settings->atdevice)==-1){
			return 0;
		}
		count++;
		sleep(1);
	}

	if(count==30){
		return -1;
	}

	return 0;
}

int modem_common_power_up(struct settings_entry *settings, struct modems_ops *modem){
	uint8_t count=0;
	//fixme pwrkey
	if(modem!=NULL)
	{
		modem->power_up(settings);
	}
	gpio_set_value(settings->pwrkey_pin,HIGH);
	sleep(1);
	gpio_set_value(settings->gsmpow_pin,LOW);
	while(count<=45){
		if(modem_common_exist(settings->atdevice)==0){
			return 0;			
		}
		count++;
		sleep(1);
	}
	
	if(count==45){
		return -1;
	}
	
	return 0;
}

int modem_common_power_reset(struct settings_entry *settings, struct modems_ops *modem){
	ubus_interface_down(settings->iface);
	sleep(1);
	if(modem_common_power_down(settings,modem)!=0){
		return -1;
	}
	sleep(1);
	if(modem_common_power_up(settings,modem)!=0){
		return -1;
	}
	return 0;
}

int modem_common_sim_pullout(struct settings_entry *settings){
	return 0;
}

int modem_common_sim_pullup(struct settings_entry *settings){
	return 0;
}

int modem_common_set_mode(struct settings_entry *settings, char *mode){
	return 0;
}

int modem_common_set_apn(struct settings_entry *settings, char *apn){
	return 0;
}