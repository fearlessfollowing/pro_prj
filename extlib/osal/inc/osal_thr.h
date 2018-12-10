#ifndef _OSAL_THR_H_
#define _OSAL_THR_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_THR_PRI_MAX                 sched_get_priority_max(SCHED_FIFO)
#define OSAL_THR_PRI_MIN                 sched_get_priority_min(SCHED_FIFO)

#define OSAL_THR_PRI_DEFAULT             ( OSAL_THR_PRI_MIN + (OSAL_THR_PRI_MAX-OSAL_THR_PRI_MIN)/2 )

#define OSAL_THR_STACK_SIZE_DEFAULT      0

typedef void * (*OSAL_ThrEntryFunc)(void *);

typedef struct {
	pthread_t hndl;
} OSAL_ThrHndl;

int     OSAL_thrCreate(OSAL_ThrHndl *hndl, OSAL_ThrEntryFunc entryFunc, Uint32 pri, Uint32 stackSize, void *prm);
int     OSAL_thrDelete(OSAL_ThrHndl *hndl);
int     OSAL_thrJoin(OSAL_ThrHndl *hndl);
int     OSAL_thrChangePri(OSAL_ThrHndl *hndl, Uint32 pri);
int     OSAL_thrExit(void *returnVal);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_THR_H_ */



