#ifndef _INCLUDE_SYS_SYSTEM_PROPERTIES_H
#define _INCLUDE_SYS_SYSTEM_PROPERTIES_H

 
#if defined(__cplusplus)
extern "C" {
#endif

#include <sys_config.h>

typedef struct prop_info prop_info;


int __system_properties_init(void);
const char* property_get(const char *name);
int property_set(const char *key, const char *value);
const prop_info *__system_property_find(const char *name);
int __system_property_read(const prop_info *pi, char *name, char *value);
const prop_info *__system_property_find_nth(unsigned n);

#if defined(__cplusplus)
}
#endif

#endif
