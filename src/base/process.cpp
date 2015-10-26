#include "include.h"
#include "utils.h"
#include "process.h" 
struct benchmark_time __time;
Process::Process(){
}
Process::~Process(){
	rdma_destroy_id(connection_identifier);
	rdma_destroy_event_channel(event_channel);	
}

void Process::initialize(){
	set_log_filename();
}

void Process::run(){
	initialize();
	if(client)
		get_server_address_from_string();
	else
		get_self_address();
	connect();
	event_loop();
}
