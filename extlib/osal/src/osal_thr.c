
#include <osal_thr.h>

int OSAL_thrCreate(OSAL_ThrHndl *hndl, OSAL_ThrEntryFunc entryFunc, Uint32 pri, Uint32 stackSize, void *prm)
{
	int status = OSAL_SOK;
	pthread_attr_t thread_attr;
	struct sched_param schedprm;

	// initialize thread attributes structure
	status = pthread_attr_init(&thread_attr);

	if (status != OSAL_SOK) {
		OSAL_ERROR("SAL_thrCreate() - Could not initialize thread attributes\n");
		return status;
	}

	if (stackSize != OSAL_THR_STACK_SIZE_DEFAULT)
	    pthread_attr_setstacksize(&thread_attr, stackSize);

	status |= pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
	status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);

	if (pri > OSAL_THR_PRI_MAX)   
		pri = OSAL_THR_PRI_MAX;
	else if (pri < OSAL_THR_PRI_MIN)   
		pri = OSAL_THR_PRI_MIN;
    
	schedprm.sched_priority = pri;
	status |= pthread_attr_setschedparam(&thread_attr, &schedprm);

	if (status != OSAL_SOK) {
		OSAL_ERROR("SAL_thrCreate() - Could not initialize thread attributes\n");
		goto error_exit;
	}

	status = pthread_create(&hndl->hndl, &thread_attr, entryFunc, prm);
	if (status != OSAL_SOK) {
		OSAL_ERROR("SAL_thrCreate() - Could not create thread [%d]\n", status);
	}

error_exit:  
	pthread_attr_destroy(&thread_attr);
	return status;
}


int OSAL_thrJoin(OSAL_ThrHndl *hndl)
{
	int status = OSAL_SOK;
	void *returnVal;

	status = pthread_join(hndl->hndl, &returnVal); 
	return status;    
}



int OSAL_thrDelete(OSAL_ThrHndl *hndl)
{
	int status = OSAL_SOK;

	status |= pthread_cancel(hndl->hndl); 
	status |= OSAL_thrJoin(hndl);

	return status;    
}

int OSAL_thrChangePri(OSAL_ThrHndl *hndl, Uint32 pri)
{
	int status = OSAL_SOK;
	struct sched_param schedprm;  

	if (pri > OSAL_THR_PRI_MAX)   
		pri = OSAL_THR_PRI_MAX;
	else if (pri < OSAL_THR_PRI_MIN)   
		pri = OSAL_THR_PRI_MIN;

	schedprm.sched_priority = pri;  
	status |= pthread_setschedparam(hndl->hndl, SCHED_FIFO, &schedprm);

	return status;
}


int OSAL_thrExit(void *returnVal)
{
	pthread_exit(returnVal);
	return OSAL_SOK;
}

