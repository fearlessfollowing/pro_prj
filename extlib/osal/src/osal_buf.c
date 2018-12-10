

#include <osal_buf.h>
#include <string.h>

int OSAL_bufDelete(OSAL_BufHndl *hndl)
{
	int status = OSAL_SOK;

	if (hndl == NULL)
		return OSAL_EFAIL;

	status = OSAL_queDelete(&hndl->emptyQue);
	status |= OSAL_queDelete(&hndl->fullQue);

	return status;
}

int OSAL_bufCreate(OSAL_BufHndl *hndl, OSAL_BufCreate *bufInit)
{
	int status = OSAL_SOK;
	int i;

	if (hndl == NULL || bufInit == NULL)
		return OSAL_EFAIL;

  	if (bufInit->numBuf >  OSAL_BUF_NUM_MAX )
    	return OSAL_EFAIL;

  	memset(hndl, 0, sizeof(OSAL_BufHndl));

  	status = OSAL_queCreate(&hndl->emptyQue, bufInit->numBuf);

  	if (status != OSAL_SOK) {
    	OSAL_ERROR("SAL_bufCreate() = %d \r\n", status);
    	return status;
  	}

  	status = OSAL_queCreate(&hndl->fullQue, bufInit->numBuf);

  	if (status != OSAL_SOK) {
    	OSAL_queDelete(&hndl->emptyQue);
    	OSAL_ERROR("SAL_bufCreate() = %d \r\n", status);
    	return status;
  	}

  	hndl->numBuf   = bufInit->numBuf;

  	for (i = 0; i < hndl->numBuf; i++) {
		hndl->bufInfo[i].size = 0;
		hndl->bufInfo[i].flags = 0;
		hndl->bufInfo[i].timestamp = 0;
		hndl->bufInfo[i].physAddr = bufInit->bufPhysAddr[i];
		hndl->bufInfo[i].virtAddr = bufInit->bufVirtAddr[i];
		OSAL_quePut(&hndl->emptyQue, i, OSAL_TIMEOUT_FOREVER);
	}

	return status;
}

int OSAL_bufGetEmpty(OSAL_BufHndl *hndl, int *bufId, Uint32 timeout)
{
	int status;

	if (hndl == NULL || bufId == NULL)
		return OSAL_EFAIL;

	status = OSAL_queGet(&hndl->emptyQue, bufId, timeout);

	if (status != OSAL_SOK) {
		*bufId = OSAL_BUF_ID_INVALID;
	}
	return status;
}

int OSAL_bufPutFull (OSAL_BufHndl *hndl, int bufId)
{
	int status;

	if (hndl == NULL)
		return OSAL_EFAIL;

	if (bufId >= hndl->numBuf || bufId < 0)
		return OSAL_EFAIL;

	status = OSAL_quePut(&hndl->fullQue, bufId, OSAL_TIMEOUT_FOREVER);
	return status;
}

int OSAL_bufGetFull(OSAL_BufHndl *hndl, int *bufId, Uint32 timeout)
{
	int status;

	if (hndl == NULL || bufId == NULL)
		return OSAL_EFAIL;

	status = OSAL_queGet(&hndl->fullQue, bufId, timeout);
	if (status != OSAL_SOK) {
		*bufId = OSAL_BUF_ID_INVALID;
	}

	return status;
}

int OSAL_bufPutEmpty(OSAL_BufHndl *hndl, int bufId)
{
	int status;

	if (hndl == NULL)
		return OSAL_EFAIL;

	if (bufId >= hndl->numBuf || bufId < 0)
		return OSAL_EFAIL;

	status = OSAL_quePut(&hndl->emptyQue, bufId, OSAL_TIMEOUT_FOREVER);

	return status;
}


int OSAL_bufSwitchFull(OSAL_BufHndl *hndl, int *bufId)
{
	int status;
	int newBufId;

	status = OSAL_bufGetEmpty(hndl, &newBufId, OSAL_TIMEOUT_NONE);
	if (status == OSAL_SOK) {
		if (*bufId != OSAL_BUF_ID_INVALID) {
			OSAL_bufPutFull(hndl, *bufId);
		}
		*bufId = newBufId;
	}

	return status;
}

int OSAL_bufSwitchEmpty(OSAL_BufHndl *hndl, int *bufId)
{
	int status;
	int newBufId;

	status = OSAL_bufGetFull(hndl, &newBufId, OSAL_TIMEOUT_NONE);

	if (status == OSAL_SOK) {
		if (*bufId != OSAL_BUF_ID_INVALID) {
			OSAL_bufPutEmpty(hndl, *bufId);
		}
		*bufId = newBufId;
	}

	return status;
}

OSAL_BufInfo *OSAL_bufGetBufInfo(OSAL_BufHndl *hndl, int bufId)
{
	if (hndl == NULL)
		return NULL;

	if (bufId >= hndl->numBuf)
		return NULL;
	
	return &hndl->bufInfo[bufId];
}


