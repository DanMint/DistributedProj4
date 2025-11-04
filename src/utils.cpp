#include "utils.h"

#include <string>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

std::string Utils::getContainerID () {
    char hostname[256];
    gethostname(hostname, 256);
    return hostname;
}

std::string Utils::getIpOfBoostrapServer() {
    while (true) {
        struct hostent* host_entry = gethostbyname("bootstrap");
        
        if (host_entry == nullptr) {
            std::cout << "Failed to resolve bootstrap hostname\n";
            sleep(5);
            continue;
        }
        
        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        return std::string(inet_ntoa(addr));
    }
}

bool Utils::clientConnectionToServer(const int &sock, const std::string &bootstrapServerIp) {
    // REMOVED: fcntl(sock, F_SETFL, O_NONBLOCK);  // Don't set non-blocking here!

    while (true) {
        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(8080);
        if (inet_pton(AF_INET, bootstrapServerIp.c_str(), &server.sin_addr) <= 0) {
            std::cout << "Invalid server address, server not up yet.. \n";
            sleep(1);
            continue;
        }

        // Try to connect with blocking socket
        if (connect(sock, (sockaddr*)&server, sizeof(server)) == 0) {
            // Connection successful! NOW set it to non-blocking
            fcntl(sock, F_SETFL, O_NONBLOCK);
            break;
        }
        
        std::cerr << "Connection failed, retrying...\n";
        sleep(1);
    }

    return true;
}

void Utils::setUpServersTcp(const int &sock) {
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "Bind failed: " << strerror(errno) << std::endl;
        return;
    }
    
    if (listen(sock, 10) < 0) {  // Changed from 1 to 10
        std::cout << "Listen failed: " << strerror(errno) << std::endl;
        return;
    }
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
}

std::string Utils::removeNFromContainerID(std::string copyOfContainerID) {
    if (copyOfContainerID.empty() || copyOfContainerID[0] != 'n')
        return "";
    return copyOfContainerID.substr(1);
}

void Utils::createPeer(Peer &peer, int argc, char* argv[]) {
    for (int argIndex = 1; argIndex < argc; argIndex++) {
        if (std::strcmp(argv[argIndex], "-d") == 0) {
            if (argIndex + 1 < argc) {
                int delay = std::atoi(argv[argIndex + 1]);
                std::cerr << "Sleeping for " << delay << " seconds before starting..." << std::endl;  
                sleep(delay);
                argIndex++; 
            }
        }
    }
}
