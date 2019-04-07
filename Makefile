TOPDIR  := $(shell cd ..; pwd)
#include $(TOPDIR)/Rules.make
include ./Rules.make

CC = g++

APP = sbus

all: $(APP)

$(APP): main.cpp
	$(CC) main.cpp -o $(APP) $(CFLAGS)	
	
clean:
	-rm -f *.o ; rm $(APP)
