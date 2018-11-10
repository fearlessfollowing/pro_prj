#ifndef _OSAL_MBX_H_
#define _OSAL_MBX_H_

#include <osal_msgq.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_MBX_WAIT_ACK      0x0002   ///< Message Flag : Wait for ACK
#define OSAL_MBX_FREE_PRM      0x0004   ///< Message Flag : Message parameters are malloc'ed and need to be free'ed

#define OSAL_MBX_BROADCAST_MAX     10   ///< Max message queues/PRC's that can be broadcast to

/**
  \brief Mailbox handle
*/
typedef struct {
    OSAL_MsgqHndl rcvMbx;    ///< Receive mailbox
	OSAL_MsgqHndl ackMbx;    ///< ACK mailbox
} OSAL_MbxHndl;


int OSAL_mbxCreate(OSAL_MbxHndl *pHndl);
int OSAL_mbxDelete(OSAL_MbxHndl *pHndl);
int OSAL_mbxSendMsg(OSAL_MbxHndl *pTo, OSAL_MbxHndl *pFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int OSAL_mbxBroadcastMsg(OSAL_MbxHndl *pToList[], OSAL_MbxHndl *pFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int OSAL_mbxAckOrFreeMsg(OSAL_MsgHndl *pMsg, int ackRetVal);
int OSAL_mbxWaitMsg(OSAL_MbxHndl *pHndl, OSAL_MsgHndl **pMsg);
int OSAL_mbxCheckMsg(OSAL_MbxHndl *pHndl, OSAL_MsgHndl **pMsg);
int OSAL_mbxWaitCmd(OSAL_MbxHndl *pHndl, OSAL_MsgHndl **pMsg, Uint16 waitCmd);
int OSAL_mbxFlush(OSAL_MbxHndl *pHndl);


#if defined(__cplusplus)
}
#endif


#endif /* _OSAL_MBX_H_ */



