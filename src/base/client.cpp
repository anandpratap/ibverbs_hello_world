#include "include.h"
#include "utils.h"
#include "process.h"

int main(int argc, char** argv){
	Process client;
	
	if(argc != 3){
		die("usage: client <server-ip> mode");
	}
	
	int mode = atoi(argv[2]);
	if(mode == 0)
		client.set_mode_of_operation(MODE_SEND_RECEIVE);
	else if(mode == 1)
		client.set_mode_of_operation(MODE_RDMA_WRITE);
	else if(mode == 2)
		client.set_mode_of_operation(MODE_RDMA_READ);
	else
		std::cout<<"Wrong mode of operation"<<std::endl;
	client.set_as_client();
	client.set_ip_string(argv[1]);
	
	struct benchmark_time btime;
	start_time_keeping(&btime);
	
	client.run();
	
	double dt = end_time_keeping(&btime);
	char msg[100];
	sprintf(msg, "RUNTIME:%0.8f", dt);
	printf("%s\n", msg);
	logevent(client.logfilename, msg);
	return 0;
}
