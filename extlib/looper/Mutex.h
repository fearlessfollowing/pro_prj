#ifndef _UTILS_MUTEX_H
#define _UTILS_MUTEX_H

# include <pthread.h>

class Condition;

class Mutex {

public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();
    Mutex(const char* name);
    Mutex(int type, const char* name = NULL);
    ~Mutex();    

    int         lock();
    void        unlock();
    int         tryLock();

    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };


private:
    friend class Condition;
                
                Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);
    
    pthread_mutex_t mMutex;

};


inline Mutex::Mutex() 
{
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::Mutex(__attribute__((unused)) const char* name) 
{
    pthread_mutex_init(&mMutex, NULL);
}


inline Mutex::Mutex(int type, __attribute__((unused)) const char* name) 
{
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}


inline Mutex::~Mutex() 
{
    pthread_mutex_destroy(&mMutex);
}

inline int Mutex::lock() 
{
    return -pthread_mutex_lock(&mMutex);
}

inline void Mutex::unlock() 
{
    pthread_mutex_unlock(&mMutex);
}

inline int Mutex::tryLock() 
{
    return -pthread_mutex_trylock(&mMutex);
}


typedef Mutex::Autolock AutoMutex;


#endif  /* _UTILS_MUTEX_H */