#include "include.h"
#include "utils.h"
#include "process.h"

int Process::post_send(void *context){
	
	Connection *conn = static_cast<Connection*>(context);

	assert(conn != nullptr);
	assert(conn->identifier != nullptr);

	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	assert(&message != nullptr);
	calc_message_numerical(&message);
	assert(&message != nullptr);
	assert(conn->send_region != nullptr);
	memcpy(conn->send_region, message.x, message.size*sizeof(char));
	
	memsetzero(&wr);

	wr.wr_id = reinterpret_cast<uintptr_t>(conn);
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = reinterpret_cast<uintptr_t>(conn->send_region);
	sge.length = message.size;
	sge.lkey = conn->send_memory_region->lkey;
	
	TEST_NZ(ibv_post_send(conn->queue_pair, &wr, &bad_wr));
	
	return 0;
}


int Process::send_message(Connection *conn){
	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	assert(conn != nullptr);
	assert(conn->identifier != nullptr);

	memset(&wr, 0, sizeof(wr));
	
	wr.wr_id = reinterpret_cast<uintptr_t>(conn);
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
    
	sge.addr = reinterpret_cast<uintptr_t>(conn->send_message);
	sge.length = sizeof(struct message_sync);
	sge.lkey = conn->send_message_memory_region->lkey;
	while (!conn->connected);
	ibv_post_send(conn->queue_pair, &wr, &bad_wr);
	return 0;
}

int Process::send_memory_region(Connection *conn){
	conn->send_message->type = MSG_MR;
#ifdef DEBUG
	assert(conn != nullptr);
	assert(conn->identifier != nullptr);
	assert(conn->recv_message != nullptr);
	assert(conn->rdma_remote_memory_region != nullptr);
	assert(&conn->send_message->data.mr !=nullptr);
#endif
	memcpy(&conn->send_message->data.mr, conn->rdma_remote_memory_region, sizeof(struct ibv_mr));
	send_message(conn);
	return 0;
}
