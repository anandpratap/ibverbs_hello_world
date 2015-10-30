#include "include.h"
#include "utils.h"
#include "process.h"
// Special behavior for ++sstate
sstate& operator++( sstate &c ) {
	using IntType = typename std::underlying_type<sstate>::type;
	c = static_cast<sstate>( static_cast<IntType>(c) + 1 );
	if ( c == sstate::END_OF_LIST )
		c = static_cast<sstate>(0);
	return c;
}

// Special behavior for sstate++
sstate operator++( sstate &c, int ) {
	sstate result = c;
	++c;
	return result;
}

// Special behavior for ++rstate
rstate& operator++( rstate &c ) {
	using IntType = typename std::underlying_type<rstate>::type;
	c = static_cast<rstate>( static_cast<IntType>(c) + 1 );
	if ( c == rstate::END_OF_LISTT )
		c = static_cast<rstate>(0);
	return c;
}

// Special behavior for rstate++
rstate operator++( rstate &c, int ) {
	rstate result = c;
	++c;
	return result;
}

void calc_message_numerical(struct message_numerical *message){
	std::random_device rd;
	std::uniform_int_distribution<int> dist(1,127);
	int size = message->size;
	int sum = 0;
	for(int i=0; i<size-4; i++){
		message->x[i] = dist(rd);
		sum += message->x[i];
	}
	memcpy(message->x+(message->size-4)*sizeof(char), &sum, sizeof(int));
	printf("\t\tMESSAGE GENERATED:SUM %i\n", sum);
}

void verify_message_numerical(struct message_numerical *message){
	printf("\t\tSANITY CHECK...");
	int size = message->size;
	int sum = 0;
	int sum_orig; 

	for(int i=0; i<size-4; i++){
		sum += message->x[i];
	}
	memcpy(&sum_orig,  message->x+(message->size-4)*sizeof(char), sizeof(int));
	
	assert(sum_orig == sum);
	printf("PASSED!\n");
}


void resolve_wc_error(ibv_wc *wc){
	ibv_wc_status status = wc->status;
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
	std::cout<<"WC ERROR:"<<wc->opcode<<std::endl;

	exit(EXIT_FAILURE);
}


void logevent(std::string logfilename, char* msg){
	std::ofstream logfile;
	logfile.open(logfilename, std::ios::app);
	logfile<<msg<<std::endl;
	logfile.close();
}
