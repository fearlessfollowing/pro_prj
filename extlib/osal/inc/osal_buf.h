#ifndef _OSAL_BUF_H_
#define _OSAL_BUF_H_

#include <osal_que.h>
#include <osal_mutex.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_BUF_NUM_MAX       (8*4)

#define OSAL_BUF_ID_INVALID    (-1)

typedef struct {
	int 		size;
	int 		count;
	int 		flags;
	Uint32	timestamp;
	int 		width;
	int 		height;
	Uint32 	isKeyFrame;
	Uint32 	temporalId;
	Uint16 	codecType;
	void 		*physAddr;
	void 		*virtAddr;
} OSAL_BufInfo;

typedef struct {
    OSAL_BufInfo bufInfo[OSAL_BUF_NUM_MAX];
    OSAL_QueHndl emptyQue;
    OSAL_QueHndl fullQue;
      
    int numBuf;

} OSAL_BufHndl;

typedef struct {
    void *bufPhysAddr[OSAL_BUF_NUM_MAX];
    void *bufVirtAddr[OSAL_BUF_NUM_MAX];  
    int numBuf;
} OSAL_BufCreate;

int             OSAL_bufCreate(OSAL_BufHndl *hndl, OSAL_BufCreate *bufInit);
int             OSAL_bufDelete(OSAL_BufHndl *hndl);
int             OSAL_bufGetFull(OSAL_BufHndl *hndl, int *bufId, Uint32 timeout);
int             OSAL_bufPutEmpty(OSAL_BufHndl *hndl, int bufId);
int             OSAL_bufGetEmpty(OSAL_BufHndl *hndl, int *bufId, Uint32 timeout);
int             OSAL_bufPutFull(OSAL_BufHndl *hndl, int bufId);
int             OSAL_bufSwitchFull (OSAL_BufHndl *hndl, int *bufId);
int             OSAL_bufSwitchEmpty(OSAL_BufHndl *hndl, int *bufId);
OSAL_BufInfo*   OSAL_bufGetBufInfo(OSAL_BufHndl *hndl, int bufId);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_BUF_H_ */



