#ifndef DY_RDMASERVER_H
#define DY_RDMASERVER_H

#include <rdma/rdma_cma.h>
#include <netdb.h>

namespace dy_test {

class RDMAServer {
    public:
        RDMAServer();
        int Initialize();
        int Run();
    private:
        uint16_t port_;
        struct rdma_cm_event* cm_event_;
        struct rdma_cm_id* listener_;
        struct rdma_event_channel* ec_;
        struct sockaddr_in6 addr_;
};

}

#endif // DY_RDMASERVER_H
