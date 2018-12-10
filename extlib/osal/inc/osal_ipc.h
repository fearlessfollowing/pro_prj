#ifndef _OSAL_IPC_
#define _OSAL_IPC_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OSAL_IPC_FLAG_OPEN      (0x0)
#define OSAL_IPC_FLAG_CREATE    (0x1)

#define OSAL_IPC_KEY_INVALID    ((Uint32)-1)

#define OSAL_IPC_MBX_MSG_SIZE_MAX    (KB)


#define OSAL_IPC_MBX_MSG_TYPE_ID    0xFF

#define OSAL_IPC_SEM_MEMBERS_MAX    1

typedef struct {
	int id;
	char *ptr;
} OSAL_IpcShmHndl;

typedef struct {
	int id;
} OSAL_IpcMbxHndl;

typedef struct {
	int id;
	int members;
	int maxVal;
} OSAL_IpcSemHndl;

typedef struct {
	long  type;
	Uint8 data[OSAL_IPC_MBX_MSG_SIZE_MAX];
} OSAL_IpcMsgHndl;


Uint32 OSAL_ipcMakeKey(char *path, char id);

// shared memory
char *OSAL_ipcShmOpen(OSAL_IpcShmHndl *hndl, Uint32 key, Uint32 size, Uint32 flags);
int   OSAL_ipcShmClose(OSAL_IpcShmHndl *hndl);

// message box
int OSAL_ipcMbxOpen(OSAL_IpcMbxHndl *hndl, Uint32 key, Uint32 flags);
int OSAL_ipcMbxSend(OSAL_IpcMbxHndl *hndl, OSAL_IpcMsgHndl *msgData, Uint32 msgSize);
int OSAL_ipcMbxRecv(OSAL_IpcMbxHndl *hndl, OSAL_IpcMsgHndl *msgData, Uint32 msgSize);
int OSAL_ipcMbxClose(OSAL_IpcMbxHndl *hndl);

// semaphore
int OSAL_ipcSemOpen(OSAL_IpcSemHndl *hndl, Uint32 key, Uint32 members, Uint32 initVal, Uint32 maxVal, Uint32 flags);
int OSAL_ipcSemLock(OSAL_IpcSemHndl *hndl, Uint32 member, Uint32 timeout);
int OSAL_ipcSemUnlock(OSAL_IpcSemHndl *hndl, Uint32 member, Uint32 timeout);
int OSAL_ipcSemGetVal(OSAL_IpcSemHndl *hndl, Uint32 member);
int OSAL_ipcSemClose(OSAL_IpcSemHndl *hndl);


#if defined(__cplusplus)
}
#endif

#endif  /*  _SAL_IPC_ */




