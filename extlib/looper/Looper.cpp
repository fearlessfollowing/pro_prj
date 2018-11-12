#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "Looper.h"


// Hint for number of file descriptors to be associated with the epoll instance.
static const int EPOLL_SIZE_HINT = 8;

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;

static pthread_once_t   gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t    gTLSKey = 0;


#define LOOPER_LOG_ASSERT(x)  \
    if( (x) == 0) { \
        fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
        while (getchar()!='q'); \
    } 


static nsecs_t systemTime(int /*clock*/)
{
    // Clock support varies widely across hosts. Mac OS doesn't support
    // posix clocks, older glibcs don't support CLOCK_BOOTTIME and Windows is windows.
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return nsecs_t(t.tv_sec)*1000000000LL + nsecs_t(t.tv_usec)*1000LL;
}


static int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime)
{
    int timeoutDelayMillis;
    if (timeoutTime > referenceTime) {
        uint64_t timeoutDelay = uint64_t(timeoutTime - referenceTime);
        if (timeoutDelay > uint64_t((INT_MAX - 1) * 1000000LL)) {
            timeoutDelayMillis = -1;
        } else {
            timeoutDelayMillis = (timeoutDelay + 999999LL) / 1000000LL;
        }
    } else {
        timeoutDelayMillis = 0;
    }
    return timeoutDelayMillis;
}


/***************************************************************************************
** 函数名称: Looper::Looper
** 函数功能: Looper对象构造函数
** 入口参数: allowNonCallbacks - 是否支持回调
** 返 回 值: 无
**
**
*****************************************************************************************/
Looper::Looper(bool allowNonCallbacks): mSendingMessage(false),
									    mResponseIndex(0), 
									    mNextMessageUptime(LLONG_MAX)
{
    int wakeFds[2];

	/* 创建匿名管道 */
    int result = pipe(wakeFds);	
    LOOPER_LOG_ASSERT(result != 0)

	/* Looper对象保存管道两端的文件描述符 */
    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];

	/* 设置对管道的读为非阻塞方式 */
    result = fcntl(mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
    LOOPER_LOG_ASSERT(result != 0)


	/* 设置对管道的写为非阻塞方式 */
    result = fcntl(mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
    LOOPER_LOG_ASSERT(result != 0)


    mIdling = false;

    /* 创建epoll对象 */
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    LOOPER_LOG_ASSERT(mEpollFd < 0)


    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(epoll_event)); 
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mWakeReadPipeFd;

	/* 将管道读端文件描述符加入到epoll对象中, 检测发往管道的数据 */
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, &eventItem);
    LOOPER_LOG_ASSERT(result != 0)

}



/***************************************************************************************
** 函数名称: Looper::~Looper
** 函数功能: Looper对象析构函数
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
Looper::~Looper() 
{
	/* 关闭管道两端的文件描述符 */
    close(mWakeReadPipeFd);
    close(mWakeWritePipeFd);
	
	/* 关闭epoll文件描述符 */
    close(mEpollFd);
}


void Looper::initTLSKey()
{
    int result = pthread_key_create(&gTLSKey, NULL);
    LOOPER_LOG_ASSERT(result != 0)
}


int Looper::pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) 
{
    int result = 0;

    for (;;) {

        if (result != 0) {
            fprintf(stdout, "%p ~ pollOnce - returning result %d\n", this, result);			
            if (outFd != NULL) 
				*outFd = 0;
            if (outEvents != NULL)
				*outEvents = 0;
            if (outData != NULL) 
				*outData = NULL;
            return result;
        }

        result = pollInner(timeoutMillis);	/* 调用pllInner等待数据 */
    }
}




int Looper::pollInner(int timeoutMillis)
{
    fprintf(stdout, "%p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);

	/* 1.计算出本次轮询等待的超时时间 */
    // Adjust the timeout based on when the next message is due.
    if (timeoutMillis != 0 && mNextMessageUptime != LLONG_MAX) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        int messageTimeoutMillis = toMillisecondTimeoutDelay(now, mNextMessageUptime);
		
        if (messageTimeoutMillis >= 0 && (timeoutMillis < 0 || messageTimeoutMillis < timeoutMillis)) {
            timeoutMillis = messageTimeoutMillis;
        }
		
        fprintf(stdout, "%p ~ pollOnce - next message in %lldns, adjusted timeout: timeoutMillis=%d\n",
                this, mNextMessageUptime - now, timeoutMillis);
    }

    // Poll.
    int result = POLL_WAKE;

    mIdling = true;		/* 设置状态为Idle状态 */

	/* 3.调用epoll_wait等待事件的到来或者等待超时 */
    struct epoll_event eventItems[EPOLL_MAX_EVENTS];
    int eventCount = epoll_wait(mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);

    mIdling = false;	/* 等待返回,不再处于Idle状态 */

    // Acquire lock.
    mLock.lock();

    if (eventCount < 0) {	/* 调用epoll_wait出错 */
        if (errno == EINTR) {	/* 被信号中断 */
            goto Done;
        }
        fprintf(stderr, "Poll failed with an unexpected error, errno=%d\n", errno);
        result = POLL_ERROR;
        goto Done;
    }

    if (eventCount == 0) {
        fprintf(stdout, "%p ~ pollOnce - timeout\n", this);		
        result = POLL_TIMEOUT;
        goto Done;
    }

    fprintf(stdout, "%p ~ pollOnce - handling events from %d fds", this, eventCount);

    for (int i = 0; i < eventCount; i++) {
        int fd = eventItems[i].data.fd;
        uint32_t epollEvents = eventItems[i].events;
		
        if (fd == mWakeReadPipeFd) {		/* 监控的文件描述符为管道读文件描述符 */
            if (epollEvents & EPOLLIN) {	/* 管道有数据可读 */		
                awoken();
            } else {
                fprintf(stderr, "Ignoring unexpected epoll events 0x%x on wake read pipe.\n", epollEvents);
            }
        }
    }
	
Done: ;

    /* 检查Looper的消息容器是否有待处理的消息
     * 如果有消息,将调用消息的callbacks来处理消息
     */
    mNextMessageUptime = LLONG_MAX;

    while (mMessageEnvelopes.size() != 0) {		/* 消息容器中是否有消息待处理 */
		
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);	/* 获取当前的系统时间 */

        const MessageEnvelope& messageEnvelope = mMessageEnvelopes.itemAt(0);
        if (messageEnvelope.uptime <= now) {	/* 消息的截止时间已过,立即处理它 */
            { // obtain handler
                std::shared_ptr<MessageHandler> handler = messageEnvelope.handler;
                Message message = messageEnvelope.message;
                mMessageEnvelopes.removeAt(0);
                mSendingMessage = true;
                mLock.unlock();

                fprintf(stdout, "%p ~ pollOnce - sending message: handler=%p, what=%d",
                        this, handler.get(), message.what);

				handler->handleMessage(message);	/* 处理该消息 */
            } // release handler

            mLock.lock();
            mSendingMessage = false;
            result = POLL_CALLBACK;
        } else {	/* 如果消息队列中的第一个消息的截止时间未到,mNextMessageUptime记录该时间 */
            mNextMessageUptime = messageEnvelope.uptime;
            break;
        }
    }

    mLock.unlock();
    return result;
}




int Looper::pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData)
{
    if (timeoutMillis <= 0) {
        int result;
        do {
            result = pollOnce(timeoutMillis, outFd, outEvents, outData);
        } while (result == POLL_CALLBACK);
        return result;
    } else {
        nsecs_t endTime = systemTime(SYSTEM_TIME_MONOTONIC) + milliseconds_to_nanoseconds(timeoutMillis);

        for (;;) {
            int result = pollOnce(timeoutMillis, outFd, outEvents, outData);
            if (result != POLL_CALLBACK) {
                return result;
            }

            nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            timeoutMillis = toMillisecondTimeoutDelay(now, endTime);
            if (timeoutMillis == 0) {
                return POLL_TIMEOUT;
            }
        }
    }
}



/***************************************************************************************
** 函数名称: Looper::wake
** 函数功能: 往Looper的管道的写端写入数据,以便立即唤醒调用Looper.pollOnce的线程
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::wake() 
{
    fprintf(stdout, "%p ~ wake", this);

    ssize_t nWrite;
    do {
        nWrite = write(mWakeWritePipeFd, "W", 1);
    } while (nWrite == -1 && errno == EINTR);

    if (nWrite != 1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Could not write wake signal, errno=%d\n", errno);
        }
    }
}


void Looper::awoken() 
{
    fprintf(stdout, "%p ~ awoken\n", this);

    char buffer[16];
    ssize_t nRead;
    do {
        nRead = read(mWakeReadPipeFd, buffer, sizeof(buffer));
    } while ((nRead == -1 && errno == EINTR) || nRead == sizeof(buffer));
}


/***************************************************************************************
** 函数名称: Looper::sendMessage
** 函数功能: 发送消息到Looper的消息队列中
** 入口参数: handler - 消息的处理对象指针
**			 message - 消息对象
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::sendMessage(const sp<MessageHandler>& handler, const Message& message) 
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now, handler, message);
}



/***************************************************************************************
** 函数名称: Looper::sendMessageDelayed
** 函数功能: 发送延时消息到Looper的消息队列中
** 入口参数: uptimeDelay - 消息的延时值
**			 handler - 消息的处理对象指针
**			 message - 消息对象
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::sendMessageDelayed(nsecs_t uptimeDelay, const std::shared_ptr<MessageHandler>& handler, const Message& message) 
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now + uptimeDelay, handler, message);
}



/***************************************************************************************
** 函数名称: Looper::sendMessageAtTime
** 函数功能: 发送消息到Looper的消息队列中
** 入口参数: uptime - 消息的截至时刻
**			 handler - 消息的处理对象指针
**			 message - 消息对象
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::sendMessageAtTime(nsecs_t uptime, const std::shared_ptr<MessageHandler>& handler, const Message& message) 
{

    size_t i = 0;
    {
        AutoMutex _l(mLock);

        size_t messageCount = mMessageEnvelopes.size();		/* 获取Looper消息队列当前消息的个数 */

		/* 判断待发送的消息在消息队列中的排放位置,按消息的到期时间为键值 */
        while (i < messageCount && uptime >= mMessageEnvelopes.itemAt(i).uptime) {
            i += 1;
        }

		/* 构造一个消息信封对象 */
        MessageEnvelope messageEnvelope(uptime, handler, message);

		/* 将消息信封插入队列容器中 */
        mMessageEnvelopes.insertAt(messageEnvelope, i, 1);

        /* 如果mSendingMessage为true,表明线程正在派送消息处于活动状态 */
        if (mSendingMessage) {
            return;
        }
    } 

    /* 如果消息队列原本为空,唤醒读消息的线程 */
    if (i == 0) {
        wake();		/* 唤醒调用Looper.pollOnce的线程 */
    }
}



/***************************************************************************************
** 函数名称: Looper::removeMessages
** 函数功能: 从Looper的消息容器中移除指定(指定处理Handler)的消息
** 入口参数: handler - 消息对应的Handler对象指针
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::removeMessages(const sp<MessageHandler>& handler)
{
    {
        AutoMutex _l(mLock);

        for (size_t i = mMessageEnvelopes.size(); i != 0; ) {
            const MessageEnvelope& messageEnvelope = mMessageEnvelopes.itemAt(--i);
            if (messageEnvelope.handler == handler) {
                mMessageEnvelopes.removeAt(i);
            }
        }
    }
}


/***************************************************************************************
** 函数名称: Looper::removeMessages
** 函数功能: 从Looper的消息容器中移除指定(指定处理Handler及类型what)的消息
** 入口参数: handler - 消息对应的Handler对象指针
**			 what - 消息的类型值
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::removeMessages(const sp<MessageHandler>& handler, int what) 
{
    {
        AutoMutex _l(mLock);

        for (size_t i = mMessageEnvelopes.size(); i != 0; ) {
            const MessageEnvelope& messageEnvelope = mMessageEnvelopes.itemAt(--i);
            if (messageEnvelope.handler == handler && messageEnvelope.message.what == what) {
                mMessageEnvelopes.removeAt(i);
            }
        }
    }
}


/***************************************************************************************
** 函数名称: Looper::isIdling
** 函数功能: 判断Looper线程是否为空闲状态
** 入口参数: 无
** 返 回 值: 空闲状态返回true;否则返回false
**
**
*****************************************************************************************/
bool Looper::isIdling() const 
{
    return mIdling;
}