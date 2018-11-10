#ifndef _OSAL_PRF_H_
#define _OSAL_PRF_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_PRF_PRINT_DEFAULT   (OSAL_PRF_PRINT_TIME|OSAL_PRF_PRINT_VALUE)

#define OSAL_PRF_PRINT_ALL       (0xFFFF)

#define OSAL_PRF_PRINT_TIME      (0x0001)
#define OSAL_PRF_PRINT_VALUE     (0x0002)
#define OSAL_PRF_PRINT_MIN_MAX   (0x0004)
#define OSAL_PRF_PRINT_COUNT     (0x0008)

/*
Profile info  : <name>
======================
Iterations    :
Avg Time (ms) :
Max Time (ms) :
Min Time (ms) :
Avg Value     :
Avg Value/sec :
Max Value     :
Min Value     :
*/

#ifdef OSAL_PRF_ENABLE

typedef struct {

	Uint32 totalTime;
	Uint32 maxTime;
	Uint32 minTime;

	Uint32 totalValue;
	Uint32 maxValue;
	Uint32 minValue;

	Uint32 count;
	Uint32 curTime;
	Uint32 curValue;

} OSAL_PrfHndl;

int OSAL_prfBegin(OSAL_PrfHndl *hndl);
int OSAL_prfEnd(OSAL_PrfHndl *hndl, Uint32 value);
int OSAL_prfReset(OSAL_PrfHndl *hndl);
int OSAL_prfPrint(OSAL_PrfHndl *hndl, char *name, Uint32 flags);

#else

typedef struct {
    int rsv;
} OSAL_PrfHndl;

#define OSAL_prfBegin(hndl)
#define OSAL_prfEnd(hndl, value)
#define OSAL_prfReset(hndl)
#define OSAL_prfPrint(hndl, name, flags)

#endif


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_PRF_H_ */



