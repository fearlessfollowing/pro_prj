
#ifndef _OSAL_DEBUG_H_
#define _OSAL_DEBUG_H_

#include <stdio.h>
#include <osal_mutex.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern OSAL_MutexHndl gSAL_printfLock;
           
// printf wrapper that can be turned on and off by defining/undefining
#ifdef OSAL_DEBUG_MODE
           
#define OSAL_printf(...)    \
{                           \
    OSAL_mutexLock(&gSAL_printfLock); \
    printf(__VA_ARGS__); \
    OSAL_mutexUnlock(&gSAL_printfLock); \
}

#define OSAL_assert(x)  \
    if( (x) == 0) { \
        fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
        while (getchar()!='q'); \
    } 

#define OSAL_DEBUG \
    fprintf(stderr, " %s:%s:%d Press Any key to continue !!!", __FILE__, __func__, __LINE__); 


#define OSAL_DEBUG_WAIT \
    OSAL_DEBUG \
    getchar();
            
#else
    #define OSAL_printf(...)
    #define OSAL_assert(x)
    #define OSAL_DEBUG    
    #define OSAL_DEBUG_WAIT  
#endif

// printf wrapper that can be used to display errors. Prefixes all text with
// "ERROR" and inserts the file and line number of where in the code it was
// called
#define OSAL_ERROR(...) \
    do { \
        fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
    } while(0);

#define OSAL_assertSuccess(ret)  OSAL_assert(ret==OSAL_SOK)


#if defined(__cplusplus)
}
#endif


#endif  //_SAL_DEBUG_H_



