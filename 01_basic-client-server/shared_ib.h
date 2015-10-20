#ifndef _IB_SHARED_
#define _IB_SHARED_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include "globals_ib.h"

void die(const char *reason);
#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

struct context {
	struct ibv_context *ctx;
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_comp_channel *comp_channel;
	
	pthread_t cq_poller_thread;
};

struct connection {
	struct rdma_cm_id *id;
	struct ibv_qp *qp;
	
	struct ibv_mr *recv_mr;
	struct ibv_mr *send_mr;
	
	char *recv_region;
	char *send_region;

	int num_completions;
};

void build_context(struct ibv_context *verbs);
void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
void * poll_cq(void *);
void post_receives(struct connection *conn);
void register_memory(struct connection *conn);

void on_completion(struct ibv_wc *wc);
int on_connect_request(struct rdma_cm_id *id);
int on_connection(void *context);
int on_disconnect(struct rdma_cm_id *id);
int on_event(struct rdma_cm_event *event);

// client specific
int on_route_resolved(struct rdma_cm_id *id);
int on_addr_resolved(struct rdma_cm_id *id);
int on_connection_client(void *context);
int on_disconnect_client(struct rdma_cm_id *id);



#endif
