#include <iostream>
#include <string>
#include <windows.h>
#include <sysinfoapi.h>
#define GB_SIZE (1024.0*1024.0*1024.0)

void handleCommand(const std::string& cmd) {
    if (cmd == "EXIT") {
        std::exit(0);
    }
    if (cmd == "RAM") {
        MEMORYSTATUSEX state = {0};
        state.dwLength = sizeof(state);
        if (!GlobalMemoryStatusEx(&state)) {
            DWORD errorCode = GetLastError();
            std::cout << "Unable to get memory status. Error code:" << errorCode << std::endl;
        } else {
            double availInGB = state.ullAvailPhys / GB_SIZE;
            double totalInGB = state.ullTotalPhys / GB_SIZE;
            double usedInGB = totalInGB - state.ullAvailPhys / GB_SIZE;
            DWORD percentage = state.dwMemoryLoad;
            std:: cout << "RAM In Use: " << usedInGB << "GB / " << totalInGB <<
                " GB (" << percentage << "%)" << std::endl;
            std::cout << "RAM Available: " << availInGB << " GB" << std::endl;
        }
    } else {
        std::cout << "Unknown command: '" << cmd << "'. Try again" << std::endl;
    }
}

int main() {
    std::string command;

    while (true) {
        std::cout << "Memory> ";
        getline(std::cin, command);
        handleCommand(command);
    }
    return 0;
}
