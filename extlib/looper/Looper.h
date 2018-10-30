#ifndef _LOOPER_H_
#define _LOOPER_H_

#include <iostream>
#include <memory>
#include <sys/epoll.h>

typedef int (*Looper_callbackFunc)(int fd, int events, void* data);

typedef long long nsecs_t;

struct Message {

    Message() : what(0), data(NULL) {}
    Message(int what) : what(what), data(NULL) { }
    
    int     what;
    void*   data;
};

class MessageHandler {
protected:
    virtual ~MessageHandler() { }

public:
    virtual void handleMessage(const Message& message) = 0;
};


class Looper {

protected:
    virtual ~Looper();

public:
    
    Looper(bool allowNonCallbacks);

    bool getAllowNonCallbacks() const;

    int pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData);

    inline int pollOnce(int timeoutMillis) {
        return pollOnce(timeoutMillis, NULL, NULL, NULL);
    }

    int pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData);
	
    inline int pollAll(int timeoutMillis) {
        return pollAll(timeoutMillis, NULL, NULL, NULL);
    }

    void wake();

    int removeFd(int fd);

    void sendMessage(const std::shared_ptr<MessageHandler>& handler, const Message& message);

    void sendMessageDelayed(nsecs_t uptimeDelay, const std::shared_ptr<MessageHandler>& handler,
            const Message& message);

    void sendMessageAtTime(nsecs_t uptime, const std::shared_ptr<MessageHandler>& handler,
            const Message& message);

    void removeMessages(const std::shared_ptr<MessageHandler>& handler);

    void removeMessages(const std::shared_ptr<MessageHandler>& handler, int what);

    bool isIdling() const;

    static std::shared_ptr<Looper> prepare(int opts);

    static void setForThread(const std::shared_ptr<Looper>& looper);

    static std::shared_ptr<Looper> getForThread();

private:
    struct Request {
        int fd;
        int ident;
        std::shared_ptr<LooperCallback> callback;
        void* data;
    };

    struct Response {
        int events;
        Request request;
    };

    struct MessageEnvelope {
        MessageEnvelope() : uptime(0) { }

        MessageEnvelope(nsecs_t uptime, const std::shared_ptr<MessageHandler> handler,const Message& message) : 
																						uptime(uptime), 
																						handler(handler),
																						message(message) 
		{
        }

        nsecs_t uptime;
        std::shared_ptr<MessageHandler> handler;
        Message message;
    };

    const bool mAllowNonCallbacks;					/* 是否允许没有Callback */

    int mWakeReadPipeFd;  							/* 保存管道读端文件描述符 */
    int mWakeWritePipeFd; 							/* 保存管道写端文件描述符 */
    Mutex mLock;

    Vector<MessageEnvelope> mMessageEnvelopes; 		/* Looper的消息容器,如消息队列 */
    bool mSendingMessage; 							/* 是否正在发送消息,需要mLock保护 */

    volatile bool mIdling;							/* 标记Looper是否处空闲状态 */

    int mEpollFd; 									/* epoll对象文件描述符: 调用epoll_create时返回 */

    KeyedVector<int, Request> mRequests;  			/* 保存所有需要epoll监听的文件描述符及对应的Request,需要mLock保护 */

    Vector<Response> mResponses;					/* 保存pollOnce得到的事件, Response容器 */
    size_t mResponseIndex;							/* 当前正在处理的Response在容器中的索引 */
    nsecs_t mNextMessageUptime; 					/* 下一个消息的到期时刻,如果没有设置为LLONG_MAX */ 

    int pollInner(int timeoutMillis);
    void awoken();
    void pushResponse(int events, const Request& request);

    static void initTLSKey();
    static void threadDestructor(void *st);
     

};

#endif /* _LOOPER_H_ */