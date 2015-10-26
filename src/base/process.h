#ifndef BASE_PROCESS_H
#define BASE_PROCESS_H

#include "include.h"

extern struct benchmark_time __time;

enum mtype{
	MSG_MR,
	MSG_DONE
};

enum sstate{
	SS_INIT,
	SS_MR_SENT,
	SS_RDMA_SENT,
	SS_DONE_SENT,
	END_OF_LIST 
};

enum rstate{
	RS_INIT,
	RS_MR_RECV,
	RS_DONE_RECV,
	END_OF_LISTT 
};

sstate& operator++(sstate &c);
sstate operator++(sstate &c, int);
rstate& operator++(rstate &c);
rstate operator++(rstate &c, int);

struct message_numerical{
	char x[MESSAGE_SIZE];
};

struct message_sync{
	mtype type;
	union{
		struct  ibv_mr mr;
	} data;
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

	int connected;
	// for send and recv
	struct ibv_mr *recv_memory_region;
	struct ibv_mr *send_memory_region;
	char *recv_region;
	char *send_region;


	// for RDMA
	struct ibv_mr *rdma_local_memory_region;
	struct ibv_mr *rdma_remote_memory_region;
	struct ibv_mr *send_message_memory_region;
	struct ibv_mr *recv_message_memory_region;
	struct ibv_mr remote_memory_region;
	char *rdma_local_region;
	char *rdma_remote_region;


	struct message_sync *recv_message;
	struct message_sync *send_message;
	
	sstate send_state;
	rstate recv_state;
	
	int number_of_recvs;
	int number_of_sends;
	
};

enum mode{
	MODE_SEND_RECEIVE,
	MODE_RDMA_READ,
	MODE_RDMA_WRITE
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
	
	mode mode_of_operation = MODE_SEND_RECEIVE;
	struct message_numerical message;

	std::string ipstring = "100.100.100.100";
	uint16_t port = 0;
	int client = 0;
	int max_number_of_recvs = 1;
	int max_number_of_sends = 1;
	
	// process.cpp
	void initialize();
	
	// process_connection.cpp
	void connect();
	int build_connection(struct rdma_cm_id *id);
	void build_context(struct ibv_context *verbs);
	void build_queue_pair_attributes();
	void build_params(struct rdma_conn_param *params);

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
	char* get_remote_message_region(struct connection *conn);
	char* get_local_message_region(void *context);
	// process_comm_send.cpp and process_comm_recv.cpp
	void post_recv();
	int post_send();
	int send_message();
	int send_memory_region();

	// process_utils.cpp
	void set_log_filename();
	void get_self_address();
	void get_server_address_from_string();

 public:
	// process.cpp
	std::string logfilename;
	Process();
	~Process();
	void run();
	// process_utils.cpp
	void set_as_client();
	void set_ip_string(char *str);
	void set_mode_of_operation(mode m);
	void set_max_recv(int n);
	void set_max_send(int n);
};


#endif
