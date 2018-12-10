/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>

#define LOG_TAG "NetlinkListener"
#include <cutils/log.h>
#include <cutils/uevent.h>

#include <sysutils/NetlinkEvent.h>

#if 1

/**************************************************************************************
** 函数名称: NetlinkListener::NetlinkListener
** 函数功能: 构造函数
** 入口参数: socket - 套接字文件描述符
** 返 回 值: 无
** 调    用: 
****************************************************************************************/
NetlinkListener::NetlinkListener(int socket) : SocketListener(socket, false)
{
    mFormat = NETLINK_FORMAT_ASCII;
}
#endif


/**************************************************************************************
** 函数名称: NetlinkListener::NetlinkListener
** 函数功能: 构造函数
** 入口参数: socket - 套接字文件描述符
**			 format - 接收数据的格式
** 返 回 值: 无
** 调    用: 
****************************************************************************************/
NetlinkListener::NetlinkListener(int socket, int format) :
                            SocketListener(socket, false), mFormat(format) 
{
}




/**************************************************************************************
** 函数名称: NetlinkListener::onDataAvailable
** 函数功能: 接收并处理来自NetLink套接字的数据
** 入口参数: cli - SocketClient对象指针
** 返 回 值: 成功返回0; 失败返回-1
** 调    用: 
****************************************************************************************/
bool NetlinkListener::onDataAvailable(SocketClient *cli)
{
    int socket = cli->getSocket();
    ssize_t count;
    uid_t uid = -1;

	/* 读取内核传递的数据放入NetlinkListener对象的内部缓冲区mBuffer中 */
    count = TEMP_FAILURE_RETRY(uevent_kernel_multicast_uid_recv(
                                       socket, mBuffer, sizeof(mBuffer), &uid));
    if (count < 0) {
        if (uid > 0)
            LOG_EVENT_INT(65537, uid);
		
        SLOGE("recvmsg failed (%s)", strerror(errno));
        return false;
    }

    NetlinkEvent *evt = new NetlinkEvent();			/* 构造一个NetlinkEvent对象 */
    if (evt->decode(mBuffer, count, mFormat)) {		/* 解析内核传递的数据 */
        onEvent(evt);								/* 交给上传NetLinkHandler来处理该NetLinkEvent事件 */
    } else if (mFormat != NETLINK_FORMAT_BINARY) {
        SLOGE("Error decoding NetlinkEvent");
    }

    delete evt;		/* 处理完成后是否该NetLinkEvent对象 */
    return true;	/* 返回true表示正常处理了 */
}

