#include "include.h"
#include "utils.h"
#include "process.h"
void Process::set_as_client(){
	this->client = 1;
}


void Process::set_log_filename(){
	if(client)
		logfilename = "client.log";
	else
		logfilename = "server.log";
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
	this->mode_of_operation = m;
}

void Process::set_max_recv(int n){
	max_number_of_recvs = n;
}

void Process::set_max_send(int n){
	max_number_of_sends = n;
}

