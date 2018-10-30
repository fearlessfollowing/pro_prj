#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "Looper.h"


// Hint for number of file descriptors to be associated with the epoll instance.
static const int EPOLL_SIZE_HINT = 8;

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;

static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSKey = 0;





/***************************************************************************************
** 函数名称: Looper::Looper
** 函数功能: Looper对象构造函数
** 入口参数: allowNonCallbacks - 是否支持回调
** 返 回 值: 无
**
**
*****************************************************************************************/
Looper::Looper(bool allowNonCallbacks): mAllowNonCallbacks(allowNonCallbacks), 
											mSendingMessage(false),
									        mResponseIndex(0), 
									        mNextMessageUptime(LLONG_MAX)
{
    int wakeFds[2];

	/* 创建匿名管道 */
    int result = pipe(wakeFds);	
    // LOG_ALWAYS_FATAL_IF(result != 0, "Could not create wake pipe.  errno=%d", errno);

	/* Looper对象保存管道两端的文件描述符 */
    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];

	/* 设置对管道的读为非阻塞方式 */
    result = fcntl(mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
    // LOG_ALWAYS_FATAL_IF(result != 0, "Could not make wake read pipe non-blocking.  errno=%d", errno);

	/* 设置对管道的写为非阻塞方式 */
    result = fcntl(mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
    // LOG_ALWAYS_FATAL_IF(result != 0, "Could not make wake write pipe non-blocking.  errno=%d", errno);

    mIdling = false;

    /* 创建epoll对象 */
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    // LOG_ALWAYS_FATAL_IF(mEpollFd < 0, "Could not create epoll instance.  errno=%d", errno);

    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(epoll_event)); 
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mWakeReadPipeFd;

	/* 将管道读端文件描述符加入到epoll对象中, 检测发往管道的数据 */
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, &eventItem);
    // LOG_ALWAYS_FATAL_IF(result != 0, "Could not add wake read pipe to epoll instance.  errno=%d", errno);
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
    int result = pthread_key_create(& gTLSKey, NULL);
    // LOG_ALWAYS_FATAL_IF(result != 0, "Could not allocate TLS key.");
}




int Looper::pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) 
{

    int result = 0;

    for (;;) {
		
        while (mResponseIndex < mResponses.size()) {	/* 缓存里面还有未被读取的数据 */
            const Response& response = mResponses.itemAt(mResponseIndex++);
            int ident = response.request.ident;
            if (ident >= 0) {
                int fd = response.request.fd;
                int events = response.events;
                void* data = response.request.data;
				
				#if DEBUG_POLL_AND_WAKE
                ALOGD("%p ~ pollOnce - returning signalled identifier %d: "
                        "fd=%d, events=0x%x, data=%p",
                        this, ident, fd, events, data);
				#endif

				if (outFd != NULL) 
					*outFd = fd;
                if (outEvents != NULL) 
					*outEvents = events;
                if (outData != NULL) 
					*outData = data;
				
                return ident;
            }
        }

        if (result != 0) {
			#if DEBUG_POLL_AND_WAKE
            ALOGD("%p ~ pollOnce - returning result %d", this, result);
			#endif
			
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
	#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
	#endif

	/* 1.计算出本次轮询等待的超时时间 */
    // Adjust the timeout based on when the next message is due.
    if (timeoutMillis != 0 && mNextMessageUptime != LLONG_MAX) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        int messageTimeoutMillis = toMillisecondTimeoutDelay(now, mNextMessageUptime);
		
        if (messageTimeoutMillis >= 0 && (timeoutMillis < 0 || messageTimeoutMillis < timeoutMillis)) {
            timeoutMillis = messageTimeoutMillis;
        }
		
		#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - next message in %lldns, adjusted timeout: timeoutMillis=%d",
                this, mNextMessageUptime - now, timeoutMillis);
		#endif
    }

    // Poll.
    int result = POLL_WAKE;
    mResponses.clear();
    mResponseIndex = 0;

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
        ALOGW("Poll failed with an unexpected error, errno=%d", errno);
        result = POLL_ERROR;
        goto Done;
    }

    /* eventCount为0表明等待超时 */
    if (eventCount == 0) {
		#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - timeout", this);
		#endif
		
        result = POLL_TIMEOUT;
        goto Done;
    }

	#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - handling events from %d fds", this, eventCount);
	#endif

	/* 处理读取到的事件 */
    for (int i = 0; i < eventCount; i++) {
        int fd = eventItems[i].data.fd;
        uint32_t epollEvents = eventItems[i].events;
		
        if (fd == mWakeReadPipeFd) {		/* 监控的文件描述符为管道读文件描述符 */
            if (epollEvents & EPOLLIN) {	/* 管道有数据可读 */		
                awoken();
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on wake read pipe.", epollEvents);
            }
        } else {	/* 监听的其它文件描述符有事件到来 */

            ssize_t requestIndex = mRequests.indexOfKey(fd);	/* 根据文件描述符找到对应的请求索引 */
            if (requestIndex >= 0) {
                int events = 0;
                if (epollEvents & EPOLLIN) 
					events |= EVENT_INPUT;
                if (epollEvents & EPOLLOUT) 
					events |= EVENT_OUTPUT;
                if (epollEvents & EPOLLERR) 
					events |= EVENT_ERROR;
                if (epollEvents & EPOLLHUP) 
					events |= EVENT_HANGUP;
                pushResponse(events, mRequests.valueAt(requestIndex));	/* 将读到的事件推入mResponses列表中 */
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on fd %d that is "
                        "no longer registered.", epollEvents, fd);
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
                sp<MessageHandler> handler = messageEnvelope.handler;
                Message message = messageEnvelope.message;
                mMessageEnvelopes.removeAt(0);
                mSendingMessage = true;
                mLock.unlock();

				#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                ALOGD("%p ~ pollOnce - sending message: handler=%p, what=%d",
                        this, handler.get(), message.what);
				#endif

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

    // Release lock.
    mLock.unlock();

    /* 处理处理接收到的事件: 存放在mResponses列表中 */
    for (size_t i = 0; i < mResponses.size(); i++) {
		
        Response& response = mResponses.editItemAt(i);
		
        if (response.request.ident == POLL_CALLBACK) {	/* 如果该注册的request需要回调处理 */
            int fd = response.request.fd;
            int events = response.events;
            void* data = response.request.data;
			
			#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
            ALOGD("%p ~ pollOnce - invoking fd event callback %p: fd=%d, events=0x%x, data=%p",
                    this, response.request.callback.get(), fd, events, data);
			#endif

			int callbackResult = response.request.callback->handleEvent(fd, events, data);
            if (callbackResult == 0) {
                removeFd(fd);
            }

			// Clear the callback reference in the response structure promptly because we
            // will not clear the response vector itself until the next poll.
            response.request.callback.clear();
            result = POLL_CALLBACK;
        }
    }
	
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
        nsecs_t endTime = systemTime(SYSTEM_TIME_MONOTONIC)
                + milliseconds_to_nanoseconds(timeoutMillis);

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
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ wake", this);
#endif

    ssize_t nWrite;
    do {
        nWrite = write(mWakeWritePipeFd, "W", 1);
    } while (nWrite == -1 && errno == EINTR);

    if (nWrite != 1) {
        if (errno != EAGAIN) {
            ALOGW("Could not write wake signal, errno=%d", errno);
        }
    }
}




void Looper::awoken() 
{

#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ awoken", this);
#endif

    char buffer[16];
    ssize_t nRead;
    do {
        nRead = read(mWakeReadPipeFd, buffer, sizeof(buffer));
    } while ((nRead == -1 && errno == EINTR) || nRead == sizeof(buffer));
}


/***************************************************************************************
** 函数名称: Looper::pushResponse
** 函数功能: 根据请求对象Request和获取到的事件来构造一个Reponse对象并加入mResponses列表
** 入口参数: events - 调用epoll_wait得到的事件
**			 request - 产生事件的Request
** 返 回 值: 无
**
**
*****************************************************************************************/
void Looper::pushResponse(int events, const Request& request) 
{
    Response response;
    response.events = events;
    response.request = request;
    mResponses.push(response);
}



/***************************************************************************************
** 函数名称: Looper::addFd
** 函数功能: 往Looper的监听队列中加入一个监听对象
** 入口参数: fd - 监听对象的文件描述符
**			 callback - 回调函数
**			 data - 附加数据
** 返 回 值: 
**
**
*****************************************************************************************/
int Looper::addFd(int fd, int ident, int events, Looper_callbackFunc callback, void* data) 
{
    return addFd(fd, ident, events, callback ? new SimpleLooperCallback(callback) : NULL, data);
}




/***************************************************************************************
** 函数名称: Looper::addFd
** 函数功能: 往Looper的监听队列中加入一个监听对象
** 入口参数: fd - 监听对象的文件描述符
**			 callback - LooperCallback强指针对象引用
**			 data - 附加数据
** 返 回 值: 
**
**
*****************************************************************************************/
int Looper::addFd(int fd, int ident, int events, const sp<LooperCallback>& callback, void* data) 
{
#if DEBUG_CALLBACKS
    ALOGD("%p ~ addFd - fd=%d, ident=%d, events=0x%x, callback=%p, data=%p", this, fd, ident,
            events, callback.get(), data);
#endif

    if (!callback.get()) {		/* LooperCallback对象为NULL */
        if (! mAllowNonCallbacks) {
            ALOGE("Invalid attempt to set NULL callback but not allowed for this looper.");
            return -1;
        }

        if (ident < 0) {
            ALOGE("Invalid attempt to set NULL callback with ident < 0.");
            return -1;
        }
    } else {	/* LooperCallback对象存在 */
        ident = POLL_CALLBACK;
    }

    int epollEvents = 0;
    if (events & EVENT_INPUT) epollEvents |= EPOLLIN;
    if (events & EVENT_OUTPUT) epollEvents |= EPOLLOUT;

    { 
        AutoMutex _l(mLock);

		/* 构建一个request */
        Request request;
        request.fd = fd;
        request.ident = ident;
        request.callback = callback;
        request.data = data;

        struct epoll_event eventItem;
        memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
        eventItem.events = epollEvents;
        eventItem.data.fd = fd;

        ssize_t requestIndex = mRequests.indexOfKey(fd);	/* 根据fd在mRequests列表中查找对应的Request是否存在 */
        if (requestIndex < 0) {		/* 如果不存在,将该fd加入epoll监听列表中 */
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem);
            if (epollResult < 0) {
                ALOGE("Error adding epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
            mRequests.add(fd, request);	/* 以<fd, Request>的形式加入mRequests列表中 */
        } else {	/* 如果该fd已经存在epoll监听列表中,修改该fd对应的eventItem */
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &eventItem);
            if (epollResult < 0) {
                ALOGE("Error modifying epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
            mRequests.replaceValueAt(requestIndex, request);	/* 同时也修改mRequests列表中对应的项 */
        }
    } 
	
    return 1;
}



/***************************************************************************************
** 函数名称: Looper::removeFd
** 函数功能: 移除指定文件描述符的监听对象
** 入口参数: fd - 监听对象的文件描述符
** 返 回 值: 
**
**
*****************************************************************************************/
int Looper::removeFd(int fd)
{

    { // acquire lock
        AutoMutex _l(mLock);
        ssize_t requestIndex = mRequests.indexOfKey(fd);
        if (requestIndex < 0) {
            return 0;
        }

        int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL);
        if (epollResult < 0) {
            ALOGE("Error removing epoll events for fd %d, errno=%d", fd, errno);
            return -1;
        }

        mRequests.removeItemsAt(requestIndex);	/* 在mRequests列表中移除对应的<fd, Request> */
    } // release lock
    return 1;
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
void Looper::sendMessageAtTime(nsecs_t uptime, const sp<MessageHandler>& handler, const Message& message) 
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