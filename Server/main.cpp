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
#include <mutex>
#include <pdh.h>
#include <sstream>
#include <thread>
#define GB_SIZE (1024.0*1024.0*1024.0)
#define DEFAULT_PORT 8080
#define DEFAULT_BUFLEN 4096

const uint32_t MAX_MESSAGE_SIZE = 1024*1024;

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;
static std::mutex cpuMutex;

SOCKET createListeningSocket(int port);
void initWinsock();
void initCpuCounter();
std::string listFilesInDir(const std::string& dirPath);
double getCurrentCPUUsage();
void trim(std::string& str);
std::string handleCommand(const std::string& input);
void handleClientConnection(SOCKET clientSocket);
bool sendMessage(SOCKET clientSocket, const std::string& message);
bool sendAll(SOCKET clientSocket, const char* buf, int length);
bool recvMessage(SOCKET clientSocket);
bool recvAll(SOCKET clientSocket, char* buf, int length);

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

std::string listFilesInDir(const std::string& dirPath) {
    std::ostringstream res;
    if (dirPath.length() > MAX_PATH - 3) {
        res << "Directory path is too long.\n" << std::endl;
        return res.str();
    }
    std::string searchPath = dirPath + "\\*";
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA findFileData;
    hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        res << "Could not open directory or directory is empty. Error: " << GetLastError() << "\n\n";
        return res.str();
    }

    do {
        std::string fileName = findFileData.cFileName;
        if (fileName == "." || fileName == "..") {
            continue;
        }

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            res << "[DIR]  " << fileName << std::endl;
        } else {
            res << "       " << fileName << std::endl;
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    DWORD errorCode = GetLastError();
    if (errorCode != ERROR_NO_MORE_FILES) {
        std::cout << "FindNextFileA failed. Error code: " << errorCode << "\n\n";
        FindClose(hFind);
        return res.str();
    }

    FindClose(hFind);
    res << "\n";
    return res.str();
}

/* Samples performance data regarding cpu usage and returning formatted value in percentage
 */
double getCurrentCPUUsage() {
    std::lock_guard<std::mutex> lock(cpuMutex);
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
    while (!str.empty() && (str.back() == ' ' || str.back() == '\n' || str.back() == '\r')) {
        str.pop_back();
    }
    while (!str.empty() && (str.front() == ' ' || str.front() == '\n' || str.front() == '\r')) {
        str.erase(str.begin(), str.begin() + 1);
    }
}

std::string handleCommand(const std::string& input) {
    std::string trimmedInput(input);
    std::ostringstream msg;
    trim(trimmedInput);
    if (trimmedInput.empty()) {
        return "No valid command was entered.\n\n";
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

    if (command == "RAM") {
        if (!arg1.empty()) {
            msg << "RAM does not take any parameters. Try again.\n" << std::endl;
            return msg.str();
        }
        MEMORYSTATUSEX state = {0};
        state.dwLength = sizeof(state);
        if (!GlobalMemoryStatusEx(&state)) {
            DWORD errorCode = GetLastError();
            msg << "Unable to get memory status. Error code:" << errorCode << "\n" << std::endl;
        } else {
            double availInGB = state.ullAvailPhys / GB_SIZE;
            double totalInGB = state.ullTotalPhys / GB_SIZE;
            double usedInGB = totalInGB - availInGB;
            DWORD percentage = state.dwMemoryLoad;
            msg << std::fixed << std::setprecision(1);
            msg << "RAM In Use: " << usedInGB << " GB / " << totalInGB <<
                " GB (" << percentage << "%)" << std::endl;
            msg << "RAM Available: " << availInGB << " GB\n" << std::endl;
        }
    } else if (command == "CPU") {
        if (!arg1.empty()) {
            msg << "CPU does not take any parameters. Try again.\n" << std::endl;
            return msg.str();
        }
        double cpuUsage = getCurrentCPUUsage();
        msg << "CPU Usage: " << cpuUsage << "%\n" << std::endl;
    } else if (command == "DIR") {
        if (arg1.empty()) {
            msg << "DIR requires a path parameter (e.g., DIR C:\\)\n" << std::endl;
            return msg.str();
        }
        msg << listFilesInDir(arg1);
    } else {
        msg << "Unknown command: '" << command << "'. Try again\n" << std::endl;
    }
    return msg.str();
}

bool sendAll(SOCKET clientSocket, const char* buf, int length) {
    int totalSent = 0;
    while (totalSent < length) {
        int sent = send(clientSocket, buf + totalSent, length - totalSent, 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}

/* Sends a message. first 4 bytes sent are the message length, then the actual message is sent.
 */
bool sendMessage(SOCKET clientSocket, const std::string& message) {
    if (message.length() > UINT32_MAX) { // message.length is up to 64 bits so we can't send messages that are too large
        return false;
    }

    // converting length to network byte order (big endian) before sending
    uint32_t messageLength = htonl(static_cast<uint32_t>(message.length()));

    if (!sendAll(clientSocket, reinterpret_cast<const char *>(&messageLength), sizeof(messageLength))) {
        return false;
    }
    if (!sendAll(clientSocket, message.data(), static_cast<int>(message.length()))) {
        return false;
    }
    return true;
}

/* Receives a message: first 4 bytes are the length, then the actual message is written into the message variable.
 */
bool recvMessage(SOCKET clientSocket, std::string& message) {
    uint32_t messageLength = 0;
    if (!recvAll(clientSocket, reinterpret_cast<char *>(&messageLength), sizeof(messageLength))) {
        return false;
    }

    // converting from network byte order to host byte order to get correct value of message length
    messageLength = ntohl(messageLength);

    if (messageLength > MAX_MESSAGE_SIZE) { // allow up to some maximum reasonable size to avoid huge memory allocation
        return false;
    }

    message.resize(messageLength);

    if (messageLength == 0) { // received empty message
        return true;
    }

    if (!recvAll(clientSocket, message.data(), static_cast<int>(messageLength))) {
        return false;
    }

    return true;
}

bool recvAll(SOCKET clientSocket, char* buf, int length) {
    int totalReceived = 0;
    while (totalReceived < length) {
        int bytesReceived = recv(clientSocket, buf + totalReceived, length - totalReceived, 0);
        if (bytesReceived <= 0) {
            return false;
        }
        totalReceived += bytesReceived;
    }
    return true;
}

void handleClientConnection(SOCKET clientSocket) {
    while (true) {
        std::string command;
        if (!recvMessage(clientSocket, command)) {
            std::cout << "Receiving failed. Error code: " << WSAGetLastError() << "\n\n";
            break;
        }

        std::cout << "[CLIENT]: " << command << "\n\n";

        std::string msg = handleCommand(command);
        if (!sendMessage(clientSocket, msg)) {
            std::cout << "Send failed. Error code: " << WSAGetLastError() << "\n\n";
            break;
        }
    }
    closesocket(clientSocket);
}

int main() {
    initCpuCounter();
    initWinsock();

    SOCKET serverSocket = createListeningSocket(DEFAULT_PORT);

    while (true) {
        sockaddr_in clientAddr = {};
        int clientAddrSize = sizeof(clientAddr);
        std::cout << "Waiting for a connection...\n" << std::endl;
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept failed. Error code: " << WSAGetLastError() << "\n\n";
            continue;
        }

        std::cout << "Client connected!\n";

        std::thread t(handleClientConnection, clientSocket);
        t.detach();


    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
