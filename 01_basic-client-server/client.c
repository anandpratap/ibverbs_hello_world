#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include "shared_ib.h"
#include "globals_ib.h"

const int TIMEOUT_IN_MS = 500; /* ms */
struct context *s_ctx = NULL;
const int ifclient = 1;
int main(int argc, char **argv)
{
	struct addrinfo *addr;
	struct rdma_cm_event *event = NULL;
	struct rdma_cm_id *conn= NULL;
	struct rdma_event_channel *ec = NULL;

	if (argc != 3)
		die("usage: client <server-address> <server-port>");
  
	TEST_NZ(getaddrinfo(argv[1], argv[2], NULL, &addr));
  
	TEST_Z(ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_resolve_addr(conn, NULL, addr->ai_addr, TIMEOUT_IN_MS));
  
	freeaddrinfo(addr);
  
	while (rdma_get_cm_event(ec, &event) == 0) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);
    
		if (on_event(&event_copy))
			break;
	}
  
	rdma_destroy_event_channel(ec);

	return 0;
}


