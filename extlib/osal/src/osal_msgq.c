#include <osal_msgq.h>
#include <stdio.h>


int OSAL_msgqCreate(OSAL_MsgqHndl *hndl)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status = OSAL_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
	status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

	hndl->curRd = hndl->curWr = 0;
	hndl->count = 0;
	hndl->len   = OSAL_MSGQ_LEN_MAX;

	if(status != OSAL_SOK)
		OSAL_ERROR("SAL_msgqCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int OSAL_msgqDelete(OSAL_MsgqHndl *hndl)
{
	pthread_cond_destroy(&hndl->condRd);
	pthread_cond_destroy(&hndl->condWr);
	pthread_mutex_destroy(&hndl->lock);  

	return OSAL_SOK;
}

OSAL_MsgHndl *OSAL_msgqAllocMsgHndl(OSAL_MsgqHndl *to, OSAL_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags)
{
	OSAL_MsgHndl *msg;

	msg = OSAL_memAlloc(sizeof(OSAL_MsgHndl));
	if (msg != NULL) {
		msg->pTo = to;
		msg->pFrom = from;
		msg->pPrm = prm;
		msg->status = OSAL_SOK;
		msg->cmd = cmd;
		msg->flags = msgFlags;
	}

	return msg;    
}

int OSAL_msgqSend(OSAL_MsgqHndl *hndl, OSAL_MsgHndl *msg, Uint32 timeout)
{
	int status = OSAL_EFAIL;

	pthread_mutex_lock(&hndl->lock);
	//printf("OSAL_msgqSend: send before msg count = %d\n", hndl->count);	

	while (1) {
		if (hndl->count < hndl->len) {
			hndl->queue[hndl->curWr] = msg;
			hndl->curWr = (hndl->curWr+1)%hndl->len;
			hndl->count++;
			//printf("OSAL_msgqSend: send after msg count = %d\n", hndl->count);	

			status = OSAL_SOK;
			pthread_cond_signal(&hndl->condRd);
			break;
		} else {
			if (timeout == OSAL_TIMEOUT_NONE)
	    		break;

	  		status = pthread_cond_wait(&hndl->condWr, &hndl->lock);
		}
	}

	pthread_mutex_unlock(&hndl->lock);

	return status;
}


int OSAL_msgqSendMsg(OSAL_MsgqHndl *to, OSAL_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags, OSAL_MsgHndl **msg)
{
	int status;
	OSAL_MsgHndl *data;

	data = OSAL_msgqAllocMsgHndl(to, from, cmd, prm, msgFlags);
	if (data == NULL)
		return OSAL_EFAIL;

  	status = OSAL_msgqSend(to, data, OSAL_TIMEOUT_FOREVER);
  	if (status == OSAL_SOK) {
    	if (msg != NULL)
      		*msg = data;
  	}

  	return status;
}

int OSAL_msgqRecvMsg(OSAL_MsgqHndl *hndl, OSAL_MsgHndl **msg, Uint32 timeout)
{
	int status = OSAL_EFAIL;

	pthread_mutex_lock(&hndl->lock);
	
	//printf("OSAL_msgqRecvMsg: msg count = %d\n", hndl->count);	
	while(1) {
		if (hndl->count > 0) {

	  		if (msg != NULL) {
	    		*msg = hndl->queue[hndl->curRd];
	  		}
	  
			hndl->curRd = (hndl->curRd+1)%hndl->len;
			hndl->count--;
			status = OSAL_SOK;
			pthread_cond_signal(&hndl->condWr);
			break;
		} else {
			if (timeout == OSAL_TIMEOUT_NONE)
				break;
			status = pthread_cond_wait(&hndl->condRd, &hndl->lock);
		}
	}

	pthread_mutex_unlock(&hndl->lock);
	return status;
}

int OSAL_msgqSendAck(OSAL_MsgHndl *msg, int ackRetVal)
{
	int status;

	msg->status = ackRetVal;

	status = OSAL_msgqSend(msg->pFrom, msg, OSAL_TIMEOUT_FOREVER);

	return status;
}

int OSAL_msgqFreeMsgHndl(OSAL_MsgHndl *msg)
{
	OSAL_memFree(msg);
	return OSAL_SOK;
}



