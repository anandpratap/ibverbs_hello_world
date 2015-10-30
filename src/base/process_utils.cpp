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


