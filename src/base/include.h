#ifndef BASE_INCLUDE_H
#define BASE_INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <cassert>
#include <string>
#include <boost/lexical_cast.hpp>

#include <rdma/rdma_cma.h>
#include <arpa/inet.h>
#include <netdb.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


#define TIMEOUT_IN_MS 50
/*
  8
  128
  2048
  32768
  524288
  8388608
  134217728
  2147483648
*/
#define DEFAULT_PORT 48105
inline void die(const char *reason){
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)


#endif
