#ifndef _ANDROID_CUTILS_LOGD_H
#define _ANDROID_CUTILS_LOGD_H

/* the stable/frozen log-related definitions have been
 * moved to this header, which is exposed by the NDK
 */
#include <android/log.h>

/* the rest is only used internally by the system */
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <log/uio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int __android_log_bwrite(int32_t tag, const void *payload, size_t len);
int __android_log_btwrite(int32_t tag, char type, const void *payload,
    size_t len);
int __android_log_bswrite(int32_t tag, const char *payload);

#ifdef __cplusplus
}
#endif

#endif /* _LOGD_H */
