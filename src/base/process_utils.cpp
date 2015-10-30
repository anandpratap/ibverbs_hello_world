#include "include.h"
#include "utils.h"
#include "process.h"
void Process::set_as_client(){
	this->client = 1;
}


void Process::set_logfilename(char *str){
	sprintf(logfilename, str);
}
void Process::get_self_address(){
	memsetzero(&address);
	address.sin_family = AF_INET;
	address.sin_port = htons(DEFAULT_PORT);
}

void Process::get_server_address_from_string(){
	address.sin_family = AF_INET;
	address.sin_port = htons(DEFAULT_PORT);
	inet_pton(AF_INET, &this->ipstring[0], &address.sin_addr); 
}

void Process::set_ip_string(char *str){
	ipstring = std::string(str);
}

void Process::set_mode_of_operation(mode m){
    if(m < 0 || m > 2){
        printf("ERROR:WRONG MODE OF OPERATION\n");
        exit(EXIT_FAILURE);
    }
	this->mode_of_operation = m;
}

void Process::set_max_recv(int n){
	max_number_of_recvs = n;
}

void Process::set_max_send(int n){
	max_number_of_sends = n;
}


int Process::set_message_size(unsigned int n){
	if(n < 8){
		printf("\tERROR:MESSAGE SIZE CANNOT BE LESS THAN 8 BYTES BY DESIGN\n");
        exit(EXIT_FAILURE);
	}
	
	this->message.size = n;
	this->message.x = new char[n];
	return EXIT_SUCCESS;
}

int Process::reset_message(){
	delete[] this->message.x;
	this->message.x = nullptr;
	this->message.size = 0;
	return EXIT_SUCCESS;
}



char* Process::get_local_message_region(void *context){
    if (mode_of_operation == MODE_RDMA_WRITE)
        return (static_cast<Connection*>(context))->rdma_local_region;
    else
        return (static_cast<Connection*>(context))->rdma_remote_region;
}

char* Process::get_remote_message_region(Connection *conn){
    if (mode_of_operation == MODE_RDMA_WRITE)
        return conn->rdma_remote_region;
    else
        return conn->rdma_local_region;
}

void Process::set_send_work_request_attributes(struct ibv_send_wr *wr, struct ibv_sge *sge, Connection *conn, int submode){
	memsetzero(wr);
	wr->wr_id = reinterpret_cast<uintptr_t>(conn);
	if(mode_of_operation == MODE_SEND_RECEIVE || submode == 0){
		wr->wr_id = reinterpret_cast<uintptr_t>(conn);
		wr->opcode = IBV_WR_SEND;
		wr->sg_list = sge;
		wr->num_sge = 1;
		wr->send_flags = IBV_SEND_SIGNALED;
	}
	else{
		wr->opcode = (mode_of_operation == MODE_RDMA_WRITE) ? IBV_WR_RDMA_WRITE : IBV_WR_RDMA_READ;
		wr->sg_list = sge;
		wr->num_sge = 1;
		wr->send_flags = IBV_SEND_SIGNALED;
		wr->wr.rdma.remote_addr = reinterpret_cast<uintptr_t>(conn->remote_memory_region.addr);
		wr->wr.rdma.rkey = conn->remote_memory_region.rkey;
	}
}

void Process::set_send_scatter_gather_attributes(struct ibv_sge *sge, Connection *conn, int submode){
	if(mode_of_operation == MODE_SEND_RECEIVE){
		sge->addr = reinterpret_cast<uintptr_t>(conn->send_region);
		sge->length = message.size;
		sge->lkey = conn->send_memory_region->lkey;
	}
	else{
		if(submode == 0){
			sge->addr = reinterpret_cast<uintptr_t>(conn->send_message);
			sge->length = sizeof(struct message_sync);
			sge->lkey = conn->send_message_memory_region->lkey;
		}
		else{
			sge->addr = reinterpret_cast<uintptr_t>(conn->rdma_local_region);
			sge->length = message.size;
			sge->lkey = conn->rdma_local_memory_region->lkey;
		}
	}
}


void Process::set_recv_work_request_attributes(struct ibv_recv_wr *wr, struct ibv_sge *sge, Connection *conn, int submode){
	memsetzero(wr);
	wr->wr_id = reinterpret_cast<uintptr_t>(conn);
	wr->next = nullptr;
	wr->sg_list = sge;
	wr->num_sge = 1;
}


void Process::set_recv_scatter_gather_attributes(struct ibv_sge *sge, Connection *conn, int submode){

	if(mode_of_operation == MODE_SEND_RECEIVE){
		sge->addr = reinterpret_cast<uintptr_t>(conn->recv_region);
		sge->length = message.size;
		sge->lkey = conn->recv_memory_region->lkey;
	}
	else{
		assert(conn->recv_message != nullptr);
        sge->addr = reinterpret_cast<intptr_t>(conn->recv_message);
		sge->length = sizeof(struct message_sync);
		sge->lkey = conn->recv_message_memory_region->lkey;
	}
}    
