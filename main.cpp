#include <iostream>
#include <unistd.h>
#include <thread>          
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "utils.h"
#include "reciever.h"

int main() {
    const std::string containerID = Utils::getContainerID();

    // create socket for TCP connections
    const int port = 12345;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (containerID == "bootstrap") {
        std::cout << "IM BOOTSTRAP HELLO!!" << std::endl;
        Utils::setUpServersTcp(sock);
        Reciever reciever(containerID, sock);
        std::thread t1([&]{ reciever.start(); });
        t1.join();
    }
    else {
        sleep(1);
        std::cout << "IM PEER" << std::endl;
        const std::string &bootstrapServerIp = Utils::getIpOfBoostrapServer();
        std::cout << bootstrapServerIp << "\n";
        Utils::clientConnectionToServer(sock, bootstrapServerIp);
    }
}