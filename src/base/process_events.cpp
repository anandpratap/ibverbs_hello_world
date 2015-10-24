#include "include.h"
#include "utils.h"
#include "process.h"
void Process::poll_cq(void *ctx){
	struct ibv_wc wc;
	
	while (1) {
		TEST_NZ(ibv_get_cq_event(s_ctx->completion_channel, &s_ctx->completion_queue, &ctx));
		ibv_ack_cq_events(s_ctx->completion_queue, 1);
		TEST_NZ(ibv_req_notify_cq(s_ctx->completion_queue, 0));
		while (ibv_poll_cq(s_ctx->completion_queue, 1, &wc)){
			on_completion(&wc);
		}
	}
}


void* Process::poll_cq_thunk(void* self){
	void *ptr = NULL;
 	static_cast<Process*>(self)->poll_cq(ptr);
	return NULL;
}

int Process::on_connection(void *context)
{
	post_send();
	return 0;
}


int Process::on_disconnect(struct rdma_cm_id *id){
	printf("peer disconnected.\n");

	rdma_destroy_qp(id);

	struct timespec tstart={0,0}, tend={0,0};
	double dt;
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	
	ibv_dereg_mr(connection_->send_memory_region);
	ibv_dereg_mr(connection_->recv_memory_region);

	clock_gettime(CLOCK_MONOTONIC, &tend);
	dt = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
		((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);

	printf("memory de-registration took about %.8f micro seconds\n",
	       dt);

	if(client)
		timelogfile.open("client_time.log", std::ios::app);
	else
		timelogfile.open("server_time.log", std::ios::app);
	timelogfile<<"DEREG:" <<dt<<std::endl;
	timelogfile.close();

	delete[] connection_->send_region;
	delete[] connection_->recv_region;

	delete[] connection_;
	rdma_destroy_id(id);
	if(client)
		return 1;
	else
		return 0;
}


int Process::on_route_resolved(struct rdma_cm_id *id){
	struct rdma_conn_param connection_manager_params;
	printf("route resolved.\n");
	memsetzero(&connection_manager_params);
	TEST_NZ(rdma_connect(id, &connection_manager_params));
	return 0;
}


int Process::on_address_resolved(struct rdma_cm_id *id){
	printf("address resolved.\n");
	build_context(id->verbs);
	build_queue_pair_attributes();
	
	TEST_NZ(rdma_create_qp(id, s_ctx->protection_domain, &this->queue_pair_attributes));
	
	id->context = connection_ = new connection();
	
	connection_->identifier = id;
	connection_->queue_pair = id->qp;
	connection_->number_of_recvs = 0;
	connection_->number_of_sends = 0;
	
	register_memory();
	post_recvs();
	
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
		r = on_address_resolved(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED){
		std::cout<<"EVENT:ROUTE_RESOLVED"<<std::endl;
		r = on_route_resolved(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ADDR_ERROR){
		std::cout<<"EVENT:ADDRESS_RESOLVED_FAIL"<<std::endl;
		exit(EXIT_FAILURE);
	}
	else if (event->event == RDMA_CM_EVENT_ROUTE_ERROR){
		std::cout<<"EVENT:ROUTE_RESOLVED_FAIL"<<std::endl;     
		exit(EXIT_FAILURE);
	}
	else if (event->event == RDMA_CM_EVENT_CONNECT_ERROR){
		std::cout<<"EVENT:ROUTE_CONNECT_FAIL"<<std::endl;
		exit(EXIT_FAILURE);
	}
	else if (event->event == RDMA_CM_EVENT_UNREACHABLE){
		std::cout<<"EVENT:UNREACHABLE"<<std::endl;
		exit(EXIT_FAILURE);
	}
	else if (event->event == RDMA_CM_EVENT_REJECTED){
		std::cout<<"EVENT:REJECTED May be the server is not online?"<<std::endl;
		exit(EXIT_FAILURE);
	}
	else{
		std::cout<<event->event<<std::endl;
		die("on_event: unknown event.");
	}
	return r;
}


int Process::on_connect_request(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("received connection request.\n");
	
	build_context(id->verbs);
	build_queue_pair_attributes();

	TEST_NZ(rdma_create_qp(id, s_ctx->protection_domain, &this->queue_pair_attributes));

	id->context = connection_ = new connection();
	connection_->queue_pair = id->qp;

	register_memory();
	post_recvs();
	memsetzero(&cm_params);
	TEST_NZ(rdma_accept(id, &cm_params));

	return 0;
}


void Process::event_loop(){
	while (rdma_get_cm_event(this->event_channel, &event) == 0) {
		// create a copy of the event and acknowledge
		// acknowledgement is important
		std::cout<<"IN WHILE"<<std::endl;
		struct rdma_cm_event event_copy;
		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);
		if (on_event(&event_copy))
			break;
	}
}
