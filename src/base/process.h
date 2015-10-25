#ifndef BASE_PROCESS_H
#define BASE_PROCESS_H

#include "include.h"

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
 private:
	struct rdma_event_channel *event_channel = NULL;
	struct rdma_cm_id *connection_identifier = NULL;
	struct context *s_ctx = NULL;
	struct connection *connection_ = NULL;
	struct rdma_cm_event *event = NULL;

	struct ibv_qp_init_attr queue_pair_attributes;
	struct sockaddr_in address;
	
	struct message_numerical message;

	std::string logfilename;
	std::string ipstring = "100.100.100.100";
	uint16_t port = 0;
	int client = 0;

	// process.cpp
	void initialize();
	
	// process_connection.cpp
	void connect();
	void build_context(struct ibv_context *verbs);
	void build_queue_pair_attributes();
	void register_memory();
	

	// process_events.cpp
	void event_loop();
	int on_event(struct rdma_cm_event *event);
	virtual void poll_cq(void *);
	static void* poll_cq_thunk(void*);

	int on_connection(void *context);
	int on_route_resolved(struct rdma_cm_id *id);
	int on_address_resolved(struct rdma_cm_id *id);
	int on_connect_request(struct rdma_cm_id *id);
	int on_disconnect(struct rdma_cm_id *id);
	
	// process_on_completion.cpp
	void on_completion(struct ibv_wc *wc);
	void on_completion_wc_recv(struct ibv_wc *wc);
	void on_completion_wc_send(struct ibv_wc *wc);
	void on_completion_not_implemented(struct ibv_wc *wc);
	
	// process_comm_send.cpp and process_comm_recv.cpp
	void post_recvs();
	int post_send();

	// process_utils.cpp
	void set_log_filename();
	void get_self_address();
	void get_server_address_from_string();

 public:
	// process.cpp
	Process();
	~Process();
	void run();
	// process_utils.cpp
	void set_as_client();
	void set_ip_string(char *str);
};



#endif
