

#include <osal.h>

#ifdef OSAL_PRF_ENABLE

int OSAL_prfBegin(OSAL_PrfHndl *hndl)
{
	hndl->curTime = OSAL_getCurTimeInMsec();
	return OSAL_SOK;
}

int OSAL_prfEnd(OSAL_PrfHndl *hndl, Uint32 value)
{
	hndl->curTime = SAL_getCurTimeInMsec() - hndl->curTime;

	hndl->count++;

	hndl->totalTime += hndl->curTime;
	if (hndl->curTime > hndl->maxTime)
		hndl->maxTime = hndl->curTime;
	if (hndl->curTime < hndl->minTime)
		hndl->minTime = hndl->curTime;

	hndl->totalValue += value;
	if (value > hndl->maxValue)
		hndl->maxValue = value;
	if (value < hndl->minTime)
		hndl->minValue = hndl->curValue;

	return OSAL_SOK;
}

int OSAL_prfReset(OSAL_PrfHndl *hndl)
{
	memset(hndl, 0, sizeof(*hndl) );
	return OSAL_SOK;
}

int OSAL_prfPrint(OSAL_PrfHndl *hndl, char *name, Uint32 flags)
{
	OSAL_printf(" \n");
	OSAL_printf(" Profile Info  : %s \n", name);
	OSAL_printf(" ======================\n");

	if (flags == 0)
		flags = OSAL_PRF_PRINT_DEFAULT;

	if (flags & OSAL_PRF_PRINT_COUNT ) {
		OSAL_printf(" Iterations    : %d \n", hndl->count);
	}

	if (flags & OSAL_PRF_PRINT_TIME ) {
		OSAL_printf(" Avg Time (ms) : %9.2f \n", (float)hndl->totalTime/hndl->count);

		if (flags & OSAL_PRF_PRINT_MIN_MAX ) {
  			OSAL_printf(" Min Time (ms) : %d \n", hndl->minTime);
		}

		if (flags & OSAL_PRF_PRINT_MIN_MAX ) {
  			OSAL_printf(" Max Time (ms) : %d \n", hndl->maxTime);
		}
	}

	if (flags & SOAL_PRF_PRINT_VALUE) {

		OSAL_printf(" Avg Value     : %9.2f \n", (float)hndl->totalValue/hndl->count);
		OSAL_printf(" Avg Value/sec : %9.2f \n", (float)(hndl->totalValue*1000)/hndl->totalTime);

		if (flags & OSAL_PRF_PRINT_MIN_MAX ) {
  			OSAL_printf(" Min Value     : %d \n", hndl->minValue);
		}

		if (flags & OSAL_PRF_PRINT_MIN_MAX ) {
  			OSAL_printf(" Max Value     : %d \n", hndl->maxValue);
		}
	}
	
	OSAL_printf(" \n");  

	return OSAL_SOK;
}

#endif
