#include <iostream>
#include <string>
#include <vector>
#include "../kernel/OS_Mutex.h"
#include "MemoryService.h"
#include "../kernel/Globals.h"

using namespace std;

MemoryService::MemoryService() {
    pageSize = 4096;      // 4KB page size
    totalPages = 256;     // 1MB total RAM simulation
    usedPages = 0;
    physicalMemory.assign(totalPages, false); 
}

void MemoryService::handleMessage(Message msg) {
    if (msg.type == "memory") {
        int amount = 0;
        try {
            amount = stoi(msg.data);
        } catch(...) { return; }

        int pagesNeeded = (amount + pageSize - 1) / pageSize; 
        
        if (usedPages + pagesNeeded > totalPages) {
            OS_LockGuard lock(printMutex);
            cout << "[MemoryService] ERROR: OOM for PID: " << msg.sender << "\n";
            return;
        }

        vector<int> allocatedFrames;
        for (int i = 0; i < totalPages; i++) {
            if (!physicalMemory[i]) {
                allocatedFrames.push_back(i);
                physicalMemory[i] = true;
                usedPages++;
                processMemoryMap[msg.sender].push_back(i);
                if (allocatedFrames.size() == pagesNeeded) break;
            }
        }
        
        {
            OS_LockGuard lock(printMutex);
            cout << "[MemoryService] Allocated " << allocatedFrames.size() 
                 << " pages (Frames";
            for(int f : allocatedFrames) cout << " " << f;
            cout << ") for PID: " << msg.sender << "\n";
            cout << "[MemoryService] Memory Status: " << usedPages << "/" << totalPages << " pages used.\n";
        }

        // We could broadcast success here, but simulation works without the Process explicitly tracking the exact Frame #s
    } else if (msg.type == "free") {
        int amount = 0;
        try { amount = stoi(msg.data); } catch(...) { return; }
        
        int pagesToFree = (amount + pageSize - 1) / pageSize;
        
        if (processMemoryMap.find(msg.sender) != processMemoryMap.end()) {
            auto& frames = processMemoryMap[msg.sender];
            int ownedPages = (int)frames.size();
            
            if (pagesToFree > ownedPages) {
                OS_LockGuard lock(printMutex);
                cout << "[MemoryService] WARNING: PID " << msg.sender 
                     << " requested to free " << pagesToFree 
                     << " pages but only owns " << ownedPages << " pages.\n";
                cout << "[MemoryService] Freeing all " << ownedPages << " owned pages instead.\n";
                pagesToFree = ownedPages;
            }
            
            int freed = 0;
            while (freed < pagesToFree && !frames.empty()) {
                int frame = frames.back();
                frames.pop_back();
                physicalMemory[frame] = false;
                usedPages--;
                freed++;
            }
            
            {
                OS_LockGuard lock(printMutex);
                cout << "[MemoryService] Freed " << freed << " pages for PID: " << msg.sender << "\n";
                int remaining = (int)frames.size();
                cout << "[MemoryService] PID " << msg.sender << " now owns " << remaining << " pages.\n";
                cout << "[MemoryService] Memory Status: " << usedPages << "/" << totalPages << " pages used.\n";
            }
            
            if (frames.empty()) {
                processMemoryMap.erase(msg.sender);
            }
        } else {
            OS_LockGuard lock(printMutex);
            cout << "[MemoryService] PID: " << msg.sender << " has no memory to free.\n";
        }
    } else if (msg.type == "mem_status") {
        OS_LockGuard lock(printMutex);
        cout << "\n===== Memory Status =====\n";
        cout << "Page Size: " << pageSize << " bytes | Total Pages: " << totalPages 
             << " | Used: " << usedPages << " | Free: " << (totalPages - usedPages) << "\n";
        if (processMemoryMap.empty()) {
            cout << "  No memory allocated to any process.\n";
        } else {
            for (auto& entry : processMemoryMap) {
                int pid = entry.first;
                auto& frames = entry.second;
                cout << "  PID " << pid << ": " << frames.size() << " pages (Frames:";
                for (int f : frames) cout << " " << f;
                cout << ")\n";
            }
        }
        cout << "=========================\n\n";
    } else if (msg.type == "process_dead") {
        freeAll(msg.sender);
    }
}

void MemoryService::freeAll(int pid) {
    if (processMemoryMap.find(pid) != processMemoryMap.end()) {
        auto& frames = processMemoryMap[pid];
        int freed = frames.size();
        for (int frame : frames) {
            physicalMemory[frame] = false;
            usedPages--;
        }
        processMemoryMap.erase(pid);

        OS_LockGuard lock(printMutex);
        cout << "[MemoryService] Freed all " << freed << " pages for DEAD PID: " << pid << "\n";
        cout << "[MemoryService] Memory Status: " << usedPages << "/" << totalPages << " pages used.\n";
    }
}
