#include "include.h"
#include "utils.h"
#include "process.h"

int Process::post_send(void *context){
	
	auto *conn = static_cast<Connection*>(context);

	assert(conn != nullptr);
	assert(conn->identifier != nullptr);

	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	assert(&message != nullptr);
	calc_message_numerical(&message);
	assert(&message != nullptr);
	assert(conn->send_region != nullptr);
	memcpy(conn->send_region, message.x, message.size*sizeof(char));

	set_send_work_request_attributes(&wr, &sge, conn);
	set_send_scatter_gather_attributes(&sge, conn);
	ibv_post_send(conn->queue_pair, &wr, &bad_wr);
	
	return 0;
}


int Process::send_message(Connection *conn){
	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	assert(conn != nullptr);
	assert(conn->identifier != nullptr);


	set_send_work_request_attributes(&wr, &sge, conn, 0);
	set_send_scatter_gather_attributes(&sge, conn, 0);

	while (!conn->connected);
	ibv_post_send(conn->queue_pair, &wr, &bad_wr);
	return 0;
}

int Process::send_memory_region(Connection *conn){
	conn->send_message->type = MSG_MR;

	assert(conn != nullptr);
	assert(conn->identifier != nullptr);
	assert(conn->recv_message != nullptr);
	assert(conn->rdma_remote_memory_region != nullptr);
	assert(&conn->send_message->data.mr !=nullptr);
	
	memcpy(&conn->send_message->data.mr, conn->rdma_remote_memory_region, sizeof(struct ibv_mr));
	send_message(conn);
	return 0;
}
