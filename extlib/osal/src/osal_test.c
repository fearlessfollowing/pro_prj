

#include <osal/osal_tsk.h>

#define AUTO_TEST_DELAY    (1)
#define MANUAL_TEST_DELAY  (100)

#define SAL_TEST_STATE__READY  0
#define SAL_TEST_STATE__RUN    1
#define SAL_TEST_STATE__PAUSE  2

#define SAL_TEST_CMD__INIT     0
#define SAL_TEST_CMD__START    1
#define SAL_TEST_CMD__STOP     2
#define SAL_TEST_CMD__PAUSE    3
#define SAL_TEST_CMD__RESUME   4

#define SAL_TEST_PRC_STACK (1*KB)

OSAL_MbxHndl gSAL_testMbx;

OSAL_TskHndl gSAL_testPseq;
OSAL_TskHndl gSAL_testPstA;
OSAL_TskHndl gSAL_testPstB;
OSAL_TskHndl gSAL_testPstC;

int OSAL_TEST_PSEQ_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState );
int OSAL_TEST_PSTA_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState );
int OSAL_TEST_PSTB_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState );
int OSAL_TEST_PSTC_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState );

int OSAL_TEST_Init();
int OSAL_TEST_Exit();

int OSAL_TEST_Start(Uint32 delay);
int OSAL_TEST_Pause();
int OSAL_TEST_Resume();
int OSAL_TEST_Stop();

int OSAL_TEST_ManualTestRun();
int OSAL_TEST_AutoTestRun();

char OSAL_TEST_GetInput();

char gSAL_TEST_strMainMenu[] =
{
// *INDENT-OFF*
"\r\n"
"\r\n =================="
"\r\n SAL Test Main Menu"
"\r\n =================="
"\r\n"
"\r\n 1: Manual Test"
"\r\n 2: Auto Test"
"\r\n"
"\r\n 0: Exit"
"\r\n"
"\r\n Enter Choice : "
// *INDENT-ON*
};

int OSAL_tskTestMain(int argc, char **argv)
{
    char ch;
    Bool done = FALSE;

    OSAL_printf(" \r\n");
    OSAL_printf(" Thread Priority [ min = %d , max = %d, default = %d ] \r\n", SAL_THR_PRI_MIN, SAL_THR_PRI_MAX, SAL_THR_PRI_DEFAULT);

    do {
        OSAL_printf(gSAL_TEST_strMainMenu);

        ch = OSAL_TEST_GetInput();
        OSAL_printf("\r\n");

        switch (ch) {
            case '1':
                OSAL_TEST_ManualTestRun();
                break;

            case '2':
                OSAL_TEST_AutoTestRun();
                break;

            case '0':
                done = TRUE;
                break;
        }
    } while(!done);

    return OSAL_SOK;
}


int OSAL_TEST_PSEQ_Start(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    OSAL_TskHndl *prcToList[] =  { 
        &gSAL_testPstA,
        &gSAL_testPstB,
        &gSAL_testPstC,
        NULL
    };
  
    Uint32 delay;

    delay = *(Uint32*)OSAL_msgGetPrm(pMsg);

    OSAL_tskSendMsg(&gSAL_testPstA, pPrc, OSAL_TEST_CMD__INIT, &delay, OSAL_MBX_WAIT_ACK);
    OSAL_tskSendMsg(&gSAL_testPstB, pPrc, OSAL_TEST_CMD__INIT, &delay, OSAL_MBX_WAIT_ACK);
    OSAL_tskSendMsg(&gSAL_testPstC, pPrc, OSAL_TEST_CMD__INIT, &delay, OSAL_MBX_WAIT_ACK);

    OSAL_tskBroadcastMsg(prcToList, pPrc, OSAL_TEST_CMD__START, NULL, 0);

    OSAL_printf("\r\n PSEQ  : STARTED ");

    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);

    return OSAL_SOK;    
}

int OSAL_TEST_PSEQ_Stop(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    OSAL_tskSendMsg(&gSAL_testPstC, pPrc, OSAL_TEST_CMD__STOP, NULL, OSAL_MBX_WAIT_ACK);
    OSAL_tskSendMsg(&gSAL_testPstB, pPrc, OSAL_TEST_CMD__STOP, NULL, OSAL_MBX_WAIT_ACK);
    OSAL_tskSendMsg(&gSAL_testPstA, pPrc, OSAL_TEST_CMD__STOP, NULL, OSAL_MBX_WAIT_ACK);

    OSAL_printf("\r\n PSEQ  : STOPPED ");

    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__READY);

    return OSAL_SOK;    
}

int OSAL_TEST_PSEQ_Pause(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    OSAL_TskHndl *prcToList[] =  { 
        &gSAL_testPstA,
        &gSAL_testPstB,
        &gSAL_testPstC,
        NULL
    };

    OSAL_tskBroadcastMsg(prcToList, pPrc, OSAL_TEST_CMD__PAUSE, NULL, OSAL_MBX_WAIT_ACK);

    OSAL_printf("\r\n PSEQ  : PAUSED ");

    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__PAUSE);

    return OSAL_SOK;    
}

int OSAL_TEST_PSEQ_Resume(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    OSAL_TskHndl *prcToList[] =  { 
        &gSAL_testPstA,
        &gSAL_testPstB,
        &gSAL_testPstC,
        NULL
    };

    OSAL_tskBroadcastMsg(prcToList, pPrc, OSAL_TEST_CMD__RESUME, NULL, OSAL_MBX_WAIT_ACK);

    OSAL_printf("\r\n PSEQ  : RESUMED ");

    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);

    return OSAL_SOK;    
}

int OSAL_TEST_PSEQ_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState )
{
    int retVal = OSAL_SOK;
    Uint16 cmd = SAL_msgGetCmd(pMsg);

    switch (curState) {
        case OSAL_TEST_STATE__READY:
            switch (cmd) {
                case OSAL_TEST_CMD__START:
                    OSAL_TEST_PSEQ_Start(pPrc, pMsg, curState);
                    OSAL_tskAckOrFreeMsg(pMsg, retVal);
                    break;

                default:
                    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
                    break;
            }
            break;

        case OSAL_TEST_STATE__RUN:

            switch(cmd) {
                case OSAL_TEST_CMD__STOP:
                    OSAL_TEST_PSEQ_Stop(pPrc, pMsg, curState);
                    OSAL_tskAckOrFreeMsg(pMsg, retVal);
                    break;

                case OSAL_TEST_CMD__PAUSE:
                    OSAL_TEST_PSEQ_Pause(pPrc, pMsg, curState);
                    OSAL_tskAckOrFreeMsg(pMsg, retVal);
                    break;
        
                default:
                    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
                    break;
            }
            break;

        case OSAL_TEST_STATE__PAUSE:
            switch (cmd) {
                case OSAL_TEST_CMD__STOP:
                    OSAL_TEST_PSEQ_Stop(pPrc, pMsg, curState);
                    OSAL_tskAckOrFreeMsg(pMsg, retVal);
                    break;

                case OSAL_TEST_CMD__START:
                case OSAL_TEST_CMD__RESUME:
                    OSAL_TEST_PSEQ_Resume(pPrc, pMsg, curState);
                    OSAL_tskAckOrFreeMsg(pMsg, retVal);
                    break;
        
                default:
                    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
                    break;
            }
            break;

        default:
          OSAL_assert(0);
        break;
    }

    return retVal;
}

int OSAL_TEST_PSTA_MainRun(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    int retVal;
    Bool done=FALSE;
    Uint16 cmd;
    Uint32 delay;

    delay = *(Uint32*)OSAL_msgGetPrm(pMsg);

    // init
    OSAL_waitMsecs(delay);
    OSAL_printf("\r\n PST A : INIT DONE ");
    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);

    OSAL_tskWaitCmd(pPrc, NULL, OSAL_TEST_CMD__START);

    OSAL_printf("\r\n PST A : STARTED");
    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);

    // run
    while (!done) {
        OSAL_waitMsecs(delay);

        if (SAL_tskGetState(pPrc)==OSAL_TEST_STATE__RUN) {
            OSAL_printf("\r\n PST A : RUNNING ");
        }

        retVal = OSAL_tskCheckMsg(pPrc, &pMsg);
        if (retVal!=OSAL_SOK)
            continue;

        cmd = OSAL_msgGetCmd(pMsg);

        switch(cmd) {
            case OSAL_TEST_CMD__STOP:
                done = TRUE;
                break;
      
            case OSAL_TEST_CMD__PAUSE:
                OSAL_waitMsecs(delay);
                OSAL_printf("\r\n PST A : PAUSED ");
                OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
                OSAL_tskSetState(pPrc, OSAL_TEST_STATE__PAUSE);
                break;  

            case OSAL_TEST_CMD__RESUME:
                OSAL_waitMsecs(delay);
                OSAL_printf("\r\n PST A : RESUMED ");
                OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
                OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);
                break;  
      
            default:
                OSAL_assert(0);
                break;
        }
    }

    // exit
    OSAL_waitMsecs(delay);
    OSAL_printf("\r\n PST A : STOPPED");
    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__READY);

    return OSAL_SOK;
}

int OSAL_TEST_PSTA_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState )
{
    Uint16 cmd = OSAL_msgGetCmd(pMsg);

    OSAL_assert(curState == OSAL_TEST_STATE__READY);

    switch(cmd) {
        case OSAL_TEST_CMD__INIT:
            OSAL_TEST_PSTA_MainRun(pPrc, pMsg, curState);
            break;
        
        default:
            OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
        break;
    }

    return OSAL_SOK;
}

int OSAL_TEST_PSTB_MainRun(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
    int retVal;
    Bool done=FALSE;
    Uint16 cmd;
    Uint32 delay;

    delay = *(Uint32*)OSAL_msgGetPrm(pMsg);

    // init
    OSAL_waitMsecs(delay);
    OSAL_printf("\r\n PST B : INIT DONE ");
    OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);

    OSAL_tskWaitCmd(pPrc, NULL, OSAL_TEST_CMD__START);

    OSAL_printf("\r\n PST B : STARTED");
    OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);

  // run
  while(!done) {

    OSAL_waitMsecs(delay);

    if(OSAL_tskGetState(pPrc)==OSAL_TEST_STATE__RUN) {
      SAL_printf("\r\n PST B : RUNNING ");
    }

    retVal = OSAL_tskCheckMsg(pPrc, &pMsg);
    if(retVal!=OSAL_SOK)
      continue;

    cmd = OSAL_msgGetCmd(pMsg);

    switch(cmd) {
      case OSAL_TEST_CMD__STOP:
        done = TRUE;
        break;
      case OSAL_TEST_CMD__PAUSE:
        OSAL_waitMsecs(delay);
        OSAL_printf("\r\n PST B : PAUSED ");
        OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
        OSAL_tskSetState(pPrc, OSAL_TEST_STATE__PAUSE);
        break;  
      case OSAL_TEST_CMD__RESUME:
        OSAL_waitMsecs(delay);
        OSAL_printf("\r\n PST B : RESUMED ");
        OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
        OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);
        break;  
      default:
        OSAL_assert(0);
        break;
    }
  }

  // exit
  OSAL_waitMsecs(delay);
  OSAL_printf("\r\n PST B : STOPPED");
  OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
  OSAL_tskSetState(pPrc, OSAL_TEST_STATE__READY);

  return SAL_SOK;
}

int OSAL_TEST_PSTB_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState )
{
    Uint16 cmd = OSAL_msgGetCmd(pMsg);

    OSAL_assert(curState == OSAL_TEST_STATE__READY);

    switch(cmd) {
        case OSAL_TEST_CMD__INIT:
            OSAL_TEST_PSTB_MainRun(pPrc, pMsg, curState);
            break;
        default:
            OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
            break;
    }

    return OSAL_SOK;
}

int OSAL_TEST_PSTC_MainRun(OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState)
{
  int retVal;
  Bool done=FALSE;
  Uint16 cmd;
  Uint32 delay;

  delay = *(Uint32*)OSAL_msgGetPrm(pMsg);

  // init
  OSAL_waitMsecs(delay);
  OSAL_printf("\r\n PST C : INIT DONE ");
  OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);

  OSAL_tskWaitCmd(pPrc, NULL, OSAL_TEST_CMD__START);

  OSAL_printf("\r\n PST C : STARTED");
  OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);

  // run
  while(!done) {

    OSAL_waitMsecs(delay);

    if(OSAL_tskGetState(pPrc)==OSAL_TEST_STATE__RUN) {
      OSAL_printf("\r\n PST C : RUNNING ");
    }

    retVal = OSAL_tskCheckMsg(pPrc, &pMsg);
    if(retVal!=OSAL_SOK)
      continue;

    cmd = OSAL_msgGetCmd(pMsg);

    switch(cmd) {
      case OSAL_TEST_CMD__STOP:
        done = TRUE;
        break;
      case OSAL_TEST_CMD__PAUSE:
        OSAL_waitMsecs(delay);
        OSAL_printf("\r\n PST C : PAUSED ");
        OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
        OSAL_tskSetState(pPrc, OSAL_TEST_STATE__PAUSE);
        break;  
      case OSAL_TEST_CMD__RESUME:
        OSAL_waitMsecs(delay);
        OSAL_printf("\r\n PST C : RESUMED ");
       	OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
        OSAL_tskSetState(pPrc, OSAL_TEST_STATE__RUN);
        break;  
      default:
        OSAL_assert(0);
        break;
    }
  }

  // exit
  OSAL_waitMsecs(delay);
  OSAL_printf("\r\n PST C : STOPPED");
  OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
  OSAL_tskSetState(pPrc, OSAL_TEST_STATE__READY);

  return OSAL_SOK;
}

int OSAL_TEST_PSTC_Main(struct OSAL_TskHndl *pPrc, OSAL_MsgHndl *pMsg, Uint32 curState )
{
  Uint16 cmd = OSAL_msgGetCmd(pMsg);

  OSAL_assert(curState == OSAL_TEST_STATE__READY);

  switch(cmd) {
    case OSAL_TEST_CMD__INIT:
      OSAL_TEST_PSTC_MainRun(pPrc, pMsg, curState);
      break;
    default:
      OSAL_tskAckOrFreeMsg(pMsg, OSAL_SOK);
      break;
  }

  return OSAL_SOK;
}

int OSAL_TEST_Init()
{
    OSAL_tskCreate(&gSAL_testPseq, OSAL_TEST_PSEQ_Main, 40, OSAL_TEST_PRC_STACK, OSAL_TEST_STATE__READY);
    OSAL_tskCreate(&gSAL_testPstA, OSAL_TEST_PSTA_Main, 50, OSAL_TEST_PRC_STACK, OSAL_TEST_STATE__READY);
    OSAL_tskCreate(&gSAL_testPstB, OSAL_TEST_PSTB_Main, 50, OSAL_TEST_PRC_STACK, OSAL_TEST_STATE__READY);
    OSAL_tskCreate(&gSAL_testPstC, OSAL_TEST_PSTC_Main, 50, OSAL_TEST_PRC_STACK, OSAL_TEST_STATE__READY);

    OSAL_mbxCreate(&gSAL_testMbx);

    return OSAL_SOK;
}

int OSAL_TEST_Exit()
{
    OSAL_tskDelete(&gSAL_testPseq);
    OSAL_tskDelete(&gSAL_testPstA);
    OSAL_tskDelete(&gSAL_testPstB);
    OSAL_tskDelete(&gSAL_testPstC);

    OSAL_mbxDelete(&gSAL_testMbx);

    return OSAL_SOK;
}

char OSAL_TEST_GetInput() 
{
    return getchar();
}

char gSAL_TEST_strMenu[] =
{
// *INDENT-OFF*
"\r\n"
"\r\n ================"
"\r\n KerLib Test Menu"
"\r\n ================"
"\r\n"
"\r\n 1: Start"
"\r\n 2: Stop"
"\r\n 3: Pause"
"\r\n 4: Resume"
"\r\n"
"\r\n 0: Exit"
"\r\n"
"\r\n Enter Choice : "
// *INDENT-ON*
};

int OSAL_TEST_ManualTestRun()
{
  char ch;
  Bool done = FALSE;

  OSAL_TEST_Init();

  do {
    OSAL_printf(gSAL_TEST_strMenu);

    ch = OSAL_TEST_GetInput();
    OSAL_printf("\r\n");

    switch (ch) {

      case '1':
        OSAL_TEST_Start(MANUAL_TEST_DELAY);
        break;

      case '2':
        OSAL_TEST_Stop();
        break;

      case '3':
        OSAL_TEST_Pause();
        break;

      case '4':
        OSAL_TEST_Resume();
        break;

      case '0':
        OSAL_TEST_Stop();
        done = TRUE;
        break;
    }
  } while(!done);

  OSAL_TEST_Exit();

  return OSAL_SOK;
}

int OSAL_TEST_AutoTestRun()
{
    int i, delay, loop, loop2, loop3;
    int count=100;

    loop3 = count;

    delay = AUTO_TEST_DELAY;

    do {
        OSAL_TEST_Init();
        loop2 = 2;

        do {
            loop  = 2;

            // start/stop test
            for (i = 0; i < loop; i++) {
                OSAL_TEST_Start(delay);
                OSAL_waitMsecs(delay);
                OSAL_TEST_Stop();
            }

            // send command in incorrect state
            OSAL_TEST_Pause();
            OSAL_TEST_Resume();
            OSAL_TEST_Stop();

            // pause/resume test
            OSAL_TEST_Start(delay);
      
            for (i = 0; i < loop; i++) {
                OSAL_TEST_Pause();
                OSAL_waitMsecs(delay);
                OSAL_TEST_Resume();
            }

            OSAL_TEST_Stop();
        } while (loop2--);
        OSAL_TEST_Exit();
    } while (loop3--);

    return OSAL_SOK;
}

int OSAL_TEST_SendCmdToPseq(Uint16 cmd, Uint32 delay) {

    Uint32 prm = delay;
    return OSAL_mbxSendMsg( &gSAL_testPseq.mbxHndl, &gSAL_testMbx, cmd, &prm, SAL_MBX_WAIT_ACK);
}

int OSAL_TEST_Start(Uint32 delay)
{
    return OSAL_TEST_SendCmdToPseq(OSAL_TEST_CMD_START, delay);
}

int OSAL_TEST_Pause()
{
    return OSAL_TEST_SendCmdToPseq(OSAL_TEST_CMD_PAUSE, 0);
}

int OSAL_TEST_Resume()
{
    return OSAL_TEST_SendCmdToPseq(OSAL_TEST_CMD_RESUME, 0);
}

int OSAL_TEST_Stop()
{
    return OSAL_TEST_SendCmdToPseq(OSAL_TEST_CMD_STOP, 0);
}




