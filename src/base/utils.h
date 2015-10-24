#ifndef BASE_UTILS_H
#define BASE_UTILS_H
#include <random>
#include <cassert>
#include "utils.h"
#include "process.h"


void calc_message_numerical(struct message_numerical *msg);
void verify_message_numerical(struct message_numerical *msg);

template <class T>
T sum(T a, T b)
{
  return a+b;
}

template <class T>
void memsetzero(T *x){
	memset(x, 0, sizeof(*x));
}

void resolve_wc_error(ibv_wc_status status);
#endif
