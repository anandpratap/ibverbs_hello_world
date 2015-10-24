#include "include.h"
#include "utils.h"
#include "process.h"

class ClientProcess: public Process{
public:
	void go();
};

void ClientProcess::go(){
	client = 1;
	// set port for the server
	getaddrinfo(DEFAULT_ADDRESS, DEFAULT_PORT_S, NULL, &this->addressi);
	
	// create event channel, connection identifier, and post resolve addressess 
	event_channel = rdma_create_event_channel();
	rdma_create_id(event_channel, &connection_identifier, NULL, RDMA_PS_TCP);
	rdma_resolve_addr(connection_identifier, NULL, addressi->ai_addr, TIMEOUT_IN_MS);
	
  	// free addressess info
	freeaddrinfo(this->addressi);
	
	std::cout<<"CLIENT:PORT "<<DEFAULT_PORT_S<<std::endl;
	
	event_loop();
}

int main(void){
	ClientProcess proc;
	proc.go();
	return 0;
}
