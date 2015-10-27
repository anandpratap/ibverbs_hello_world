#include "include.h"
#include "utils.h"
#include "process.h"

void Process::on_completion(struct ibv_wc *wc){
	struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;
	if (wc->status)
		resolve_wc_error(wc);
	if (wc->opcode == IBV_WC_RECV){
		std::cout<<"\x1b[31m WC:OPCODE:RECV\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_SEND_RECEIVE){
			double dt = end_time_keeping(&__time);
			printf("DATATIME: %.8f mus\n", dt);
			char msg[100];
			sprintf(msg, "DATATIME:%0.8f", dt);
			logevent(this->logfilename, msg);
			on_completion_wc_recv(wc);
		}
		else{
			connection_->recv_state++;
			if(connection_->recv_message->type == MSG_MR){
				memcpy(&connection_->remote_memory_region, &connection_->recv_message->data.mr, sizeof(connection_->remote_memory_region));
				post_recv(); 
				if (connection_->send_state == SS_INIT)
					send_memory_region();
				
			}
		}
		connection_->number_of_recvs++;
	}
	else if (wc->opcode == IBV_WC_SEND){
		std::cout<<"\x1b[31m WC:OPCODE:SEND\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_SEND_RECEIVE){
			on_completion_wc_send(wc);
			start_time_keeping(&__time);
		}
		else{
			connection_->send_state++;
		}
		connection_->number_of_sends++;
		
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
			sprintf(msg, "DATATIME:%0.8f", dt);
			logevent(this->logfilename, msg);
			connection_->send_state++;
		}
	}
	else if (wc->opcode == IBV_WC_RDMA_READ){
		std::cout<<"\x1b[31m WC:OPCODE:RDMA_READ\x1b[0m"<<std::endl;
		if(mode_of_operation == MODE_RDMA_READ){
			double dt = end_time_keeping(&__time);
			printf("DATATIME: %.8f mus\n", dt);
			char msg[100];
			sprintf(msg, "DATATIME:%0.8f", dt);
			logevent(this->logfilename, msg);
			connection_->send_state++;
		}
	}
	else if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM){
		std::cout<<"\x1b[31m WC:OPCODE:RECV_RDMA_WITH_IMM\x1b[0m"<<std::endl;
		on_completion_not_implemented(wc);
	}
	else
		die("wc->opcode not recognised.");
	
	if(mode_of_operation != MODE_SEND_RECEIVE && !wc->status){
		if (connection_->send_state == SS_MR_SENT && connection_->recv_state == RS_MR_RECV) {
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
			wr.wr.rdma.remote_addr = (uintptr_t)connection_->remote_memory_region.addr;
			wr.wr.rdma.rkey = connection_->remote_memory_region.rkey;
			
			sge.addr = (uintptr_t)connection_->rdma_local_region;
			sge.length = message.size;
			sge.lkey = connection_->rdma_local_memory_region->lkey;
			
			TEST_NZ(ibv_post_send(connection_->queue_pair, &wr, &bad_wr));
			start_time_keeping(&__time);

			connection_->send_message->type = MSG_DONE;
			send_message();
		} else if (connection_->send_state == SS_DONE_SENT && connection_->recv_state == RS_DONE_RECV) {
			struct message_numerical recv_message;
			recv_message.x = get_remote_message_region(connection_);
			recv_message.size = message.size;
			
			int sum = 0;
			
			memcpy(&sum, recv_message.x + (message.size-4)*sizeof(char), sizeof(int));
			std::cout<<"RECV COUNT: "<<connection_->number_of_recvs << " SUM:"<<sum<<std::endl;
			verify_message_numerical(&recv_message);
			rdma_disconnect(connection_->identifier);
		}
	}

	if((connection_->number_of_recvs == max_number_of_recvs) && client && mode_of_operation == MODE_SEND_RECEIVE){
		rdma_disconnect(connection_->identifier);
	}
	

}

void Process::on_completion_wc_recv(struct ibv_wc *wc){
	struct message_numerical recv_message;
	recv_message.x = connection_->recv_region;
	recv_message.size = message.size;
	int sum = 0;
	memcpy(&sum, recv_message.x + (message.size-4)*sizeof(char), sizeof(int));
	std::cout<<"RECV COUNT: "<<connection_->number_of_recvs << " SUM:"<<sum<<std::endl;
	verify_message_numerical(&recv_message);
}


void Process::on_completion_wc_send(struct ibv_wc *wc){
	std::cout<<"Request sent on completion."<<std::endl;
}

void Process::on_completion_not_implemented(struct ibv_wc *wc){
	std::cout<<"On completion response for this request is not implemented."<<std::endl;
	die("Killing myself..");
}

char* Process::get_local_message_region(void *context){
	if (mode_of_operation == MODE_RDMA_WRITE)
		return ((struct connection *)context)->rdma_local_region;
	else
		return ((struct connection *)context)->rdma_remote_region;
}

char* Process::get_remote_message_region(struct connection *conn){
	if (mode_of_operation == MODE_RDMA_WRITE)
		return conn->rdma_remote_region;
	else
		return conn->rdma_local_region;
}
