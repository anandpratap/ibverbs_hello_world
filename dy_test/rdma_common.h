#ifndef DY_RDMACOMMON_H
#define DY_RDMACOMMON_H

#include <stdlib.h>
#include <rdma/rdma_cma.h>
#include <netdb.h>
#include <pthread.h>

namespace dy_test {

struct RDMAConnection {
    struct rdma_cm_id* id;
    struct ibv_qp* queur_pair;
    struct ibv_mr* memory_region;
    struct ibv_mr* remote_memory_region;
    struct ibv_context* context;
    struct ibv_pd* protection_domain;
    struct ibv_cq* completion_queue;
    struct ibv_comp_channel* completion_channel;

    pthread_t completion_queue_poller_thread;
    bool connected;
};

class RDMACommon {
    public:
        RDMACommon();
        RDMAConnection* CreateConnection(struct rdma_cm_id* id);
        static void* PollCompletionQueue();
    private:
};

}

#endif // DY_RDMACOMMON_H
