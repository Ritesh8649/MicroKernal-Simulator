#ifndef SCHEDULER_SERVICE_H
#define SCHEDULER_SERVICE_H

#include "Service.h"
#include "ProcessServer.h"
#include "../kernel/ProcessContext.h"
#include <queue>
#include <map>
#include <memory>

class SchedulerService : public Service {
private:
    std::queue<PCB> readyQueue;
    int timeQuantum;
    ProcessServer* processServer;

    // Real thread management
    std::map<int, std::unique_ptr<ProcessContext>> contexts;
    int currentRunningPID = -1;

    void suspendProcess(int pid);
    void resumeProcess(int pid);

public:
    SchedulerService(ProcessServer* ps);
    void handleMessage(Message msg) override;
    void addProcess(PCB p);
    void terminateAll();
    ~SchedulerService();
};

#endif
