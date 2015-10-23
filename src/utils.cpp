#include "utils.h"
void calc_message_numerical(struct message_numerical *msg){
	std::default_random_engine de(time(0)); 
	std::normal_distribution<double> distribution(0.0, 1.0);
	int size = MESSAGE_SIZE;
	char sum = 0;
	for(int i=0; i<size-1; i++){
		msg->x[i] = distribution(de);
		sum += msg->x[i];
	}
	msg->x[size-1] = sum;
	std::cout<<"Message generated, sum "<<sum<<std::endl;
}

void verify_message_numerical(struct message_numerical *msg){
	std::cout<<"SANITY CHECK...";

	int size = MESSAGE_SIZE;
	char sum = 0;
	for(int i=0; i<size-1; i++){
		sum += msg->x[i];
	}
	assert(msg->x[size-1] == sum);
	std::cout<<"PASSED."<<std::endl;
}
