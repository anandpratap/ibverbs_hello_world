#include "process.h"
int main(int argc, char **argv){
	Process server;
	if (argc != 3){
		die("usage: server mode msgsize");
	}
	int mode = atoi(argv[1]);
	unsigned int message_size = boost::lexical_cast<unsigned int>(argv[2]);
	server.set_message_size(message_size);
	if(mode == 0)
		server.set_mode_of_operation(MODE_SEND_RECEIVE);
	else if(mode == 1)
		server.set_mode_of_operation(MODE_RDMA_WRITE);
	else if(mode == 2)
		server.set_mode_of_operation(MODE_RDMA_READ);
	else
			std::cout<<"Wrong mode of operation"<<std::endl;
	server.run();
	return 0;
}
