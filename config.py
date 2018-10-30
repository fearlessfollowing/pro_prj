# -*- coding: utf-8 -*-  
###########################################################################################################
# File: config.py
# Author: skymixos
# Created: 2018-07-05
# Descriptor: Defined some Enviorn var and Control Options
# Version: V1.0
##########################################################################################################
import os
import platform


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

SDK_ROOT += '/'
SDK_BIN_OUT = SDK_ROOT + 'out'

MONITOR_DIR         = SDK_ROOT + 'src/init'
MONITOR_SConscript  = MONITOR_DIR + '/Sconscript'
MONITOR_TARGET      = SDK_BIN_OUT + '/monitor'

LOGD_DIR        = SDK_ROOT + 'src/logd'
LOGCAT_DIR      = SDK_ROOT + 'src/logcat'
PROP_TOOLS      = SDK_ROOT + 'src/prop_tools'
EXTLIBS         = SDK_ROOT + 'extlib/'


LIBCUTILS       = EXTLIBS + 'libcutils'
LIBLOG          = EXTLIBS + 'liblog'
LIBSYSUTILS     = EXTLIBS + 'libsysutils'
LIBUTILS        = EXTLIBS + 'libutils'
LIBEV           = EXTLIBS + 'libev-master'
LIBOSAL         = EXTLIBS + 'osal'


# INC_PATH
INC_PATH = []
INC_PATH += [SDK_ROOT + 'inc']
INC_PATH += [SDK_ROOT + 'inc/init']
INC_PATH += [SDK_ROOT + 'extlib/install/include']

INC_PATH += [SDK_ROOT + 'extlib/osal/inc']


# Common flags
COM_FLAGS = ''
COM_FLAGS += ' -DHAVE_SYS_UIO_H -DHAVE_PTHREADS -DHAVE_ANDROID_OS '


#COM_FLAGS += $(EXTRA_INC_PATH)


COM_FLAGS += ' -fexceptions -Wall -Wunused-variable '



# toolchains options
HW_PLATFORM = platform.machine()

#------- toolchains path -------------------------------------------------------

#BUILD = 'debug'
BUILD = 'release'


TARGET_MONIOTR_NAME = 'monitor'

#------- GCC settings ----------------------------------------------------------
if HW_PLATFORM == 'gcc':
    # toolchains
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'axf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'


    CFLAGS = ''
    #LFLAGS = DEVICE
    #LFLAGS += ' -Wl,--gc-sections,-cref,-Map=' + MAP_FILE
    #LFLAGS += ' -T ' + LINK_FILE + '.ld'

    CPATH = ''
    LPATH = ''



    if BUILD == 'debug':
    	CFLAGS += ' -O0 -g'
    else:
        CFLAGS += ' -O2'

elif HW_PLATFORM == 'aarch64':
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    CFLAGS = COM_FLAGS
    CXXFLAGS = COM_FLAGS
    CXXFLAGS += ' -std=c++11 -frtti '
    CPPPATH = INC_PATH
    LDFLAGS = ''
    CPATH = ''
    LPATH = ''

elif HW_PLATFORM == 'x86_64':
    # toolchains
    PREFIX = '/opt/hisi-linux/x86-arm/arm-hisiv400-linux/bin/arm-hisiv400-linux-gnueabi-' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    CFLAGS = COM_FLAGS
    CXXFLAGS = COM_FLAGS
    CXXFLAGS += ' -std=c++11 -frtti '
    CPPPATH = INC_PATH
    LDFLAGS = ''
    CPATH = ''
    LPATH = ''


#    if BUILD == 'debug':
#        CFLAGS += ' -O0 -g'
#    else:
#        CFLAGS += ' -O2'


