#ifndef __common_h_
#define __common_h_

#include "config.h"

/* Check the program was started as root */
extern bool rootCheck();

/* send the stop signal to a socket */
extern int send_stop(int sockfd);

/* return a socket based on config */
extern int get_socket();

/* Open a device file descriptor */
extern int openDeviceFile(char *deviceFile);

/* convert to uppercase */
extern void stoupper(char s[]);

/* Logging */
int write_log(const char *file, const char *function, int line, const char *format, ...);

#endif
