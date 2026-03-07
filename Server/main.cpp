#include <iostream>
#include <string>
#include <windows.h>
#include <sysinfoapi.h>
#define GB_SIZE (1024*1024*1024)

void handleCommand(const std::string& cmd) {
    if (cmd == "EXIT") {
        std::exit(0);
    }
    if (cmd == "RAM") {
        MEMORYSTATUSEX state;
        state.dwLength = sizeof(state);
        if (!GlobalMemoryStatusEx(&state)) {
            DWORD errorCode = GetLastError();
            std::cout << "Unable to get memory status. Error code:" << errorCode << std::endl;
        } else {
            double availInGB = static_cast<double>(state.ullAvailPhys) / GB_SIZE;
            double totalInGB = static_cast<double>(state.ullTotalPhys) / GB_SIZE;
            DWORD percentage = state.dwMemoryLoad;
            std:: cout << "RAM: " << availInGB << " / " << totalInGB << "(" << percentage << "%)" << std::endl;
        }
    } else {
        std::cout << "Unknown command: " << cmd << ". Try again" << std::endl;
    }
}

int main() {
    std::string command;

    while (true) {
        std::cout << "Memory> ";
        getline(std::cin, command);
        handleCommand(command);
        break;
    }
    return 0;
}
