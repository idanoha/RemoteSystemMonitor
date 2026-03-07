#include <algorithm>
#include <iostream>
#include <string>
#include <windows.h>
#include <sysinfoapi.h>
#include <iomanip>
#include <pdh.h>
#define GB_SIZE (1024.0*1024.0*1024.0)

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

/* Creates a PDH query and adds relevant performance counter for monitoring cpu usage
 */
void initCpuCounter() {
    PDH_STATUS openQueryStatus = PdhOpenQueryW(NULL, 0, &cpuQuery);
    if (openQueryStatus != ERROR_SUCCESS) {
        std::cout << "PdhOpenQuery failed. Error code: " << openQueryStatus << std::endl;
    }
    PDH_STATUS addCounterStatus = PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time",
        0, &cpuTotal);
    if (addCounterStatus != ERROR_SUCCESS) {
        std::cout << "PdhAddEnglishCounter failed. Error code: " << addCounterStatus << std::endl;
    }
    PDH_STATUS collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << std::endl;
    }
}

/* Samples performance data regarding cpu usage and returning formatted value in percentage
 */
double getCurrentCPUUsage(){
    PDH_FMT_COUNTERVALUE counterVal;

    PDH_STATUS collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << std::endl;
    }
    Sleep(1000);
    collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << std::endl;
    }
    PDH_STATUS getFormattedStatus = PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    if (getFormattedStatus != ERROR_SUCCESS) {
        std::cout << "PdhGetFormattedCounterValue failed. Error code: " << getFormattedStatus << std::endl;
    }
    return counterVal.doubleValue;
}

void trim(std::string &str) { // Note: also deletes interior whitespaces, could be bad for future commands
    std::string::iterator end_pos = std::remove(str.begin(), str.end(), ' ');
    str.erase(end_pos, str.end());
}

void handleCommand(const std::string& input) {
    std::string cmd = input;
    trim(cmd);
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
            double usedInGB = totalInGB - availInGB;
            DWORD percentage = state.dwMemoryLoad;
            std::cout << std::fixed << std::setprecision(1);
            std::cout << "RAM In Use: " << usedInGB << " GB / " << totalInGB <<
                " GB (" << percentage << "%)" << std::endl;
            std::cout << "RAM Available: " << availInGB << " GB" << std::endl;
        }
    } else if (cmd == "CPU") {
        std::cout << "Calculating..." << std::endl;
        double cpuUsage = getCurrentCPUUsage();
        std::cout << "CPU Usage: " << cpuUsage << "%" << std::endl;

    } else {
        std::cout << "Unknown command: '" << cmd << "'. Try again" << std::endl;
    }
}

int main() {
    initCpuCounter();
    std::string command;

    while (true) {
        std::cout << "Monitor> ";
        getline(std::cin, command);
        handleCommand(command);
    }
    return 0;
}
