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
		std::cout<<"\x1b[32m EVENT:CONNECT_REQUEST\x1b[0m"<<std::endl;
		r = on_connect_request(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ESTABLISHED){
		std::cout<<"\x1b[32m EVENT:ESTABLISHED\x1b[0m"<<std::endl;
		r = on_connection(event->id->context);
	}
	else if (event->event == RDMA_CM_EVENT_DISCONNECTED){
		std::cout<<"\x1b[32m EVENT:DISCONNECTED\x1b[0m"<<std::endl;		
		r = on_disconnect(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED){
		std::cout<<"\x1b[32m EVENT:ADDR_RESOLVED\x1b[0m"<<std::endl;
		r = on_address_resolved(event->id);
	}
	else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED){
		std::cout<<"\x1b[32m EVENT:ROUTE_RESOLVED\x1b[0m"<<std::endl;
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
	struct ibv_cq *cq;
	while (1) {
		TEST_NZ(ibv_get_cq_event(s_ctx->completion_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));
		while (ibv_poll_cq(cq, 1, &wc)){
			on_completion(&wc);
		}
	}
}


void* Process::poll_cq_thunk(void* self){
	void *ptr = nullptr;
 	static_cast<Process*>(self)->poll_cq(ptr);
	return nullptr;
}

int Process::on_connection(void *context){
	Connection *conn = (Connection *) context; 
	assert(context != nullptr);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_send(context);
	}
	else{
		conn->connected = 1;
		if(client)
			send_memory_region(conn);
	}
	  return 0;
}

int Process::on_route_resolved(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("route resolved.\n");
	memsetzero(&cm_params);
	if(mode_of_operation != MODE_SEND_RECEIVE)
		build_params(&cm_params);
	TEST_NZ(rdma_connect(id, &cm_params));
	return 0;
}

int Process::on_address_resolved(struct rdma_cm_id *id){
	printf("address resolved.\n");
	build_connection(id);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_recv((Connection *)id->context);
	}
	else{
		calc_message_numerical(&message);
		memcpy(get_local_message_region(id->context), message.x, message.size*sizeof(char));

	}
	rdma_resolve_route(id, TIMEOUT_IN_MS);
	return 0;
}



int Process::on_connect_request(struct rdma_cm_id *id){
	struct rdma_conn_param cm_params;
	printf("received connection request.\n");
	build_connection(id);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		post_recv((Connection *) id->context);
		memsetzero(&cm_params);
	}
	else{
		build_params(&cm_params);
		calc_message_numerical(&message);
		memcpy(get_local_message_region(id->context), message.x, message.size*sizeof(char));
	}
	
	rdma_accept(id, &cm_params);
	return 0;
}



int Process::on_disconnect(struct rdma_cm_id *id){
	printf("IN DISCONNE\n");
	Connection *conn = (Connection *) id->context;
	rdma_destroy_qp(conn->identifier);

	struct benchmark_time btime;
	start_time_keeping(&btime);

	if(mode_of_operation == MODE_SEND_RECEIVE){
		ibv_dereg_mr(conn->send_memory_region);
		ibv_dereg_mr(conn->recv_memory_region);
	}
	else{
		 ibv_dereg_mr(conn->send_message_memory_region);
		 ibv_dereg_mr(conn->recv_message_memory_region);
		 ibv_dereg_mr(conn->rdma_local_memory_region);
		 ibv_dereg_mr(conn->rdma_remote_memory_region);
	}
	double dt = end_time_keeping(&btime);
	printf("MEMORY DEREG: %.8f mus\n", dt);
	char msg[100];
	sprintf(msg, "DEREG:%0.15f", dt);
	logevent(this->logfilename, msg);


	start_time_keeping(&btime);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		delete[] conn->send_region;
		delete[] conn->recv_region;
	}
	else{
		assert(conn->send_message!=nullptr);
		delete conn->send_message;
		assert(conn->recv_message!=nullptr);
		delete conn->recv_message;

		assert(conn->rdma_local_region!=nullptr);
		delete[] conn->rdma_local_region;
		assert(conn->rdma_remote_region!=nullptr);
		delete[] conn->rdma_remote_region;
		
		// set to null
		conn->send_message = nullptr;
		conn->recv_message = nullptr;
		conn->rdma_local_region = nullptr;
		conn->rdma_remote_region = nullptr;
	}
	
	dt = end_time_keeping(&btime);
	printf("DEALLOCATION: %.8f mus\n", dt);
	sprintf(msg, "DEALLOCATION:%0.15f", dt);
	logevent(this->logfilename, msg);
	rdma_destroy_id(conn->identifier);
	delete conn;
	conn = nullptr;
	if(client)
		return 1;
	else
		return 0;
}


