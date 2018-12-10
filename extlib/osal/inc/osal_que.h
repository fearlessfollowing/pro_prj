#ifndef _OSAL_QUE_H_
#define _OSAL_QUE_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {

	Uint32 curRd;
	Uint32 curWr;
	Uint32 len;
	Uint32 count;

	Int32 *queue;

	pthread_mutex_t lock;
	pthread_cond_t  condRd;
	pthread_cond_t  condWr;
  
} OSAL_QueHndl;

int     OSAL_queCreate(OSAL_QueHndl *hndl, Uint32 maxLen);
int     OSAL_queDelete(OSAL_QueHndl *hndl);
int     OSAL_quePut(OSAL_QueHndl *hndl, Int32 value, Uint32 timeout);
int     OSAL_queGet(OSAL_QueHndl *hndl, Int32 *value, Uint32 timeout);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_QUE_H_ */



