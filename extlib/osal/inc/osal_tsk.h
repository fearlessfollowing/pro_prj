#ifndef _OSAL_TSK_H_
#define _OSAL_TSK_H_

#include <osal_thr.h>
#include <osal_mbx.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef TASK_NAME_MAX_LEN
#define TASK_NAME_MAX_LEN       32
#endif

struct OSAL_TskHndl;

typedef int (*OSAL_TskFncMain)(struct OSAL_TskHndl *pTsk, OSAL_MsgHndl *pMsg, Uint32 curState);

enum {
    OSAL_TASK_STATE_CRATE   = (1 << 0),         /* 创建状态：任务被创建后处于该状态 */
    OSAL_TASK_STATE_WAITING = (1 << 1),         /* 等待消息状态：任务处于等待消息的到来的状态 */
    OSAL_TASK_STATE_RUNNING = (1 << 2),         /* 运行状态：任务正在处于消息 */
    OSAL_TASK_STATE_ZOMBIE  = (1 << 3),         /* 僵死状态：线程已经退出，但是任务结构没有被释放（静态定义的任务在线程退出时会处于这种状态） */
};


/*
 * OSAL任务结构
 */
typedef struct OSAL_TskHndl {

#ifdef ENABLE_DEBUG_TASK
    Uint32              mDealMsgCnt;        /* 处理消息的个数 */
    Uint32              mSurviTime;         /* 任务的生成时间（从被创建到当前时刻的时间） */
    Uint32              mRunTime;           /* 处于运行状态的时间 */

    Uint8               mTaskName[TASK_NAME_MAX_LEN];   /* 任务的名称，最长为32字节 */
#endif
    
    OSAL_MbxHndl        mbxHndl;            /* 任务的邮箱: 用于收发消息 */
    OSAL_ThrHndl        thrHndl;            /* 任务的线程实体 */
    Uint32              curState;           /* 任务的状态: CREATE, INIT, RUNNING, STOP */
    OSAL_TskFncMain     fncMain;            /* 任务的执行体 */
} OSAL_TskHndl;

int     OSAL_tskCreate(OSAL_TskHndl *pTsk, OSAL_TskFncMain fncMain, Uint32 tskPri, Uint32 tskStackSize, Uint32 initState);
int     OSAL_tskDelete(OSAL_TskHndl *pTsk);
int     OSAL_tskSendMsg(OSAL_TskHndl *pTskTo, OSAL_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int     OSAL_tskBroadcastMsg(OSAL_TskHndl *pTskToList[], OSAL_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);
int     OSAL_tskAckOrFreeMsg(OSAL_MsgHndl *pMsg, int ackRetVal);
int     OSAL_tskWaitMsg(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg);
int     OSAL_tskCheckMsg(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg);
int     OSAL_tskWaitCmd(OSAL_TskHndl *pTsk, OSAL_MsgHndl **pMsg, Uint16 waitCmd);
int     OSAL_tskFlushMsg(OSAL_TskHndl *pTsk);

int     OSAL_tskSetState(OSAL_TskHndl *pPrc, Uint32 curState);
Uint32  OSAL_tskGetState(OSAL_TskHndl *pPrc);


#if defined(__cplusplus)
}
#endif

#endif /* _SAL_TSK_H_ */




