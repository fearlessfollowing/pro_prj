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
#include <string.h>
#include <stdlib.h>

#define LOG_TAG "FrameworkListener"

#include <cutils/log.h>

#include <sysutils/FrameworkListener.h>
#include <sysutils/FrameworkCommand.h>
#include <sysutils/SocketClient.h>

static const int CMD_BUF_SIZE = 1024;

#define UNUSED __attribute__((unused))



FrameworkListener::FrameworkListener(const char *socketName, bool withSeq) :
                            SocketListener(socketName, true, withSeq) 
{
    init(socketName, withSeq);
}

FrameworkListener::FrameworkListener(const char *socketName) : SocketListener(socketName, true, false) 
{
    init(socketName, false);
}


FrameworkListener::FrameworkListener(int sock) : SocketListener(sock, true)
{
    init(NULL, false);
}


void FrameworkListener::init(const char *socketName UNUSED, bool withSeq) 
{
    mCommands = new FrameworkCommandCollection();
    errorRate = 0;
    mCommandCount = 0;
    mWithSeq = withSeq;
}



/**************************************************************************************
** 函数名称: FrameworkListener::onDataAvailable
** 函数功能: 当监听线程监听到客户端有数据到来时,调用该函数来处理
** 入口参数: c - 客户端对象指针
** 返 回 值: 成功返回true;失败返回false
** 调    用: SocketListener::runListener
****************************************************************************************/
bool FrameworkListener::onDataAvailable(SocketClient *c) 
{
    char buffer[CMD_BUF_SIZE];
    int len;

	/* 从套接字中读取数据 */
    len = TEMP_FAILURE_RETRY(read(c->getSocket(), buffer, sizeof(buffer)));
    if (len < 0) {
        SLOGE("read() failed (%s)", strerror(errno));
        return false;
    } else if (!len)
        return false;
	
   if (buffer[len-1] != '\0')
        SLOGW("String is not zero-terminated");

    int offset = 0;
    int i;

    for (i = 0; i < len; i++) {		/* 依次处理各个字符串命令 */
        if (buffer[i] == '\0') {
            /* IMPORTANT: dispatchCommand() expects a zero-terminated string */
            dispatchCommand(c, buffer + offset);
            offset = i + 1;
        }
    }

    return true;
}



/**************************************************************************************
** 函数名称: FrameworkListener::registerCmd
** 函数功能: 注册一个FrameworkCommand对象到FrameworkListener命令链表中
** 入口参数: cmd - 命令对象指针
** 返 回 值: 无
** 调    用: 
****************************************************************************************/
void FrameworkListener::registerCmd(FrameworkCommand *cmd) 
{
    mCommands->push_back(cmd);
}


/*
	mConnector.execute("volume", "mount", path);
	命令: volume 对应VolumeCmd
	"mount path"

*/

/**************************************************************************************
** 函数名称: FrameworkListener::dispatchCommand
** 函数功能: 注册一个FrameworkCommand对象到FrameworkListener命令链表中
** 入口参数: cmd - 命令对象指针
** 返 回 值: 无
** 调    用: 
****************************************************************************************/
void FrameworkListener::dispatchCommand(SocketClient *cli, char *data) 
{
    FrameworkCommandCollection::iterator i;
    int argc = 0;
    char *argv[FrameworkListener::CMD_ARGS_MAX];
    char tmp[CMD_BUF_SIZE];
    char *p = data;
    char *q = tmp;
    char *qlimit = tmp + sizeof(tmp) - 1;
    bool esc = false;
    bool quote = false;
    bool haveCmdNum = !mWithSeq;

    memset(argv, 0, sizeof(argv));
    memset(tmp, 0, sizeof(tmp));
	
    while (*p) {
        if (*p == '\\') {
            if (esc) {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
                esc = false;
            } else
                esc = true;
            p++;
            continue;
        } else if (esc) {
            if (*p == '"') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '"';
            } else if (*p == '\\') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
            } else {
                cli->sendMsg(500, "Unsupported escape sequence", false);
                goto out;
            }
            p++;
            esc = false;
            continue;
        }

        if (*p == '"') {
            if (quote)
                quote = false;
            else
                quote = true;
            p++;
            continue;
        }

        if (q >= qlimit)
            goto overflow;
		
        *q = *p++;
		
		/* 如果是在成对的引号'"'外面出现空格, 表示一个字段解析完成 */
        if (!quote && *q == ' ') {	/* ["volume" "mount" "-t vfat /dev/xxx /mnt/storage/xxx"  */
            *q = '\0';				/* 将空格替换成'\0' */
			
            if (!haveCmdNum) {		/* 如果字串中还没有解析出命令,命令在字串的最前面 */
                char *endptr;
                int cmdNum = (int)strtol(tmp, &endptr, 0);	/* 解析出命令号 */
                if (endptr == NULL || *endptr != '\0') {
                    cli->sendMsg(500, "Invalid sequence number", false);
                    goto out;
                }
                cli->setCmdNum(cmdNum);		/* 设置命令号, 如: 500 */
                haveCmdNum = true;			/* 表示字串中已经解析出了命令号字段 */
            } else {
                if (argc >= CMD_ARGS_MAX)	/* 参数的个数过多 */
                    goto overflow;
				
                argv[argc++] = strdup(tmp);	/* 提取参数 */
            }
            memset(tmp, 0, sizeof(tmp));
            q = tmp;
            continue;
        }
        q++;
    }

    *q = '\0';
    if (argc >= CMD_ARGS_MAX)
        goto overflow;
	
    argv[argc++] = strdup(tmp);
	
#if 0
    for (int k = 0; k < argc; k++) {
        SLOGD("arg[%d] = '%s'", k, argv[k]);
    }
#endif

    if (quote) {	/* 引号'"'不是成对出现,表示命令串有问题 */
        cli->sendMsg(500, "Unclosed quotes error", false);
        goto out;
    }

    if (errorRate && (++mCommandCount % errorRate == 0)) {
        /* ignore this command - let the timeout handler handle it */
        SLOGE("Faking a timeout");
        goto out;
    }

	/* 遍历mCommands链表,判断是否有FrameworkCommand对象能解析该命令 */
    for (i = mCommands->begin(); i != mCommands->end(); ++i) {
        FrameworkCommand *c = *i;

        if (!strcmp(argv[0], c->getCommand())) {	/* 匹配的条件: 接收到的命令串与FrameworkCommand对象的mCommand名称一致 */
            if (c->runCommand(cli, argc, argv)) {	/* 调用FrameworkCommand.runCommand来处理该命令 */
                SLOGW("Handler '%s' error (%s)", c->getCommand(), strerror(errno));
            }
            goto out;
        }
    }

	/* 如果没有FrameworkCommand对象能解析该命令返回"Command not recognized"消息 */
    cli->sendMsg(500, "Command not recognized", false);
	
out:
    int j;
    for (j = 0; j < argc; j++)
        free(argv[j]);
    return;

overflow:
    LOG_EVENT_INT(78001, cli->getUid());
    cli->sendMsg(500, "Command too long", false);
    goto out;
}

