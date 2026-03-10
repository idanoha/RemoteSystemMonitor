#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <cstdint>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT 8080
#define DEFAULT_BUFLEN 4096

const uint32_t MAX_MESSAGE_SIZE = 1024 * 1024;

void initWinsock();
SOCKET connectToServer();
bool sendMessage(SOCKET clientSocket, const std::string& message);
bool sendAll(SOCKET clientSocket, const char* buf, int length);
bool recvMessage(SOCKET clientSocket);
bool recvAll(SOCKET clientSocket, char* buf, uint32_t length);

SOCKET connectToServer() {
    SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed.\n";
        WSACleanup();
        std::exit(1);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Unable to connect to server!\n";
        closesocket(connectSocket);
        WSACleanup();
        std::exit(1);
    }
    return connectSocket;
}

void initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed.\n";
        std::exit(1);
    }
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

int main() {
    initWinsock();
    SOCKET connectSocket = connectToServer();

    std::cout << "Connected to Remote Monitor Server!\n\n";

    std::string command, message;

    while (true) {
        std::cout << "Monitor> ";
        std::getline(std::cin, command);

        if (command.empty()) {
            continue;
        }

        if (command == "EXIT") {
            break;
        }

        if (!sendMessage(connectSocket, command)) {
            std::cout << "Send failed. Error code: " << WSAGetLastError() << "\n\n";
            break;
        }

        if (!recvMessage(connectSocket, message)) {
            std::cout << "Receive failed. Error code: " << WSAGetLastError() << "\n\n";
            break;
        }

        std::cout << message;
    }

    closesocket(connectSocket);
    WSACleanup();
    return 0;
}