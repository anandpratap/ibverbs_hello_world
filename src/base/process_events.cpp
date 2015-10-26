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
		((struct connection *)context)->connected = 1;
		if(client)
			send_memory_region();
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
		post_recv();
	}
	else{
		sprintf(get_local_message_region(id->context), "message from active/client side with pid %d", getpid());
		calc_message_numerical(&message);
		memcpy(get_local_message_region(id->context), &message, BUFFER_SIZE*sizeof(char));

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
		memsetzero(&cm_params);
	}
	else{
		build_params(&cm_params);
		sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
		calc_message_numerical(&message);
		memcpy(get_local_message_region(id->context), &message, BUFFER_SIZE*sizeof(char));
	}
	
	rdma_accept(id, &cm_params);
	return 0;
}



int Process::on_disconnect(struct rdma_cm_id *id){
	rdma_destroy_qp(id);

	struct benchmark_time btime;
	start_time_keeping(&btime);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		ibv_dereg_mr(connection_->send_memory_region);
		ibv_dereg_mr(connection_->recv_memory_region);
	}
	else{
		 ibv_dereg_mr(connection_->send_message_memory_region);
		 ibv_dereg_mr(connection_->recv_message_memory_region);
		 ibv_dereg_mr(connection_->rdma_local_memory_region);
		 ibv_dereg_mr(connection_->rdma_remote_memory_region);
	}
	double dt = end_time_keeping(&btime);
	printf("MEMORY DEREG: %.8f mus\n", dt);
	char msg[100];
	sprintf(msg, "DEREG:%0.8f", dt);
	logevent(this->logfilename, msg);


	start_time_keeping(&btime);
	if(mode_of_operation == MODE_SEND_RECEIVE){
		delete[] connection_->send_region;
		delete[] connection_->recv_region;
	}
	else{
		delete[] connection_->send_message;
		delete[] connection_->recv_message;
		delete[] connection_->rdma_local_region;
		delete[] connection_->rdma_remote_region;
		connection_->send_message = NULL;
		connection_->recv_message = NULL;
		connection_->rdma_local_region = NULL;
		connection_->rdma_remote_region = NULL;
	}
	
	dt = end_time_keeping(&btime);
	printf("DEALLOCATION: %.8f mus\n", dt);
	sprintf(msg, "DEALLOCATION:%0.8f", dt);
	logevent(this->logfilename, msg);

	delete[] s_ctx;
	s_ctx = NULL;
	//	ibv_destroy_qp(connection_->queue_pair);
	delete[] connection_;
	connection_ = NULL;
	rdma_destroy_id(id);
	
	if(client)
		return 1;
	else
		return 0;
}


