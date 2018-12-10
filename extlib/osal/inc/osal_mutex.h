#ifndef _OSAL_MUTEX_H_
#define _OSAL_MUTEX_H_

#include <pthread.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	pthread_mutex_t lock;
} OSAL_MutexHndl;

#include <osal.h>


int     OSAL_mutexCreate(OSAL_MutexHndl *hndl);
int     OSAL_mutexDelete(OSAL_MutexHndl *hndl);
int     OSAL_mutexLock(OSAL_MutexHndl *hndl);
int     OSAL_mutexUnlock(OSAL_MutexHndl *hndl);

#if defined(__cplusplus)
}
#endif

#endif /* _OSAL_MUTEX_H_ */



