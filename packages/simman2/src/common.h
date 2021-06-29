#ifndef _common_h
#define _common_h

#include <stdint.h>
#include <syslog.h>

#define	INFO_SIM		0
#define	INFO_CCID		1
#define	INFO_PINSTAT	2
#define	INFO_SIGLEV		3
#define	INFO_REGSTAT	4
#define	INFO_BASESTID	5
#define	INFO_BASESTBW	6
#define	INFO_NETTYPE	7
#define	INFO_GPRSSTAT	8
#define	INFO_PACKTYPE	9
#define	INFO_IMEI		10
#define	INFO_MODEM		11
#define	INFO_IMSI		12
#define	INFO_FW 		13
#define	INFO_ALL 		14

#define IN		0
#define OUT		1
#define	LOW		0
#define HIGH	1

#define LOG(fmt, ...) do { \
		syslog(LOG_NOTICE,"simman: "fmt, ## __VA_ARGS__); \
		fprintf(stderr, "simman: "fmt, ## __VA_ARGS__); \
	} while (0)

#define ERROR(fmt, ...) do { \
		syslog(LOG_ERR,"simman: "fmt, ## __VA_ARGS__); \
		fprintf(stderr, "simman: "fmt, ## __VA_ARGS__); \
	} while (0)

//-----------------------------------OLD----------------------------------
void execCommandNoWait(char **cmd);
void execCommand(char **cmd); 

/**
* @brief Read parameter fro UCI
* @param path UCI string
* @return Return string parameter value or null if parameter not exists
*/
int ping(char *ip, char *iface);
//int ping(char *ip);

//-----------------------------------OLD----------------------------------

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

struct settings_entry{
	uint8_t *name;
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
	int16_t pwrkey_pin;
	int16_t gsmpow_pin;
	int16_t simdet_pin;
	int16_t simaddr_pin;
	int16_t simdet0_pin;
	int16_t simdet1_pin;
};

struct modems_ops{
	const char *name;
	int (*probe)(char *);
	int (*init)(struct settings_entry *);
	int (*version)(char *,char *);
	int (*imei)(char *,char *);
	int (*ccid)(char *,char *);
	int (*pin_state)(char *,char *);
	int (*csq)(char *,char *);
	int (*bs_info)(char *,char *);
	int (*registration)(char *,char *);
	int (*band_info)(char *,char *);
	int (*network_type)(char *,char *);
	int (*data_registration)(char *,char *);
	int (*data_type)(char *,char *);
	int (*sim_pullout)(struct settings_entry *);
	int (*sim_pullup)(struct settings_entry *);
	int (*power_down)(struct settings_entry *);
	int (*power_up)(struct settings_entry *);
	int (*imsi)(char *,char *);
};

extern struct modems_ops ehs5_ops;
extern struct modems_ops sim7600_ops;
extern struct modems_ops sim5360_ops;
extern struct modems_ops sim5300_ops;

int modem_common_send_at(char *device);
int modem_common_pin_state(char *receive, char *device);
int modem_common_csq(char *receive, char *device);
int modem_common_imsi(char *receive, char *device);
int modem_common_registration(char *receive, char *device);
int modem_common_data_registration(char *receive, char *device);
int modem_common_exist(char *device);
int modem_common_power_down(struct settings_entry *settings, struct modems_ops *modem);
int modem_common_power_up(struct settings_entry *settings, struct modems_ops *modem);
int modem_common_power_reset(struct settings_entry *settings, struct modems_ops *modem);
int modem_common_sim_pullout(struct settings_entry *settings);
int modem_common_sim_pullup(struct settings_entry *settings);

int modem_send_command(char *receive, char *device, char *at_command, char *wait_output);
int common_awk_f(char *source_str, char *delim, uint8_t num);
int switch_sim(struct settings_entry *settings, struct modems_ops *modem, uint8_t sim_n, uint8_t first_start);
int gpio_read(int16_t gpio);
int gpio_set_value(int16_t gpio, uint8_t value);
char *uci_get_value(char *uci_path);
char *modem_summary(struct modems_ops *modem, uint8_t InfoParam, char *dev);

#endif

