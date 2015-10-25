#ifndef BASE_UTILS_H
#define BASE_UTILS_H
#include "include.h"

void calc_message_numerical(struct message_numerical *msg);
void verify_message_numerical(struct message_numerical *msg);

template <class T>
void memsetzero(T *x){
	memset(x, 0, sizeof(*x));
}

void resolve_wc_error(ibv_wc_status status);

void logevent(std::string logfilename, char* msg);

struct benchmark_time{
	struct timespec tstart, tend;
};

inline void start_time_keeping(struct benchmark_time *btime){
	btime->tstart={0,0};
	btime->tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &btime->tstart);
}

inline double end_time_keeping(struct benchmark_time *btime){
	double dt;
	clock_gettime(CLOCK_MONOTONIC, &btime->tend);
	dt = ((double)btime->tend.tv_sec + 1.0e-9*btime->tend.tv_nsec) - 
		((double)btime->tstart.tv_sec + 1.0e-9*btime->tstart.tv_nsec);
	return dt;
}
#endif
