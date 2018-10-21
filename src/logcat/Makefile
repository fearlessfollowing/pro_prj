# monitor Makefile
# Create by skyway 20161216

CPPEXEC = logcat
DIR = logcat

SOURCES := 

SOURCES += $(LIBLOG)/logd_write.c
SOURCES += $(LIBLOG)/log_event_write.c
SOURCES += logcat.cpp


ifndef WITH_MINIGW
SOURCES += $(LIBLOG)/logprint.c
SOURCES += $(LIBLOG)/event_tag_map.c
else
SOURCES += $(LIBLOG)/uio.c
endif

SOURCES += $(LIBLOG)/log_read.c
SOURCES += $(LIBLOG)/log_time.cpp

TAGS_DIRS = $(NULL)



TARGET_CFLAGS += -I ../include -I../include/init -I./ -I./include


include $(SDK_DIR)/common_defs.mk


#External library directories listed in EXTLIB_DIR variable
ifdef EXTLIB_DIR
#TARGET_CFLAGS += $(foreach path,$(EXTLIB_DIR),-I$(path)/include)
#TARGET_LDFLAGS += $(foreach path,$(EXTLIB_DIR),-L$(path)/lib)
endif


# Common standard libs
TARGET_LDLIBS = -lrt


.PHONY: all default install

all: $(CPPEXEC)

default: all

install: all
	$(INSTALL) $(BUILD)/$(CPPEXEC) 
	$(INSTALL_ROOT)/bin/$(CPPEXEC)	
	
include $(SDK_DIR)/common_cc.mk

