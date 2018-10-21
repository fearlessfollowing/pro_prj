/*
 * Copyright (C) 2008-2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _SOCKETLISTENER_H
#define _SOCKETLISTENER_H

#include <pthread.h>

#include <sysutils/SocketClient.h>
#include "SocketClientCommand.h"

class SocketListener {

    bool                    mListen;			/* 是否需要使用listen进行监听 */
    const char              *mSocketName;		/* 监听器的名称 */
    int                     mSock;				/* 监听器的套接字文件描述符 */
    SocketClientCollection  *mClients;			/* 连接到该监听器的Client链表 */
    pthread_mutex_t         mClientsLock;		/* 保护监听器的线程互斥锁 */
    int                     mCtrlPipe[2];		/* 监听器的管道 */
    pthread_t               mThread;			/* 监听线程的线程ID */
    bool                    mUseCmdNum;

public:
    SocketListener(const char *socketName, bool listen);
    SocketListener(const char *socketName, bool listen, bool useCmdNum);
    SocketListener(int socketFd, bool listen);

    virtual ~SocketListener();
    int startListener();
    int startListener(int backlog);
    int stopListener();

    void sendBroadcast(int code, const char *msg, bool addErrno);

    void runOnEachSocket(SocketClientCommand *command);

    bool release(SocketClient *c) { return release(c, true); }

protected:
    virtual bool onDataAvailable(SocketClient *c) = 0;

private:
    bool release(SocketClient *c, bool wakeup);
    static void *threadStart(void *obj);
    void runListener();
    void init(const char *socketName, int socketFd, bool listen, bool useCmdNum);
};

#endif
