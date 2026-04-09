#include "SchedulerService.h"
#include "../kernel/Globals.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

// ========================================================
// PROCESS WORKLOAD — Each process runs THIS in its own thread
// It simulates a real process doing CPU work in its own
// isolated address space.
// ========================================================
void processWorkload(ProcessContext* ctx) {
    while (!ctx->terminated && ctx->programCounter < ctx->totalWork) {
        // ---- WAIT for scheduler to give us the CPU ----
        {
            std::unique_lock<std::mutex> lock(ctx->mtx);
            ctx->cv.wait(lock, [ctx] {
                return ctx->canRun.load() || ctx->terminated.load();
            });
        }

        if (ctx->terminated) break;

        // ---- EXECUTE: Simulate one time quantum of CPU work ----
        int workDone = 0;
        while (workDone < 2 && ctx->programCounter < ctx->totalWork) {
            // Write to our PRIVATE address space (isolated memory)
            int idx = ctx->programCounter % (int)ctx->addressSpace.size();
            ctx->accumulator += (ctx->pid * 10 + ctx->programCounter);
            ctx->addressSpace[idx] = ctx->accumulator;
            ctx->programCounter++;
            workDone++;

            // Small delay to simulate actual CPU computation
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // ---- YIELD: Give CPU back to the scheduler ----
        ctx->canRun = false;
    }

    ctx->terminated = true;
}

SchedulerService::SchedulerService(ProcessServer* ps) : processServer(ps) {
    timeQuantum = 2; // fixed time slice
}

SchedulerService::~SchedulerService() {
    terminateAll();
}

void SchedulerService::terminateAll() {
    for (auto& pair : contexts) {
        pair.second->terminated = true;
        pair.second->canRun = true;
        pair.second->cv.notify_all();
    }
    contexts.clear();
}

void SchedulerService::addProcess(PCB p) {
    readyQueue.push(p);

    // Create a real thread for this process with its own context
    auto ctx = std::make_unique<ProcessContext>(p.pid, p.burstTime * 2);
    ctx->workerThread = std::thread(processWorkload, ctx.get());

    {
        OS_LockGuard lock(printMutex);
        cout << "[SchedulerService] Spawned thread for PID: " << p.pid
             << " | Address Space: " << ctx->addressSpace.size()
             << " cells | Work Units: " << ctx->totalWork << "\n";
    }

    contexts[p.pid] = std::move(ctx);
}

void SchedulerService::suspendProcess(int pid) {
    if (contexts.find(pid) != contexts.end()) {
        auto& ctx = contexts[pid];
        ctx->canRun = false;

        OS_LockGuard lock(printMutex);
        cout << "[Context Switch] SAVING PID " << pid
             << " | PC=" << ctx->programCounter
             << " ACC=" << ctx->accumulator << "\n";
    }
}

void SchedulerService::resumeProcess(int pid) {
    if (contexts.find(pid) != contexts.end()) {
        auto& ctx = contexts[pid];

        {
            OS_LockGuard lock(printMutex);
            cout << "[Context Switch] RESTORING PID " << pid
                 << " | PC=" << ctx->programCounter
                 << " ACC=" << ctx->accumulator << "\n";
        }

        // Signal the process thread to run
        {
            std::lock_guard<std::mutex> lk(ctx->mtx);
            ctx->canRun = true;
        }
        ctx->cv.notify_one();
    }
}

void SchedulerService::handleMessage(Message msg) {
    if (msg.type == "interrupt" && msg.data == "timer") {
        if (readyQueue.empty()) return;

        // ---- SUSPEND current running process ----
        if (currentRunningPID != -1) {
            suspendProcess(currentRunningPID);
        }

        // ---- Pick next process (Round-Robin) ----
        PCB current = readyQueue.front();
        readyQueue.pop();

        // Check if the process thread is still alive
        if (contexts.find(current.pid) != contexts.end()) {
            auto& ctx = contexts[current.pid];

            if (ctx->terminated) {
                // Process has finished all its work
                {
                    OS_LockGuard lock(printMutex);
                    cout << "[SchedulerService] Process " << current.pid
                         << " FINISHED | Final PC=" << ctx->programCounter
                         << " ACC=" << ctx->accumulator << "\n";
                    cout << "[SchedulerService] Address Space Dump for PID " << current.pid << ": [";
                    for (int i = 0; i < (int)ctx->addressSpace.size(); i++) {
                        if (i > 0) cout << ", ";
                        cout << ctx->addressSpace[i];
                    }
                    cout << "]\n";
                }

                // Cleanup
                currentRunningPID = -1;
                processServer->removeProcess(current.pid);
                contexts.erase(current.pid);
                return;
            }

            // ---- RESUME this process (context switch) ----
            currentRunningPID = current.pid;
            current.state = "RUNNING";

            {
                OS_LockGuard lock(printMutex);
                cout << "[SchedulerService] ▶ Running PID: " << current.pid << "\n";
            }

            resumeProcess(current.pid);

            // Wait for the process to finish its quantum
            // (it will set canRun=false when done)
            auto& ctxRef = contexts[current.pid];
            int waitCount = 0;
            while (ctxRef->canRun && !ctxRef->terminated && waitCount < 20) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                waitCount++;
            }

            // Re-add to ready queue if not finished
            if (!ctxRef->terminated) {
                current.state = "READY";
                readyQueue.push(current);
            } else {
                OS_LockGuard lock(printMutex);
                cout << "[SchedulerService] Process " << current.pid
                     << " FINISHED | Final PC=" << ctxRef->programCounter
                     << " ACC=" << ctxRef->accumulator << "\n";
                cout << "[SchedulerService] Address Space Dump for PID " << current.pid << ": [";
                for (int i = 0; i < (int)ctxRef->addressSpace.size(); i++) {
                    if (i > 0) cout << ", ";
                    cout << ctxRef->addressSpace[i];
                }
                cout << "]\n";

                currentRunningPID = -1;
                processServer->removeProcess(current.pid);
                contexts.erase(current.pid);
            }
        }
    }
}
