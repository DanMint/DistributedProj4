#include "reciever.h"

#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

Reciever::Reciever(const std::string &containerID, const int &socket) : 
containerID(containerID), socket(socket) {}

void Reciever::bootstrapListener() {
    std::vector<int> clientSockets;
    while (true) {
        int clientSocket = accept(socket, nullptr, nullptr);
        if (clientSocket < 0) {
            // No more to accept right now
            continue;
        }
        int flags = fcntl(clientSocket, F_GETFL, 0);
        fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

        std::cout << "Client connected! fd=" << clientSocket << "\n";
        clientSockets.push_back(clientSocket);
    }
}

void Reciever::start() {
    if (containerID == "bootstrap") {
        std::cout << "ENETRED BOOSTRAP IN RECIEVER" << std::endl;
        Reciever::bootstrapListener();
    }
}