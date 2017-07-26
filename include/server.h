#ifndef __server_h_
#define __server_h_

#include "config.h"
#include "common.h"

/* server loop */
int server_run_loop();

/* get the address */
void *get_in_addr(struct sockaddr *sa);

#endif
