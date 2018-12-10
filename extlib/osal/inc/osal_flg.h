#ifndef _OSAL_FLG_H_
#define _OSAL_FLG_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	Uint32 pattern;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
} OSAL_FlgHndl;

// flags
#define OSAL_FLG_MODE_AND       0x1
#define OSAL_FLG_MODE_OR        0x2
#define OSAL_FLG_MODE_CLR       0x4

#define OSAL_FLG_MODE_AND_CLR   (OSAL_FLG_MODE_AND|OSAL_FLG_MODE_CLR)
#define OSAL_FLG_MODE_OR_CLR    (OSAL_FLG_MODE_OR |OSAL_FLG_MODE_CLR)

int  OSAL_flgCreate(OSAL_FlgHndl *hndl, Uint32 initPattern);
int  OSAL_flgWait(OSAL_FlgHndl *hndl, Uint32 pattern, Uint32 mode, Uint32 timeout);
int  OSAL_flgSet(OSAL_FlgHndl *hndl, Uint32 pattern);
int  OSAL_flgClear(OSAL_FlgHndl *hndl, Uint32 pattern);
Bool OSAL_flgIsSet(OSAL_FlgHndl *hndl, Uint32 pattern);
int  OSAL_flgDelete(OSAL_FlgHndl *hndl);

#if defined(__cplusplus)
}
#endif

#endif /* _SAL_FLG_H_ */
