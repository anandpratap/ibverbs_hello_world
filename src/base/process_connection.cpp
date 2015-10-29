#include "include.h"
#include "utils.h"
#include "process.h"

void Process::connect(){
	// create event channel, connection identifier, and post resolve addressess 
	event_channel = rdma_create_event_channel();
	rdma_create_id(event_channel, &connection_identifier, nullptr, RDMA_PS_TCP);
	if(client){
		rdma_resolve_addr(connection_identifier, nullptr, (struct sockaddr*)&address, TIMEOUT_IN_MS);
	}
	else{
		rdma_bind_addr(connection_identifier, (struct sockaddr *)&address);
		rdma_listen(connection_identifier, 10);
	}
	std::cout<<"CONNECTING:PORT "<<DEFAULT_PORT<<std::endl;
}

int Process::build_connection(struct rdma_cm_id *id){
	build_context(id->verbs);
	build_queue_pair_attributes();
	rdma_create_qp(id, s_ctx->protection_domain, &this->queue_pair_attributes);

	Connection *conn;
	id->context = conn = new Connection();
	
	
	conn->identifier = id;
	std::cout<<"LOCATION CONN -> ID"<<conn->identifier<<"\n";
	std::cout<<"LOCATION -> ID"<<id<<"\n";
	conn->queue_pair = id->qp;
	conn->number_of_recvs = 0;
	conn->number_of_sends = 0;


	if(mode_of_operation != MODE_SEND_RECEIVE){
		conn->send_state = SS_INIT;
		conn->recv_state = RS_INIT;
		conn->connected = 0;
		register_memory(conn);
		printf("POSTINZG RECV\n");
		post_recv(conn);
	}
	else{
		std::cout<<"POINTER"<<conn<<std::endl;
		register_memory(conn);
	}
	
	return EXIT_SUCCESS;
}

 void Process::build_context(struct ibv_context *verbs){
	 if (s_ctx) {
		 if (s_ctx->ctx != verbs)
			 die("cannot handle events in more than one context.");
		 return;
	 }
	
	s_ctx = new context();
	s_ctx->ctx = verbs;
	std::cout<<s_ctx->ctx<<std::endl;
	s_ctx->protection_domain = ibv_alloc_pd(s_ctx->ctx);
	s_ctx->completion_channel = ibv_create_comp_channel(s_ctx->ctx);
	s_ctx->completion_queue = ibv_create_cq(s_ctx->ctx, 10, nullptr, s_ctx->completion_channel, 0);
	ibv_req_notify_cq(s_ctx->completion_queue, 0);
	pthread_create(&s_ctx->completion_queue_poller_thread, nullptr, &Process::poll_cq_thunk, static_cast<void*>(this));
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

void Process::build_params(struct rdma_conn_param *params){
	memset(params, 0, sizeof(*params));
	params->initiator_depth = params->responder_resources = 1;
	params->rnr_retry_count = 7; /* infinite retry */
}

void Process::register_memory(Connection *conn){
	struct benchmark_time btime;	
	start_time_keeping(&btime);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		conn->send_region = new char[message.size]();
		conn->recv_region = new char[message.size]();
	}
	else{
		conn->send_message = new message_sync();
		conn->recv_message = new message_sync();
		std::cout<<"~~~~~~~~~~~~~~~~~~"<<message.size<<std::endl;
		
		conn->rdma_local_region = new char[message.size]();
		conn->rdma_remote_region = new char[message.size]();
	}
	
	double dt = end_time_keeping(&btime);
	printf("ALLOCATION: %.8f mus\n", dt);
	char msg[100];
	sprintf(msg, "ALLOCATION:%0.15f", dt);
	logevent(this->logfilename, msg);

	start_time_keeping(&btime);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		conn->send_memory_region = ibv_reg_mr(
							     s_ctx->protection_domain, 
							     conn->send_region, 
							     message.size, 
							     IBV_ACCESS_LOCAL_WRITE);
		
		conn->recv_memory_region = ibv_reg_mr(
							     s_ctx->protection_domain, 
							     conn->recv_region, 
							     message.size, 
							     IBV_ACCESS_LOCAL_WRITE);
	}
	else{
		int mode;
		if(mode_of_operation == MODE_RDMA_READ)
			mode = IBV_ACCESS_REMOTE_READ;
		else
			mode = IBV_ACCESS_REMOTE_WRITE;
		
		conn->send_message_memory_region = ibv_reg_mr(
								 s_ctx->protection_domain, 
								 conn->send_message, 
								 sizeof(message_sync), 
								 IBV_ACCESS_LOCAL_WRITE);

		conn->recv_message_memory_region = ibv_reg_mr(
								 s_ctx->protection_domain, 
								 conn->recv_message, 
								 sizeof(message_sync), 
								 IBV_ACCESS_LOCAL_WRITE | mode);
		
		conn->rdma_local_memory_region = ibv_reg_mr(
								   s_ctx->protection_domain, 
								   conn->rdma_local_region, 
								   message.size, 
								   IBV_ACCESS_LOCAL_WRITE);

		conn->rdma_remote_memory_region = ibv_reg_mr(
								    s_ctx->protection_domain, 
								    conn->rdma_remote_region, 
								    message.size, 
								    IBV_ACCESS_LOCAL_WRITE | mode);
		
	}

	dt = end_time_keeping(&btime);
	printf("MEMORY REG: %.8f mus\n", dt);
	sprintf(msg, "REG:%0.15f", dt);
	logevent(this->logfilename, msg);
}

