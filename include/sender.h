#ifndef SENDER_H
#define SENDER_H

#include <string>

class Sender {
    public:
        Sender(const std::string &containerID, const int &socket, const std::string &bootstrapServerIp);

        void createTcpConnectionWithBootstrap();

        void start();

    private:
        const std::string containerID;
        const int socket;
        const std::string bootstrapServerIp;
};

#endif