/*
 * Looper线程 - 通过该类可快速实现一个Looper线程
 */
#ifndef _LOOPER_THREAD_H_
#define _LOOPER_THREAD_H_

#include <memory>

class LooperThread {

public:
    LooperThread();
    
    /*
     * 启动Looper线程
     */
    void start();

private:

};

#endif /* _LOOPER_THREAD_H_ */