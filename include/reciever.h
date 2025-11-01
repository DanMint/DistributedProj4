#ifndef RECIEVER_H
#define RECIEVER_H

#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <mutex>
#include <set>

#include "peer.h"

class Reciever {
public:
    // Constructor
    Reciever(const std::string &containerID, 
             const int &socket, 
             std::unordered_map<int, int> &clientIdToSocket, 
             std::mutex &clientIdToSocketMutex, 
             std::vector<Peer> &ring, 
             std::mutex &ringMutex,
             const std::string &objectsFile);

    // Main entry point
    void start();

private:
    // Member variables
    const std::string containerID;
    const int socket;
    std::unordered_map<int, int> &clientIdToSocket;
    std::mutex &clientIdToSocketMutex;
    std::vector<Peer> &ring;
    std::mutex &ringMutex;
    std::vector<int> clientSockets;
    const std::string objectsFile;
    std::set<std::pair<int, int>> storedObjects;
    std::mutex objectsMutex;
    int myPeerId;
    int predecessor;
    int successor;
    std::unordered_map<std::string, int> requestToClientSocket; // Maps request to original client socket
    std::mutex requestMapMutex;

    // Bootstrap server methods
    void bootstrapListener();
    void insertPeerIntoRing(int newPeerId);
    void sendUpdateToPeer(int peerId, int pred, int succ);
    void handleClientRequest(const std::string &request, int clientSocket);
    int findResponsiblePeer(int objectId);
    
    // Peer methods  
    void peerListener();
    void loadObjectsFile();
    void saveObjectsFile();
    void printObjectsFile();
    bool isResponsibleForObject(int objectId);
};

#endif