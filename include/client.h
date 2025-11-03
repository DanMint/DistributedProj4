#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
    Client(const int &socket, const std::string &bootstrapServer, const std::string &testcase);
    void start();

private:
    const int socket;
    const std::string bootstrapServer;
    const std::string testcase;
    int requestId;
    
    void connectToBootstrap();
    void sendStoreRequest(int objectId, int clientId);
    void sendRetrieveRequest(int objectId, int clientId);
    void runTestCase();
};

#endif