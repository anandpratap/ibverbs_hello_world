#include "network_info.h"
#include <assert.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_link.h>
#include <linux/netdevice.h>
#include <ifaddrs.h>
#include <netdb.h>

const int BUF_SIZE = 8192;

using namespace std;
using namespace dy_test;

int main()
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
         perror("getifaddrs");
    }

    char host[NI_MAXHOST];
    int family, n,s;

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa=ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;
        printf("%-8s %s (%d)\n",
                ifa->ifa_name,
                (family == AF_PACKET) ? "AF_PACKET" :
                (family == AF_INET) ? "AF_INET" :
                (family == AF_INET6) ? "AF_INET6" : "???",
                family);

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                    sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
            }

            printf("\t\taddress: <%s>\n", host);


        } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
            struct rtnl_link_stats *stats = (struct rtnl_link_stats *) ifa->ifa_data;

            printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
                    "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
                    stats->tx_packets, stats->rx_packets,
                    stats->tx_bytes, stats->rx_bytes);
        }
    } // end for
    freeifaddrs(ifaddr);

    cout << "test" << endl;

    NetworkInfo net_info;
    net_info.RetrieveNetworkInfo();
    cout << net_info.ethernet_addr() << endl;
    cout << net_info.infiniband_addr() << endl;


    return 0;
}
