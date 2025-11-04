#include <iostream>
#include <unistd.h>
#include <thread>          
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <mutex>

#include "utils.h"
#include "reciever.h"
#include "sender.h"
#include "peer.h"
#include "client.h"

int main(int argc, char *argv[]) {
    const std::string containerID = Utils::getContainerID();

    // create socket for TCP connections
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (containerID == "bootstrap") {
        Utils::setUpServersTcp(sock);

        std::unordered_map<int, int> clientIdToSocket;
        std::mutex clientIdToSocketMutex;

        std::vector<Peer> ring;
        std::mutex ringMutex;

        Reciever reciever(containerID, sock, clientIdToSocket, clientIdToSocketMutex, ring, ringMutex, "");
        std::thread recieverThread([&]{ reciever.start(); });
        recieverThread.join();
    }
    else if (containerID == "client") {
        // Parse command line arguments for client
        std::string bootstrapServer = "";
        std::string testcase = "";
        int delay = 0;
        
        for (int i = 1; i < argc; i++) {
            if (std::strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
                bootstrapServer = argv[++i];
            }
            else if (std::strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                testcase = argv[++i];
            }
            else if (std::strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
                delay = std::atoi(argv[++i]);
            }
        }
        
        if (delay > 0) {
            std::cerr << "Client sleeping for " << delay << " seconds..." << std::endl;
            sleep(delay);
        }
        
        Client client(sock, bootstrapServer, testcase);
        client.start();
    }
    else {
        // Parse command line arguments and apply delay
        Peer peer;
        std::string objectsFile = "";
        
        for (int i = 1; i < argc; i++) {
            if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                objectsFile = argv[++i];
            }
        }
        
        Utils::createPeer(peer, argc, argv);
        
        const std::string &bootstrapServerIp = Utils::getIpOfBoostrapServer();
        Utils::clientConnectionToServer(sock, bootstrapServerIp);

        // Send peerID to bootstrap
        Sender sender(containerID, sock, bootstrapServerIp);
        std::thread senderThread([&]{ sender.start(); });
        
        // Create dummy maps for peer receiver
        std::unordered_map<int, int> clientIdToSocket;
        std::mutex clientIdToSocketMutex;
        std::vector<Peer> ring;
        std::mutex ringMutex;
        
        // Start receiver to listen for messages from bootstrap
        Reciever reciever(containerID, sock, clientIdToSocket, clientIdToSocketMutex, ring, ringMutex, objectsFile);
        std::thread recieverThread([&]{ reciever.start(); });
        
        // Wait for threads
        senderThread.join();
        recieverThread.join();
    }
    
    close(sock);
    return 0;
}