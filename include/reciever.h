#ifndef RECIEVER_H
#define RECIEVER_H

#include "string"

class Reciever {
    public:
        Reciever(const std::string &containerID, const int &socket);

        void bootstrapListener();

        void peerListner();

        void start();

    private:
        const std::string containerID;
        const int socket;

};

#endif