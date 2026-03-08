#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT 8080
#define DEFAULT_BUFLEN 4096

void initWinsock();
SOCKET connectToServer();

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



int main() {
    initWinsock();
    SOCKET connectSocket = connectToServer();

    std::cout << "Connected to Remote Monitor Server!\n\n";

    char recvbuf[DEFAULT_BUFLEN];
    std::string command;

    while (true) {
        std::cout << "Monitor> ";
        std::getline(std::cin, command);

        if (command.empty()) {
            continue;
        }

        if (command == "EXIT") {
            break;
        }

        send(connectSocket, command.c_str(), command.length(), 0);


        ZeroMemory(recvbuf, DEFAULT_BUFLEN);
        int bytesReceived = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);

        if (bytesReceived > 0) {
            std::cout << std::string(recvbuf, bytesReceived);
        } else if (bytesReceived == 0) {
            std::cout << "Server closed the connection.\n";
            break;
        } else {
            std::cout << "recv failed.\n";
            break;
        }
    }

    closesocket(connectSocket);
    WSACleanup();
    return 0;
}