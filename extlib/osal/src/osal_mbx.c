#include <osal_mbx.h>


int OSAL_mbxCreate(OSAL_MbxHndl *pMbxHndl)
{
	int status = OSAL_SOK;

	status |= OSAL_msgqCreate(&pMbxHndl->rcvMbx);
	status |= OSAL_msgqCreate(&pMbxHndl->ackMbx);

	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_mbxCreate() = %d \r\n", status);

	return status;
}

int OSAL_mbxDelete(OSAL_MbxHndl *pMbxHndl)
{
	OSAL_msgqDelete(&pMbxHndl->rcvMbx);
	OSAL_msgqDelete(&pMbxHndl->ackMbx);

	return OSAL_SOK;
}

int OSAL_mbxFreeMsg(OSAL_MsgHndl *pMsg)
{
	if (pMsg->flags & OSAL_MBX_FREE_PRM) {
		if (pMsg->pPrm != NULL) {
	  		OSAL_memFree(pMsg->pPrm);
		}
	}

	OSAL_assertSuccess(OSAL_msgqFreeMsgHndl(pMsg));
	return OSAL_SOK;
}

int OSAL_mbxSendMsg(OSAL_MbxHndl *pMbxTo, OSAL_MbxHndl *pMbxFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	OSAL_MsgHndl *pSentMsg, *pRcvMsg;
	Bool waitAck;
	int retVal = OSAL_SOK;
	OSAL_MsgqHndl *ackMbx;

	OSAL_assert(pMbxTo != NULL);

	if (pMbxFrom == NULL) {
		if (flags & OSAL_MBX_WAIT_ACK) {
			// if from mail box is NULL, then cannot wait for ACK
			OSAL_assert(0);
		}
		ackMbx = NULL;
	} else {
		ackMbx = &pMbxFrom->ackMbx;
	}

	retVal = OSAL_msgqSendMsg(&pMbxTo->rcvMbx, ackMbx, cmd, pPrm, flags, &pSentMsg);
	OSAL_assertSuccess(retVal);

	if ((flags & OSAL_MBX_WAIT_ACK) && ackMbx != NULL) {
	    waitAck = TRUE;
	    do {
      		// wait for ACK
      		retVal = OSAL_msgqRecvMsg(ackMbx, &pRcvMsg, OSAL_TIMEOUT_FOREVER);
      		OSAL_assertSuccess(retVal);

      		if (pRcvMsg == pSentMsg) {
        		// ACK received for sent MSG
        		waitAck = FALSE;
        		retVal = OSAL_msgGetAckStatus(pRcvMsg);
      		}
     	 	OSAL_mbxFreeMsg(pRcvMsg);
    	} while (waitAck);
  	}

  	return retVal;
}

int OSAL_mbxBroadcastMsg(OSAL_MbxHndl *pMbxToList[], OSAL_MbxHndl *pMbxFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	OSAL_MsgHndl *pSentMsgList[OSAL_MBX_BROADCAST_MAX];
	Bool  ackList[OSAL_MBX_BROADCAST_MAX];
	OSAL_MsgHndl *pRcvMsg;
	Bool waitAck, unknownAck;
	Uint32 i, numMsg;
	int retVal = OSAL_SOK, ackRetVal = OSAL_SOK;
	OSAL_MsgqHndl *ackMbx;

  	OSAL_assert(pMbxToList != NULL);

  	if (pMbxFrom == NULL) {
    	if (flags & OSAL_MBX_WAIT_ACK) {
      		// if from mail box is NULL, then cannot wait for ACK
      		OSAL_assert(0);
    	}
    	ackMbx = NULL;
  	} else {
    	ackMbx = &pMbxFrom->ackMbx;
  	}

   	// count number of mailboxes to boadcast to
   	numMsg = 0;
   	while (pMbxToList[numMsg] != NULL) {
		numMsg++;
     	if (numMsg >= OSAL_MBX_BROADCAST_MAX) {
        	// cannot broadcast to more than SAL_MBX_BROADCAST_MAX mailboxes
        	OSAL_assert(0);
     	}
   	}

   	if (numMsg == 0) {
		// no mailboxes in 'to' mailbox list
		return OSAL_SOK;
   	}

   	// Cannot broadcast with malloc'ed prm and not wait for ACK
   	if (flags & OSAL_MBX_FREE_PRM) {
		if (!(flags & OSAL_MBX_WAIT_ACK) && numMsg > 1) {
       		OSAL_assert(0);
     	}
   	}

   	// reset sent MSG list and ACK list
   	for (i = 0; i < OSAL_MBX_BROADCAST_MAX; i++) {
		ackList[i]  = FALSE;
		pSentMsgList[i] = NULL;
   	}

   	// send MSG to all mailboxes
   	for (i = 0; i < numMsg; i++) {
		retVal = OSAL_msgqSendMsg(&pMbxToList[i]->rcvMbx, ackMbx, cmd, pPrm, flags, &pSentMsgList[i]);
      	OSAL_assertSuccess(retVal);
   	}

   	if ((flags & OSAL_MBX_WAIT_ACK) && ackMbx != NULL) {
		// wait for ACKs
      	do {
        	// wait ACK
        	retVal = OSAL_msgqRecvMsg(ackMbx, &pRcvMsg, OSAL_TIMEOUT_FOREVER);
        	OSAL_assertSuccess(retVal);

        	unknownAck = TRUE;
	        // mark ACK as received for sent MSG
        	for (i = 0; i < numMsg; i++) {
          		if (pRcvMsg == pSentMsgList[i] ) {
            		ackList[i] = TRUE;
            		unknownAck = FALSE;
            		if (ackRetVal == OSAL_SOK) {
              			ackRetVal  = OSAL_msgGetAckStatus(pRcvMsg);
            		}
            		break;
          		}
        	}

        	// check if all ACKs received
        	waitAck = FALSE;
        	for (i = 0; i < numMsg; i++) {
          		if (ackList[i] == FALSE ) {
            		waitAck = TRUE;
            		break;
          		}
        	}

        	if (unknownAck) {
          		// ACK received is for some old message, hence free MSG and prm
          		OSAL_mbxFreeMsg(pRcvMsg);
        	} else {
          		// only free MSG now, free prm after all ACKs are received
          		OSAL_assertSuccess( OSAL_msgqFreeMsgHndl(pRcvMsg) );
        	}

      	} while (waitAck);

      	if (flags & OSAL_MBX_FREE_PRM) {
        	if (pPrm != NULL) {
          		OSAL_memFree(pPrm);
        	}
      	}

      	retVal = ackRetVal;
  	}
   	return retVal;
}


int OSAL_mbxWaitMsg(OSAL_MbxHndl *pMbxHndl, OSAL_MsgHndl **pMsg)
{
	int retVal;

	retVal = OSAL_msgqRecvMsg(&pMbxHndl->rcvMbx, pMsg, OSAL_TIMEOUT_FOREVER);
	OSAL_assertSuccess(retVal);

	return retVal;
}

int OSAL_mbxCheckMsg(OSAL_MbxHndl *pMbxHndl, OSAL_MsgHndl **pMsg)
{
	int retVal;

	retVal = OSAL_msgqRecvMsg(&pMbxHndl->rcvMbx, pMsg, OSAL_TIMEOUT_NONE);

	return retVal;
}

int OSAL_mbxAckOrFreeMsg(OSAL_MsgHndl *pMsg, int ackRetVal)
{
	int retVal = OSAL_SOK;

	if (pMsg == NULL)
		return OSAL_EFAIL;

	if (pMsg->flags & OSAL_MBX_WAIT_ACK) {
 		// ACK message
 		retVal = OSAL_msgqSendAck(pMsg, ackRetVal);
 		OSAL_assertSuccess(retVal);
	} else {
		// FREE message and prm
		OSAL_mbxFreeMsg(pMsg);
	}

	return retVal;
}

int OSAL_mbxFlush(OSAL_MbxHndl *pMbxHndl)
{
	int retVal;
	OSAL_MsgHndl *pMsg;

	// flush receive mailbox
	do {
		retVal = OSAL_mbxCheckMsg(pMbxHndl, &pMsg);
		if (retVal == OSAL_SOK) {
	  		OSAL_mbxAckOrFreeMsg(pMsg, OSAL_SOK);
		}
	} while (retVal == OSAL_SOK);

	// flush ACK mailbox
	do {
		retVal = OSAL_msgqRecvMsg(&pMbxHndl->ackMbx, &pMsg, OSAL_TIMEOUT_NONE);
		if (retVal == OSAL_SOK) {
	  		OSAL_mbxFreeMsg(pMsg);
		}
	} while (retVal == OSAL_SOK);

	return retVal;
}


int OSAL_mbxWaitCmd(OSAL_MbxHndl *pMbxHndl, OSAL_MsgHndl **pMsg, Uint16 waitCmd)
{
	OSAL_MsgHndl *pRcvMsg;

	while(1) {
		OSAL_mbxWaitMsg(pMbxHndl, &pRcvMsg);
		if (OSAL_msgGetCmd(pRcvMsg) == waitCmd)
	  		break;
		OSAL_mbxAckOrFreeMsg(pRcvMsg, OSAL_SOK);
	}

	if (pMsg == NULL) {
		OSAL_mbxAckOrFreeMsg(pRcvMsg, OSAL_SOK);
	} else {
		*pMsg = pRcvMsg;
	}

	return OSAL_SOK;
}

