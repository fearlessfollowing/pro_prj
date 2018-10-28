#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <poll.h>

#include <sys/mman.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/syscall.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <_system_properties.h>

#include <_futex.h>



//#include <sys/atomics.h>

static const char property_service_socket[] = "/dev/socket/"PROP_SERVICE_NAME;

static unsigned dummy_props = 0;

prop_area *__system_property_area__ = (void*) &dummy_props;


/*
 * __system_properties_init - 所有需要加入属性系统的进程必须先调用
 * 此函数进程初始化
 */
int __system_properties_init(void)
{
    prop_area *pa;
    int fd;
    unsigned sz;
    //char *env;

    if (__system_property_area__ != ((void*) &dummy_props)) {
        return 0;
    }

	/* 为了让非monitor进程启动的子进程都能使用属性系统
	 * 将不使用环境变量的方式,而是使用访问属性文件的方式
	 */
	#if 0
    env = getenv("ANDROID_PROPERTY_WORKSPACE");
    if (!env) {
        return -1;
    }
	
    fd = atoi(env);
    env = strchr(env, ',');
    if (!env) {
        return -1;
    }
    sz = atoi(env + 1);
	#else
	
    fd = open(PROPERTY_SPACE_PATH, O_RDONLY);	/* 以只读的方式打开,这样进程就没有直接修改属性的权限了 */
    if (fd < 0)
        return -1;
	
	sz = PA_SIZE;	/* 属性区域的大小固定 */

	#endif

	
    pa = mmap(0, sz, PROT_READ, MAP_SHARED, fd, 0);
    if (pa == MAP_FAILED) {
        return -1;
    }

    if ((pa->magic != PROP_AREA_MAGIC) || (pa->version != PROP_AREA_VERSION)) {
        munmap(pa, sz);
        return -1;
    }

	// printf("prop_area.magic = 0x%x, prop_area.version = 0x%x\n", PROP_AREA_MAGIC, PROP_AREA_VERSION);
	
    __system_property_area__ = pa;
    return 0;
}


const prop_info *__system_property_find_nth(unsigned n)
{
    prop_area *pa = __system_property_area__;

    if(n >= pa->count) {
        return 0;
    } else {
        return TOC_TO_INFO(pa, pa->toc[n]);
    }
}

const prop_info *__system_property_find(const char *name)
{
    prop_area *pa = __system_property_area__;
    unsigned count = pa->count;
    unsigned *toc = pa->toc;
    unsigned len = strlen(name);
    prop_info *pi;

    while (count--) {
        unsigned entry = *toc++;
        if (TOC_NAME_LEN(entry) != len) continue;
        
        pi = TOC_TO_INFO(pa, entry);
        if (memcmp(name, pi->name, len)) continue;

        return pi;
    }

    return 0;
}

int __system_property_read(const prop_info *pi, char *name, char *value)
{
    unsigned serial, len;
    
    for(;;) {
        serial = pi->serial;
        while(SERIAL_DIRTY(serial)) {
            __futex_wait((volatile void *)&pi->serial, serial, 0);
            serial = pi->serial;
        }
        len = SERIAL_VALUE_LEN(serial);
        memcpy(value, pi->value, len + 1);
        if (serial == pi->serial) {
            if(name != 0) {
                strcpy(name, pi->name);
            }
            return len;
        }
    }
}



const char* property_get(const char *name)
{
    const prop_info *pi = __system_property_find(name);

    if (pi != 0) {
    	printf("prop: %s exist\n", name);
        return pi->value;
    } else {
    	printf("prop: %s not exist\n", name);
        return 0;
    }
}


static int send_prop_msg(prop_msg *msg)
{
    struct pollfd pollfds[1];
    struct sockaddr_un addr;
    socklen_t alen;
    size_t namelen;
    int s;
    int r;
    int result = -1;

    s = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(s < 0) {
        return result;
    }

    memset(&addr, 0, sizeof(addr));
    namelen = strlen(property_service_socket);
    strncpy(addr.sun_path, property_service_socket, sizeof addr.sun_path);
    addr.sun_family = AF_LOCAL;
    alen = namelen + offsetof(struct sockaddr_un, sun_path) + 1;

    if (TEMP_FAILURE_RETRY(connect(s, (struct sockaddr *) &addr, alen)) < 0) {
        close(s);
        return result;
    }

    r = TEMP_FAILURE_RETRY(send(s, msg, sizeof(prop_msg), 0));

    if (r == sizeof(prop_msg)) {
        // We successfully wrote to the property server but now we
        // wait for the property server to finish its work.  It
        // acknowledges its completion by closing the socket so we
        // poll here (on nothing), waiting for the socket to close.
        // If you 'adb shell setprop foo bar' you'll see the POLLHUP
        // once the socket closes.  Out of paranoia we cap our poll
        // at 250 ms.
        pollfds[0].fd = s;
        pollfds[0].events = 0;
        r = TEMP_FAILURE_RETRY(poll(pollfds, 1, 250 /* ms */));
        if (r == 1 && (pollfds[0].revents & POLLHUP) != 0) {
            result = 0;
        } else {
            // Ignore the timeout and treat it like a success anyway.
            // The init process is single-threaded and its property
            // service is sometimes slow to respond (perhaps it's off
            // starting a child process or something) and thus this
            // times out and the caller thinks it failed, even though
            // it's still getting around to it.  So we fake it here,
            // mostly for ctl.* properties, but we do try and wait 250
            // ms so callers who do read-after-write can reliably see
            // what they've written.  Most of the time.
            // TODO: fix the system properties design.
            result = 0;
        }
    }

    close(s);
    return result;
}

int property_set(const char *key, const char *value)
{
    int err;
    prop_msg msg;

    if (key == 0) return -1;
    if (value == 0) value = "";
    if (strlen(key) >= PROP_NAME_MAX) return -1;
    if (strlen(value) >= PROP_VALUE_MAX) return -1;

    memset(&msg, 0, sizeof msg);
    msg.cmd = PROP_MSG_SETPROP;
    strncpy(msg.name, key, sizeof msg.name);
    strncpy(msg.value, value, sizeof msg.value);

	printf("property_set: key[%s], value [%s] \n", key, value);

    err = send_prop_msg(&msg);
    if (err < 0) {
        return err;
    }

    return 0;
}

int __system_property_wait(const prop_info *pi)
{
    unsigned n;
    if(pi == 0) {
        prop_area *pa = __system_property_area__;
        n = pa->serial;
        do {
            __futex_wait(&pa->serial, n, 0);
        } while(n == pa->serial);
    } else {
        n = pi->serial;
        do {
            __futex_wait((volatile void *)&pi->serial, n, 0);
        } while(n == pi->serial);
    }
    return 0;
}
