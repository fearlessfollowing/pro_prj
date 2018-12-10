
#include <osal_tsk.h>

void *OSAL_tskThrMain(void *pPrm)
{
	OSAL_MsgHndl *pMsg;
	OSAL_TskHndl *pPrc;

	pPrc = (OSAL_TskHndl *)pPrm;
	OSAL_assert(pPrc!=NULL);

	while (1) {
		OSAL_assertSuccess( OSAL_tskWaitMsg(pPrc, &pMsg) );
		OSAL_assert(pPrc->fncMain != NULL);
		pPrc->fncMain(pPrc, pMsg, pPrc->curState);
	}

	return NULL;
} 

int OSAL_tskCreate(OSAL_TskHndl *pPrc, OSAL_TskFncMain fncMain, Uint32 tskPri, Uint32 tskStackSize, Uint32 initState)
{
	pPrc->curState      = initState;
	pPrc->fncMain       = fncMain;

	OSAL_assert(pPrc->fncMain != NULL);
	OSAL_mbxCreate(&pPrc->mbxHndl);
	OSAL_assertSuccess(OSAL_thrCreate(&pPrc->thrHndl, OSAL_tskThrMain, tskPri, tskStackSize,  pPrc));

	return OSAL_SOK;
}

int OSAL_tskDelete(OSAL_TskHndl *pPrc)
{
	OSAL_thrDelete(&pPrc->thrHndl);
	OSAL_mbxDelete(&pPrc->mbxHndl);

	pPrc->curState      = 0;
	pPrc->fncMain       = NULL;
	
	return OSAL_SOK;
}

int OSAL_tskSendMsg(OSAL_TskHndl *pPrcTo, OSAL_TskHndl *pPrcFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	int retVal;
	OSAL_MbxHndl *pMbxFrom;

	OSAL_assert(pPrcTo != NULL);
	if (pPrcFrom == NULL) {
		pMbxFrom = NULL;
	} else {
		pMbxFrom = &pPrcFrom->mbxHndl;
	}

	retVal = OSAL_mbxSendMsg(&pPrcTo->mbxHndl, pMbxFrom, cmd, pPrm, flags);
	return retVal;
}

int OSAL_tskBroadcastMsg(OSAL_TskHndl *pPrcToList[], OSAL_TskHndl *pPrcFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	int retVal;

	OSAL_MbxHndl *pMbxToList[OSAL_MBX_BROADCAST_MAX];
	Uint32 i, numMsg;
	OSAL_MbxHndl *pMbxFrom;

	OSAL_assert(pPrcToList!=NULL);

	if (pPrcFrom == NULL) {
		pMbxFrom = NULL;
	} else {
		pMbxFrom = &pPrcFrom->mbxHndl;
	}

	for (i = 0; i < OSAL_MBX_BROADCAST_MAX; i++) {
		pMbxToList[i] = NULL;
	}

	numMsg = 0;
	while (pPrcToList[numMsg] != NULL) {
		pMbxToList[numMsg] = &pPrcToList[numMsg]->mbxHndl;
		numMsg++;
		if (numMsg >= OSAL_MBX_BROADCAST_MAX) {
	  		// cannot broadcast to more than SAL_mbx_BROADCAST_MAX mailboxes
			OSAL_assert(0);
		}
	}

	if (numMsg == 0) {
		// no mailboxes in 'to' mailbox list
	 	return OSAL_SOK;
	}

	retVal = OSAL_mbxBroadcastMsg(&pMbxToList[0], pMbxFrom, cmd, pPrm, flags);
	return retVal;
}


int OSAL_tskWaitMsg(OSAL_TskHndl *pPrc, OSAL_MsgHndl **pMsg)
{
	int retVal;
	retVal = OSAL_mbxWaitMsg(&pPrc->mbxHndl, pMsg);
	return retVal;
}

int OSAL_tskCheckMsg(OSAL_TskHndl *pPrc, OSAL_MsgHndl **pMsg)
{
	int retVal;
	retVal = OSAL_mbxCheckMsg(&pPrc->mbxHndl, pMsg);
	return retVal;
}



int OSAL_tskAckOrFreeMsg(OSAL_MsgHndl *pMsg, int ackRetVal)
{
	int retVal;
	retVal = OSAL_mbxAckOrFreeMsg(pMsg, ackRetVal);
	return retVal;
}


int OSAL_tskFlushMsg(OSAL_TskHndl *pPrc)
{
	int retVal;
	retVal = OSAL_mbxFlush(&pPrc->mbxHndl);
	return retVal;
}


int OSAL_tskWaitCmd(OSAL_TskHndl *pPrc, OSAL_MsgHndl **pMsg, Uint16 waitCmd)
{
	int retVal;
	retVal = OSAL_mbxWaitCmd(&pPrc->mbxHndl, pMsg, waitCmd);
	return retVal;
}


int OSAL_tskSetState(OSAL_TskHndl *pPrc, Uint32 curState)
{
	pPrc->curState = curState;
	return OSAL_SOK;
}


Uint32 OSAL_tskGetState(OSAL_TskHndl *pPrc)
{
	return pPrc->curState;
}

