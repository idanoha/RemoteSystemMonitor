#include <iostream>
#include <string>
#include <windows.h>

void handleCommand(std::string command) {
    if (command == "EXIT") {
        std::exit(0);
    } else if (command == "RAM") {

    } else {
        std::cout << "Unknown command: " << command << ". Try again" << std::endl;
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