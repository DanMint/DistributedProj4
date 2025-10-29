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
        
        std::cout << "HELLO" << std::endl;
        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        return std::string(inet_ntoa(addr));
    }
}

bool Utils::clientConnectionToServer(const int &sock, const std::string &bootstrapServerIp) {
    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (true) {
        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(8080);
        if (inet_pton(AF_INET, bootstrapServerIp.c_str(), &server.sin_addr) <= 0) {
            std::cout << "Invalid server address, server not up yet.. \n";
            continue;
        }

        if (connect(sock, (sockaddr*)&server, sizeof(server)) == 0) 
            break;
        
        std::cerr << "Connection failed, retrying...\n";
        sleep(1);
    }

    std::cout << "Client TCP started \n";
    return true;
}

void Utils::setUpServersTcp(const int &sock) {
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));
    listen(sock, 1);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    std::cout << "Server listening on port 8080...\n";
}