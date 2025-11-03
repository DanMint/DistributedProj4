#include "client.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

Client::Client(const int &socket, const std::string &bootstrapServer, const std::string &testcase) 
    : socket(socket), bootstrapServer(bootstrapServer), testcase(testcase), requestId(1) {}

void Client::connectToBootstrap() {
    // Get IP of bootstrap server
    struct hostent* host_entry = gethostbyname(bootstrapServer.c_str());
    if (host_entry == nullptr) {
        std::cerr << "Failed to resolve bootstrap hostname" << std::endl;
        return;
    }
    
    struct in_addr addr;
    memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
    std::string bootstrapIp = inet_ntoa(addr);
    
    // Connect to bootstrap
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, bootstrapIp.c_str(), &server.sin_addr);
    
    if (connect(socket, (sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Failed to connect to bootstrap" << std::endl;
        return;
    }
    
    std::cout << "Client connected to bootstrap" << std::endl;
}

void Client::sendStoreRequest(int objectId, int clientId) {
    // REQUEST:reqID,operationType,objectID,clientID
    std::string request = "REQUEST:" + std::to_string(requestId++) + ",STORE," + 
                         std::to_string(objectId) + "," + std::to_string(clientId);
    
    std::cout << "Sending: " << request << std::endl;
    send(socket, request.c_str(), request.size(), 0);
    
    // Wait for response
    char buffer[1024];
    int n = recv(socket, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        std::string response = buffer;
        std::cout << "Received: " << response << std::endl;
        if (response.find("OBJ_STORED") != std::string::npos) {
            std::cout << "STORED: " << objectId << std::endl;
        }
    }
}

void Client::sendRetrieveRequest(int objectId, int clientId) {
    // REQUEST:reqID,operationType,objectID,clientID
    std::string request = "REQUEST:" + std::to_string(requestId++) + ",RETRIEVE," + 
                         std::to_string(objectId) + "," + std::to_string(clientId);
    
    std::cout << "Sending: " << request << std::endl;
    send(socket, request.c_str(), request.size(), 0);
    
    // Wait for response
    char buffer[1024];
    int n = recv(socket, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        std::string response = buffer;
        std::cout << "Received: " << response << std::endl;
        if (response.find("OBJ_FOUND") != std::string::npos) {
            std::cout << "RETRIEVED: " << objectId << std::endl;
        } else if (response == "-1" || response.find("NOT_FOUND") != std::string::npos) {
            std::cout << "NOT FOUND: " << objectId << std::endl;
        }
    }
}

void Client::runTestCase() {
    if (testcase == "3") {
        // Test case 3: Store an object
        std::cout << "Running Test Case 3: Store object" << std::endl;
        sendStoreRequest(42, 10);
    }
    else if (testcase == "4") {
        // Test case 4: Retrieve an existing object
        std::cout << "Running Test Case 4: Retrieve existing object" << std::endl;
        sendRetrieveRequest(42, 10);
    }
    else if (testcase == "5") {
        // Test case 5: Retrieve a non-existing object
        std::cout << "Running Test Case 5: Retrieve non-existing object" << std::endl;
        sendRetrieveRequest(99, 10);
    }
    else {
        std::cout << "Unknown test case: " << testcase << std::endl;
    }
}

void Client::start() {
    connectToBootstrap();
    runTestCase();
}