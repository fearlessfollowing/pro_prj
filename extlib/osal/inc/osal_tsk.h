#ifndef _OSAL_TSK_H_
#define _OSAL_TSK_H_

#include <osal_thr.h>
#include <osal_mbx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct OSAL_TskHndl;

typedef int (*OSAL_TskFncMain)(struct OSAL_TskHndl *pTsk, OSAL_MsgHndl *pMsg, Uint32 curState );

typedef struct OSAL_TskHndl {
    OSAL_MbxHndl mbxHndl;       ///< Mailbox handle
    OSAL_ThrHndl thrHndl;       ///< OS thread handle
    Uint32 curState;            ///< Task state as defined by user
    OSAL_TskFncMain fncMain;    ///< Task Main, this function is entered when a message is received by the process
} OSAL_TskHndl;

int     OSAL_tskCreate(OSAL_TskHndl *pTsk, OSAL_TskFncMain fncMain, Uint32 tskPri, Uint32 tskStackSize, Uint32 initState);
int     OSAL_tskDelete(OSAL_TskHndl *pTsk);
int     OSAL_tskSendMsg(OSAL_TskHndl *pTskTo, OSAL_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int     OSAL_tskBroadcastMsg(OSAL_TskHndl *pTskToList[], OSAL_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int     OSAL_tskAckOrFreeMsg(OSAL_MsgHndl *pMsg, int ackRetVal);
int     OSAL_tskWaitMsg(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg);
int     OSAL_tskCheckMsg(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg);
int     OSAL_tskWaitCmd(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg, Uint16 waitCmd);
int     OSAL_tskFlushMsg(OSAL_TskHndl *pTsk);

int     OSAL_tskSetState(OSAL_TskHndl *pPrc, Uint32 curState);
Uint32  OSAL_tskGetState(OSAL_TskHndl *pPrc);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_TSK_H_ */




