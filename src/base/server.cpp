#include "process.h"
int main(int argc, char **argv){
	Process server;
	if (argc != 2){
		die("usage: server mode");
	}
	int mode = atoi(argv[1]);
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
