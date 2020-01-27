STAG_DIR = ./../../../staging_dir/target-arm_arm926ej-s_uClibc-0.9.33.2_eabi
CC = gcc 
CFLAGS = -Os -Wall -Wmissing-declarations -g3
CPPFLAGS = -I../inc -I./ -I$(STAG_DIR)/usr/include
LDFLAGS  = -L$(STAG_DIR)/usr/lib -L./

LDLIBS = -luci 

PROGRAMS=simman simman_getinfo

all: $(PROGRAMS)

simman: simman.o common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

simman_getinfo: simman_getinfo.o common.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

clean:
	rm -f *.o $(PROGRAMS)  


