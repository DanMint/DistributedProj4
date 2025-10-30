#ifndef RECIEVER_H
#define RECIEVER_H

#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <mutex>

#include "peer.h"

class Reciever {
public:
    // Constructor
    Reciever(const std::string &containerID, 
             const int &socket, 
             std::unordered_map<int, int> &clientIdToSocket, 
             std::mutex &clientIdToSocketMutex, 
             std::vector<Peer> &ring, 
             std::mutex &ringMutex);

    // Main entry point - decides whether to run as bootstrap or peer
    void start();

private:
    // Member variables
    const std::string containerID;                    // Container ID (e.g., "bootstrap", "n1", "n5")
    const int socket;                                 // Main socket for communication
    std::unordered_map<int, int> &clientIdToSocket;  // Maps peer ID to socket fd
    std::mutex &clientIdToSocketMutex;               // Mutex for clientIdToSocket map
    std::vector<Peer> &ring;                         // Ring structure (mainly used by bootstrap)
    std::mutex &ringMutex;                           // Mutex for ring vector
    std::vector<int> clientSockets;                  // List of all connected client sockets

    // Bootstrap server methods
    void bootstrapListener();                        // Main loop for bootstrap server
    void insertPeerIntoRing(int newPeerId);         // Add a new peer to the ring and update neighbors
    void sendUpdateToPeer(int peerId, int pred, int succ); // Send UPDATE message to a specific peer

    // Peer methods  
    void peerListener();                             // Main loop for peer nodes
};

#endif