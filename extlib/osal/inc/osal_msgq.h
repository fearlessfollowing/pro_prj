#ifndef _OSAL_MSGQ_H_
#define _OSAL_MSGQ_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_MSGQ_LEN_MAX    32

struct OSAL_MsgHndl;

typedef struct {

	Uint32 curRd;
	Uint32 curWr;
	Uint32 len;
	Uint32 count;

	struct OSAL_MsgHndl *queue[OSAL_MSGQ_LEN_MAX];

	pthread_mutex_t lock;
	pthread_cond_t  condRd;
	pthread_cond_t  condWr;
  
} OSAL_MsgqHndl;

typedef struct OSAL_MsgHndl {

	OSAL_MsgqHndl *pTo;
	OSAL_MsgqHndl *pFrom;  
	void         *pPrm;
	int           status;
	Uint16        cmd;
	Uint16        flags;

} OSAL_MsgHndl;

#define OSAL_msgGetCmd(msg)         ( (msg)->cmd )
#define OSAL_msgGetPrm(msg)         ( (msg)->pPrm )
#define OSAL_msgGetAckStatus(msg)   ( (msg)->status )

int     OSAL_msgqCreate(OSAL_MsgqHndl *hndl);
int     OSAL_msgqDelete(OSAL_MsgqHndl *hndl);
int     OSAL_msgqSendMsg(OSAL_MsgqHndl *to, OSAL_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags, OSAL_MsgHndl **msg);
int     OSAL_msgqRecvMsg(OSAL_MsgqHndl *hndl, OSAL_MsgHndl **msg, Uint32 timeout);
int     OSAL_msgqSendAck(OSAL_MsgHndl *msg, int ackRetVal);
int     OSAL_msgqFreeMsgHndl(OSAL_MsgHndl *msg);

#if defined(__cplusplus)
}
#endif

#endif /* _SAL_FLG_H_ */



