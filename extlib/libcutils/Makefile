# Makefile for libsysutils.so

TARGET = libcutils.so

WITH_MINGW :=

NETLINK_SUPOORT := false
SERVICE_MANAGER_SUPPORT := false

CSRC := socket_local_server.c
CSRC += socket_local_client.c
CSRC += klog.c
CSRC += uevent.c
CSRC += system_properties.c

CXXSRC := 

CFLAGS += -fPIC -O2 -g -Wall -Wno-unused-parameter
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS

LDFLAGS := -L ../liblog

INCLUDE_PATH = -I ../include
INCLUDE_PATH += -I ../include/init

LIBS = #-llog

TOOLCHAIN = aarch64-linux-gnu-
CC=$(TOOLCHAIN)gcc
CXX=$(TOOLCHAIN)g++

COBJS=$(CSRC:.c=.o)
CXXOBJS=$(CXXSRC:.cpp=.o)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH)


%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH)

all:$(TARGET)

$(TARGET): $(COBJS) $(CXXOBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $^ $(LIBS)
	@cp $(TARGET) /usr/local/lib
	@cp $(TARGET) ../out/
	@cp $(TARGET) ../out/update/pro_update/lib
	@ldconfig
clean:
	rm *.o $(TARGET)

	
