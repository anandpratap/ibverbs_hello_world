#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <netdb.h>
#include <iostream>

#define BUFFER_SIZE 1024
#define TIMEOUT_IN_MS 50

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DEFAULT_ADDRESS "localhost"
#define DEFAULT_PORT 48105
#define DEFAULT_PORT_S STR(DEFAULT_PORT)

inline void die(const char *reason){
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

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


class Process{
public:
	struct context *s_ctx = NULL;
	struct ibv_qp_init_attr qp_attr;
	struct connection *conn = NULL;

	struct sockaddr_in6 addr;
	struct addrinfo *addri;
		
	struct rdma_cm_event *event = NULL;
	struct rdma_cm_id *conn_id = NULL;
	struct rdma_event_channel *ec = NULL;
	uint16_t port = 0;

	Process();
	~Process();
	void go();
	void build_context(struct ibv_context *verbs);
	void build_qp_attr();
	virtual void poll_cq(void *);
	static void* poll_cq_thunk(void*);

	void post_receives();
	void register_memory();
	
	// 
	int on_route_resolved(struct rdma_cm_id *id);
	int on_addr_resolved(struct rdma_cm_id *id);
	int on_connect_request(struct rdma_cm_id *id);
	int on_disconnect(struct rdma_cm_id *id);
	
	//
	int on_event(struct rdma_cm_event *event);
	
	//
	int on_connection(void *context);
	
	//
	void on_completion(struct ibv_wc *wc);
};


class ClientProcess: public Process{
 public:
	void go();
};

#endif
