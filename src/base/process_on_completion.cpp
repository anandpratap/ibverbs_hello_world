#include "include.h"
#include "utils.h"
#include "process.h"
void Process::on_completion(struct ibv_wc *wc){
    Connection *conn = reinterpret_cast<Connection*>(reinterpret_cast<uintptr_t>(wc->wr_id));
    if (wc->status)
        resolve_wc_error(wc);
    if (wc->opcode == IBV_WC_RECV){
        std::cout<<"\x1b[31m\tWC:OPCODE:RECV\x1b[0m"<<std::endl;
        if(mode_of_operation == MODE_SEND_RECEIVE){
            double dt = end_time_keeping(&__time);
            printf("\t\tTIME:DATATIME: %.8f mus\n", dt);
            char msg[100];
            sprintf(msg, "DATATIME:%0.15f", dt);
            logevent(this->logfilename, msg);
            on_completion_wc_recv(wc);
        }
        else{
            conn->recv_state++;
            if(conn->recv_message->type == MSG_MR){
                memcpy(&conn->remote_memory_region, &conn->recv_message->data.mr, sizeof(conn->remote_memory_region));
                post_recv(conn); 
                if (conn->send_state == SS_INIT){
                    send_memory_region(conn);
                }
            }
        }
        this->number_of_recvs++;
        conn->number_of_recvs++;
    }
    else if (wc->opcode == IBV_WC_SEND){
        std::cout<<"\x1b[31m\tWC:OPCODE:SEND\x1b[0m"<<std::endl;
        if(mode_of_operation == MODE_SEND_RECEIVE){
            on_completion_wc_send(wc);
            start_time_keeping(&__time);
        }
        else{
            conn->send_state++;
        }

        conn->number_of_sends++;
        if(disconnect && !client && conn->send_state == SS_DONE_SENT){
            ;
        }
    }
    else if (wc->opcode == IBV_WC_COMP_SWAP){ 
        std::cout<<"\x1b[31m\tWC:OPCODE:COMP_SWAP\x1b[0m"<<std::endl;
        on_completion_not_implemented(wc);
    }
    else if (wc->opcode == IBV_WC_FETCH_ADD){
        std::cout<<"\x1b[31m\tWC:OPCODE:FETCH_ADD\x1b[0m"<<std::endl;
        on_completion_not_implemented(wc);
    }
    else if (wc->opcode == IBV_WC_BIND_MW){
        std::cout<<"\x1b[31m\tWC:OPCODE:BIND_MW\x1b[0m"<<std::endl;
        on_completion_not_implemented(wc);
    }
    else if (wc->opcode == IBV_WC_RDMA_WRITE){
        std::cout<<"\x1b[31m\tWC:OPCODE:RDMA_WRITE\x1b[0m"<<std::endl;
        if(mode_of_operation == MODE_RDMA_WRITE){
            double dt = end_time_keeping(&__time);
            printf("\t\tTIME:DATATIME: %.8f mus\n", dt);
            char msg[100];
            sprintf(msg, "DATATIME:%0.15f", dt);
            logevent(this->logfilename, msg);
            conn->send_state++;
        }
    }
    else if (wc->opcode == IBV_WC_RDMA_READ){
        std::cout<<"\x1b[31m\tWC:OPCODE:RDMA_READ\x1b[0m"<<std::endl;
        if(mode_of_operation == MODE_RDMA_READ){
            double dt = end_time_keeping(&__time);
            printf("\t\tTIME:DATATIME: %.8f mus\n", dt);
            char msg[100];
            sprintf(msg, "DATATIME:%0.15f", dt);
            logevent(this->logfilename, msg);
            conn->send_state++;
        }
    }
    else if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM){
        std::cout<<"\x1b[31m\tWC:OPCODE:RECV_RDMA_WITH_IMM\x1b[0m"<<std::endl;
        on_completion_not_implemented(wc);
    }
    else
        die("wc->opcode not recognised.");
    if(mode_of_operation != MODE_SEND_RECEIVE && !wc->status){
        if (conn->send_state == SS_MR_SENT && conn->recv_state == RS_MR_RECV) {
            struct ibv_send_wr wr, *bad_wr = nullptr;
            struct ibv_sge sge;
            if (mode_of_operation == MODE_RDMA_WRITE){
                printf("\t\tWRITING RDMA\n");
            }
            else{
                printf("\t\tREADING RDMA\n");
            }
			set_send_work_request_attributes(&wr, &sge, conn);
			set_send_scatter_gather_attributes(&sge, conn);
			ibv_post_send(conn->queue_pair, &wr, &bad_wr);
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
            printf("\tRECEIVED VERIFICATION DISCONNECTING\n");
            rdma_disconnect(conn->identifier);
        }
    }

    if((conn->number_of_recvs == max_number_of_recvs) &&(conn->number_of_sends == max_number_of_sends) && client && (mode_of_operation == MODE_SEND_RECEIVE)){
        printf("Calling disconnect\n");
        // this is not safe will result is seg fault for large
        // message sizes as the connection will terminate before
        // the verification of the send message
        rdma_disconnect(conn->identifier);
    }
}

void Process::on_completion_wc_recv(struct ibv_wc *wc){
    Connection *conn = reinterpret_cast<Connection*>(reinterpret_cast<uintptr_t>(wc->wr_id));   
    struct message_numerical recv_message;
    assert(conn->recv_region !=nullptr);
    recv_message.x = conn->recv_region;
    recv_message.size = message.size;
    int sum = 0;
    memcpy(&sum, recv_message.x + (message.size-4)*sizeof(char), sizeof(int));
    verify_message_numerical(&recv_message);
}


void Process::on_completion_wc_send(struct ibv_wc *wc){
    ;
}

void Process::on_completion_not_implemented(struct ibv_wc *wc){
    printf("ERROR:ON COMPLETION OF THIS REQUEST IS NOT IMPLEMENTED\n");
    exit(EXIT_FAILURE);
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
