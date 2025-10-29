#include "sender.h"

Sender::Sender(const std::string &containerID, const int &socket, const std::string &bootstrapServerIp) : 
containerID(containerID), socket(socket), bootstrapServerIp(bootstrapServerIp)
{}



void Sender::createTcpConnectionWithBootstrap() {

}