#include "include.h"
#include "utils.h"
#include "process.h"

int main(int argc, char** argv){
	Process client;
	
	if(argc != 4){
		die("usage: client <server-ip> mode msgsize");
	}
	
	int mode = atoi(argv[2]);
	unsigned int message_size = boost::lexical_cast<unsigned int>(argv[3]);
	client.set_message_size(message_size);
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
