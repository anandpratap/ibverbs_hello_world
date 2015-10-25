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

