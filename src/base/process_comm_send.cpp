#include "include.h"
#include "utils.h"
#include "process.h"

int Process::post_send(){
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;
	calc_message_numerical(&message);
	
	memcpy(connection_->send_region, &message, BUFFER_SIZE*sizeof(char));
	printf("connected. posting send...\n");
	
	memsetzero(&wr);

	wr.wr_id = (uintptr_t)connection_;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = (uintptr_t)connection_->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = connection_->send_memory_region->lkey;
	
	TEST_NZ(ibv_post_send(connection_->queue_pair, &wr, &bad_wr));
	
	return 0;
}
