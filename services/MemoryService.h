#ifndef MEMORY_SERVICE_H
#define MEMORY_SERVICE_H

#include <map>
#include "../ipc/IPC.h"
#include "Service.h"

class MemoryService : public Service {
private:
    std::vector<bool> physicalMemory; // false = free, true = used
    std::map<int, std::vector<int>> processMemoryMap;
    int pageSize;
    int totalPages;
    int usedPages;

public:
    MemoryService();

    void handleMessage(Message msg) override;
    void freeAll(int pid);   // ✅ ADD THIS
};

#endif
