#include <osal_flg.h>

int  OSAL_flgCreate(OSAL_FlgHndl *hndl, Uint32 initPattern)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status = OSAL_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->cond, &cond_attr);  

	hndl->pattern = initPattern;

	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_flgCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}


int OSAL_flgWait(OSAL_FlgHndl *hndl, Uint32 pattern, Uint32 mode, Uint32 timeout)
{
	int status = OSAL_EFAIL, found;
	Uint32 setPattern;

	pthread_mutex_lock(&hndl->lock);

	while (1) {

		found = 0;
		setPattern = hndl->pattern & pattern;

		if (mode & OSAL_FLG_MODE_AND) {
			if (setPattern == pattern)
				found = 1;
		} else {
			if (setPattern)
				found = 1;
		}

    	if (found) {
      		if (mode & OSAL_FLG_MODE_CLR)
        		hndl->pattern &= ~setPattern;
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

int  OSAL_flgSet(OSAL_FlgHndl *hndl, Uint32 pattern)
{
	int status = OSAL_SOK;

	pthread_mutex_lock(&hndl->lock);

	hndl->pattern |= pattern;
	status |= pthread_cond_signal(&hndl->cond);

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

int OSAL_flgClear(OSAL_FlgHndl *hndl, Uint32 pattern)
{
	int status = OSAL_SOK;

	pthread_mutex_lock(&hndl->lock);

	hndl->pattern &= ~pattern;

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

Bool OSAL_flgIsSet(OSAL_FlgHndl *hndl, Uint32 pattern)
{
	int status;

	status = OSAL_flgWait(hndl, pattern, OSAL_FLG_MODE_AND, OSAL_TIMEOUT_NONE);
	if (status != OSAL_SOK)
		return FALSE;

	return TRUE;    
}

int OSAL_flgDelete(OSAL_FlgHndl *hndl)
{
	pthread_cond_destroy(&hndl->cond);
	pthread_mutex_destroy(&hndl->lock);  

	return OSAL_SOK;
}



