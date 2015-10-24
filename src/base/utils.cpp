#include "include.h"
#include "utils.h"
#include "process.h"

void calc_message_numerical(struct message_numerical *msg){
	std::random_device rd;
	std::uniform_int_distribution<int> dist(1,127);
	int size = MESSAGE_SIZE;
	int sum = 0;
	for(int i=0; i<size-4; i++){
		msg->x[i] = dist(rd);
		sum += msg->x[i];
	}

	memcpy(&msg->x[size-4], &sum, sizeof(int));
	std::cout<<"Message generated, sum "<<sum<<std::endl;
}

void verify_message_numerical(struct message_numerical *msg){
	std::cout<<"SANITY CHECK...";

	int size = MESSAGE_SIZE;
	int sum = 0;
	int sum_orig; 
	for(int i=0; i<size-4; i++){
		sum += msg->x[i];
	}
	memcpy(&sum_orig,  &msg->x[size-4], sizeof(int));
	
	assert(sum_orig == sum);
	std::cout<<"PASSED."<<std::endl;
}


void resolve_wc_error(ibv_wc_status status){
	std::map<int, std::string> wc_status_map;
	wc_status_map[0] = "IBV_WC_SUCCESS";
	wc_status_map[1] = "IBV_WC_LOC_LEN_ERR";
	wc_status_map[2] = "IBV_WC_LOC_QP_OP_ERR";
	wc_status_map[3] = "IBV_WC_LOC_EEC_OP_ERR";
	wc_status_map[4] = "IBV_WC_LOC_PROT_ERR";
	wc_status_map[5] = "IBV_WC_WR_FLUSH_ERR";
	wc_status_map[6] = "IBV_WC_MW_BIND_ERR";
	wc_status_map[7] = "IBV_WC_BAD_RESP_ERR";
	wc_status_map[8] = "IBV_WC_LOC_ACCESS_ERR";
	wc_status_map[9] = "IBV_WC_REM_INV_REQ_ERR";
	wc_status_map[10] = "IBV_WC_REM_ACCESS_ERR";
	wc_status_map[11] = "IBV_WC_REM_OP_ERR";
	wc_status_map[12] = "IBV_WC_RETRY_EXC_ERR";
	wc_status_map[13] = "IBV_WC_RNR_RETRY_EXC_ERR";
	wc_status_map[14] = "IBV_WC_LOC_RDD_VIOL_ERR";
	wc_status_map[15] = "IBV_WC_REM_INV_RD_REQ_ERR";
	wc_status_map[16] = "IBV_WC_REM_ABORT_ERR";
	wc_status_map[17] = "IBV_WC_INV_EECN_ERR";
	wc_status_map[18] = "IBV_WC_INV_EEC_STATE_ERR";
	wc_status_map[19] = "IBV_WC_FATAL_ERR";
	wc_status_map[20] = "IBV_WC_RESP_TIMEOUT_ERR";
	wc_status_map[21] = "IBV_WC_GENERAL_ERR";
	std::cout<<"WC ERROR:"<<wc_status_map[(int)status]<<std::endl;
	exit(EXIT_FAILURE);
}
