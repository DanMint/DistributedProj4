#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>

#include "sender.h"
#include "utils.h"

Sender::Sender(const std::string &containerID, const int &socket, const std::string &bootstrapServerIp) : 
containerID(containerID), socket(socket), bootstrapServerIp(bootstrapServerIp)
{}

void Sender::peerSender() {
    std::string message = "peerID:" + Utils::removeNFromContainerID(containerID);
    send(socket, message.c_str(), message.size(), 0);
    sleep(5);
}

void Sender::start() {
    Sender::peerSender();
}