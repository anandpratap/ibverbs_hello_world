#include "include.h"
#include "utils.h"
#include "process.h"

void Process::on_completion(struct ibv_wc *wc){
	Connection *conn = (Connection *)(uintptr_t)wc->wr_id;
	printf("IN ON COMPLETION\n");
	std::cout<<"LOCATION CONN -> ID"<<conn->identifier<<"\n";
	std::cout<<"POINTER ID"<<connection_identifier<<std::endl<<std::flush;


	if (wc->status)
		resolve_wc_error(wc);
	if (wc->opcode == IBV_WC_RECV){
		std::cout<<"\x1b[31m WC:OPCODE:RECV\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_SEND_RECEIVE){
			double dt = end_time_keeping(&__time);
			printf("DATATIME: %.8f mus\n", dt);
			char msg[100];
			sprintf(msg, "DATATIME:%0.15f", dt);
			logevent(this->logfilename, msg);
			on_completion_wc_recv(wc);
		}
		else{
			conn->recv_state++;
			//			assert(&conn->recv_message->type != nullptr);
			if(conn->recv_message->type == MSG_MR){
				memcpy(&conn->remote_memory_region, &conn->recv_message->data.mr, sizeof(conn->remote_memory_region));
				std::cout<<"MR: "<<conn->remote_memory_region.addr<<std::endl;
				std::cout<<"MR: "<<conn->remote_memory_region.rkey<<std::endl;
				std::cout<<"MR: "<<conn->remote_memory_region.length<<std::endl;
				std::cout<<"MR: "<<conn->remote_memory_region.lkey<<std::endl;

				post_recv(conn); 
				if (conn->send_state == SS_INIT)
					send_memory_region(conn);
				
			}
		}
		this->number_of_recvs++;
		conn->number_of_recvs++;
		std::cout<<"RECV CONN COUNT: "<<conn->number_of_recvs<<" "<<max_number_of_recvs<<std::endl;
		std::cout<<"RECV PROCESS COUNT: "<<this->number_of_recvs<<std::endl;

	}
	else if (wc->opcode == IBV_WC_SEND){
		std::cout<<"\x1b[31m WC:OPCODE:SEND\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_SEND_RECEIVE){
			on_completion_wc_send(wc);
			start_time_keeping(&__time);
		}
		else{
			conn->send_state++;
		}
		conn->number_of_sends++;

		if(disconnect && !client && conn->send_state == SS_DONE_SENT){
			printf("DISCONNECTING-SERVERERERERERERER-----------------------\n");
			//rdma_disconnect(connection_identifier);
		}
	}
	else if (wc->opcode == IBV_WC_COMP_SWAP){ 
		std::cout<<"\x1b[31m WC:OPCODE:COMP_SWAP\x1b[0m"<<std::endl;
		on_completion_not_implemented(wc);
	}
	else if (wc->opcode == IBV_WC_FETCH_ADD){
		std::cout<<"\x1b[31m WC:OPCODE:FETCH_ADD\x1b[0m"<<std::endl;
		on_completion_not_implemented(wc);
	}
	else if (wc->opcode == IBV_WC_BIND_MW){
		std::cout<<"\x1b[31m WC:OPCODE:BIND_MW\x1b[0m"<<std::endl;
		on_completion_not_implemented(wc);
	}
	else if (wc->opcode == IBV_WC_RDMA_WRITE){
		std::cout<<"\x1b[31m WC:OPCODE:RDMA_WRITE\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_RDMA_WRITE){
			double dt = end_time_keeping(&__time);
			printf("DATATIME: %.8f mus\n", dt);
			char msg[100];
			sprintf(msg, "DATATIME:%0.15f", dt);
			logevent(this->logfilename, msg);
			conn->send_state++;
		}
	}
	else if (wc->opcode == IBV_WC_RDMA_READ){
		std::cout<<"\x1b[31m WC:OPCODE:RDMA_READ\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_RDMA_READ){
			double dt = end_time_keeping(&__time);
			printf("DATATIME: %.8f mus\n", dt);
			char msg[100];
			sprintf(msg, "DATATIME:%0.15f", dt);
			logevent(this->logfilename, msg);
			conn->send_state++;
		}
	}
	else if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM){
		std::cout<<"\x1b[31m WC:OPCODE:RECV_RDMA_WITH_IMM\x1b[0m"<<std::endl;
		on_completion_not_implemented(wc);
	}
	else
		die("wc->opcode not recognised.");
	std::cout<<"state "<<conn->send_state<<" "<<conn->recv_state<<std::endl;
	if(mode_of_operation != MODE_SEND_RECEIVE && !wc->status){
		if (conn->send_state == SS_MR_SENT && conn->recv_state == RS_MR_RECV) {
			struct ibv_send_wr wr, *bad_wr = nullptr;
			struct ibv_sge sge;
			if (mode_of_operation == MODE_RDMA_WRITE)
				printf("received MSG_MR. writing message to remote memory...\n");
			else
				printf("received MSG_MR. reading message from remote memory...\n");
			
			memset(&wr, 0, sizeof(wr));
			wr.wr_id = (uintptr_t)conn;
			wr.opcode = (mode_of_operation == MODE_RDMA_WRITE) ? IBV_WR_RDMA_WRITE : IBV_WR_RDMA_READ;
			wr.sg_list = &sge;
			wr.num_sge = 1;
			wr.send_flags = IBV_SEND_SIGNALED;
			wr.wr.rdma.remote_addr = (uintptr_t)conn->remote_memory_region.addr;
			wr.wr.rdma.rkey = conn->remote_memory_region.rkey;
			
			sge.addr = (uintptr_t)conn->rdma_local_region;
			sge.length = message.size;
			sge.lkey = conn->rdma_local_memory_region->lkey;
			
			TEST_NZ(ibv_post_send(conn->queue_pair, &wr, &bad_wr));
			start_time_keeping(&__time);

			conn->send_message->type = MSG_DONE;
			send_message(conn);
		} else if (conn->send_state == SS_DONE_SENT && conn->recv_state == RS_DONE_RECV) {
			struct message_numerical recv_message;
			recv_message.x = get_remote_message_region(conn);
			recv_message.size = message.size;
			
			int sum = 0;
			memcpy(&sum, recv_message.x + (message.size-4)*sizeof(char), sizeof(int));
			verify_message_numerical(&recv_message);
			/////
			post_recv(conn);
			conn->send_message->type = MSG_DONE;
			send_message(conn);
			/////
		}
		else if (conn->send_state == SS_DONE_SENT2 && conn->recv_state == RS_DONE_RECV2) {
			rdma_disconnect(conn->identifier);
		}
	}

	if((this->number_of_recvs == max_number_of_recvs) && !client){
		std::cout<<"RECV PROCESS COUNT: "<<this->number_of_recvs <<std::endl;
		std::cout<<"RECV PROCESS COUNT: "<<this->connection_identifier<<std::endl;
		//Connection *conn_ = (Connection *) connection_identifier->context;
		//		rdma_disconnect(conn_->identifier);
		//rdma_disconnect(conn->identifier);
	}

	

}

void Process::on_completion_wc_recv(struct ibv_wc *wc){
	Connection *conn = (Connection *)(uintptr_t)wc->wr_id;   
	struct message_numerical recv_message;
	assert(conn->recv_region !=nullptr);
	recv_message.x = conn->recv_region;
	recv_message.size = message.size;
	int sum = 0;
	memcpy(&sum, recv_message.x + (message.size-4)*sizeof(char), sizeof(int));
	std::cout<<"RECV COUNT: "<<conn->number_of_recvs << " SUM:"<<sum<<std::endl;
	verify_message_numerical(&recv_message);
}


void Process::on_completion_wc_send(struct ibv_wc *wc){
	Connection *conn = (Connection *)(uintptr_t)wc->wr_id;   
	std::cout<<"Request sent on completion."<<std::endl;
	if((conn->number_of_recvs == max_number_of_recvs) && client && (mode_of_operation == MODE_SEND_RECEIVE)){
		std::cout<<"POINTER"<<conn->identifier<<std::endl<<std::flush;
		std::cout<<"POINTER_"<<connection_identifier<<std::endl<<std::flush;
		printf("Calling disconnect\n");
		//rdma_disconnect(connection_identifier);
		rdma_disconnect(conn->identifier);
	}


}

void Process::on_completion_not_implemented(struct ibv_wc *wc){
	std::cout<<"On completion response for this request is not implemented."<<std::endl;
	die("Killing myself..");
}

char* Process::get_local_message_region(void *context){
	if (mode_of_operation == MODE_RDMA_WRITE)
		return ((Connection *)context)->rdma_local_region;
	else
		return ((Connection *)context)->rdma_remote_region;
}

char* Process::get_remote_message_region(Connection *conn){
	if (mode_of_operation == MODE_RDMA_WRITE)
		return conn->rdma_remote_region;
	else
		return conn->rdma_local_region;
}
