#include "include.h"
#include "utils.h"
#include "process.h"

Process::Process(){
	this->client = 0;

	this->s_ctx = NULL;
	this->connection_ = NULL;
	this->event_channel = NULL;
	connection_identifier = NULL;
	this-> addressi = NULL;
}
Process::~Process(){
	// rdma_destroy_id(connection_identifier);
	// rdma_destroy_event_channel(event_channel);	
}
void Process::go(){
	// set port for the server
	memsetzero(&address);
	address.sin6_family = AF_INET6;
	address.sin6_port = htons(DEFAULT_PORT);
	
	// create event channel, connection identifier, bind that to the addressess, 
	// and start listening
	event_channel = rdma_create_event_channel();
	rdma_create_id(event_channel, &connection_identifier, NULL, RDMA_PS_TCP);
	rdma_bind_addr(connection_identifier, (struct sockaddr *)&address);
	rdma_listen(connection_identifier, 10);
	port = ntohs(rdma_get_src_port(connection_identifier));

	std::cout<<"SERVER:PORT "<<this->port<<std::endl;
	
	event_loop();

}
