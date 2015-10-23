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
			&RDMACommon::PollCompletionQueue, (void*) conn) == 0);

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

static void* RDMACommon::PollCompletionQueue(void* connection) {
	RDMAConnection* conn = (RDMAConnection *) connection;
	struct ibv_wc work_completion;

	while (true) {
		assert(ibv_get_cq_event(conn->completion_channel, &conn->completion_queue, &conn->context) == 0);
		ibv_ack_cq_events(conn->completion_queue, 1);
		assert(ibv_req_notify_cq(conn->completion_queue, 0) == 0);

		while (ibv_poll_cq(conn->completion_queue, 1, &work_completion)) {
			OnCompletion(&work_completion);
		}
	}

}

static void OnCompletion(struct ibv_wc* work_completion) {
	if (work_completion->status != IBV_WC_SUCCESS) {
		perror("work not successful.");
		exit(-1);
	}

	switch (work_completion->opcode) {
		case IBV_WC_SEND:
			OnSendCompletion(work_completion);
		case IBV_WC_RDMA_WRITE:
		case IBV_WC_RDMA_READ:
		case IBV_WC_COMP_SWAP:
		case IBV_WC_FETCH_ADD:
		case IBV_WC_BIND_MW:
		case IBV_WC_RECV:
		case IBV_WC_RECV_RDMA_WITH_IMM:
		default:
			break;
	}
}

}
