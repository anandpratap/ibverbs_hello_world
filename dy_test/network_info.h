#ifndef DY_NETWORKINFO_H
#define DY_NETWORKINFO_H

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_link.h>
#include <linux/netdevice.h>
#include <ifaddrs.h>
#include <netdb.h>

namespace dy_test {

class NetworkInfo {
    public:
        NetworkInfo();
        int RetrieveNetworkInfo();
        std::string ethernet_addr();
        std::string infiniband_addr();

    private:
        std::string ethernet_addr_;
        std::string infiniband_addr_;
};

}

#endif // DY_NETWORK_INFO_H
