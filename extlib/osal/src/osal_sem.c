#include <osal_sem.h>
#include <pthread.h>


int OSAL_semCreate(OSAL_SemHndl *hndl, Uint32 maxCount, Uint32 initVal)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status = OSAL_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->cond, &cond_attr);  

	hndl->count = initVal;
	hndl->maxCount = maxCount;

	if (hndl->maxCount == 0)  
		hndl->maxCount = 1;

	if (hndl->count > hndl->maxCount)
		hndl->count = hndl->maxCount;

	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_semCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int OSAL_semWait(OSAL_SemHndl *hndl, Uint32 timeout)
{
	int status = OSAL_EFAIL;

	pthread_mutex_lock(&hndl->lock);

	while (1) {
		if (hndl->count > 0) {
			hndl->count--;
			status = OSAL_SOK;
			break;
		} else {
			if (timeout == OSAL_TIMEOUT_NONE)
				break;
		  pthread_cond_wait(&hndl->cond, &hndl->lock);
		}
	}

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

int OSAL_semSignal(OSAL_SemHndl *hndl)
{
	int status = OSAL_SOK;

	pthread_mutex_lock(&hndl->lock);

	if (hndl->count<hndl->maxCount) {
		hndl->count++;
		status |= pthread_cond_signal(&hndl->cond);
	}

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

int OSAL_semDelete(OSAL_SemHndl *hndl)
{
	pthread_cond_destroy(&hndl->cond);
	pthread_mutex_destroy(&hndl->lock);  

	return OSAL_SOK;
}


