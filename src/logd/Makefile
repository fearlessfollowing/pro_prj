# Makefile for logd
# create by skymixos 2018-06-11

CPPEXEC = logd
DIR = logd

TARGET_USES_LOGD=true

SOURCES := 

SOURCES += LogCommand.cpp 
SOURCES += CommandListener.cpp
SOURCES += LogListener.cpp
SOURCES += LogReader.cpp
SOURCES += FlushCommand.cpp
SOURCES += LogBuffer.cpp
SOURCES += LogBufferElement.cpp
SOURCES += LogTimes.cpp
SOURCES += LogStatistics.cpp
SOURCES += LogWhiteBlackList.cpp

SOURCES += main.cpp 

#CXXSRC +=event.logtags
SOURCES += $(LIBLOG)/log_time.cpp

SOURCES += $(LIBLOG)/logd_write.c 
SOURCES += $(LIBLOG)/log_event_write.c
SOURCES += $(LIBLOG)/logprint.c
SOURCES += $(LIBLOG)/event_tag_map.c
SOURCES += $(LIBLOG)/log_read.c


SOURCES += $(LIBCUTILS)/socket_local_server.c
SOURCES += $(LIBCUTILS)/socket_local_client.c
SOURCES += $(LIBCUTILS)/klog.c
SOURCES += $(LIBCUTILS)/uevent.c
SOURCES += $(LIBCUTILS)/system_properties.c

SOURCES += $(LIBSYSUTILS)/SocketListener.cpp
SOURCES += $(LIBSYSUTILS)/FrameworkListener.cpp
SOURCES += $(LIBSYSUTILS)/FrameworkCommand.cpp
SOURCES += $(LIBSYSUTILS)/SocketClient.cpp

SOURCES += $(LIBUTILS)/Static.cpp
SOURCES += $(LIBUTILS)/String8.cpp
SOURCES += $(LIBUTILS)/String16.cpp
SOURCES += $(LIBUTILS)/Unicode.cpp
SOURCES += $(LIBUTILS)/SharedBuffer.cpp


#INCLUDE_PATH = -I ../include
#INCLUDE_PATH += -I ../include/init
#INCLUDE_PATH += -I ./


TAGS_DIRS = $(NULL)



EXTRA_INC_PATH += -I./


include $(SDK_DIR)/common_defs.mk


#External library directories listed in EXTLIB_DIR variable
ifdef EXTLIB_DIR
#TARGET_CFLAGS += $(foreach path,$(EXTLIB_DIR),-I$(path)/include)
#TARGET_LDFLAGS += $(foreach path,$(EXTLIB_DIR),-L$(path)/lib)
endif


# Common standard libs
TARGET_LDLIBS = -lrt -lpthread 


.PHONY: all default install

all: $(CPPEXEC)

default: all

install: all
	$(INSTALL) $(BUILD)/$(CPPEXEC) 
	$(INSTALL_ROOT)/bin/$(CPPEXEC)	
	
include $(SDK_DIR)/common_cc.mk


