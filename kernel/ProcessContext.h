#ifndef PROCESS_CONTEXT_H
#define PROCESS_CONTEXT_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <iostream>
#include "Globals.h"

struct ProcessContext {
    int pid;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> canRun{false};
    std::atomic<bool> terminated{false};

    // ===== Simulated CPU Registers =====
    int programCounter = 0;   // PC register
    int accumulator = 0;      // ACC register

    // ===== Simulated Isolated Address Space =====
    // Each process gets its own PRIVATE memory (like a real OS)
    std::vector<int> addressSpace;

    // Total work units (mapped from burstTime)
    int totalWork;

    std::thread workerThread;

    ProcessContext(int id, int work) : pid(id), totalWork(work) {
        // Each process gets 16 cells of private address space
        addressSpace.assign(16, 0);
    }

    ~ProcessContext() {
        terminated = true;
        canRun = true;
        cv.notify_all();
        if (workerThread.joinable()) workerThread.join();
    }
};

#endif
