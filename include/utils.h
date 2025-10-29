#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <unistd.h>


class Utils {
    public:
        static std::string getContainerID();

        static std::string getIpOfBoostrapServer();

        static bool clientConnectionToServer(const int &sock, const std::string &serverIp);

        static void setUpServersTcp(const int &sock);

};

#endif