#include "shared_ib.h"
#include "globals_ib.h"
#include "time.h"
void die(const char *reason)
{
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

void build_context(struct ibv_context *verbs)
{
	if (s_ctx) {
		if (s_ctx->ctx != verbs)
			die("cannot handle events in more than one context.");
		return;
	}

	s_ctx = (struct context *)malloc(sizeof(struct context));

	s_ctx->ctx = verbs;

	TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
	TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
	TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

	TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
	memset(qp_attr, 0, sizeof(*qp_attr));

	qp_attr->send_cq = s_ctx->cq;
	qp_attr->recv_cq = s_ctx->cq;
	qp_attr->qp_type = IBV_QPT_RC;

	qp_attr->cap.max_send_wr = 10;
	qp_attr->cap.max_recv_wr = 10;
	qp_attr->cap.max_send_sge = 1;
	qp_attr->cap.max_recv_sge = 1;
}



void * poll_cq(void *ctx)
{
	struct ibv_cq *cq;
	struct ibv_wc wc;

	while (1) {
		TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));

		while (ibv_poll_cq(cq, 1, &wc))
			if(client){
				on_completion_client(&wc);
			}
			else{
				on_completion(&wc);
			}
	}

	return NULL;
}

void post_receives(struct connection *conn)
{
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


void register_memory(struct connection *conn)
{
	struct timespec tstart={0,0}, tend={0,0};
	conn->send_region = malloc(BUFFER_SIZE);
	conn->recv_region = malloc(BUFFER_SIZE);
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



void on_completion(struct ibv_wc *wc)
{
	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");

	if (wc->opcode & IBV_WC_RECV) {
		struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

		printf("received message: %s\n", conn->recv_region);

	} else if (wc->opcode == IBV_WC_SEND) {
		printf("send completed successfully.\n");
	}
}

void on_completion_client(struct ibv_wc *wc)
{
	struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");
	if (wc->opcode & IBV_WC_RECV)
		printf("received message: %s\n", conn->recv_region);
	else if (wc->opcode == IBV_WC_SEND)
		printf("send completed successfully.\n");
	else
		die("on_completion: completion isn't a send or a receive.");

	if (++conn->num_completions == 2)
		rdma_disconnect(conn->id);
}


int on_connect_request(struct rdma_cm_id *id)
{
	struct ibv_qp_init_attr qp_attr;
	struct rdma_conn_param cm_params;
	struct connection *conn;

	printf("received connection request.\n");

	build_context(id->verbs);
	build_qp_attr(&qp_attr);

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));
	conn->qp = id->qp;

	register_memory(conn);
	post_receives(conn);

	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_accept(id, &cm_params));

	return 0;
}

int on_connection(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	snprintf(conn->send_region, BUFFER_SIZE, "message from passive/server side with pid %d", getpid());

	printf("connected. posting send...\n");

	memset(&wr, 0, sizeof(wr));

	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;

	sge.addr = (uintptr_t)conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->send_mr->lkey;

	TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));

	return 0;
}

int on_connection_client(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;
	
	snprintf(conn->send_region, BUFFER_SIZE, "message from active/client side with pid %d", getpid());
	
	printf("connected. posting send...\n");
	
	memset(&wr, 0, sizeof(wr));
	
	wr.wr_id = (uintptr_t)conn;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	
	sge.addr = (uintptr_t)conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->send_mr->lkey;
	
	TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
	
	return 0;
}


int on_disconnect(struct rdma_cm_id *id)
{
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

int on_disconnect_client(struct rdma_cm_id *id)
{
	struct connection *conn = (struct connection *)id->context;
	
	printf("disconnected.\n");
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
	
	return 1; /* exit event loop */
}



int on_event(struct rdma_cm_event *event)
{
	int r = 0;

	if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST){
		r = on_connect_request(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ESTABLISHED){
		if(client)
			r = on_connection_client(event->id->context);
		else
			r = on_connection(event->id->context);
	}
	else if (event->event == RDMA_CM_EVENT_DISCONNECTED){
		if(client)
			r = on_disconnect_client(event->id);
		else
			r = on_disconnect(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED)
		r = on_addr_resolved(event->id);
	else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED)
		r = on_route_resolved(event->id);
	else
		die("on_event: unknown event.");

	return r;
}



int on_route_resolved(struct rdma_cm_id *id)
{
	struct rdma_conn_param cm_params;
	printf("route resolved.\n");
	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_connect(id, &cm_params));
	return 0;
}


int on_addr_resolved(struct rdma_cm_id *id)
{
	struct ibv_qp_init_attr qp_attr;
	struct connection *conn;

	printf("address resolved.\n");

	build_context(id->verbs);
	build_qp_attr(&qp_attr);

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));

	conn->id = id;
	conn->qp = id->qp;
	conn->num_completions = 0;

	register_memory(conn);
	post_receives(conn);

	TEST_NZ(rdma_resolve_route(id, TIMEOUT_IN_MS));

	return 0;
}



