#include "reciever.h"
#include "utils.h"

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
#include <sstream>

Reciever::Reciever(const std::string &containerID, const int &socket, std::unordered_map<int, int> &clientIdToSocket, std::mutex &clientIdToSocketMutex, std::vector<Peer> &ring, std::mutex &ringMutex) : 
containerID(containerID), socket(socket), clientIdToSocket(clientIdToSocket), clientIdToSocketMutex(clientIdToSocketMutex), ring(ring), ringMutex(ringMutex) {}

void Reciever::sendUpdateToPeer(int peerId, int pred, int succ) {
    clientIdToSocketMutex.lock();
    auto it = clientIdToSocket.find(peerId);
    if (it != clientIdToSocket.end()) {
        int peerSocket = it->second;
        std::string message = "UPDATE:" + std::to_string(pred) + "," + std::to_string(succ);
        send(peerSocket, message.c_str(), message.size(), 0);
        std::cout << "Sent update to peer " << peerId << ": pred=" << pred << ", succ=" << succ << std::endl;
    }
    clientIdToSocketMutex.unlock();
}

void Reciever::insertPeerIntoRing(int newPeerId) {
    ringMutex.lock();
    
    // Get all peer IDs currently in the ring EXCEPT the new one
    std::vector<int> ringPeerIds;
    clientIdToSocketMutex.lock();
    for (const auto& pair : clientIdToSocket) {
        if (pair.first != newPeerId) {  // Don't include the new peer yet!
            ringPeerIds.push_back(pair.first);
        }
    }
    clientIdToSocketMutex.unlock();
    
    // Sort the peer IDs
    std::sort(ringPeerIds.begin(), ringPeerIds.end());
    
    if (ringPeerIds.size() == 0) {
        // First peer in the ring - points to itself
        sendUpdateToPeer(newPeerId, newPeerId, newPeerId);
    }
    else if (ringPeerIds.size() == 1) {
        // Second peer in the ring
        int firstPeer = ringPeerIds[0];
        
        // Update both peers to point to each other
        sendUpdateToPeer(firstPeer, newPeerId, newPeerId);
        sendUpdateToPeer(newPeerId, firstPeer, firstPeer);
    }
    else {
        // Three or more peers (2 existing + 1 new = 3 total)
        int n = ringPeerIds.size();
        
        // We need to determine where this peer fits in the circular sorted order
        int predId = -1, succId = -1;
        
        // Check each adjacent pair in the ring to see where new peer fits
        for (int i = 0; i < n; i++) {
            int curr = ringPeerIds[i];
            int next = ringPeerIds[(i + 1) % n];  // Wrap around
            
            // Case 1: Normal case - newPeerId goes between curr and next
            if (curr < next) {
                if (curr < newPeerId && newPeerId < next) {
                    predId = curr;
                    succId = next;
                    break;
                }
            }
            // Case 2: Wrap-around case (curr > next means we're at the wrap point)
            else {
                // newPeerId is either larger than curr OR smaller than next
                if (newPeerId > curr || newPeerId < next) {
                    predId = curr;
                    succId = next;
                    break;
                }
            }
        }
        
        // If still not found, it must go at the wrap-around point
        if (predId == -1) {
            // This happens when all existing peers are equal (shouldn't happen)
            // or when newPeerId should go between last and first
            predId = ringPeerIds[n-1];  
            succId = ringPeerIds[0];     
        }
        
        // Send complete update to the new peer
        sendUpdateToPeer(newPeerId, predId, succId);
        
        // Update the predecessor's successor to point to new peer
        sendUpdateToPeer(predId, -1, newPeerId);
        
        // Update the successor's predecessor to point to new peer
        sendUpdateToPeer(succId, newPeerId, -1);
    }
    
    // Add the new peer to our list for printing
    ringPeerIds.push_back(newPeerId);
    std::sort(ringPeerIds.begin(), ringPeerIds.end());
    
    // Print the ring
    std::cout << "{RING:[";
    for (size_t i = 0; i < ringPeerIds.size(); i++) {
        std::cout << ringPeerIds[i];
        if (i < ringPeerIds.size() - 1) std::cout << ",";
    }
    std::cout << "]}" << std::endl;
    
    ringMutex.unlock();
}

void Reciever::bootstrapListener() {
    std::cout << "Bootstrap listener started" << std::endl;
    
    while (true) {
        // Accept ALL pending connections in one go
        while (true) {
            int clientSocket = accept(socket, nullptr, nullptr);
            if (clientSocket < 0) {
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
                    std::string peerIdStr = receivedMessage.substr(7);
                    int peerId = std::stoi(peerIdStr);
                    std::cout << "Received peerID: " << peerId << " from fd=" << clientSocket << std::endl;
                    
                    // Store the mapping
                    clientIdToSocketMutex.lock();
                    clientIdToSocket[peerId] = clientSocket;
                    clientIdToSocketMutex.unlock();
                    
                    // Insert peer into ring and update neighbors
                    insertPeerIntoRing(peerId);
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
                
                // Handle late peerID messages
                if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "peerID:") {
                    std::string peerIdStr = receivedMessage.substr(7);
                    int peerId = std::stoi(peerIdStr);
                    
                    clientIdToSocketMutex.lock();
                    if (clientIdToSocket.find(peerId) == clientIdToSocket.end()) {
                        std::cout << "Late peerID: " << peerId << " from fd=" << cs << std::endl;
                        clientIdToSocket[peerId] = cs;
                        clientIdToSocketMutex.unlock();
                        insertPeerIntoRing(peerId);
                    } else {
                        clientIdToSocketMutex.unlock();
                    }
                }
            }
        }
        
        // Small delay to prevent CPU spinning
        usleep(200000); // 200ms
    }
}

void Reciever::peerListener() {
    // Extract peer ID from container ID
    std::string peerIdStr = Utils::removeNFromContainerID(containerID);
    if (peerIdStr.empty()) {
        std::cout << "Error: Could not extract peer ID from container ID" << std::endl;
        return;
    }
    int myPeerId = std::stoi(peerIdStr);
    
    // Initialize predecessor and successor to self
    int predecessor = myPeerId;
    int successor = myPeerId;
    
    std::cout << "Peer " << myPeerId << " listener started" << std::endl;
    
    while (true) {
        char buffer[1024];
        ssize_t n = recv(socket, buffer, sizeof(buffer) - 1, 0);
        
        if (n > 0) {
            buffer[n] = '\0';
            std::string receivedMessage = buffer;
            
            // Handle UPDATE message from bootstrap
            if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "UPDATE:") {
                std::string updateData = receivedMessage.substr(7);
                
                // Parse pred,succ
                size_t commaPos = updateData.find(',');
                if (commaPos != std::string::npos) {
                    std::string predStr = updateData.substr(0, commaPos);
                    std::string succStr = updateData.substr(commaPos + 1);
                    
                    int newPred = std::stoi(predStr);
                    int newSucc = std::stoi(succStr);
                    
                    // Update predecessor if not -1
                    if (newPred != -1) {
                        predecessor = newPred;
                    }
                    
                    // Update successor if not -1
                    if (newSucc != -1) {
                        successor = newSucc;
                    }
                    
                    // Print current state as required by assignment
                    std::cout << "{peer_id:" << myPeerId 
                              << ", predecessor:" << predecessor 
                              << ", successor:" << successor << "}" << std::endl;
                }
            }
        } else if (n == 0) {
            // Connection closed
            std::cout << "Connection to bootstrap closed" << std::endl;
            break;
        } else {
            // Error or would block
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "Receive error: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        usleep(100000); // 100ms delay
    }
}

void Reciever::start() {
    if (containerID == "bootstrap") {
        std::cout << "Bootstrap receiver started" << std::endl;
        bootstrapListener();
    } else {
        // This is a peer node
        peerListener();
    }
}