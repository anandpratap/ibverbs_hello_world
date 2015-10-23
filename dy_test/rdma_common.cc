#include "rdma_common.h"

namespace dy_test {

RDMACommon::RDMACommon() {

}

RDAMConnection* RDMACommon::CreateConnection(struct rdma_cm_id* id) {
    RDMAConnection* conn = new RDMAConnection;

    // set up context
    conn->context = id->verbs;
    conn->protection_domain = ibv_alloc_pd(conn->context);
    assert(conn->protection_domain != NULL);
    conn->completion_channel = ibv_create_comp_channel(conn->context);
    assert(conn->completion_channel != NULL);
    conn->completion_queue = ibv_create_cq(conn->context, 10,
            NULL, conn->completion_channel, 0);
    assert(conn->completion_queue != NULL);
    assert(ibv_req_notify_cq(conn->context, 0) == 0);
    conn->completion_queue_poller_thread = new pthread_t;
    assert(pthread_create(&conn->completion_queue_poller_thread, NULL,
            &RDMACommon::PollCompletionQueue, NULL) == 0);

    // set queue pair initialization attributes
    struct ibv_qp_init_attr qp_attr;
    qp_attr.send_cq = context->completion_queue;
    qp_attr.recv_cq = context->completion_queue;
    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;

    // create queue pair
    assert(rdma_create_qp(id, context->protection_domain, &qp_attr) == 0);

    conn->id = id;
    conn->queue_pair = id->qp;
    conn->connected = false;


}

}
