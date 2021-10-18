CC = gcc
CFLAGS = -O3 -Wall -Wmissing-declarations -g3
LDLIBS = -luci -lubox

PROGRAMS=simman2 simman2_cli

all: $(PROGRAMS)

simman2: simman.o common.o modem_ehs5.c modem_sim7600.o modem_a7600.o modem_sim5360.o modem_sim5300.o modem_common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

simman2_cli: simman_cli.o common.o modem_ehs5.c modem_sim7600.o modem_a7600.o modem_sim5360.o modem_sim5300.o modem_common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

clean:
	rm -f *.o $(PROGRAMS)  


