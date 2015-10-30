#include "include.h"
#include "utils.h"
#include "process.h"

#define DEBUG

void Process::post_recv(Connection *conn){
	assert(conn != nullptr);
	assert(conn->identifier != nullptr);

	struct ibv_recv_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;
	wr.wr_id = reinterpret_cast<uintptr_t>(conn);
	wr.next = nullptr;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	if(mode_of_operation == MODE_SEND_RECEIVE){
		sge.addr = reinterpret_cast<uintptr_t>(conn->recv_region);
		sge.length = message.size;
		sge.lkey = conn->recv_memory_region->lkey;
	}
	else{
		assert(conn->recv_message != nullptr);
        sge.addr = reinterpret_cast<intptr_t>(conn->recv_message);
		sge.length = sizeof(struct message_sync);
		sge.lkey = conn->recv_message_memory_region->lkey;
	}
    
	int st = ibv_post_recv(conn->queue_pair, &wr, &bad_wr);

	if(st != 0)
		printf("ERROR:UNABLE TO POST RECEIVE %i\n", st);
}
