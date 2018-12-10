# Makefile for liblog.so

TARGET = liblog.so

TARGET_USES_LOGD=true
WITH_MINGW :=

CSRC :=
CXXSRC :=

ifneq ($(TARGET_USES_LOGD), false)
CSRC += logd_write.c log_event_write.c
else
CSRC += logd_write_kern.c
endif

ifndef WITH_MINIGW
CSRC += logprint.c
CSRC += event_tag_map.c
else
CSRC += uio.c
endif

CXXSRC += log_time.cpp

ifneq ($(TARGET_USES_LOGD), false)
CSRC += log_read.c
else
CSRC += log_read_kern.c
endif

CFLAGS += -fPIC -O2 -g -Wall -Wno-unused-parameter
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS

LDFLAGS :=

INCLUDE_PATH = -I ../include

LIBS = -lrt -lpthread

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

	
