#include <osal_mutex.h>
#include <pthread.h>

int OSAL_mutexCreate(OSAL_MutexHndl *hndl)
{
	pthread_mutexattr_t mutex_attr;
	int status = OSAL_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);

	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_mutexCreate() = %d \r\n", status);

	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int OSAL_mutexDelete(OSAL_MutexHndl *hndl)
{
	pthread_mutex_destroy(&hndl->lock);  
	return OSAL_SOK;
}

int OSAL_mutexLock(OSAL_MutexHndl *hndl)
{
	return pthread_mutex_lock(&hndl->lock);
}

int OSAL_mutexUnlock(OSAL_MutexHndl *hndl)
{
	return pthread_mutex_unlock(&hndl->lock);
}


