
#include <osal.h>
#include <osal_thr.h>
#include <osal_mutex.h>
#include <sys/time.h>
#include <unistd.h>

OSAL_MutexHndl gSAL_printfLock;

int OSAL_init()
{
	int status;

	status = OSAL_mutexCreate(&gSAL_printfLock);
	if (status != OSAL_SOK)
		OSAL_ERROR("SAL_mutexCreate()\n");

	OSAL_printf(" SAL: THR_PRI = %d .. %d\n", OSAL_THR_PRI_MIN, OSAL_THR_PRI_MAX);  
	return status;  
}

int OSAL_exit()
{
	int status;
	status = OSAL_mutexDelete(&gSAL_printfLock);
	return status;  
}


int OSAL_attachSignalHandler(int sigId, void (*handler)(int ) )
{
	struct sigaction sigAction;

	/* insure a clean shutdown if user types ctrl-c */
	sigAction.sa_handler = handler;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	sigaction(sigId, &sigAction, NULL);

	return OSAL_SOK;
}

Uint32 OSAL_getCurTimeInMsec()
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0) 
		return 0;

	return tv.tv_sec * 1000 + tv.tv_usec/1000;
}

void OSAL_waitMsecs(Uint32 msecs)
{
	struct timespec delayTime, elaspedTime;

	delayTime.tv_sec  = msecs/1000;
	delayTime.tv_nsec = (msecs%1000)*1000000;

	nanosleep(&delayTime, &elaspedTime);
}

static char xtod(char c) 
{
	if (c>='0' && c<='9') return c-'0';
	if (c>='A' && c<='F') return c-'A'+10;
	if (c>='a' && c<='f') return c-'a'+10;
	return c=0;        // not Hex digit
}
  
static int HextoDec(char *hex, int l)
{
	if (*hex == 0) 
		return (l);

	return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}
  
int xstrtoi(char *hex)      // hex string to integer
{
	return HextoDec(hex,0);
}

int OSAL_getHostName(char *hostname, int maxHostNameLen)
{
	int status;
	hostname[0] = 0;

	status = gethostname(hostname, maxHostNameLen);
	if (status < 0)
		return OSAL_EFAIL;

	return OSAL_SOK;
}
