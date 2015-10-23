#include "rdma_server.h"

namespace dy_test {

RDMAServer::RDMAServer() {
    this->cm_event_ = NULL;
    this->listener_ = NULL;
    this->ec_ = NULL;
    this->port_ = 0;
}

// Initialize, bind and listen.
int RDMAServer::Initialize() {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET6;

    this->ec_ = rdma_create_event_channel();
    assert(this->ec_ != NULL);

    assert(rdma_create_id(ec_, &listener_, NULL, RDMA_AS_TCP) == 0);
    assert(rdma_bind_addr(listener_, (struct sockaddr *)addr_) == 0);
    assert(rdma_listen(listener, 10) == 0);

    port = ntohs(rdma_get_src_port(listener));

    return 0;
}

int RDMAServer::Run() {

}

}

