#ifndef OS_MUTEX_H
#define OS_MUTEX_H

#include <mutex>

class OS_Mutex {
private:
    std::mutex mtx;
public:
    OS_Mutex() {}
    ~OS_Mutex() {}
    
    void lock() {
        mtx.lock();
    }
    
    void unlock() {
        mtx.unlock();
    }
};

class OS_LockGuard {
private:
    OS_Mutex& mtx;
public:
    OS_LockGuard(OS_Mutex& m) : mtx(m) {
        mtx.lock();
    }
    
    ~OS_LockGuard() {
        mtx.unlock();
    }
};

#endif
