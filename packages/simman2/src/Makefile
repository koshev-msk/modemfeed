STAG_DIR = /home/kalashnikov/work/OpenWrt/staging_dir/target-arm_arm926ej-s_uClibc-0.9.33.2_eabi
CC = /home/kalashnikov/work/OpenWrt/staging_dir/toolchain-arm_arm926ej-s_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/bin/arm-openwrt-linux-uclibcgnueabi-gcc
CFLAGS = -Os -Wall -Wmissing-declarations -g3
CPPFLAGS = -I../inc -I./ -I$(STAG_DIR)/usr/include
LDFLAGS  = -L$(STAG_DIR)/usr/lib -L./

LDLIBS = -luci -lubox

PROGRAMS=simman2 simman2_cli

all: $(PROGRAMS)

simman2: simman.o common.o modem_ehs5.c modem_sim7600.o modem_sim5360.o modem_sim5300.o modem_common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

simman2_cli: simman_cli.o common.o modem_ehs5.c modem_sim7600.o modem_sim5360.o modem_sim5300.o modem_common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

clean:
	rm -f *.o $(PROGRAMS)  


