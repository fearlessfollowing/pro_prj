#include <osal_tsk.h>
#include <stdio.h>
#include <unistd.h>

/*
 * 创建一个Task,以一定的间隔给Task发送各种不同类型的消息
 */


static int Test_Main(struct OSAL_TskHndl *pTsk, OSAL_MsgHndl *pMsg, Uint32 curState)
{

}

int main(int argc, char* argv[])
{
    OSAL_TskHndl menuTask;
    int iRet = -1;
    iRet = OSAL_tskCreate(&menuTask, Test_Main, 0, 0, OSAL_TASK_STATE_CRATE);
    if (OSAL_SOK != iRet) {
        fprintf(stderr, "Create Menu Task Failed");
    } else {
        fprintf(stdout, "Create Menu UI Task Success!!!");
    }

    while (true) {
        sleep(100);
    }

    return 0;
}