#include "include.h"
#include "utils.h"
#include "process.h"

int Process::post_send(){
	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	calc_message_numerical(&message);
	
	memcpy(connection_->send_region, message.x, message.size*sizeof(char));
	printf("connected. posting send...\n");
	
	memsetzero(&wr);

	wr.wr_id = (uintptr_t)connection_->identifier;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = (uintptr_t)connection_->send_region;
	sge.length = message.size;
	sge.lkey = connection_->send_memory_region->lkey;
	
	TEST_NZ(ibv_post_send(connection_->queue_pair, &wr, &bad_wr));
	
	return 0;
}


int Process::send_message(){
	struct ibv_send_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	
	memset(&wr, 0, sizeof(wr));
	
	wr.wr_id = (uintptr_t)connection_->identifier;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = (uintptr_t)connection_->send_message;
	sge.length = sizeof(struct message_sync);
	sge.lkey = connection_->send_message_memory_region->lkey;
	while (!connection_->connected);
	ibv_post_send(connection_->queue_pair, &wr, &bad_wr);
	printf("SEND_MESSAGE\n");
	return 0;
}

int Process::send_memory_region(){
	connection_->send_message->type = MSG_MR;
	memcpy(&connection_->send_message->data.mr, connection_->rdma_remote_memory_region, sizeof(struct ibv_mr));
	send_message();
	return 0;
}
