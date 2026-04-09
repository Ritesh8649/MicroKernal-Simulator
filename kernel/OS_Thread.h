#ifndef OS_THREAD_H
#define OS_THREAD_H

#include <thread>
#include <memory>

class OS_Thread {
private:
    std::unique_ptr<std::thread> t;

public:
    OS_Thread() {}
    
    ~OS_Thread() {
        join(); // Ensure thread cleanup on destruction
    }

    void start(void* (*func)(void*), void* param) {
        if (!t) {
            t = std::make_unique<std::thread>(func, param);
        }
    }

    void join() {
        if (t && t->joinable()) {
            t->join();
            t.reset();
        }
    }

    bool joinable() {
        return t && t->joinable();
    }
};

#endif
