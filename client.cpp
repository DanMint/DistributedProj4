#include "client.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// DHTClient implementation

std::string DHTClient::getBootstrapIP() {
    while (true) {
        struct hostent* host_entry = gethostbyname(bootstrapServer.c_str());
        
        if (host_entry == nullptr) {
            std::cout << "Waiting for bootstrap server...\n";
            sleep(1);
            continue;
        }
        
        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        return std::string(inet_ntoa(addr));
    }
}

bool DHTClient::connectToBootstrap(const std::string& bootstrapIP) {
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8081);  // Client port for bootstrap
    
    if (inet_pton(AF_INET, bootstrapIP.c_str(), &server.sin_addr) <= 0) {
        std::cerr << "Invalid bootstrap address\n";
        return false;
    }
    
    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection to bootstrap failed\n";
        return false;
    }
    
    return true;
}

std::string DHTClient::sendRequest(const std::string& request) {
    // Send request
    send(sock, request.c_str(), request.size(), 0);
    
    // Receive response
    char buffer[1024] = {0};
    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (n > 0) {
        buffer[n] = '\0';
        return std::string(buffer);
    }
    
    return "";
}

DHTClient::DHTClient(const std::string& bootstrap, int testCase) 
    : bootstrapServer(bootstrap), testCase(testCase) {
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
}

DHTClient::~DHTClient() {
    close(sock);
}

void DHTClient::runTestCase() {
    std::string bootstrapIP = getBootstrapIP();
    
    if (!connectToBootstrap(bootstrapIP)) {
        return;
    }
    
    std::cout << "Connected to bootstrap server\n";
    
    switch(testCase) {
        case 3: {
            // TESTCASE 3: Store object
            std::string request = "REQUEST:1,STORE,25,3";  // reqID:1, STORE, objectID:25, clientID:3
            std::string response = sendRequest(request);
            
            if (response.find("STORED") != std::string::npos) {
                std::cout << "STORED: 25" << std::endl;
            }
            break;
        }
        
        case 4: {
            // TESTCASE 4: Retrieve existing object
            // First store it
            std::string storeReq = "REQUEST:1,STORE,25,3";
            sendRequest(storeReq);
            sleep(1);  // Give time for storage
            
            // Now retrieve it
            std::string retrieveReq = "REQUEST:2,RETRIEVE,25,3";
            std::string response = sendRequest(retrieveReq);
            
            if (response.find("RETRIEVED") != std::string::npos) {
                std::cout << "RETRIEVED: 25" << std::endl;
            }
            break;
        }
        
        case 5: {
            // TESTCASE 5: Retrieve non-existing object
            std::string request = "REQUEST:1,RETRIEVE,99,3";  // Object that doesn't exist
            std::string response = sendRequest(request);
            
            if (response == "-1" || response.find("NOT_FOUND") != std::string::npos) {
                std::cout << "NOT FOUND: 99" << std::endl;
            }
            break;
        }
        
        default:
            std::cerr << "Invalid test case: " << testCase << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string bootstrapServer = "";
    int testCase = 3;
    int delay = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            bootstrapServer = argv[++i];
        } else if (std::strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            testCase = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay = std::atoi(argv[++i]);
        }
    }
    
    if (bootstrapServer.empty()) {
        std::cerr << "Bootstrap server not specified. Use -b <server>\n";
        return 1;
    }
    
    if (delay > 0) {
        std::cout << "Sleeping for " << delay << " seconds...\n";
        sleep(delay);
    }
    
    DHTClient client(bootstrapServer, testCase);
    client.runTestCase();
    
    return 0;
}