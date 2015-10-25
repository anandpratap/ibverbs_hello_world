#include "include.h"
#include "utils.h"
#include "process.h"

void Process::event_loop(){
	while (rdma_get_cm_event(this->event_channel, &event) == 0){
		// create a copy of the event and acknowledge
		// acknowledgement is important
		struct rdma_cm_event event_copy;
		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);
		if (on_event(&event_copy))
			break;
	}
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

int Process::on_connection(void *context){
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_send();
	}
	else{
		std::cout<<"MODE_OF_OPERATION is not correct."<<std::endl;
		exit(EXIT_FAILURE);
	}
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
	build_connection(id);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_recv();
	}
	else{
		std::cout<<"MODE_OF_OPERATION is not correct."<<std::endl;
		exit(EXIT_FAILURE);
	}
	rdma_resolve_route(id, TIMEOUT_IN_MS);
	return 0;
}



int Process::on_connect_request(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("received connection request.\n");
	build_connection(id);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_recv();
	}
	else{
		std::cout<<"MODE_OF_OPERATION is not correct."<<std::endl;
		exit(EXIT_FAILURE);
	}
	
	memsetzero(&cm_params);
	rdma_accept(id, &cm_params);
	return 0;
}



int Process::on_disconnect(struct rdma_cm_id *id){
	rdma_destroy_qp(id);

	struct benchmark_time btime;
	start_time_keeping(&btime);
	
	ibv_dereg_mr(connection_->send_memory_region);
	ibv_dereg_mr(connection_->recv_memory_region);
	
	double dt = end_time_keeping(&btime);
	printf("MEMORY DEREG: %.8f mus\n", dt);
	char msg[100];
	sprintf(msg, "DEREG:%0.8f", dt);
	logevent(this->logfilename, msg);

	delete[] connection_->send_region;
	delete[] connection_->recv_region;

	delete[] connection_;
	rdma_destroy_id(id);
	if(client)
		return 1;
	else
		return 0;
}


