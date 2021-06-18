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


enum {
	INIT = 0,
	MODEM_INIT,
	IDLE,
	GET_CCID,
	GET_IMEI,
	CH_SIM,
};

int    sim1_status=-1,        // SIM1 status, 0 - detect, 1 - not detect, -1 - unknown
	   sim2_status=-1,        // SIM2 status
	   first_start,
	   active_sim;         // active SIM, 0 - SIM1, 1 - SIM2, -1 - unknown

int8_t state,
	   changeCounter,
	   changeCounterForReboot,
	   retry;	   

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

int ModemReset(struct settings_entry *settings, struct modems_ops *modem)
{
	return (modem_common_power_reset(settings,modem));
}

int SetSim(struct settings_entry *settings, struct modems_ops *modem, uint8_t sim)
{
	changeCounterForReboot++;
	changeCounter++;
	//LOG("changeCounter now %d and changeCounterForReboot %d \n", changeCounter, changeCounterForReboot);
	if ((settings->only_first_sim == 0) || (first_start == 1))
	{
		switch_sim(settings, modem, sim, first_start);
		first_start = 0;

	}
	else
	{	
		LOG("switching to over SIM is not necessary\n");
		switch_sim(settings, modem, (sim==0)?1:0, first_start);
	}

	if(settings->sw_before_sysres != 0)
	{
		if(changeCounterForReboot > settings->sw_before_sysres)
		{
			LOG("Sim switched %d times\n", changeCounterForReboot);
			LOG("Reboot...\n");
			sync();
			reboot(RB_AUTOBOOT);
		}
	}

	if(settings->sw_before_modres != 0)
	{
		if(changeCounter > settings->sw_before_modres)
		{
			LOG("Sim switched %d times\n", changeCounter);
			LOG("Modem reset...\n");
			
			modem_common_power_reset(settings,modem);
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
	struct settings_entry settings;
	int ch, ret = 0;
	int i,
		tmp, 
		ch_sim,
		num_sim,
		sig_level,
		hot_change;

	time_t now_time, prev_time, prev_delay_time;
	double diff, diff_delay;

	struct modems_ops *modem = NULL;

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

	state = INIT;
	first_start = 1;
	hot_change = 1;
	ch_sim = 0;
	changeCounter = 0;
	changeCounterForReboot = 0;

	LOG("service started\n");

	if (uci_read_configuration(&settings) == 0)  
		do{
			// get now time
			time(&now_time);
			// in seconds
			diff = difftime(now_time, prev_time);
			diff_delay = difftime(now_time, prev_delay_time);

			// check if modem exists
			if (first_start && ModemStarted(settings.atdevice) < 0 )
			{
				LOG("waiting for modem to turn on\n");
				for(int i=0; i<30; i++){
					sleep(1);
					if(ModemStarted(settings.atdevice) >= 0)
						break;
				}
			}

			if ( ModemStarted(settings.atdevice) < 0 )
			{
				LOG("modem not found, try to turn on\n");
				state = MODEM_INIT;
			}
			else
				if (state == INIT) 
				{
					modem = modems_backend(settings.atdevice);
					if(modem!=NULL)
					{
						LOG("modem started\n");
						modem->init(&settings);
						state = GET_IMEI;
						first_start = 1;
					} else
					{
						LOG("modem does not respond to AT-commands\n");
						ModemReset(&settings,modem);
						state = INIT;
					}
				}	

			// read num active SIM card
			active_sim = gpio_read(settings.simaddr_pin);

			// check SIM1 status
			tmp  = gpio_read(settings.simdet0_pin);
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
			tmp = gpio_read(settings.simdet1_pin);
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

			if (first_start && state == GET_IMEI)
			{
				if (settings.sim[0].prio < settings.sim[1].prio)
				{
					if (active_sim == 0)
					{
						if (sim2_status==0) //если SIM2 обнаружена
						{
							SetSim(&settings,modem,1);
							LOG("SIM2 has the highest priority\n");
						}
						else
						{
							LOG("SIM2 is not available\n");
							SetSim(&settings,modem,0);
						}
					}
					else
					{
						if (active_sim == 1)
						{
							if ((sim2_status != 0)&&(sim1_status == 0))
							{
								LOG("Only SIM1 detected\n");
								SetSim(&settings,modem,0);
							}
							else 
							{
								LOG("SIM2 is active\n");
								SetSim(&settings,modem,active_sim);
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
							SetSim(&settings,modem,0);
							LOG("SIM1 has the highest priority\n");
						}
						else
						{
							LOG("SIM1 is not available\n");
							SetSim(&settings,modem,1);
						}
					}
					else
					{
						if (active_sim == 0)
						{
							if ((sim1_status != 0)&&(sim2_status == 0))
							{
								LOG("Only SIM2 detected\n");
								SetSim(&settings,modem,1);
							}
							else 
							{
								LOG("SIM1 is active\n");
								SetSim(&settings,modem,active_sim);
							}
						}
						else
							LOG("No one SIM is available\n");
					}
				}
				first_start = 0;
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
				case MODEM_INIT:
							if(ModemReset(&settings,modem)==0)
							{
								state = INIT;
							}
							break;
				case INIT:	break;
				case GET_IMEI:
							// get modem IMEI
							settings.imei = modem_summary(modem,INFO_IMEI, settings.atdevice);

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
							break;
				case GET_CCID:
							if ( (long int)diff >= 3)
							{
								settings.ccid = modem_summary(modem,INFO_CCID, settings.atdevice);

								prev_time = now_time;

								if (strstr(settings.ccid,"NONE") != NULL || strstr(settings.ccid,"ERROR") != NULL)
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
								sig_level = modem_summary(modem,INFO_SIGLEV, settings.atdevice);
								if (sig_level == 99) {
									sleep(1);
									sig_level = modem_summary(modem,INFO_SIGLEV, settings.atdevice);
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

								SetSim(&settings,modem,num_sim);
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

