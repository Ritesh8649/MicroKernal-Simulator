#include <iostream>
#include <sstream>
#include "../kernel/OS_Mutex.h"
#include "Shell.h"
#include "../kernel/Globals.h"

using namespace std;

Shell::Shell(Kernel* k) {
    kernel = k;
}

void Shell::run() {
    string command;

    while (true) {
        {
        OS_LockGuard lock(printMutex);
        cout << ">> ";
        }
        kernel->stopScheduler();   // pause background noise
        getline(cin, command);
        kernel->startScheduler();  // resume. 

        if (command == "exit") break;

        if (command == "help") {
            cout << "\n===== MicroKernel Shell Commands =====\n";
            cout << "  create_process        - Create a new process\n";
            cout << "  list_process          - List all active processes\n";
            cout << "  status                - Show processes & memory usage\n";
            cout << "  alloc <pid> <bytes>   - Allocate memory to a process\n";
            cout << "  free <pid> <bytes>    - Free memory from a process\n";
            cout << "  create_file <name>    - Create a file\n";
            cout << "  write_file <name>     - Write to a file\n";
            cout << "  read_file <name>      - Read a file\n";
            cout << "  delete_file <name>    - Delete a file\n";
            cout << "  kill_service          - Simulate FileService crash\n";
            cout << "  exit                  - Exit the simulator\n";
            cout << "======================================\n\n";
            continue;
        }

        Message msg;
        msg.sender = 1;      // Shell PID
        msg.receiver = 0;    // Kernel
        if (command == "status") {
            // First list processes
            msg.type = "command";
            msg.data = "list_process";
            kernel->sendMessage(msg);
            kernel->processMessages();

            // Then show memory status
            Message memMsg;
            memMsg.sender = 1;
            memMsg.receiver = 0;
            memMsg.type = "mem_status";
            kernel->sendMessage(memMsg);
            kernel->processMessages();
            continue;
        }
        else if (command.find("alloc") == 0) {
         msg.type = "memory";
         msg.capabilityToken = "CAP_MEM";

         stringstream ss(command);
         string cmd;
         int amount = 0, pid = 0;

         ss >> cmd >> pid >> amount;

         if (ss.fail()) {
         cout << "Usage: alloc <pid> <amount>\n";
         continue;
        }

         msg.data = to_string(amount);
         msg.sender = pid;   // ✅ IMPORTANT: assign to target process
        }
        
        else if (command.find("free") == 0) {
          msg.type = "free";

          stringstream ss(command);
          string cmd;
          int amount = 0, pid = 0;
      
          ss >> cmd >> pid >> amount;
          
          if (ss.fail()) {
              cout << "Usage: free <pid> <amount>\n";
              continue;
          }
      
          msg.data = to_string(amount);
          msg.sender = pid;
        }
        else if (command.find("create_file") == 0 ||
                 command.find("read_file") == 0 ||
                 command.find("delete_file") == 0 ||
                 command.find("write_file") == 0) {
            msg.type = "file";
            msg.data = command;
            msg.capabilityToken = "CAP_FILE"; // Prove we have the token
        }
        else if (command.find("kill_service") == 0) {
            msg.type = "kill_service";
            msg.data = "FileService"; 
        }
        else {
          msg.type = "command";
          msg.data = command;
        }
        kernel->sendMessage(msg);
        kernel->processMessages();
    }
}
