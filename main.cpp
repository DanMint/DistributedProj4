#include <iostream>
#include <unistd.h>
#include <thread>          
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unordered_map>

#include "utils.h"
#include "reciever.h"
#include "sender.h"
#include "peer.h"

int main(int argc, char *argv[]) {
    const std::string containerID = Utils::getContainerID();

    // create socket for TCP connections
    const int port = 12345;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (containerID == "bootstrap") {
        std::cout << "IM BOOTSTRAP HELLO!!" << std::endl;
        Utils::setUpServersTcp(sock);

        std::unordered_map<int, int> clientIdToSocket;
        std::mutex clientIdToSocketMutex;

        std::vector<Peer> ring;
        std::mutex ringMutex;

        Reciever reciever(containerID, sock, clientIdToSocket, clientIdToSocketMutex, ring, ringMutex);
        std::thread recieverThread([&]{ reciever.start(); });
        recieverThread.join();
    }
    else {
        Peer peer;
        Utils::createPeer(peer, argc, argv);
        std::cout << "IM PEER" << std::endl;
        const std::string &bootstrapServerIp = Utils::getIpOfBoostrapServer();
        Utils::clientConnectionToServer(sock, bootstrapServerIp);

        Sender sender(containerID, sock, bootstrapServerIp);
        std::thread senderThread([&]{ sender.start(); });
        senderThread.join();

        while(true);
    }
}