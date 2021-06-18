CC = gcc 
CFLAGS = -Os -Wall -Wmissing-declarations -g3
LDLIBS = -luci -lpthread -lubox

PROGRAMS=pollmydevice

all: $(PROGRAMS)

pollmydevice: pollmydevice.o crc16.o modbus.o client_list.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

clean:
	rm -f *.o $(PROGRAMS)  

