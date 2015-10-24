#ifndef BASE_PROCESS_H
#define BASE_PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <map>
#include "utils.h"

struct message_numerical{
	char x[MESSAGE_SIZE];
};

struct context {
	struct ibv_context *ctx;
	struct ibv_pd *protection_domain;
	struct ibv_cq *completion_queue;
	struct ibv_comp_channel *completion_channel;
	pthread_t completion_queue_poller_thread;
};

struct connection {
	struct ibv_qp *queue_pair;
	struct rdma_cm_id *identifier;
	struct ibv_mr *recv_memory_region;
	struct ibv_mr *send_memory_region;
	
	char *recv_region;
	char *send_region;

	int number_of_recvs;
	int number_of_sends;
};


class Process{
 public:
	struct ibv_qp_init_attr queue_pair_attributes;
	struct context *s_ctx;
	struct connection *connection_;
	struct rdma_event_channel *event_channel;
	struct rdma_cm_id *connection_identifier;
	struct rdma_cm_event *event = NULL;
	// for server and client
	struct sockaddr_in6 address;
	struct addrinfo *addressi;
	
	struct message_numerical message;
	std::ofstream timelogfile;
	uint16_t port = 0;
       
	
	
 	
	void build_context(struct ibv_context *verbs);
	void build_queue_pair_attributes();
	virtual void poll_cq(void *);
	static void* poll_cq_thunk(void*);

	void post_recvs();
	void register_memory();
	
	// 
	int on_route_resolved(struct rdma_cm_id *id);
	int on_address_resolved(struct rdma_cm_id *id);
	int on_connect_request(struct rdma_cm_id *id);
	int on_disconnect(struct rdma_cm_id *id);
	
	//
	int on_event(struct rdma_cm_event *event);
	
	//
	int on_connection(void *context);
	
	//
	void on_completion(struct ibv_wc *wc);
	void on_completion_wc_recv(struct ibv_wc *wc);
	void on_completion_wc_send(struct ibv_wc *wc);
	void on_completion_not_implemented(struct ibv_wc *wc);
	
	//
	void event_loop();

	// 
	int post_send();

	int client;
	Process();
	~Process();
	void go();
	

};



#endif
