#include "include.h"
#include "utils.h"
#include "process.h"

void Process::post_recv(){
	struct ibv_recv_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	wr.wr_id = (uintptr_t)connection_->identifier;
	wr.next = nullptr;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	if(mode_of_operation == MODE_SEND_RECEIVE){
		sge.addr = (uintptr_t)connection_->recv_region;
		sge.length = message.size;
		sge.lkey = connection_->recv_memory_region->lkey;
	}
	else{
		sge.addr = (uintptr_t)connection_->recv_message;
		sge.length = sizeof(struct message_sync);
		sge.lkey = connection_->recv_message_memory_region->lkey;
	}
	int st = ibv_post_recv(connection_->queue_pair, &wr, &bad_wr);

	if(st != 0)
		printf ("ERROR: Unable to post receiver buffer.\n");
}
