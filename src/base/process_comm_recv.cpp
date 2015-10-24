#include "include.h"
#include "utils.h"
#include "process.h"

void Process::post_recvs(){
	struct ibv_recv_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	wr.wr_id = (uintptr_t)connection_;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	sge.addr = (uintptr_t)connection_->recv_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = connection_->recv_memory_region->lkey;

	TEST_NZ(ibv_post_recv(connection_->queue_pair, &wr, &bad_wr));
}
