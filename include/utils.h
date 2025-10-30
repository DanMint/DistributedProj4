#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <unistd.h>

#include "peer.h"


class Utils {
    public:
        static std::string getContainerID();

        static std::string getIpOfBoostrapServer();

        static bool clientConnectionToServer(const int &sock, const std::string &serverIp);

        static void setUpServersTcp(const int &sock);

        static std::string removeNFromContainerID(std::string copyOfContainerID);

        // populates peer with parsed arguments and sets up the peer
        static void createPeer(Peer &peer, int argc, char* argv[]);

};

#endif