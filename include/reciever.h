#ifndef RECIEVER_H
#define RECIEVER_H

#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

#include "peer.h"


class Reciever {
    public:
        Reciever(const std::string &containerID, const int &socket, std::unordered_map<int, int> &clientIdToSocket, std::mutex &clientIdToSocketMutex, std::vector<Peer> &ring, std::mutex &ringMutex);

        void bootstrapListener();

        void peerListner();

        void start();

    private:
        const std::string containerID;
        const int socket;

        std::unordered_map<int, int> &clientIdToSocket;
        std::mutex &clientIdToSocketMutex;

        std::vector<Peer> &ring;
        std::mutex &ringMutex;


        // for server use only 
        std::vector<int> clientSockets;
};

#endif