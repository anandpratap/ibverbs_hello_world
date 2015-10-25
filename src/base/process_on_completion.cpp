#include "include.h"
#include "utils.h"
#include "process.h"

void Process::on_completion(struct ibv_wc *wc){
	if (wc->status)
		resolve_wc_error(wc->status);
	if (wc->opcode == IBV_WC_RECV){
		on_completion_wc_recv(wc);
		connection_->number_of_recvs++;
	}
	else if (wc->opcode == IBV_WC_SEND){
		on_completion_wc_send(wc);
	}
	else if (wc->opcode == IBV_WC_COMP_SWAP) 
		on_completion_not_implemented(wc);
	else if (wc->opcode == IBV_WC_FETCH_ADD) 
		on_completion_not_implemented(wc);
	else if (wc->opcode == IBV_WC_BIND_MW) 
		on_completion_not_implemented(wc);
	else if (wc->opcode == IBV_WC_RDMA_WRITE) 
		on_completion_not_implemented(wc);
	else if (wc->opcode == IBV_WC_RDMA_READ) 
		on_completion_not_implemented(wc);
	else if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM) 
		on_completion_not_implemented(wc);
	else
		die("wc->opcode not recognised.");
	
	if((connection_->number_of_recvs == max_number_of_recvs) && client){
		rdma_disconnect(connection_identifier);
	}
}

void Process::on_completion_wc_recv(struct ibv_wc *wc){
	struct message_numerical* recv_message = (struct message_numerical *) connection_->recv_region;
	int sum = 0;
	memcpy(&sum, &(recv_message->x[MESSAGE_SIZE-4]), sizeof(int));
	std::cout<<"RECV COUNT: "<<connection_->number_of_recvs << " SUM:"<<sum<<std::endl;
	verify_message_numerical(recv_message);
}


void Process::on_completion_wc_send(struct ibv_wc *wc){
	std::cout<<"Request sent on completion."<<std::endl;
}

void Process::on_completion_not_implemented(struct ibv_wc *wc){
	std::cout<<"On completion response for this request is not implemented."<<std::endl;
	die("Killing myself..");
}



