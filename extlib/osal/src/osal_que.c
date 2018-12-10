#include <osal_que.h>
#include <pthread.h>


int OSAL_queCreate(OSAL_QueHndl *hndl, Uint32 maxLen)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status=OSAL_SOK;

	hndl->curRd = hndl->curWr = 0;
	hndl->count = 0;
	hndl->len   = maxLen;
	hndl->queue = OSAL_memAlloc(sizeof(Int32)*hndl->len);

	if (hndl->queue == NULL) {
		OSAL_ERROR("SAL_queCreate() = %d \r\n", status);
		return OSAL_EFAIL;
	}

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
	status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_queCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}


int OSAL_queDelete(OSAL_QueHndl *hndl)
{
	if (hndl->queue!=NULL)
		OSAL_memFree(hndl->queue);
    
	pthread_cond_destroy(&hndl->condRd);
	pthread_cond_destroy(&hndl->condWr);
	pthread_mutex_destroy(&hndl->lock);  

	return OSAL_SOK;
}


int OSAL_quePut(OSAL_QueHndl *hndl, Int32 value, Uint32 timeout)
{
	int status = OSAL_EFAIL;

	pthread_mutex_lock(&hndl->lock);

	while (1) {
		if (hndl->count < hndl->len) {
			hndl->queue[hndl->curWr] = value;
			hndl->curWr = (hndl->curWr+1)%hndl->len;
			hndl->count++;
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


int OSAL_queGet(OSAL_QueHndl *hndl, Int32 *value, Uint32 timeout)
{
	int status = OSAL_EFAIL;

	pthread_mutex_lock(&hndl->lock);

	while (1) {
		if (hndl->count > 0) {
	  		if (value != NULL) {
	    		*value = hndl->queue[hndl->curRd];
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



