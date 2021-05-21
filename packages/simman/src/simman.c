#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>

#define SYSLOG_NAMES
#include <syslog.h>

#include "common.h"
#include "log.h"

#define SETSIM_SCRIPT "/etc/simman/setsim.sh"

enum {
	INIT = 0,
	IDLE,
	GET_CCID,
	GET_IMEI,
	CH_SIM,
};

typedef struct testip{
	uint8_t *ip;
	uint16_t retry_check;
	uint8_t sim_num; // 0 - sim0, 1 - sim1, 2 - both
}testip_t;

typedef struct sim_s
{
	uint8_t init;
	uint8_t prio;
	uint8_t *pin;
	uint8_t *user;
	uint8_t *pass;
	uint8_t *apn;
}sim_t;

typedef struct settings_s{
	uint8_t only_first_sim;
	uint8_t retry_num;
	uint16_t check_period;
	uint16_t delay;
	uint16_t csq_level;
	testip_t serv[8];
	sim_t sim[2];
	uint8_t *atdevice;
	uint8_t *iface;
	uint16_t sw_before_modres;
	uint16_t sw_before_sysres;
	uint8_t *imei;
	uint8_t *ccid;
	uint16_t gsmpow_pin;
	uint16_t simdet_pin;
	uint16_t simaddr_pin;
	uint16_t simdet0_pin;
	uint16_t simdet1_pin;
}settings_t;

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

static settings_t settings;
static current_info_t siminfo;

int    sim1_status=-1,        // SIM1 status, 0 - detect, 1 - not detect, -1 - unknown
	   sim2_status=-1,        // SIM2 status
	   first_start,
	   active_sim;         // active SIM, 0 - SIM1, 1 - SIM2, -1 - unknown

int8_t state,
	   changeCounter,
	   changeCounterForReboot,
	   retry;	   

int GetSimInfo(char *device)
{
	siminfo.sim = 			GetModemInfo(INFO_SIM, device);
	siminfo.ccid = 			GetModemInfo(INFO_CCID, device);
	siminfo.pincode_stat = 	GetModemInfo(INFO_PINSTAT, device);
	siminfo.sig_lev = 		GetModemInfo(INFO_SIGLEV, device);
	siminfo.reg_stat = 		GetModemInfo(INFO_REGSTAT, device);
	siminfo.base_st_id = 	GetModemInfo(INFO_BASESTID, device);
	siminfo.base_st_bw = 	GetModemInfo(INFO_BASESTBW, device);
	siminfo.net_type = 		GetModemInfo(INFO_NETTYPE, device);
	siminfo.gprs_reg_stat = GetModemInfo(INFO_GPRSSTAT, device);
	siminfo.pack_type = 	GetModemInfo(INFO_PACKTYPE, device);
	siminfo.imei = 			GetModemInfo(INFO_IMEI, device);
	return 0;
}

int ReadConfiguration(settings_t *set)
{
	char * p, path[128];
	char i,j;

	if ((p = GetUCIParam("simman.core.only_first_sim")) == NULL)
	{
		fprintf(stderr,"Error reading only_first_sim\n");
		return -1;
	}
	settings.only_first_sim = atoi(p);

	if ((p = GetUCIParam("simman.core.retry_num")) == NULL)
	{
		fprintf(stderr,"Error reading retry_num\n");
		return -1;
	}	
	settings.retry_num = atoi(p);

	if ((p = GetUCIParam("simman.core.check_period")) == NULL)
	{
		fprintf(stderr,"Error reading check_period\n");
		return -1;
	}	
	settings.check_period = atoi(p);

	if ((p = GetUCIParam("simman.core.delay")) == NULL)
	{
		fprintf(stderr,"Error reading delay\n");
		return -1;
	}	
	settings.delay = atoi(p);

	if ((p = GetUCIParam("simman.core.csq_level")) == NULL)
	{
		settings.csq_level = 0;
	}	
	settings.csq_level = atoi(p);

	if ((p = GetUCIParam("simman.core.sw_before_modres")) == NULL)
	{
		fprintf(stderr,"Error reading sw_before_modres\n");
		settings.sw_before_modres = 0;
		return -1;
	}	
	settings.sw_before_modres = atoi(p);

	if(settings.sw_before_modres > 100)
		settings.sw_before_modres = 100;
	else if(settings.sw_before_modres < 0)
		settings.sw_before_modres = 0;

	if ((p = GetUCIParam("simman.core.sw_before_sysres")) == NULL)
	{
		fprintf(stderr,"Error reading sw_before_sysres\n");
		settings.sw_before_sysres = 0;
		return -1;
	}	
	settings.sw_before_sysres = atoi(p);
	if(settings.sw_before_sysres > 100)
		settings.sw_before_sysres = 100;
	else if(settings.sw_before_sysres < 0)
		settings.sw_before_sysres = 0;

	if ((p = GetUCIParam("simman.core.testip")) == NULL)
	{
		fprintf(stderr,"Error reading testip\n");
		return -1;
	}

	char *tok = strtok(p," ");

	for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]); i++)
	{
		settings.serv[i].ip = NULL;
		settings.serv[i].retry_check = 0;
		settings.serv[i].sim_num = NULL;
	}

	i = 0;
	while(tok && i < sizeof(settings.serv)/sizeof(settings.serv[0]))
	{
		settings.serv[i].sim_num = 2;
		settings.serv[i++].ip = tok;
		tok	= strtok(NULL," ");
	}
	j = i;
	for (i = 0; i < sizeof(settings.sim)/sizeof(settings.sim[0]); i++)
	{
		settings.sim[i].init = 0;

		sprintf(path,"simman.@sim%d[0].priority",i);
		if ((p = GetUCIParam(path)) == NULL)
		{
			fprintf(stderr,"Error reading sim[%d].priority\n",i);
			continue;
		}	
		settings.sim[i].prio = atoi(p);
		sprintf(path,"simman.@sim%d[0].testip",i);
		if ((p = GetUCIParam(path)) != NULL)
		{
			char *tok = strtok(p," ");
			while(tok && j < sizeof(settings.serv)/sizeof(settings.serv[0]))
			{
				settings.serv[j].sim_num = i;
				settings.serv[j++].ip = tok;
				tok	= strtok(NULL," ");
			}
		}
		/*
		   sprintf(path,"simman.@sim[%d].pin",i);
		   if ((p = GetUCIParam(path)) == NULL)
		   {
		   fprintf(stderr,"Error reading sim[%d].pin\n",i);
		   continue;
		   }	
		   settings.sim[i].pin = p;

		   sprintf(path,"simman.@sim[%d].GPRS_user",i);
		   if ((p = GetUCIParam(path)) == NULL)
		   {
		   fprintf(stderr,"Error reading sim[%d].user\n",i);
		   continue;
		   }	
		   settings.sim[i].user = p;

		   sprintf(path,"simman.@sim[%d].GPRS_pass",i);
		   if ((p = GetUCIParam(path)) == NULL)
		   {
		   fprintf(stderr,"Error reading sim[%d].pass\n",i);
		   continue;
		   }	
		   settings.sim[i].pass = p;

		   sprintf(path,"simman.@sim[%d].GPRS_apn",i);
		   if ((p = GetUCIParam(path)) == NULL)
		   {
		   fprintf(stderr,"Error reading sim[%d].apn\n",i);
		   continue;
		   }	
		   settings.sim[i].apn = p;
		   */
		settings.sim[i].init = 1;
	}

	if (!(settings.sim[0].init || settings.sim[1].init))
	{
		fprintf(stderr,"No find SIM card info\n");	
		return -1;
	}

	if ((p = GetUCIParam("simman.core.atdevice")) == NULL)
	{
		fprintf(stderr,"Error reading atdevice\n");
		return -1;
	}	
	settings.atdevice = p;

	if ((p = GetUCIParam("simman.core.iface")) == NULL)
	{
		fprintf(stderr,"Error reading interface\n");
		return -1;
	}	
	settings.iface = p;

	if ((p = GetUCIParam("simman.core.gsmpow_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading gsmpow_gpio_pin\n");
		return -1;
	}	
	settings.gsmpow_pin = atoi(p);

	if ((p = GetUCIParam("simman.core.simdet_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet_gpio_pin\n");
		return -1;
	}	
	settings.simdet_pin = atoi(p);

	if ((p = GetUCIParam("simman.core.simaddr_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simaddr_gpio_pin\n");
		return -1;
	}	
	settings.simaddr_pin = atoi(p);

	if ((p = GetUCIParam("simman.core.simdet0_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet0_gpio_pin\n");
		return -1;
	}	
	settings.simdet0_pin = atoi(p);

	if ((p = GetUCIParam("simman.core.simdet1_gpio_pin")) == NULL)
	{
		fprintf(stderr,"Error reading simdet1_gpio_pin\n");
		return -1;
	}	
	settings.simdet1_pin = atoi(p);

	fprintf(stderr,"retry_num=%d, check_period=%d, delay=%d, atdevice=%s, gsmpow_pin=%d, simdet_pin=%d, simaddr_pin=%d, simdet0_pin=%d, simdet1_pin=%d, ",
			settings.retry_num,
			settings.check_period,
			settings.delay,
			settings.atdevice,
			settings.gsmpow_pin, settings.simdet_pin, settings. simaddr_pin, settings.simdet0_pin, settings.simdet1_pin );
	for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]);i++)
		if (settings.serv[i].ip)
			fprintf(stderr,"%s ",settings.serv[i].ip);	

	fprintf(stderr,"\n");

	/*	for (i = 0; i < sizeof(settings.sim)/sizeof(settings.sim[0]);i++)
		if (settings.sim[i].init)
		fprintf(stderr,"sim_%i) -> prio=%d, pin=%s, user=%s, pass=%s, apn=%s\n",
		i,
		settings.sim[i].prio,
		settings.sim[i].pin,
		settings.sim[i].user,
		settings.sim[i].pass,
		settings.sim[i].apn);	
		*/
	return 0;
}

static int usage(const char *prog)
{
	fprintf(stderr, "Usage: %s\n"
			"\n", prog);
	exit(0);
}

int ModemStarted(char *atdevice)
{
	// 0 - OK, -1 - not found
	return access(atdevice, F_OK);
}

int ModemReset()
{
	gpioSet(settings.gsmpow_pin,1);
	sleep(5);
	gpioSet(settings.gsmpow_pin,0);
}

int SetSim(uint8_t sim)
{
	changeCounterForReboot++;
	changeCounter++;
	//LOG("changeCounter now %d and changeCounterForReboot %d \n", changeCounter, changeCounterForReboot);
	if ((settings.only_first_sim == 0) || (first_start == 1))
	{
		if (access(SETSIM_SCRIPT, 0) != 0)
		{
			LOG("not found %s\n", SETSIM_SCRIPT);
			return -1;
		}

		char *cmd[] = {strdup(SETSIM_SCRIPT), 
			"-s", (sim)?"1":"0",
			0, 0};

		//LOG("execute %s %s %s\n", cmd[0],
		//cmd[1], cmd[2]);

		execCommand(cmd);
		first_start = 0;

	}
	else
	{	
		LOG("switching to over SIM is not necessary");	
		if (access(SETSIM_SCRIPT, 0) != 0)
		{
			LOG("not found %s\n", SETSIM_SCRIPT);
			return -1;
		}

		char *cmd[] = {strdup(SETSIM_SCRIPT), 
			"-s", (!sim)?"1":"0",
			0, 0};

		//LOG("execute %s %s %s\n", cmd[0],
		//cmd[1], cmd[2]);

		execCommand(cmd);
	}

	if(settings.sw_before_sysres != 0)
	{
		if(changeCounterForReboot > settings.sw_before_sysres)
		{
			LOG("Sim switched %d times\n", changeCounterForReboot);
			LOG("Reboot...\n");
			sync();
			reboot(RB_AUTOBOOT);
		}
	}

	if(settings.sw_before_modres != 0)
	{
		if(changeCounter > settings.sw_before_modres)
		{
			LOG("Sim switched %d times\n", changeCounter);
			if (access(SETSIM_SCRIPT, 0) != 0)
			{
				LOG("not found %s\n", SETSIM_SCRIPT);
				return -1;
			}

			char *cmd1[] = {strdup(SETSIM_SCRIPT), "-p" };

			LOG("Modem reset...\n");
			execCommand(cmd1);
			changeCounter = 0;
			changeCounterForReboot--;
			first_start = 1;
		}

	}

	if(changeCounter>101)
		changeCounter=0;
	if(changeCounterForReboot>101)
		changeCounterForReboot=0;

	return 0;
}

int main(int argc, char **argv)
{
	int ch, ret = 0;
	int i,
		tmp, 
		ch_sim,
		num_sim,
		sig_level,
		hot_change;

	time_t now_time, prev_time, prev_delay_time;
	double diff, diff_delay;

	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
			case 'h':
			default:
				return usage(*argv);
		}
	}

	time(&now_time);
	prev_time = prev_delay_time = now_time;

	state = -1;
	first_start = 1;
	hot_change = 1;
	ch_sim = 0;
	changeCounter = 0;
	changeCounterForReboot = 0;

	LOG("service started\n");

	if ( ReadConfiguration(&settings) == 0 )  
		do{
			// get now time
			time(&now_time);
			// in seconds
			diff = difftime(now_time, prev_time);
			diff_delay = difftime(now_time, prev_delay_time);

			// check if modem exists
			if ( ModemStarted(settings.atdevice) < 0 )
			{
				if ((state < 0) || (state != INIT))
				{
					LOG("modem not found, try to turn on\n");
					ModemReset();
					sleep(60);
					first_start = 1;
					// changeCounter = settings.sw_before_modres;
					// SetSim(active_sim);
				}
				state = INIT;
			}
			else
				if (state == INIT) 
				{ 
					LOG("modem started\n");
					state = GET_IMEI;
				}	

			// read num active SIM card
			active_sim = gpioRead(settings.simaddr_pin);

			// check SIM1 status
			tmp  = gpioRead(settings.simdet0_pin);
			if ((tmp >= 0) && (tmp != sim1_status) && !first_start)   
			{  // SIM1 remove
				if (((tmp == 1)&&(active_sim == 0)) // вытянули сим 1
				   ||((tmp == 0)&&(settings.sim[0].prio > settings.sim[1].prio)) // вставили сим 1 и приоритет у нее выше  
				   ||((tmp == 0)&&(sim2_status != 0))) // вытащили сим 2 и приоритет у нее выше  
				   {
						for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]); i++)
							{
								settings.serv[i].retry_check = 0;
							}
						hot_change = 1;
						state = CH_SIM;
					}
			}	
			sim1_status = tmp;

			// check SIM2 status
			tmp = gpioRead(settings.simdet1_pin);
			if ((tmp >= 0) && (tmp != sim2_status) && !first_start)
			{  // SIM2 remove
				if (((tmp == 1)&&(active_sim > 0)) // вытянули сим 2
				    ||((tmp == 0)&&(settings.sim[0].prio < settings.sim[1].prio)) // вставили сим 2 и приоритет у нее выше
				    ||((tmp == 0)&&(sim1_status != 0))) // вытащили сим 1 и приоритет у нее выше  
					{
						for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]); i++)
							{
								settings.serv[i].retry_check = 0;
							}
						hot_change = 1;
					 	state = CH_SIM;
					 }
			}
			sim2_status = tmp;

			if (first_start)
			{
				if (settings.sim[0].prio < settings.sim[1].prio)
				{
					if (active_sim == 0)
					{
						if (sim2_status==0) //если SIM2 обнаружена
						{
							SetSim(1);
							LOG("SIM2 has the highest priority\n");
						}
						else
						{
							LOG("SIM2 is not available\n");
							SetSim(0);
						}
					}
					else
					{
						if (active_sim == 1)
						{
							if ((sim2_status != 0)&&(sim1_status == 0))
							{
								LOG("Only SIM1 detected\n");
								SetSim(0);
							}
							else 
							{
								LOG("SIM2 is active\n");
								SetSim(active_sim);
							}
						}
						else
							LOG("No one SIM is available\n");
					}
				}
				else
				{
					if (active_sim == 1)
					{
						if (sim1_status==0) //Если SIM1 обнаружена
						{
							SetSim(0);
							LOG("SIM1 has the highest priority\n");
						}
						else
						{
							LOG("SIM1 is not available\n");
							SetSim(1);
						}
					}
					else
					{
						if (active_sim == 0)
						{
							if ((sim1_status != 0)&&(sim2_status == 0))
							{
								LOG("Only SIM2 detected\n");
								SetSim(1);
							}
							else 
							{
								LOG("SIM1 is active\n");
								SetSim(active_sim);
							}
						}
						else
							LOG("No one SIM is available\n");
					}
				}
				hot_change = 0;
			}
			first_start = hot_change;
			hot_change = 0;

			// Если работаем на карте с низким приоритетом,
			// пробуем переключится на карту с высшим приоритетом
			if (((active_sim == 0 ) && (settings.sim[0].prio < settings.sim[1].prio))
					||((active_sim > 0) &&  (settings.sim[0].prio > settings.sim[1].prio)))
			{
				if ( diff_delay >= settings.delay )
				{
					prev_delay_time = now_time;
					ch_sim = 1;

					LOG("attempt to switch to the priority SIM (act.SIM%d p1/p2=%d/%d)\n",
							active_sim+1,
							settings.sim[0].prio, 
							settings.sim[1].prio);
				}
			}
			else
			{
				prev_delay_time = now_time;
			}


			fprintf(stderr,"%.0f %d %d %d\n", diff, sim1_status, sim2_status, active_sim);

			switch(state)
			{
				case INIT:	break;
				case GET_IMEI:
							// get modem IMEI
							if ( (long int)diff >= 3)
							{
								settings.imei = GetIMEI();

								prev_time = now_time;

								if (strstr(settings.imei,"NONE") != NULL)
								{
									if (++retry >= 2 )
										state = IDLE;
									else
										LOG("retry #%d reading IMEI\n", retry+1);

									break;	
								}
								LOG("found modem with IMEI %s\n",settings.imei);
								state =  GET_CCID;
							}
							break;
				case GET_CCID:
							if ( (long int)diff >= 3)
							{
								settings.ccid = GetCCID();

								prev_time = now_time;

								if (strstr(settings.ccid,"NONE") != NULL)
								{
									if (++retry >= 2 )
										state = IDLE;
									else
										LOG("retry #%d reading CCID\n", retry+1);
									break;	
								}
								LOG("act. SIM%d with CCID %s\n", active_sim+1, settings.ccid);
								state = IDLE;
							}
							break;
				case IDLE:
							if (ch_sim)
							{ // if need change SIM card
								ch_sim = 0;
								state = CH_SIM;
								break;
							}

							if (diff < settings.check_period && diff >= 0)
								break;

							if (settings.csq_level != 0){
								sig_level = GetSIG();
								if (sig_level == 99) {
									sleep(1);
									sig_level = GetSIG();
								}
								LOG("Current ASU: %d\n", sig_level);
								if (sig_level < settings.csq_level || sig_level == 99)
									LOG("ASU not detectable or below specified: %d ASU (min: %d ASU)\n", sig_level, settings.csq_level);
							}

							LOG("check servers\n");

							uint8_t need_change_sim = 0;

							for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]); i++)
							{
								if (!settings.serv[i].ip)
									break;
								if (settings.serv[i].sim_num == 2 || settings.serv[i].sim_num == active_sim) {
									int ack, cnt = 0;
									do{
										ack = ping((char*)settings.serv[i].ip, (char*)settings.iface);
										//ack = ping((char*)settings.serv[i].ip);
									}while(!ack && (++cnt < 3));

									if (!ack) 
										settings.serv[i].retry_check++;
									else if (settings.csq_level != 0 && (sig_level < settings.csq_level || sig_level == 99))
										settings.serv[i].retry_check++;
									else
									{
										settings.serv[i].retry_check = 0;
										changeCounter = 0;		
										changeCounterForReboot = 0;
									}

									if (settings.serv[i].retry_check >= settings.retry_num)
										need_change_sim++;

									if (ack)
									{										
										if (settings.csq_level != 0 && (sig_level < settings.csq_level || sig_level == 99))
											LOG("%s - LIVE; ASU LOW (%d/%d)\n", settings.serv[i].ip,settings.serv[i].retry_check, settings.retry_num);
										else
											LOG("%s - LIVE\n", settings.serv[i].ip);
									}
									else
										LOG("%s - DOWN (%d/%d)\n", settings.serv[i].ip,settings.serv[i].retry_check, settings.retry_num);
								}
								else
									need_change_sim++;
							}

							prev_time = now_time;
							/*LOG("Reading SIM info...");
							if(GetSimInfo(settings.atdevice))
								LOG("Error while reading SIM info\n");
							LOG("OK\n");*/
							// change sim if all servers id down
							if (i == need_change_sim)
							{
								ch_sim = 1;
								
								// clear 
								for (i = 0; i < sizeof(settings.serv)/sizeof(settings.serv[0]); i++)
								{
									if (!settings.serv[i].ip)
										break;

									settings.serv[i].retry_check = 0;
								}
							}
							break; 
				case CH_SIM:
							// need change SIM card	
							if (active_sim >= 0)
							{
								num_sim = active_sim;
								if (active_sim == 0)
								{
									if (sim2_status == 0)
									{
										num_sim = 1;
										LOG("attempt to switch to SIM%d\n", num_sim+1);
									}
								}
								else
								{
									if (sim1_status == 0)
									{
										num_sim = 0;
										LOG("attempt to switch to SIM%d\n", num_sim+1);
									}
								}								

								SetSim(num_sim);
							}

							prev_time = now_time;				
							retry = 0;

							state = GET_CCID;
							break;
				default: 
							state = INIT; 
							break;
			}
			// ~0.3 sec	
			usleep(300000);
		}while(1);

	LOG("service stopped\n");

	return 0;
}

