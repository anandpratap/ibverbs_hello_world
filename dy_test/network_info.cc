#include "network_info.h"

using namespace dy_test;

NetworkInfo::NetworkInfo() {
    this->ethernet_addr_ = "";
    this->infiniband_addr_ = "";
}

int NetworkInfo::RetrieveNetworkInfo() {
    char host[NI_MAXHOST];
    int family, rc;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
         perror("getifaddrs");
         return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa=ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            rc = getnameinfo(
                    ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST
                    );
            if (rc != 0) {
                 perror("getnameinfo");
                 return -1;
            }
            if (strstr(ifa->ifa_name, "eth") != NULL) {
                this->ethernet_addr_ = std::string(host);
            } else if (strstr(ifa->ifa_name,"ib") != NULL &&
                    strstr(ifa->ifa_name, "eoib") == NULL) {
                this->infiniband_addr_ = std::string(host);
            }
        }
    }
    return 0;
}

std::string NetworkInfo::ethernet_addr() {
     return this->ethernet_addr_;
}

std::string NetworkInfo::infiniband_addr() {
    return this->infiniband_addr_;
}
