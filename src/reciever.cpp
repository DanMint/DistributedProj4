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
#include <fstream>
#include <netdb.h>

Reciever::Reciever(const std::string &containerID, const int &socket, 
                   std::unordered_map<int, int> &clientIdToSocket, 
                   std::mutex &clientIdToSocketMutex, 
                   std::vector<Peer> &ring, std::mutex &ringMutex,
                   const std::string &objectsFile) : 
    containerID(containerID), socket(socket), clientIdToSocket(clientIdToSocket), 
    clientIdToSocketMutex(clientIdToSocketMutex), ring(ring), ringMutex(ringMutex),
    objectsFile(objectsFile), myPeerId(-1), predecessor(-1), successor(-1) {}

void Reciever::sendUpdateToPeer(int peerId, int pred, int succ) {
    clientIdToSocketMutex.lock();
    auto it = clientIdToSocket.find(peerId);
    if (it != clientIdToSocket.end()) {
        int peerSocket = it->second;
        std::string message = "UPDATE:" + std::to_string(pred) + "," + std::to_string(succ);
        send(peerSocket, message.c_str(), message.size(), 0);
    }
    clientIdToSocketMutex.unlock();
}

int Reciever::findResponsiblePeer(int objectId) {
    clientIdToSocketMutex.lock();
    std::vector<int> peerIds;
    for (const auto& pair : clientIdToSocket) {
        peerIds.push_back(pair.first);
    }
    clientIdToSocketMutex.unlock();
    
    if (peerIds.empty()) return -1;
    
    std::sort(peerIds.begin(), peerIds.end());
    
    // Object with ID between peer1 and peer2 is stored on peer2
    for (size_t i = 0; i < peerIds.size(); i++) {
        int currPeer = peerIds[i];
        int prevPeer = peerIds[(i + peerIds.size() - 1) % peerIds.size()];
        
        if (prevPeer < currPeer) {
            // Normal case: object ID between prevPeer and currPeer
            if (objectId > prevPeer && objectId <= currPeer) {
                return currPeer;
            }
        } else {
            // Wrap-around case
            if (objectId > prevPeer || objectId <= currPeer) {
                return currPeer;
            }
        }
    }
    
    return peerIds[0];
}

void Reciever::handleClientRequest(const std::string &request, int clientSocket) {
    // Parse REQUEST:reqID,operationType,objectID,clientID
    if (request.substr(0, 8) != "REQUEST:") return;
    
    std::string data = request.substr(8);
    std::istringstream ss(data);
    std::string reqId, opType, objectIdStr, clientIdStr;
    
    std::getline(ss, reqId, ',');
    std::getline(ss, opType, ',');
    std::getline(ss, objectIdStr, ',');
    std::getline(ss, clientIdStr, ',');
    
    int objectId = std::stoi(objectIdStr);
    int clientId = std::stoi(clientIdStr);
    int responsiblePeer = findResponsiblePeer(objectId);
    
    std::cout << "Client request: " << opType << " object " << objectId 
              << " (responsible peer: " << responsiblePeer << ")" << std::endl;
    
    if (responsiblePeer == -1) {
        send(clientSocket, "-1", 2, 0);
        return;
    }
    
    // Store mapping of request to client socket
    std::string requestKey = reqId + ":" + opType + ":" + objectIdStr + ":" + clientIdStr;
    requestMapMutex.lock();
    requestToClientSocket[requestKey] = clientSocket;
    requestMapMutex.unlock();
    
    // Forward request to peer 1 to start ring traversal
    clientIdToSocketMutex.lock();
    auto it = clientIdToSocket.find(1);
    if (it != clientIdToSocket.end()) {
        // Format: RING:reqID:opType:objectID:clientID:responsiblePeer:startPeer
        std::string ringMsg = "RING:" + reqId + ":" + opType + ":" + 
                             objectIdStr + ":" + clientIdStr + ":" +
                             std::to_string(responsiblePeer) + ":1";
        send(it->second, ringMsg.c_str(), ringMsg.size(), 0);
    }
    clientIdToSocketMutex.unlock();
}

void Reciever::insertPeerIntoRing(int newPeerId) {
    ringMutex.lock();
    
    std::vector<int> ringPeerIds;
    clientIdToSocketMutex.lock();
    for (const auto& pair : clientIdToSocket) {
        if (pair.first != newPeerId) {
            ringPeerIds.push_back(pair.first);
        }
    }
    clientIdToSocketMutex.unlock();
    
    std::sort(ringPeerIds.begin(), ringPeerIds.end());
    
    if (ringPeerIds.size() == 0) {
        sendUpdateToPeer(newPeerId, newPeerId, newPeerId);
    }
    else if (ringPeerIds.size() == 1) {
        int firstPeer = ringPeerIds[0];
        sendUpdateToPeer(firstPeer, newPeerId, newPeerId);
        sendUpdateToPeer(newPeerId, firstPeer, firstPeer);
    }
    else {
        int n = ringPeerIds.size();
        int predId = -1, succId = -1;
        
        for (int i = 0; i < n; i++) {
            int curr = ringPeerIds[i];
            int next = ringPeerIds[(i + 1) % n];
            
            if (curr < next) {
                if (curr < newPeerId && newPeerId < next) {
                    predId = curr;
                    succId = next;
                    break;
                }
            }
            else {
                if (newPeerId > curr || newPeerId < next) {
                    predId = curr;
                    succId = next;
                    break;
                }
            }
        }
        
        if (predId == -1) {
            predId = ringPeerIds[n-1];  
            succId = ringPeerIds[0];     
        }
        
        sendUpdateToPeer(newPeerId, predId, succId);
        sendUpdateToPeer(predId, -1, newPeerId);
        sendUpdateToPeer(succId, newPeerId, -1);
    }
    
    ringPeerIds.push_back(newPeerId);
    std::sort(ringPeerIds.begin(), ringPeerIds.end());
    
    std::cout << "{RING:[";
    for (size_t i = 0; i < ringPeerIds.size(); i++) {
        std::cout << ringPeerIds[i];
        if (i < ringPeerIds.size() - 1) std::cout << ",";
    }
    std::cout << "]}" << std::endl;
    
    ringMutex.unlock();
}

void Reciever::bootstrapListener() {
    while (true) {
        while (true) {
            int clientSocket = accept(socket, nullptr, nullptr);
            if (clientSocket < 0) {
                break;
            }
            
            int flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
            clientSockets.push_back(clientSocket);
            
            usleep(50000);
            char buffer[1024];
            ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::string receivedMessage = buffer;
                
                if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "peerID:") {
                    std::string peerIdStr = receivedMessage.substr(7);
                    int peerId = std::stoi(peerIdStr);
                    
                    clientIdToSocketMutex.lock();
                    clientIdToSocket[peerId] = clientSocket;
                    clientIdToSocketMutex.unlock();
                    
                    insertPeerIntoRing(peerId);
                }
                else if (receivedMessage.substr(0, 8) == "REQUEST:") {
                    handleClientRequest(receivedMessage, clientSocket);
                }
            }
        }
        
        char buffer[1024];
        for (const auto &cs : clientSockets) {
            ssize_t n = recv(cs, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::string receivedMessage = buffer;
                
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
                else if (receivedMessage.substr(0, 8) == "REQUEST:") {
                    handleClientRequest(receivedMessage, cs);
                }
                else if (receivedMessage.substr(0, 11) == "OBJ_STORED:") {
                    // Peer confirmed storage, relay to client
                    std::string data = receivedMessage.substr(11);
                    std::istringstream ss(data);
                    std::string reqId, objId, cliId, peerId;
                    std::getline(ss, reqId, ':');
                    std::getline(ss, objId, ':');
                    std::getline(ss, cliId, ':');
                    std::getline(ss, peerId, ':');
                    
                    std::string requestKey = reqId + ":STORE:" + objId + ":" + cliId;
                    requestMapMutex.lock();
                    auto it = requestToClientSocket.find(requestKey);
                    if (it != requestToClientSocket.end()) {
                        send(it->second, receivedMessage.c_str(), receivedMessage.size(), 0);
                        requestToClientSocket.erase(it);
                    }
                    requestMapMutex.unlock();
                }
                else if (receivedMessage.substr(0, 10) == "OBJ_FOUND:") {
                    std::string data = receivedMessage.substr(10);
                    std::istringstream ss(data);
                    std::string reqId, objId, cliId;
                    std::getline(ss, reqId, ':');
                    std::getline(ss, objId, ':');
                    std::getline(ss, cliId, ':');
                    
                    std::string requestKey = reqId + ":RETRIEVE:" + objId + ":" + cliId;
                    requestMapMutex.lock();
                    auto it = requestToClientSocket.find(requestKey);
                    if (it != requestToClientSocket.end()) {
                        send(it->second, "OBJ_FOUND", 9, 0);
                        requestToClientSocket.erase(it);
                    }
                    requestMapMutex.unlock();
                }
                else if (receivedMessage.substr(0, 10) == "NOT_FOUND:") {
                    std::string data = receivedMessage.substr(10);
                    std::istringstream ss(data);
                    std::string reqId, objId, cliId;
                    std::getline(ss, reqId, ':');
                    std::getline(ss, objId, ':');
                    std::getline(ss, cliId, ':');
                    
                    std::string requestKey = reqId + ":RETRIEVE:" + objId + ":" + cliId;
                    requestMapMutex.lock();
                    auto it = requestToClientSocket.find(requestKey);
                    if (it != requestToClientSocket.end()) {
                        send(it->second, "-1", 2, 0);
                        requestToClientSocket.erase(it);
                    }
                    requestMapMutex.unlock();
                }
                else if (receivedMessage.substr(0, 8) == "FORWARD:") {
                    // Forward message to next peer in ring
                    std::string data = receivedMessage.substr(8);
                    std::istringstream ss(data);
                    std::string nextPeerStr;
                    std::getline(ss, nextPeerStr, ':');
                    int nextPeer = std::stoi(nextPeerStr);
                    
                    std::string forwardData = data.substr(nextPeerStr.length() + 1);
                    
                    clientIdToSocketMutex.lock();
                    auto it = clientIdToSocket.find(nextPeer);
                    if (it != clientIdToSocket.end()) {
                        send(it->second, forwardData.c_str(), forwardData.size(), 0);
                    }
                    clientIdToSocketMutex.unlock();
                }
            }
        }
        
        usleep(200000);
    }
}

void Reciever::loadObjectsFile() {
    if (objectsFile.empty()) return;
    
    std::ifstream file(objectsFile);
    if (!file.is_open()) {
        std::cout << "Could not open objects file: " << objectsFile << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find("::");
        if (pos != std::string::npos) {
            int clientId = std::stoi(line.substr(0, pos));
            int objectId = std::stoi(line.substr(pos + 2));
            storedObjects.insert({clientId, objectId});
        }
    }
    file.close();
}

void Reciever::saveObjectsFile() {
    if (objectsFile.empty()) return;
    
    std::ofstream file(objectsFile);
    if (!file.is_open()) {
        std::cout << "Could not open objects file for writing: " << objectsFile << std::endl;
        return;
    }
    
    for (const auto& obj : storedObjects) {
        file << obj.first << "::" << obj.second << std::endl;
    }
    file.close();
}

void Reciever::printObjectsFile() {
    std::cout << "Objects stored at peer " << myPeerId << ":" << std::endl;
    for (const auto& obj : storedObjects) {
        std::cout << obj.first << "::" << obj.second << std::endl;
    }
}

bool Reciever::isResponsibleForObject(int objectId) {
    // Check if this peer is responsible for the object
    // Object between predecessor and this peer is stored on this peer
    if (predecessor < myPeerId) {
        return objectId > predecessor && objectId <= myPeerId;
    } else {
        // Wrap-around case
        return objectId > predecessor || objectId <= myPeerId;
    }
}

void Reciever::peerListener() {
    std::string peerIdStr = Utils::removeNFromContainerID(containerID);
    if (peerIdStr.empty()) {
        std::cout << "Error: Could not extract peer ID from container ID" << std::endl;
        return;
    }
    myPeerId = std::stoi(peerIdStr);
    
    predecessor = myPeerId;
    successor = myPeerId;
    
    loadObjectsFile();
    
    std::cout << "Peer " << myPeerId << " listener started" << std::endl;
    
    while (true) {
        char buffer[1024];
        ssize_t n = recv(socket, buffer, sizeof(buffer) - 1, 0);
        
        if (n > 0) {
            buffer[n] = '\0';
            std::string receivedMessage = buffer;
            
            if (receivedMessage.size() > 7 && receivedMessage.substr(0, 7) == "UPDATE:") {
                std::string updateData = receivedMessage.substr(7);
                
                size_t commaPos = updateData.find(',');
                if (commaPos != std::string::npos) {
                    std::string predStr = updateData.substr(0, commaPos);
                    std::string succStr = updateData.substr(commaPos + 1);
                    
                    int newPred = std::stoi(predStr);
                    int newSucc = std::stoi(succStr);
                    
                    if (newPred != -1) {
                        predecessor = newPred;
                    }
                    
                    if (newSucc != -1) {
                        successor = newSucc;
                    }
                    
                    std::cerr << "{peer_id:" << myPeerId 
                              << ", predecessor:" << predecessor 
                              << ", successor:" << successor << "}" << std::endl;
                }
            }
            else if (receivedMessage.substr(0, 5) == "RING:") {
                // Format: RING:reqID:opType:objectID:clientID:responsiblePeer:startPeer
                std::string data = receivedMessage.substr(5);
                std::istringstream ss(data);
                std::string reqId, opType, objectIdStr, clientIdStr, responsiblePeerStr, startPeerStr;
                
                std::getline(ss, reqId, ':');
                std::getline(ss, opType, ':');
                std::getline(ss, objectIdStr, ':');
                std::getline(ss, clientIdStr, ':');
                std::getline(ss, responsiblePeerStr, ':');
                std::getline(ss, startPeerStr, ':');
                
                int objectId = std::stoi(objectIdStr);
                int clientId = std::stoi(clientIdStr);
                int responsiblePeer = std::stoi(responsiblePeerStr);
                int startPeer = std::stoi(startPeerStr);
                
                if (opType == "STORE") {
                    if (myPeerId == responsiblePeer) {
                        // This peer is responsible for storing
                        objectsMutex.lock();
                        storedObjects.insert({clientId, objectId});
                        objectsMutex.unlock();
                        
                        saveObjectsFile();
                        printObjectsFile();
                        
                        std::string response = "OBJ_STORED:" + reqId + ":" + objectIdStr + ":" + 
                                             clientIdStr + ":" + std::to_string(myPeerId);
                        send(socket, response.c_str(), response.size(), 0);
                    } else {
                        // Forward to successor
                        std::string forwardMsg = "FORWARD:" + std::to_string(successor) + ":" + receivedMessage;
                        send(socket, forwardMsg.c_str(), forwardMsg.size(), 0);
                    }
                }
                else if (opType == "RETRIEVE") {
                    // First check if this peer has the object
                    objectsMutex.lock();
                    bool found = storedObjects.find({clientId, objectId}) != storedObjects.end();
                    objectsMutex.unlock();
                    
                    if (found) {
                        // Found the object! Send success response
                        std::string response = "OBJ_FOUND:" + reqId + ":" + objectIdStr + ":" + clientIdStr;
                        send(socket, response.c_str(), response.size(), 0);
                    }
                    else if (myPeerId == startPeer && responsiblePeer == -1) {
                        // We've completed a full circle without finding the object
                        std::string response = "NOT_FOUND:" + reqId + ":" + objectIdStr + ":" + clientIdStr;
                        send(socket, response.c_str(), response.size(), 0);
                    }
                    else {
                        // Continue searching through the ring
                        std::string newResponsiblePeer = (myPeerId == responsiblePeer) ? "-1" : responsiblePeerStr;
                        std::string forwardMsg = "FORWARD:" + std::to_string(successor) + ":RING:" + 
                                                reqId + ":" + opType + ":" + objectIdStr + ":" + 
                                                clientIdStr + ":" + newResponsiblePeer + ":" + startPeerStr;
                        send(socket, forwardMsg.c_str(), forwardMsg.size(), 0);
                    }
                }
            }
        } else if (n == 0) {
            std::cout << "Connection to bootstrap closed" << std::endl;
            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "Receive error: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        usleep(100000);
    }
}

void Reciever::start() {
    if (containerID == "bootstrap") {
        std::cout << "Bootstrap receiver started" << std::endl;
        bootstrapListener();
    } else {
        peerListener();
    }
}