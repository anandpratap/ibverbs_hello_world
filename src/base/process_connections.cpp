#include "include.h"
#include "utils.h"
#include "process.h"

void Process::build_context(struct ibv_context *verbs){
	if (s_ctx) {
		if (s_ctx->ctx != verbs)
			die("cannot handle events in more than one context.");
		return;
	}
	
	s_ctx = new context();
	s_ctx->ctx = verbs;
	std::cout<<s_ctx->ctx<<std::endl;
	TEST_Z(s_ctx->protection_domain = ibv_alloc_pd(s_ctx->ctx));
	TEST_Z(s_ctx->completion_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->completion_queue = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->completion_channel, 0)); /* cqe=10 is arbitrary */
	TEST_NZ(ibv_req_notify_cq(s_ctx->completion_queue, 0));
	TEST_NZ(pthread_create(&s_ctx->completion_queue_poller_thread, NULL, &Process::poll_cq_thunk, static_cast<void*>(this)));
}

void Process::build_queue_pair_attributes(){
	memsetzero(&this->queue_pair_attributes);
	
	(&this->queue_pair_attributes)->send_cq = s_ctx->completion_queue;
	(&this->queue_pair_attributes)->recv_cq = s_ctx->completion_queue;
	(&this->queue_pair_attributes)->qp_type = IBV_QPT_RC;

	(&this->queue_pair_attributes)->cap.max_send_wr = 10;
	(&this->queue_pair_attributes)->cap.max_recv_wr = 10;
	(&this->queue_pair_attributes)->cap.max_send_sge = 1;
	(&this->queue_pair_attributes)->cap.max_recv_sge = 1;
}

void Process::register_memory(){
	double dt;
	struct timespec tstart={0,0}, tend={0,0};

	connection_->send_region = new char[BUFFER_SIZE];
	connection_->recv_region = new char[BUFFER_SIZE];
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	TEST_Z(connection_->send_memory_region = ibv_reg_mr(
							    s_ctx->protection_domain, 
							    connection_->send_region, 
							    BUFFER_SIZE, 
							    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
	
	TEST_Z(connection_->recv_memory_region = ibv_reg_mr(
							    s_ctx->protection_domain, 
							    connection_->recv_region, 
							    BUFFER_SIZE, 
							    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
	
	clock_gettime(CLOCK_MONOTONIC, &tend);
	dt = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
		((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
	printf("memory registration took about %.8f micro seconds\n",
	       dt);

	if(client)
		timelogfile.open("client_time.log", std::ios::app);
	else
		timelogfile.open("server_time.log", std::ios::app);
	timelogfile<<"REG:" <<dt<<std::endl;
	timelogfile.close();
}

