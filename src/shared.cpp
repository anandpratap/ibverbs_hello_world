#include "shared.h"
Process::Process(){
	
}

Process::~Process(){
	rdma_destroy_id(conn_id);
	rdma_destroy_event_channel(ec);	
}

void Process::build_context(struct ibv_context *verbs){
	if (s_ctx) {
		if (s_ctx->ctx != verbs){
			std::cout<<s_ctx->ctx<<" "<<verbs<<std::endl;
			die("cannot handle events in more than one context.");
		}
		return;
	}

	s_ctx = (struct context *)malloc(sizeof(struct context));
	s_ctx->ctx = verbs;

	TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
	TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
	TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));
	TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, &Process::poll_cq_thunk, static_cast<void*>(this)));
}


void Process::poll_cq(void *ctx){
	struct ibv_cq *cq;
	struct ibv_wc wc;
	
	while (1) {
		TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));
		while (ibv_poll_cq(cq, 1, &wc)){
			on_completion(&wc);
		}
	}
}


void* Process::poll_cq_thunk(void* self){
	void *ptr = NULL;
 	static_cast<Process*>(self)->poll_cq(ptr);
	return NULL;
}


void Process::build_qp_attr(){
	memset(&qp_attr, 0, sizeof(qp_attr));

	(&qp_attr)->send_cq = s_ctx->cq;
	(&qp_attr)->recv_cq = s_ctx->cq;
	(&qp_attr)->qp_type = IBV_QPT_RC;

	(&qp_attr)->cap.max_send_wr = 10;
	(&qp_attr)->cap.max_recv_wr = 10;
	(&qp_attr)->cap.max_send_sge = 1;
	(&qp_attr)->cap.max_recv_sge = 1;
}


void Process::post_receives(){
	struct ibv_recv_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	wr.wr_id = (uintptr_t)conn;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	sge.addr = (uintptr_t)conn->recv_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->recv_mr->lkey;

	TEST_NZ(ibv_post_recv(conn->qp, &wr, &bad_wr));
}


void Process::register_memory(){
	struct timespec tstart={0,0}, tend={0,0};
	conn->send_region = (char *) malloc(BUFFER_SIZE);
	conn->recv_region = (char *) malloc(BUFFER_SIZE);
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	TEST_Z(conn->send_mr = ibv_reg_mr(
					  s_ctx->pd, 
					  conn->send_region, 
					  BUFFER_SIZE, 
					  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
	
	TEST_Z(conn->recv_mr = ibv_reg_mr(
					  s_ctx->pd, 
					  conn->recv_region, 
					  BUFFER_SIZE, 
					  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
	
	clock_gettime(CLOCK_MONOTONIC, &tend);
	printf("memory registration took about %.8f micro seconds\n",
	       (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
		((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1e6);
}


void Process::on_completion(struct ibv_wc *wc){
	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");
	
	if (wc->opcode & IBV_WC_RECV) {
		struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;
		
		printf("received message: %s\n", conn->recv_region);
		
	} else if (wc->opcode == IBV_WC_SEND) {
		printf("send completed successfully.\n");
	}
}


int Process::on_connection(void *context)
{
	struct connection *ctx_conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;
	
	snprintf(ctx_conn->send_region, BUFFER_SIZE, "message from active/client side with pid %d", getpid());
	
	printf("connected. posting send...\n");
	
	memset(&wr, 0, sizeof(wr));
	
	wr.wr_id = (uintptr_t)ctx_conn;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = (uintptr_t)ctx_conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = ctx_conn->send_mr->lkey;
	
	TEST_NZ(ibv_post_send(ctx_conn->qp, &wr, &bad_wr));
	
	return 0;
}


int Process::on_disconnect(struct rdma_cm_id *id){
	struct connection *conn = (struct connection *)id->context;

	printf("peer disconnected.\n");

	rdma_destroy_qp(id);

	struct timespec tstart={0,0}, tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	
	ibv_dereg_mr(conn->send_mr);
	ibv_dereg_mr(conn->recv_mr);

	clock_gettime(CLOCK_MONOTONIC, &tend);
	printf("memory de-registration took about %.8f micro seconds\n",
	       (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
		((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1e6);


	free(conn->send_region);
	free(conn->recv_region);

	free(conn);
	rdma_destroy_id(id);
	return 0;
}


int Process::on_route_resolved(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("route resolved.\n");
	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_connect(id, &cm_params));
	return 0;
}


int Process::on_addr_resolved(struct rdma_cm_id *id){
	printf("address resolved.\n");
	build_context(id->verbs);
	build_qp_attr();

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));

	conn->id = id;
	conn->qp = id->qp;
	conn->num_completions = 0;

	register_memory();
	post_receives();

	TEST_NZ(rdma_resolve_route(id, TIMEOUT_IN_MS));

	return 0;
}



int Process::on_event(struct rdma_cm_event *event){
	int r = 0;
	if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST){
		std::cout<<"EVENT:CONNECT_REQUEST"<<std::endl;
		r = on_connect_request(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ESTABLISHED){
		std::cout<<"EVENT:ESTABLISHED"<<std::endl;
		r = on_connection(event->id->context);
	}
	else if (event->event == RDMA_CM_EVENT_DISCONNECTED){
		std::cout<<"EVENT:DISCONNECTED"<<std::endl;		
		r = on_disconnect(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED){
		std::cout<<"EVENT:ADDR_RESOLVED"<<std::endl;
		r = on_addr_resolved(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED){
		std::cout<<"EVENT:ROUTE_RESOLVED"<<std::endl;
		r = on_route_resolved(event->id);
	}
	else{
		die("on_event: unknown event.");
	}
	return r;
}


int Process::on_connect_request(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("received connection request.\n");
	
	build_context(id->verbs);
	build_qp_attr();

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));
	conn->qp = id->qp;

	register_memory();
	post_receives();

	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_accept(id, &cm_params));

	return 0;
}


void Process::go(){
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(DEFAULT_PORT);
	TEST_Z(ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(ec, &conn_id, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_bind_addr(conn_id, (struct sockaddr *)&addr));
	TEST_NZ(rdma_listen(conn_id, 10)); /* backlog=10 is arbitrary */
	port = ntohs(rdma_get_src_port(conn_id));
	printf("Server listening on port %d.\n", port);
	while (rdma_get_cm_event(ec, &event) == 0) {
		struct rdma_cm_event event_copy;
		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);
		if (on_event(&event_copy))
			break;
	}
}

void ClientProcess::go(){
	TEST_NZ(getaddrinfo(DEFAULT_ADDRESS, DEFAULT_PORT_S, NULL, &addri));
	TEST_Z(ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(ec, &conn_id, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_resolve_addr(conn_id, NULL, addri->ai_addr, TIMEOUT_IN_MS));
  	freeaddrinfo(addri);
	std::cout<<"CLIENT:PORT "<<DEFAULT_PORT_S<<std::endl;
	while (rdma_get_cm_event(ec, &event) == 0){
		struct rdma_cm_event event_copy;
		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);
		if (on_event(&event_copy))
			break;
	}
}
