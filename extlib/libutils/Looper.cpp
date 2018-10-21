//
// Copyright 2010 The Android Open Source Project
//
// A looper implementation based on epoll().
//
#define LOG_TAG "Looper"

//#define LOG_NDEBUG 0

// Debugs poll and wake interactions.
#define DEBUG_POLL_AND_WAKE 0

// Debugs callback registration and invocation.
#define DEBUG_CALLBACKS 0

#include <cutils/log.h>
#include <utils/Looper.h>
#include <utils/Timers.h>

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>


namespace android {

// --- WeakMessageHandler ---

WeakMessageHandler::WeakMessageHandler(const wp<MessageHandler>& handler) :mHandler(handler) 
{
}

WeakMessageHandler::~WeakMessageHandler() 
{
}

void WeakMessageHandler::handleMessage(const Message& message) 
{
    sp<MessageHandler> handler = mHandler.promote();
    if (handler != NULL) {
        handler->handleMessage(message);
    }
}


// --- SimpleLooperCallback ---

SimpleLooperCallback::SimpleLooperCallback(Looper_callbackFunc callback) :mCallback(callback) 
{
}

SimpleLooperCallback::~SimpleLooperCallback() 
{
}

int SimpleLooperCallback::handleEvent(int fd, int events, void* data) 
{
    return mCallback(fd, events, data);
}


// --- Looper ---

// Hint for number of file descriptors to be associated with the epoll instance.
static const int EPOLL_SIZE_HINT = 8;

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;

static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSKey = 0;



Looper::Looper(bool allowNonCallbacks): mAllowNonCallbacks(allowNonCallbacks), 
											mSendingMessage(false),
									        mResponseIndex(0), 
									        mNextMessageUptime(LLONG_MAX)
{
    int wakeFds[2];

	/* ���������ܵ� */
    int result = pipe(wakeFds);	
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not create wake pipe.  errno=%d", errno);

	/* Looper���󱣴�ܵ����˵��ļ������� */
    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];

	/* ���öԹܵ��Ķ�Ϊ��������ʽ */
    result = fcntl(mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not make wake read pipe non-blocking.  errno=%d", errno);

	/* ���öԹܵ���дΪ��������ʽ */
    result = fcntl(mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not make wake write pipe non-blocking.  errno=%d", errno);

    mIdling = false;

    /* ����epoll���� */
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    LOG_ALWAYS_FATAL_IF(mEpollFd < 0, "Could not create epoll instance.  errno=%d", errno);

    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(epoll_event)); 
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mWakeReadPipeFd;

	/* ���ܵ������ļ����������뵽epoll������, ��ⷢ���ܵ������� */
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, &eventItem);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not add wake read pipe to epoll instance.  errno=%d", errno);
}



/***************************************************************************************
** ��������: Looper::~Looper
** ��������: Looper������������
** ��ڲ���: ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
Looper::~Looper() 
{
	/* �رչܵ����˵��ļ������� */
    close(mWakeReadPipeFd);
    close(mWakeWritePipeFd);
	
	/* �ر�epoll�ļ������� */
    close(mEpollFd);
}

void Looper::initTLSKey()
{
    int result = pthread_key_create(& gTLSKey, threadDestructor);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not allocate TLS key.");
}


void Looper::threadDestructor(void *st) 
{
    Looper* const self = static_cast<Looper*>(st);
    if (self != NULL) {
        self->decStrong((void*)threadDestructor);
    }
}



void Looper::setForThread(const sp<Looper>& looper) 
{
    sp<Looper> old = getForThread(); // also has side-effect of initializing TLS

    if (looper != NULL) {
        looper->incStrong((void*)threadDestructor);
    }

    pthread_setspecific(gTLSKey, looper.get());

    if (old != NULL) {
        old->decStrong((void*)threadDestructor);
    }
}




/***************************************************************************************
** ��������: Looper::getForThread
** ��������: ��ȡ�������̶߳�Ӧ��Looper����ָ��
** ��ڲ���: ��
** �� �� ֵ: �̵߳�Looper����ָ��sp<Looper>
**
**
*****************************************************************************************/
sp<Looper> Looper::getForThread() 
{
    int result = pthread_once(&gTLSOnce, initTLSKey);
    LOG_ALWAYS_FATAL_IF(result != 0, "pthread_once failed");

    return (Looper*)pthread_getspecific(gTLSKey);
}



/***************************************************************************************
** ��������: Looper::prepare
** ��������: ��ȡ�̵߳�Looper����ָ��,����̵߳�Looper���󲻴����򴴽�
** ��ڲ���: opts - �Ƿ�����ص���־
** �� �� ֵ: �̵߳�Looper����ָ��sp<Looper>
**
**
*****************************************************************************************/
sp<Looper> Looper::prepare(int opts) 
{
    bool allowNonCallbacks = opts & PREPARE_ALLOW_NON_CALLBACKS;
    sp<Looper> looper = Looper::getForThread();
    if (looper == NULL) {
        looper = new Looper(allowNonCallbacks);
        Looper::setForThread(looper);
    }
	
    if (looper->getAllowNonCallbacks() != allowNonCallbacks) {
        ALOGW("Looper already prepared for this thread with a different value for the "
                "LOOPER_PREPARE_ALLOW_NON_CALLBACKS option.");
    }
	
    return looper;
}



/***************************************************************************************
** ��������: Looper::getAllowNonCallbacks
** ��������: �����Ƿ�����ص���־
** ��ڲ���: ��
** �� �� ֵ: �Ƿ�ص��־
**
**
*****************************************************************************************/
bool Looper::getAllowNonCallbacks() const 
{
    return mAllowNonCallbacks;
}



int Looper::pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) 
{

    int result = 0;

    for (;;) {
		
        while (mResponseIndex < mResponses.size()) {	/* �������滹��δ����ȡ������ */
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

        result = pollInner(timeoutMillis);	/* ����pllInner�ȴ����� */
    }
}



int Looper::pollInner(int timeoutMillis)
{
	#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
	#endif

	/* 1.�����������ѯ�ȴ��ĳ�ʱʱ�� */
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

    mIdling = true;		/* ����״̬ΪIdle״̬ */

	/* 3.����epoll_wait�ȴ��¼��ĵ������ߵȴ���ʱ */
    struct epoll_event eventItems[EPOLL_MAX_EVENTS];
    int eventCount = epoll_wait(mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);

    mIdling = false;	/* �ȴ�����,���ٴ���Idle״̬ */

    // Acquire lock.
    mLock.lock();

    if (eventCount < 0) {	/* ����epoll_wait���� */
        if (errno == EINTR) {	/* ���ź��ж� */
            goto Done;
        }
        ALOGW("Poll failed with an unexpected error, errno=%d", errno);
        result = POLL_ERROR;
        goto Done;
    }

    /* eventCountΪ0�����ȴ���ʱ */
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

	/* �����ȡ�����¼� */
    for (int i = 0; i < eventCount; i++) {
        int fd = eventItems[i].data.fd;
        uint32_t epollEvents = eventItems[i].events;
		
        if (fd == mWakeReadPipeFd) {		/* ��ص��ļ�������Ϊ�ܵ����ļ������� */
            if (epollEvents & EPOLLIN) {	/* �ܵ������ݿɶ� */		
                awoken();
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on wake read pipe.", epollEvents);
            }
        } else {	/* �����������ļ����������¼����� */

            ssize_t requestIndex = mRequests.indexOfKey(fd);	/* �����ļ��������ҵ���Ӧ���������� */
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
                pushResponse(events, mRequests.valueAt(requestIndex));	/* ���������¼�����mResponses�б��� */
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on fd %d that is "
                        "no longer registered.", epollEvents, fd);
            }
        }
    }
	
Done: ;

    /* ���Looper����Ϣ�����Ƿ��д��������Ϣ
     * �������Ϣ,��������Ϣ��callbacks��������Ϣ
     */
    mNextMessageUptime = LLONG_MAX;
    while (mMessageEnvelopes.size() != 0) {		/* ��Ϣ�������Ƿ�����Ϣ������ */
		
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);	/* ��ȡ��ǰ��ϵͳʱ�� */

        const MessageEnvelope& messageEnvelope = mMessageEnvelopes.itemAt(0);
        if (messageEnvelope.uptime <= now) {	/* ��Ϣ�Ľ�ֹʱ���ѹ�,���������� */
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

				handler->handleMessage(message);	/* �������Ϣ */
            } // release handler

            mLock.lock();
            mSendingMessage = false;
            result = POLL_CALLBACK;
        } else {	/* �����Ϣ�����еĵ�һ����Ϣ�Ľ�ֹʱ��δ��,mNextMessageUptime��¼��ʱ�� */
            mNextMessageUptime = messageEnvelope.uptime;
            break;
        }
    }

    // Release lock.
    mLock.unlock();

    /* ��������յ����¼�: �����mResponses�б��� */
    for (size_t i = 0; i < mResponses.size(); i++) {
		
        Response& response = mResponses.editItemAt(i);
		
        if (response.request.ident == POLL_CALLBACK) {	/* �����ע���request��Ҫ�ص����� */
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
** ��������: Looper::wake
** ��������: ��Looper�Ĺܵ���д��д������,�Ա��������ѵ���Looper.pollOnce���߳�
** ��ڲ���: ��
** �� �� ֵ: ��
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
** ��������: Looper::pushResponse
** ��������: �����������Request�ͻ�ȡ�����¼�������һ��Reponse���󲢼���mResponses�б�
** ��ڲ���: events - ����epoll_wait�õ����¼�
**			 request - �����¼���Request
** �� �� ֵ: ��
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
** ��������: Looper::addFd
** ��������: ��Looper�ļ��������м���һ����������
** ��ڲ���: fd - ����������ļ�������
**			 callback - �ص�����
**			 data - ��������
** �� �� ֵ: 
**
**
*****************************************************************************************/
int Looper::addFd(int fd, int ident, int events, Looper_callbackFunc callback, void* data) 
{
    return addFd(fd, ident, events, callback ? new SimpleLooperCallback(callback) : NULL, data);
}



/***************************************************************************************
** ��������: Looper::addFd
** ��������: ��Looper�ļ��������м���һ����������
** ��ڲ���: fd - ����������ļ�������
**			 callback - LooperCallbackǿָ���������
**			 data - ��������
** �� �� ֵ: 
**
**
*****************************************************************************************/
int Looper::addFd(int fd, int ident, int events, const sp<LooperCallback>& callback, void* data) 
{
#if DEBUG_CALLBACKS
    ALOGD("%p ~ addFd - fd=%d, ident=%d, events=0x%x, callback=%p, data=%p", this, fd, ident,
            events, callback.get(), data);
#endif

    if (!callback.get()) {		/* LooperCallback����ΪNULL */
        if (! mAllowNonCallbacks) {
            ALOGE("Invalid attempt to set NULL callback but not allowed for this looper.");
            return -1;
        }

        if (ident < 0) {
            ALOGE("Invalid attempt to set NULL callback with ident < 0.");
            return -1;
        }
    } else {	/* LooperCallback������� */
        ident = POLL_CALLBACK;
    }

    int epollEvents = 0;
    if (events & EVENT_INPUT) epollEvents |= EPOLLIN;
    if (events & EVENT_OUTPUT) epollEvents |= EPOLLOUT;

    { 
        AutoMutex _l(mLock);

		/* ����һ��request */
        Request request;
        request.fd = fd;
        request.ident = ident;
        request.callback = callback;
        request.data = data;

        struct epoll_event eventItem;
        memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
        eventItem.events = epollEvents;
        eventItem.data.fd = fd;

        ssize_t requestIndex = mRequests.indexOfKey(fd);	/* ����fd��mRequests�б��в��Ҷ�Ӧ��Request�Ƿ���� */
        if (requestIndex < 0) {		/* ���������,����fd����epoll�����б��� */
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem);
            if (epollResult < 0) {
                ALOGE("Error adding epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
            mRequests.add(fd, request);	/* ��<fd, Request>����ʽ����mRequests�б��� */
        } else {	/* �����fd�Ѿ�����epoll�����б���,�޸ĸ�fd��Ӧ��eventItem */
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &eventItem);
            if (epollResult < 0) {
                ALOGE("Error modifying epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
            mRequests.replaceValueAt(requestIndex, request);	/* ͬʱҲ�޸�mRequests�б��ж�Ӧ���� */
        }
    } 
	
    return 1;
}



/***************************************************************************************
** ��������: Looper::removeFd
** ��������: �Ƴ�ָ���ļ��������ļ�������
** ��ڲ���: fd - ����������ļ�������
** �� �� ֵ: 
**
**
*****************************************************************************************/
int Looper::removeFd(int fd)
{
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeFd - fd=%d", this, fd);
#endif

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

        mRequests.removeItemsAt(requestIndex);	/* ��mRequests�б����Ƴ���Ӧ��<fd, Request> */
    } // release lock
    return 1;
}



/***************************************************************************************
** ��������: Looper::sendMessage
** ��������: ������Ϣ��Looper����Ϣ������
** ��ڲ���: handler - ��Ϣ�Ĵ������ָ��
**			 message - ��Ϣ����
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void Looper::sendMessage(const sp<MessageHandler>& handler, const Message& message) 
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now, handler, message);
}



/***************************************************************************************
** ��������: Looper::sendMessageDelayed
** ��������: ������ʱ��Ϣ��Looper����Ϣ������
** ��ڲ���: uptimeDelay - ��Ϣ����ʱֵ
**			 handler - ��Ϣ�Ĵ������ָ��
**			 message - ��Ϣ����
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void Looper::sendMessageDelayed(nsecs_t uptimeDelay, const sp<MessageHandler>& handler, const Message& message) 
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now + uptimeDelay, handler, message);
}



/***************************************************************************************
** ��������: Looper::sendMessageAtTime
** ��������: ������Ϣ��Looper����Ϣ������
** ��ڲ���: uptime - ��Ϣ�Ľ���ʱ��
**			 handler - ��Ϣ�Ĵ������ָ��
**			 message - ��Ϣ����
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void Looper::sendMessageAtTime(nsecs_t uptime, const sp<MessageHandler>& handler, const Message& message) 
{

#if DEBUG_CALLBACKS
    ALOGD("%p ~ sendMessageAtTime - uptime=%lld, handler=%p, what=%d",
            this, uptime, handler.get(), message.what);
#endif

    size_t i = 0;
    {
        AutoMutex _l(mLock);

        size_t messageCount = mMessageEnvelopes.size();		/* ��ȡLooper��Ϣ���е�ǰ��Ϣ�ĸ��� */

		/* �жϴ����͵���Ϣ����Ϣ�����е��ŷ�λ��,����Ϣ�ĵ���ʱ��Ϊ��ֵ */
        while (i < messageCount && uptime >= mMessageEnvelopes.itemAt(i).uptime) {
            i += 1;
        }

		/* ����һ����Ϣ�ŷ���� */
        MessageEnvelope messageEnvelope(uptime, handler, message);

		/* ����Ϣ�ŷ������������� */
        mMessageEnvelopes.insertAt(messageEnvelope, i, 1);

        /* ���mSendingMessageΪtrue,�����߳�����������Ϣ���ڻ״̬ */
        if (mSendingMessage) {
            return;
        }
    } 

    /* �����Ϣ����ԭ��Ϊ��,���Ѷ���Ϣ���߳� */
    if (i == 0) {
        wake();		/* ���ѵ���Looper.pollOnce���߳� */
    }
}



/***************************************************************************************
** ��������: Looper::removeMessages
** ��������: ��Looper����Ϣ�������Ƴ�ָ��(ָ������Handler)����Ϣ
** ��ڲ���: handler - ��Ϣ��Ӧ��Handler����ָ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void Looper::removeMessages(const sp<MessageHandler>& handler)
{
	#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p", this, handler.get());
	#endif

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
** ��������: Looper::removeMessages
** ��������: ��Looper����Ϣ�������Ƴ�ָ��(ָ������Handler������what)����Ϣ
** ��ڲ���: handler - ��Ϣ��Ӧ��Handler����ָ��
**			 what - ��Ϣ������ֵ
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void Looper::removeMessages(const sp<MessageHandler>& handler, int what) 
{

	#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p, what=%d", this, handler.get(), what);
	#endif

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
** ��������: Looper::isIdling
** ��������: �ж�Looper�߳��Ƿ�Ϊ����״̬
** ��ڲ���: ��
** �� �� ֵ: ����״̬����true;���򷵻�false
**
**
*****************************************************************************************/
bool Looper::isIdling() const 
{
    return mIdling;
}

} // namespace android
