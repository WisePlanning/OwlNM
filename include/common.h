#ifndef __common_h_
#define __common_h_

#include "config.h"

// void set_addr(struct sockaddr_in6 *addr);

// bool socket_connect(struct sockaddr_in6 addr, int sockfd);

/* set socket to non-blocking */
extern void set_nonblocking(int socket);

/* Reset a timer */
extern void reset_timer(struct timeval *tv);

/* Check the program was started as root */
extern bool rootCheck();

/* send a start signal to a socket */
extern int send_start(int sockfd);

/* send the stop signal to a socket */
extern int send_stop(int sockfd);

/* return a socket based on config */
extern int get_socket();

/* Open a device file descriptor */
extern int openDeviceFile(char *deviceFile);

/* convert to uppercase */
extern void stoupper(char s[]);

/* Convert to lowercase */
extern void stolower(char s[]);

#endif
