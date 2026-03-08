#ifndef WIN32_LEAN_AND_MEAN // windows.h includes old winsock. to avoid conflicts with winsock2, this define is added.
#define WIN32_LEAN_AND_MEAN
#endif
#include <algorithm>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <sysinfoapi.h>
#include <iomanip>
#include <pdh.h>
#define GB_SIZE (1024.0*1024.0*1024.0)
#define DEFAULT_PORT 8080

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

SOCKET createListeningSocket(int port) {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket. Error code: " << WSAGetLastError() << "\n\n";
        WSACleanup();
        std::exit(1);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Bind failed. Error: " << WSAGetLastError() << "\n\n";
        closesocket(serverSocket);
        WSACleanup();
        std::exit(1);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed. Error code: " << WSAGetLastError() << "\n\n";
        closesocket(serverSocket);
        WSACleanup();
        std::exit(1);
    }

    std::cout << "Server is listening on port 8080...\n\n";
    return serverSocket;
}

void initWinsock() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed. Error code: " << iResult << "\n\n";
        std::exit(1);
    }
}

/* Creates a PDH query and adds relevant performance counter for monitoring cpu usage
 */
void initCpuCounter() {
    PDH_STATUS openQueryStatus = PdhOpenQueryW(NULL, 0, &cpuQuery);
    if (openQueryStatus != ERROR_SUCCESS) {
        std::cout << "PdhOpenQuery failed. Error code: " << openQueryStatus << "\n" << std::endl;
    }
    PDH_STATUS addCounterStatus = PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time",
        0, &cpuTotal);
    if (addCounterStatus != ERROR_SUCCESS) {
        std::cout << "PdhAddEnglishCounter failed. Error code: " << addCounterStatus << "\n" << std::endl;
    }
    PDH_STATUS collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << std::endl;
    }
}

void listFilesInDir(const std::string& dirPath) {
    if (dirPath.length() > MAX_PATH - 3) {
        std::cout << "Directory path is too long.\n" << std::endl;
        return;
    }
    std::string searchPath = dirPath + "\\*";
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA findFileData;
    hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "Could not open directory or directory is empty. Error: " << GetLastError() << "\n\n";
        return;
    }

    do {
        std::string fileName = findFileData.cFileName;
        if (fileName == "." || fileName == "..") {
            continue;
        }

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "[DIR]  " << fileName << std::endl;
        } else {
            std::cout << "       " << fileName << std::endl;
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    DWORD errorCode = GetLastError();
    if (errorCode != ERROR_NO_MORE_FILES) {
        std::cout << "FindNextFileA failed. Error code: " << errorCode << "\n\n";
        FindClose(hFind);
        return;
    }

    FindClose(hFind);
    std::cout << "\n";
}

/* Samples performance data regarding cpu usage and returning formatted value in percentage
 */
double getCurrentCPUUsage(){
    PDH_FMT_COUNTERVALUE counterVal;

    PDH_STATUS collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << "\n" << std::endl;
    }
    Sleep(1000);
    collectDataStatus = PdhCollectQueryData(cpuQuery);
    if (collectDataStatus != ERROR_SUCCESS) {
        std::cout << "PdhCollectQueryData failed. Error code: " << collectDataStatus << "\n" << std::endl;
    }
    PDH_STATUS getFormattedStatus = PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    if (getFormattedStatus != ERROR_SUCCESS) {
        std::cout << "PdhGetFormattedCounterValue failed. Error code: " << getFormattedStatus << "\n" << std::endl;
    }
    return counterVal.doubleValue;
}

/* Removes leading and trailing whitespaces from a string
 */
void trim(std::string& str) {
    while (!str.empty() && str.back() == ' ') {
        str.pop_back();
    }
    while (!str.empty() && str.front() == ' ') {
        str.erase(str.begin(), str.begin() + 1);
    }
}

void handleCommand(const std::string& input) {
    std::string trimmedInput(input);
    trim(trimmedInput);
    if (trimmedInput.empty()) {
        return;
    }

    std::string command, arg1;

    size_t spacePos = trimmedInput.find(' ');
    if (spacePos != std::string::npos) {
        command = trimmedInput.substr(0, spacePos);
        arg1 = trimmedInput.substr(spacePos + 1);
        trim(arg1);
    } else {
        command = trimmedInput;
    }

    if (command == "EXIT") {
        std::exit(0);
    }
    else if (command == "RAM") {
        if (!arg1.empty()) {
            std::cout << "RAM does not take any parameters. Try again.\n" << std::endl;
            return;
        }
        MEMORYSTATUSEX state = {0};
        state.dwLength = sizeof(state);
        if (!GlobalMemoryStatusEx(&state)) {
            DWORD errorCode = GetLastError();
            std::cout << "Unable to get memory status. Error code:" << errorCode << "\n" << std::endl;
        } else {
            double availInGB = state.ullAvailPhys / GB_SIZE;
            double totalInGB = state.ullTotalPhys / GB_SIZE;
            double usedInGB = totalInGB - availInGB;
            DWORD percentage = state.dwMemoryLoad;
            std::cout << std::fixed << std::setprecision(1);
            std::cout << "RAM In Use: " << usedInGB << " GB / " << totalInGB <<
                " GB (" << percentage << "%)" << std::endl;
            std::cout << "RAM Available: " << availInGB << " GB\n" << std::endl;
        }
    } else if (command == "CPU") {
        if (!arg1.empty()) {
            std::cout << "CPU does not take any parameters. Try again.\n" << std::endl;
            return;
        }
        std::cout << "Calculating..." << std::endl;
        double cpuUsage = getCurrentCPUUsage();
        std::cout << "CPU Usage: " << cpuUsage << "%\n" << std::endl;
    } else if (command == "DIR") {
        if (arg1.empty()) {
            std::cout << "DIR requires a path parameter (e.g., DIR C:\\)\n" << std::endl;
            return;
        }
        listFilesInDir(arg1);
    } else {
        std::cout << "Unknown command: '" << command << "'. Try again\n" << std::endl;
    }
}

int main() {
    initCpuCounter();
    initWinsock();
    std::string command;

    SOCKET serverSocket = createListeningSocket(DEFAULT_PORT);

    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Accept failed. Error code: " << WSAGetLastError() << "\n\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!\n";

    while (true) {
        std::cout << "Monitor> ";
        getline(std::cin, command);
        handleCommand(command);
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
