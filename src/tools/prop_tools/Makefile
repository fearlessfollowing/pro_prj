# Makefile for setprop getprop

arm-hisiv400-linux-gcc -o setprop setprop.c system_properties.c -I ../../../inc/init
arm-hisiv400-linux-gcc -o getprop getprop.c system_properties.c -I ../../../inc/init

all: setprop getprop

TOOLCHAIN = arm-hisiv400-linux-
CC=$(TOOLCHAIN)gcc
CXX=$(TOOLCHAIN)g++

CFLAGS += -fPIC -O2 -g -Wall -Wno-unused-parameter
CFLAGS += -D__LP64__ -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS

LIBS = 


INCLUDE_PATH = -I ../include
INCLUDE_PATH += -I ../../../inc/init
INCLUDE_PATH += -I ./
INCLUDE_PATH += -I ./include

setprop: system_properties.c setprop.c
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATH)  $(LIBS)

getprop: system_properties.c  getprop.c 
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATH)  $(LIBS)

clean:
	rm -f *.o




