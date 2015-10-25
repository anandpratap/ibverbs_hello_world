#include "include.h"
#include "utils.h"
#include "process.h"

int main(int argc, char** argv){
	if (argc != 2){
		die("usage: client <server-ip>");
	}
	Process client;
	client.set_as_client();
	client.set_ip_string(argv[1]);
	client.run();
	return 0;
}
