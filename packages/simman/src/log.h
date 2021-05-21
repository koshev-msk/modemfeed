#ifndef _log_h
#define _log_h

#include <stdio.h>
#include <syslog.h>

#define LOG(fmt, ...) do { \
		syslog(LOG_NOTICE,"simman: "fmt, ## __VA_ARGS__); \
		fprintf(stderr, "simman: "fmt, ## __VA_ARGS__); \
	} while (0)

#define ERROR(fmt, ...) do { \
		syslog(LOG_ERR,"simman: "fmt, ## __VA_ARGS__); \
		fprintf(stderr, "simman: "fmt, ## __VA_ARGS__); \
	} while (0)

#endif

