# Makefile for libsysutils.so

TARGET = libutils.so

WITH_MINGW :=

NETLINK_SUPOORT := false
SERVICE_MANAGER_SUPPORT := false

CSRC :=
CXXSRC := Static.cpp
CXXSRC += String8.cpp
CXXSRC += String16.cpp
CXXSRC += Unicode.cpp
CXXSRC += SharedBuffer.cpp
#CXXSRC += RefBase.cpp
#CXXSRC += VectorImpl.cpp

CFLAGS += -fPIC -O2 -g -Wall -Wno-unused-parameter
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CFLAGS += -DPLATFORM_X86_64
LDFLAGS := -L ../liblog

INCLUDE_PATH = -I ../include

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

	
