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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#define LOG_TAG "SocketListener"
#include <cutils/log.h>
#include <cutils/sockets.h>

#include <sysutils/SocketListener.h>
#include <sysutils/SocketClient.h>

#define CtrlPipe_Shutdown 0
#define CtrlPipe_Wakeup   1

SocketListener::SocketListener(const char *socketName, bool listen) 
{
    init(socketName, -1, listen, false);
}

SocketListener::SocketListener(int socketFd, bool listen)
{
    init(NULL, socketFd, listen, false);
}

SocketListener::SocketListener(const char *socketName, bool listen, bool useCmdNum)
{
    init(socketName, -1, listen, useCmdNum);
}

void SocketListener::init(const char *socketName, int socketFd, bool listen, bool useCmdNum) 
{
    mListen = listen;
    mSocketName = socketName;
    mSock = socketFd;
    mUseCmdNum = useCmdNum;
    pthread_mutex_init(&mClientsLock, NULL);
    mClients = new SocketClientCollection();
}

SocketListener::~SocketListener() 
{
    if (mSocketName && mSock > -1)
        close(mSock);

    if (mCtrlPipe[0] != -1) {
        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);
    }
	
    SocketClientCollection::iterator it;
    for (it = mClients->begin(); it != mClients->end();) {
        (*it)->decRef();
        it = mClients->erase(it);
    }
	
    delete mClients;
}


int SocketListener::startListener() 
{
    return startListener(4);
}



/**************************************************************************************
** 函数名称: SocketListener::startListener
** 函数功能: 启动监听器,监听指定套接字文件描述符的事件
** 入口参数: backlog - 使用listen监听时队列的长度
** 返 回 值: 成功返回0; 失败返回-1
** 调    用: 
****************************************************************************************/
int SocketListener::startListener(int backlog) 
{

    if (!mSocketName && mSock == -1) {	/* 如果没有指定套接字的名称(unix套接字)也套接字文件描述符也非法,出错返回 */
        SLOGE("Failed to start unbound listener");
        errno = EINVAL;
        return -1;
    } else if (mSocketName) {	/* 如果是指定了套接字的名称,通过名称来获取对应的文件描述符 */
        if ((mSock = android_get_control_socket(mSocketName)) < 0) {
            SLOGE("Obtaining file descriptor socket '%s' failed: %s", mSocketName, strerror(errno));
            return -1;
        }
        SLOGV("got mSock = %d for %s", mSock, mSocketName);
    }

    if (mListen && listen(mSock, backlog) < 0) {	/* 如果需要监听,调用listen进行监听,对于netlink套接字不需要监听 */
        SLOGE("Unable to listen on socket (%s)", strerror(errno));
        return -1;
    } else if (!mListen) {	/* 对于NetLink套接字构建一个SocketClient对象,加入到SocketListener.mClients链表中 */
        mClients->push_back(new SocketClient(mSock, false, mUseCmdNum));
	}

    if (pipe(mCtrlPipe)) {		/* 创建管道,用于监听线程与其它线程的通信(主要是控制监听线程结束) */
        SLOGE("pipe failed (%s)", strerror(errno));
        return -1;
    }

    if (pthread_create(&mThread, NULL, SocketListener::threadStart, this)) {	/* 创建监听线程 */
        SLOGE("pthread_create (%s)", strerror(errno));
        return -1;
    }

    return 0;
}



/**************************************************************************************
** 函数名称: SocketListener::stopListener
** 函数功能: 停止监听
** 入口参数: 
** 返 回 值: 成功返回0; 失败返回-1
** 调    用: 
****************************************************************************************/
int SocketListener::stopListener() 
{
    char c = CtrlPipe_Shutdown;		/* c = 0 */
    int  rc;	

	/* 往管道的写端写入数据,监听线程的读端检查该事件后将退出 */
    rc = TEMP_FAILURE_RETRY(write(mCtrlPipe[1], &c, 1));
    if (rc != 1) {
        SLOGE("Error writing to control pipe (%s)", strerror(errno));
        return -1;
    }

    void *ret;
    if (pthread_join(mThread, &ret)) {	/* 等待监听线程的退出 */
        SLOGE("Error joining to listener thread (%s)", strerror(errno));
        return -1;
    }
	
    close(mCtrlPipe[0]);	/* 关闭管道的读写端 */
    close(mCtrlPipe[1]);
    mCtrlPipe[0] = -1;
    mCtrlPipe[1] = -1;

	/* 关闭套接字 */
    if (mSocketName && mSock > -1) {
        close(mSock);
        mSock = -1;
    }

	/* 移除该监听器上的所有SocketClient对象 */
    SocketClientCollection::iterator it;
    for (it = mClients->begin(); it != mClients->end();) {
        delete (*it);
        it = mClients->erase(it);
    }
    return 0;
}



/**************************************************************************************
** 函数名称: SocketListener::threadStart
** 函数功能: 监听器线程执行函数
** 入口参数: obj - 传递给执行线程的参数
** 返 回 值: NULL
** 调    用: SocketListener::startListener
****************************************************************************************/
void *SocketListener::threadStart(void *obj) 
{
    SocketListener *me = reinterpret_cast<SocketListener *>(obj);

    me->runListener();		/* 线程的执行主体为SocketListener::runListener()方法 */
    pthread_exit(NULL);
    return NULL;
}



/**************************************************************************************
** 函数名称: SocketListener::runListener
** 函数功能: 监听器线程执行函数
** 入口参数: obj - 传递给执行线程的参数
** 返 回 值: NULL
** 调    用: SocketListener::startListener
****************************************************************************************/
void SocketListener::runListener()
{

    SocketClientCollection pendingList;

    while (1) {
		
        SocketClientCollection::iterator it;
		
        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);

        if (mListen) {						/* 如果需要接听套接字: 用于客户端发起连接的套接字(其它的用户进程) */
            max = mSock;
            FD_SET(mSock, &read_fds);
        }

        FD_SET(mCtrlPipe[0], &read_fds);	/* 设置监听对象: 管道的读端 */
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];

        pthread_mutex_lock(&mClientsLock);
        for (it = mClients->begin(); it != mClients->end(); ++it) {		/* 监听套接字: 已经与客户端建立起连接的套接字或用于与内核进行通信(NetLink) */
            int fd = (*it)->getSocket();
            FD_SET(fd, &read_fds);
            if (fd > max) {
                max = fd;
            }
        }
        pthread_mutex_unlock(&mClientsLock);
		
        //printf("mListen=%d, max=%d, mSocketName=%s\n", mListen, max, mSocketName);
		
        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	/* select无限期的监听,直到有数据的到来 */
            if (errno == EINTR)
                continue;
            printf("select failed (%s) mListen=%d, max=%d\n", strerror(errno), mListen, max);
            sleep(1);
            continue;
        } else if (!rc)
            continue;

        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	/* 如果是有管道数据到来 */
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	/* 如果是关闭管道消息,表示需要监听线程退出 */
            if (c == CtrlPipe_Shutdown) {
                break;
            }
            continue;
        }
		
        if (mListen && FD_ISSET(mSock, &read_fds)) {	/* 如果是有客户进程发起连接 */
            struct sockaddr addr;
            socklen_t alen;
            int c;

			/* 建立与客户端的连接 */
            do {
                alen = sizeof(addr);
                c = accept(mSock, &addr, &alen);
                printf("%s got %d from accept\n", mSocketName, c);
            } while (c < 0 && errno == EINTR);
			
            if (c < 0) {
                printf("accept failed (%s)\n", strerror(errno));
                sleep(1);
                continue;
            }
			
            pthread_mutex_lock(&mClientsLock);
            mClients->push_back(new SocketClient(c, true, mUseCmdNum));		/* 构造一个SocketClient对象加入监听器的mClients链表中 */
            pthread_mutex_unlock(&mClientsLock);
        }

        pendingList.clear();
		
        pthread_mutex_lock(&mClientsLock);	/* 检查是否有客户端(内核)发来数据,并将这些SocketClient临时加入pendingList链表中 */
        for (it = mClients->begin(); it != mClients->end(); ++it) {
            SocketClient* c = *it;
            int fd = c->getSocket();
            if (FD_ISSET(fd, &read_fds)) {
                pendingList.push_back(c);
                c->incRef();
            }
        }
        pthread_mutex_unlock(&mClientsLock);

        while (!pendingList.empty()) {	/* 依次处理pendingList链表中SocketClient传递的数据 */
			
            /* 从pendingList链表中取出并移除SocketClient节点 */
            it = pendingList.begin();
            SocketClient* c = *it;
            pendingList.erase(it);
			
            if (!onDataAvailable(c)) {	/* 处理该SocketClient传递的数据 */
                release(c, false);	
            }
            c->decRef();
        }
    }
}


bool SocketListener::release(SocketClient* c, bool wakeup) 
{
    bool ret = false;
	
    /* if our sockets are connection-based, remove and destroy it */
    if (mListen && c) {
        /* Remove the client from our array */
        SLOGV("going to zap %d for %s", c->getSocket(), mSocketName);
        pthread_mutex_lock(&mClientsLock);
        SocketClientCollection::iterator it;
        for (it = mClients->begin(); it != mClients->end(); ++it) {
            if (*it == c) {
                mClients->erase(it);
                ret = true;
                break;
            }
        }
        pthread_mutex_unlock(&mClientsLock);
		
        if (ret) {
            ret = c->decRef();
            if (wakeup) {
                char b = CtrlPipe_Wakeup;
                TEMP_FAILURE_RETRY(write(mCtrlPipe[1], &b, 1));
            }
        }
    }
	
    return ret;
}

void SocketListener::sendBroadcast(int code, const char *msg, bool addErrno) 
{
    SocketClientCollection safeList;

    /* Add all active clients to the safe list first */
    safeList.clear();
    pthread_mutex_lock(&mClientsLock);
    SocketClientCollection::iterator i;

    for (i = mClients->begin(); i != mClients->end(); ++i) {
        SocketClient* c = *i;
        c->incRef();
        safeList.push_back(c);
    }
    pthread_mutex_unlock(&mClientsLock);

    while (!safeList.empty()) {
		
        /* Pop the first item from the list */
        i = safeList.begin();
        SocketClient* c = *i;
        safeList.erase(i);
		
        // broadcasts are unsolicited and should not include a cmd number
        if (c->sendMsg(code, msg, addErrno, false)) {
            SLOGW("Error sending broadcast (%s)", strerror(errno));
        }
        c->decRef();
    }
}

void SocketListener::runOnEachSocket(SocketClientCommand *command) 
{
    SocketClientCollection safeList;

    /* Add all active clients to the safe list first */
    safeList.clear();
    pthread_mutex_lock(&mClientsLock);
    SocketClientCollection::iterator i;

    for (i = mClients->begin(); i != mClients->end(); ++i) {
        SocketClient* c = *i;
        c->incRef();
        safeList.push_back(c);
    }
    pthread_mutex_unlock(&mClientsLock);

    while (!safeList.empty()) {
        /* Pop the first item from the list */
        i = safeList.begin();
        SocketClient* c = *i;
        safeList.erase(i);
        command->runSocketCommand(c);
        c->decRef();
    }
}

