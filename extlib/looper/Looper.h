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

    int pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData);

    inline int pollOnce(int timeoutMillis) {
        return pollOnce(timeoutMillis, NULL, NULL, NULL);
    }

    int pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData);
	
    inline int pollAll(int timeoutMillis) {
        return pollAll(timeoutMillis, NULL, NULL, NULL);
    }

    void wake();

    void sendMessage(const std::shared_ptr<MessageHandler>& handler, const Message& message);
    void sendMessageDelayed(nsecs_t uptimeDelay, const std::shared_ptr<MessageHandler>& handler, const Message& message);

    void sendMessageAtTime(nsecs_t uptime, const std::shared_ptr<MessageHandler>& handler, const Message& message);

    void removeMessages(const std::shared_ptr<MessageHandler>& handler);
    void removeMessages(const std::shared_ptr<MessageHandler>& handler, int what);

    bool isIdling() const;

    static std::shared_ptr<Looper> prepare(int opts);

    static void setForThread(const std::shared_ptr<Looper>& looper);

    static std::shared_ptr<Looper> getForThread();

private:
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


    int                             mWakeReadPipeFd;  		/* 保存管道读端文件描述符 */
    int                             mWakeWritePipeFd; 		/* 保存管道写端文件描述符 */
    Mutex                           mLock;


    std::vector<MessageEnvelope>    mMessageEnvelopes; 		/* Looper的消息容器,如消息队列 */
    
    size_t                          mResponseIndex;			/* 当前正在处理的Response在容器中的索引 */
    nsecs_t                         mNextMessageUptime; 	/* 下一个消息的到期时刻,如果没有设置为LLONG_MAX */ 
    bool                            mSendingMessage; 		/* 是否正在发送消息,需要mLock保护 */
    volatile bool                   mIdling;                /* 标记Looper是否处空闲状态 */
    int                             mEpollFd;               /* epoll对象文件描述符: 调用epoll_create时返回 */

    int pollInner(int timeoutMillis);
    void awoken();

    static void initTLSKey();
};

#endif /* _LOOPER_H_ */