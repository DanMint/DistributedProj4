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
#include <errno.h>

Reciever::Reciever(const std::string &containerID, const int &socket, std::unordered_map<int, int> &clientIdToSocket, std::mutex &clientIdToSocketMutex, std::vector<Peer> &ring, std::mutex &ringMutex) : 
containerID(containerID), socket(socket), clientIdToSocket(clientIdToSocket), clientIdToSocketMutex(clientIdToSocketMutex), ring(ring), ringMutex(ringMutex) {}

void Reciever::bootstrapListener() {
    std::cout << "Bootstrap listener started" << std::endl;
    
    while (true) {
        // Accept ALL pending connections in one go
        while (true) {
            int clientSocket = accept(socket, nullptr, nullptr);
            if (clientSocket < 0) {
                // No more pending connections
                break;
            }
            
            // New client connected
            std::cout << "Client connected! fd=" << clientSocket << std::endl;
            
            // Make client socket non-blocking
            int flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
            clientSockets.push_back(clientSocket);
            
            // Try to immediately receive the peerID from this new connection
            usleep(50000); // Small delay to let the message arrive (50ms)
            char buffer[1024];
            ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::string receivedMessage = buffer;
                
                if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "peerID:") {
                    std::string peerID = receivedMessage.substr(7);
                    std::cout << "Received peerID: " << peerID << " from fd=" << clientSocket << std::endl;
                    
                    // Store the mapping
                    clientIdToSocketMutex.lock();
                    clientIdToSocket[std::stoi(peerID)] = clientSocket;
                    clientIdToSocketMutex.unlock();
                }
            }
        }
        
        // Continue to check for any other messages from all clients
        char buffer[1024];
        for (const auto &cs : clientSockets) {
            ssize_t n = recv(cs, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::string receivedMessage = buffer;
                
                // Handle other message types here if needed
                if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "peerID:") {
                    std::string peerID = receivedMessage.substr(7);
                    std::cout << "Late peerID: " << peerID << " from fd=" << cs << std::endl;
                    
                    clientIdToSocketMutex.lock();
                    clientIdToSocket[std::stoi(peerID)] = cs;
                    clientIdToSocketMutex.unlock();
                }
                // Add handling for other message types here as needed
            }
        }
        
        clientIdToSocketMutex.lock();
        std::cout << clientIdToSocket.size() << std::endl;
        clientIdToSocketMutex.unlock();
        // Small delay to prevent CPU spinning
        usleep(200000); // 200ms
    }
}

void Reciever::start() {
    if (containerID == "bootstrap") {
        std::cout << "Bootstrap receiver started" << std::endl;
        bootstrapListener();
    }
}