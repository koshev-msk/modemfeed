#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"

int ehs5_name(char *receive, char *device){
	strcpy(receive,"Cinterion EHS5-E");
	return 0;
}

int ehs5_probe(char *device){
	char receive[256]={0};
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rATI\r","OK")!=0){
		return -1;
	}
	if(strstr(receive,"EHS5-E")==NULL){
		return -1;
	}
	return 0;
}

int ehs5_init(struct settings_entry *settings){
	return 0;
}

int ehs5_version(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rATI\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "REVISION ", "\r")!=0){
		return -1;
	}
	return 0;
}

int ehs5_ccid(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CCID\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "CCID: ", "\r")!=0){
		strcpy(receive,"ERROR");
	}
	return 0;
}

int ehs5_bs_info(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT^SMONI\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "SMONI: ", "\r")!=0){
		return -1;
	}

	switch(receive[0]){
		case '2':
			if(common_awk_f(receive,",", 7)!=0){
				strcpy(receive,"SEARCH");
			}
			break;
		case '3':
			if(common_awk_f(receive,",", 9)!=0){
				strcpy(receive,"SEARCH");
			}
			break;
		default:
			strcpy(receive,"SEARCH");
			return 0;
	}

	return 0;
}

int ehs5_network_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT^SMONI\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "SMONI: ", ",")!=0){
		return -1;
	}
	return 0;
}

int ehs5_band_info(char *receive, char *device){
	char buf[256]={0};
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(buf,device,"\rAT^SMONI\r","OK")!=0){
		return -1;
	}
	if(cut_string(buf, "SMONI: ", "\r")!=0){
		return -1;
	}

	switch(buf[0]){
		case '2':
			strcpy(receive,"ARFCN ");
			break;
		case '3':
			strcpy(receive,"UARFCN ");
			break;
		default:
			strcpy(receive,"ERROR");
			return 0;
	}

	if(cut_string(buf, ",", ",")!=0){
		return -1;
	}
	strcat(receive,buf);
	return 0;
}

int ehs5_data_type(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT^SIND=\"psinfo\",2\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "SIND: psinfo,", "\r")!=0){
		return -1;
	}
	if(cut_string(receive, ",","")!=0){
		return -1;
	}

	switch(receive[0]){
		case '0': strcpy(receive,"GPRS/EGPRS not available"); break;
		//case '1': strcpy(receive,"GPRS available"); break;
		case '2': strcpy(receive,"GPRS"); break;
		case '3': strcpy(receive,"EGPRS available"); break;
		case '4': strcpy(receive,"EGPRS"); break;
		case '5': strcpy(receive,"WCDMA camped"); break;
		case '6': strcpy(receive,"WCDMA"); break;
		case '7': strcpy(receive,"HSDPA camped"); break;
		case '8': strcpy(receive,"HSDPA"); break;
		case '9': strcpy(receive,"HSDPA/HSUPA camped"); break;
		case '1':
			if(receive[1]=='0'){
				strcpy(receive,"HSDPA/HSUPA");
				break;
			} else {
				strcpy(receive,"GPRS available");
				break;
			}
		default: strcpy(receive,"UNKNOWN"); break;
	}

	return 0;
}

int ehs5_imei(char *receive, char *device){
	if(modem_common_send_at(device)!=0){
		return -1;
	}

	if(modem_send_command(receive,device,"\rAT+CGSN\r","OK")!=0){
		return -1;
	}
	if(cut_string(receive, "AT+CGSN\r\r\n", "\r")!=0){
		return -1;
	}
	return 0;
}

int ehs5_power_down(struct settings_entry *settings){
	return 0;
}

int ehs5_power_up(struct settings_entry *settings){
	return 0;
}

struct modems_ops ehs5_ops = {
		.name				= ehs5_name,
		.probe				= ehs5_probe,
		.init				= ehs5_init,
		.version			= ehs5_version,
		.imei				= ehs5_imei,
		.ccid				= ehs5_ccid,
		.imsi				= modem_common_imsi,
		.pin_state			= modem_common_pin_state,
		.csq				= modem_common_csq,
		.bs_info			= ehs5_bs_info,
		.registration		= modem_common_registration,
		.band_info			= ehs5_band_info,
		.network_type		= ehs5_network_type,
		.data_registration	= modem_common_data_registration,
		.data_type			= ehs5_data_type,
		.sim_pullout		= modem_common_sim_pullout,
		.sim_pullup			= modem_common_sim_pullup,
		.power_down			= ehs5_power_down,
		.power_up			= ehs5_power_up,
		.set_mode			= modem_common_set_mode,
		.set_apn			= modem_common_set_apn,
		.set_pin			= modem_common_set_pin,
		.set_auth			= modem_common_set_auth
};
