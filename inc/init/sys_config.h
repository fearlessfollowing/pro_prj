#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_


#if defined(__cplusplus)
extern "C" {
#endif

#define UNUSED __attribute__((unused))


/* 属性文件的路径 */
#define PROP_PATH_RAMDISK_DEFAULT  	"/home/nvidia/insta360/etc/default.prop"
#define PROP_PATH_SYSTEM_BUILD     	"/home/nvidia/insta360/etc/build.prop"
#define PROP_PATH_SYSTEM_DEFAULT   	"/home/nvidia/insta360/etc/default.prop"
#define PROP_PATH_LOCAL_OVERRIDE   	"/home/nvidia/insta360/etc/local.prop"
#define PERSISTENT_PROPERTY_DIR  	"/home/nvidia/insta360/etc/property"	/* 固定属性的存储路径 */


#define PROPERTY_SPACE_PATH	"/run/.__properties__"


/* (8 header words + 247 toc words) = 1020 bytes */
/* 1024 bytes header and toc + 247 prop_infos @ 128 bytes = 32640 bytes */
	
#define PA_COUNT_MAX  247
#define PA_INFO_START 1024
#define PA_SIZE       32768

#ifndef PROP_NAME_MAX
#define PROP_NAME_MAX   32
#endif

#ifndef PROP_VALUE_MAX
#define PROP_VALUE_MAX  92
#endif

#ifndef TEMP_FAILURE_RETRY
	// Used to retry syscalls that can return EINTR.
#define TEMP_FAILURE_RETRY(exp) ({         \
		typeof (exp) _rc;					   \
		do {								   \
			_rc = (exp);					   \
		} while (_rc == -1 && errno == EINTR); \
		_rc; })
#endif


#if defined(__cplusplus)
}
#endif


#endif /* _SYS_CONFIG_H_ */
