#ifndef _OSAL_SEM_H_
#define _OSAL_SEM_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct {
	Uint32 count;
	Uint32 maxCount;
	pthread_mutex_t lock;
	pthread_cond_t  cond;

} OSAL_SemHndl;

int     OSAL_semCreate(OSAL_SemHndl *hndl, Uint32 maxCount, Uint32 initVal);
int     OSAL_semWait(OSAL_SemHndl *hndl, Uint32 timeout);
int     OSAL_semSignal(OSAL_SemHndl *hndl);
int     OSAL_semDelete(OSAL_SemHndl *hndl);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_FLG_H_ */



