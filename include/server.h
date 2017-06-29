#ifndef __server_h_
#define __server_h_

#include "common.h"
#include "config.h"

/* server loop */
int server_run_loop();

void *get_in_addr(struct sockaddr *sa);

#endif
