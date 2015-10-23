#include "utils.h"
void calc_message_numerical(struct message_numerical *msg){
	std::random_device rd;
	std::uniform_int_distribution<int> dist(1,127);
	int size = MESSAGE_SIZE;
	int sum = 0;
	for(int i=0; i<size-4; i++){
		msg->x[i] = dist(rd);
		sum += msg->x[i];
	}

	memcpy(&msg->x[size-4], &sum, sizeof(int));
	std::cout<<"Message generated, sum "<<sum<<std::endl;
}

void verify_message_numerical(struct message_numerical *msg){
	std::cout<<"SANITY CHECK...";

	int size = MESSAGE_SIZE;
	int sum = 0;
	int sum_orig; 
	for(int i=0; i<size-4; i++){
		sum += msg->x[i];
	}
	memcpy(&sum_orig,  &msg->x[size-4], sizeof(int));
	
	assert(sum_orig == sum);
	std::cout<<"PASSED."<<std::endl;
}
