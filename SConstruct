import os
import platform
import sys
import config


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

Export('SDK_ROOT')
Export('config')

# 	LIBS = ['pthread', 'rt', 'ev', 'jsoncpp']

com_env = Environment(
	CC = config.CC, CCFLAGS = config.CFLAGS,
	CXX = config.CXX, CXXFLAGS = config.CXXFLAGS,
	LIBS = ['pthread', 'rt'],
	LINKFLAGS = config.LDFLAGS,
	CPPPATH = config.CPPPATH,
 	)

#com_env.Append(CCCOMSTR  ='CC <============================================ $SOURCES')
#com_env.Append(CXXCOMSTR ='CXX <=========================================== $SOURCES')
#com_env.Append(LINKCOMSTR='Link Target $SOURCES')


Export('com_env')

############################# Monitor ######################################
#monitor_obj = SConscript(config.MONITOR_SConscript)
monitor_obj = SConscript('./src/init/SConscript')
com_env.Program(target = './out/monitor', source = monitor_obj)


############################# adbd ######################################
#adbd_obj = SConscript('./src/adb/SConscript')
#com_env.Program(target = './out/adbd', source = adbd_obj)


############################# logd ######################################
#logd_obj = SConscript('./src/logd/Sconscript')
#com_env.Program(target = './out/logd', source = logd_obj)

############################# logcat ######################################
logcat_obj = SConscript('./src/logcat/Sconscript')
com_env.Program(target = './out/logcat', source = logcat_obj)


############################# log_prop_test ###############################
log_prop_obj = SConscript('./src/test/Sconscript')
com_env.Program(target = './out/log_test', source = log_prop_obj)

	


