#ifndef UTILS_H
#define UTILS_H
#include <random>
#include <cassert>
#include "shared.h"


void calc_message_numerical(struct message_numerical *msg);
void verify_message_numerical(struct message_numerical *msg);

template <class SomeType>
SomeType sum (SomeType a, SomeType b)
{
  return a+b;
}

template <class T>
void memsetzero(T *x){
	memset(x, 0, sizeof(*x));
}

#endif
