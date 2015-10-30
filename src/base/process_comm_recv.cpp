#include "include.h"
#include "utils.h"
#include "process.h"

#define DEBUG

void Process::post_recv(Connection *conn){
	assert(conn != nullptr);
	assert(conn->identifier != nullptr);

	struct ibv_recv_wr wr, *bad_wr = nullptr;
	struct ibv_sge sge;

	set_recv_work_request_attributes(&wr, &sge, conn);
	set_recv_scatter_gather_attributes(&sge, conn);

	int st = ibv_post_recv(conn->queue_pair, &wr, &bad_wr);
	if(st != 0)
		printf("ERROR:UNABLE TO POST RECEIVE %i\n", st);
}
