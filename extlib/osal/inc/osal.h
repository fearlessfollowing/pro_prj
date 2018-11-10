#ifndef _OSAL_H_
#define _OSAL_H_

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_DEBUG_MODE         // enable SAL_printf, SAL_assert
#define OSAL_DEBUG_FILE         // enable printf's during SAL_fileXxxx
#define OSAL_PRF_ENABLE         // enable profiling APIs

#define OSAL_SOK      0         /// < Status : OK
#define OSAL_EFAIL   -1         /// < Status : Generic error

#ifndef xdc_std__include

#ifndef _TI_STD_TYPES
#define _TI_STD_TYPES

#ifndef TRUE

typedef short   Bool;                 ///< Boolean type

#define TRUE		((Bool) 1)        ///< Boolean value : TRUE
#define FALSE		((Bool) 0)        ///< Boolean value : FALSE

#endif

/* unsigned quantities */
typedef unsigned long long  Uint64;      ///< Unsigned 64-bit integer
typedef unsigned int        Uint32;            ///< Unsigned 32-bit integer
typedef unsigned short      Uint16;          ///< Unsigned 16-bit integer
typedef unsigned char       Uint8;            ///< Unsigned  8-bit integer

/* signed quantities */
typedef long long           Int64;               ///< Signed 64-bit integer
typedef int                 Int32;                     ///< Signed 32-bit integer
typedef short               Int16;                   ///< Signed 16-bit integer
typedef char                Int8;                     ///< Signed  8-bit integer

#endif /* _TI_STD_TYPES */

#endif

#ifndef KB
#define KB ((Uint32)1024)
#endif

#ifndef MB
#define MB (KB*KB)
#endif

#define OSAL_TIMEOUT_NONE        ((Uint32) 0)  // no timeout
#define OSAL_TIMEOUT_FOREVER     ((Uint32)-1)  // wait forever

#define OSAL_memAlloc(size)      (void*)malloc((size))
#define OSAL_memFree(ptr)        free(ptr)

#define OSAL_align(value, align)   ((( (value) + ( (align) - 1 ) ) / (align) ) * (align) )

#define OSAL_floor(value, align)   (( (value) / (align) ) * (align) )
#define OSAL_ceil(value, align)    OSAL_align(value, align) 

#define OSAL_max(a, b)    ( (a) > (b) ? (a) : (b) )
#define OSAL_min(a, b)    ( (a) < (b) ? (a) : (b) )

#include <osal_debug.h>
#include <osal_prf.h>


// from SERVER Main to other TSKs
#define SERVER_CMD_CREATE    	(0x300)
#define SERVER_CMD_DELETE    	(0x301)
#define SERVER_CMD_START     	(0x302)
#define SERVER_CMD_NEW_DATA  	(0x303)

// from UI thread to SERVER Main
#define SERVER_MAIN_CMD_START  (0x400)
#define SERVER_MAIN_CMD_STOP   (0x401)

// SERVER Main State's
#define SERVER_MAIN_STATE_IDLE       (0)
#define SERVER_MAIN_STATE_RUNNING    (1)


Uint32 OSAL_getCurTimeInMsec();
void   OSAL_waitMsecs(Uint32 msecs);
int    OSAL_attachSignalHandler(int sigId, void (*handler)(int ) );
int    OSAL_getHostName(char *hostname, int maxHostNameLen);

int xstrtoi(char *hex);

int OSAL_init();
int OSAL_exit();


#if defined(__cplusplus)
}
#endif

#endif  /* _OSAL_H_ */



