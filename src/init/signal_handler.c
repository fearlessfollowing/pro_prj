#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <cutils/sockets.h>
#include <cutils/android_reboot.h>
#include <cutils/list.h>

#include "init.h"
#include "util.h"
#include "log.h"

static int signal_fd = -1;
static int signal_recv_fd = -1;

#define CRITICAL_CRASH_THRESHOLD    4           /* if we crash >4 times ... */
#define CRITICAL_CRASH_WINDOW       (4*60)      /* ... in 4 minutes, goto recovery*/


static void sigchld_handler(int s)
{
    write(signal_fd, &s, 1);
}


static int wait_for_one_process(int block)
{
    pid_t pid;
    int status;
    struct service *svc;
    struct socketinfo *si;
    struct listnode *node;
    struct command *cmd;

    while ( (pid = waitpid(-1, &status, block ? 0 : WNOHANG)) == -1 && errno == EINTR );
    if (pid <= 0) return -1;
    INFO("waitpid returned pid %d, status = %08x\n", pid, status);

    svc = service_find_by_pid(pid);
    if (!svc) {
        ERROR("untracked pid %d exited\n", pid);
        return 0;
    }

    NOTICE("process '%s', pid %d exited\n", svc->name, pid);

    if (!(svc->flags & SVC_ONESHOT)) {
        kill(-pid, SIGKILL);
        NOTICE("process '%s' killing any children in process group\n", svc->name);
    }

    /* remove any sockets we may have created */
    for (si = svc->sockets; si; si = si->next) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), ANDROID_SOCKET_DIR"/%s", si->name);
        unlink(tmp);
    }

    svc->pid = 0;
    svc->flags &= (~SVC_RUNNING);

        /* oneshot processes go into the disabled state on exit */
    if (svc->flags & SVC_ONESHOT) {
        svc->flags |= SVC_DISABLED;
    }

        /* disabled and reset processes do not get restarted automatically */
    if (svc->flags & (SVC_DISABLED | SVC_RESET) )  {
        notify_service_state(svc->name, "stopped");
        return 0;
    }

	
    svc->flags |= SVC_RESTARTING;

    /* Execute all onrestart commands for this service. */
    list_for_each(node, &svc->onrestart.commands) {
        cmd = node_to_item(node, struct command, clist);
        cmd->func(cmd->nargs, cmd->args);
    }
    notify_service_state(svc->name, "restarting");
    return 0;
}

void handle_signal(void)
{
    char tmp[32];

    /* we got a SIGCHLD - reap and restart as needed */
    read(signal_recv_fd, tmp, sizeof(tmp));
    while (!wait_for_one_process(0));
}

void signal_init(void)
{
    int s[2];
    struct sigaction act;
    
    memset(&act, 0, sizeof(act));
    act.sa_handler = sigchld_handler;
    act.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &act, 0);	/* 接收子进程退出的信号 */
    // sigaction(SIGINT, &act, 0);		/* 接收中断信号 */
    // sigaction(SIGKILL, &act, 0);	/* 接收强制终止信号 */

    /* create a signalling mechanism for the sigchld handler */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) == 0) {
        signal_fd = s[0];
        signal_recv_fd = s[1];
        fcntl(s[0], F_SETFD, FD_CLOEXEC);
        fcntl(s[0], F_SETFL, O_NONBLOCK);
        fcntl(s[1], F_SETFD, FD_CLOEXEC);
        fcntl(s[1], F_SETFL, O_NONBLOCK);
    }

    handle_signal();
}

int get_signal_fd()
{
    return signal_recv_fd;
}
