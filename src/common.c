#include "common.h"
#include <time.h>
/**
 * Get a socket descriptor
 * @method get_socket
 * @return file discriptors
 */
int get_socket() {

	struct addrinfo *res;
	struct addrinfo hints, *p;
	int status;
	int sockfd;

	/* clear the memory */
	memset(&hints, 0, sizeof(hints));

	/* IPV4 or IPV6 */
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(conf->server_address, conf->port, &hints, &res)) != 0) {
		logging(__FILE__,__FUNCTION__,__LINE__,"ERROR: getaddrinfo");
			if (conf->log_fd){
				fprintf(conf->log_fd,"%s\n", strerror(errno));
			}
		return -1;
	}

	/* loop through all the results and connect to the first we can */
	for (p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			logging(__FILE__,__FUNCTION__,__LINE__,"ERROR: socket");
				if (conf->log_fd){
				fprintf(conf->log_fd,"%s\n", strerror(errno));
			}
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			logging(__FILE__,__FUNCTION__,__LINE__,"ERROR: connect");
				if (conf->log_fd){
				fprintf(conf->log_fd,"%s\n", strerror(errno));
			}
			close(sockfd);
			continue;
		}
		break; // if we get here, we must have connected successfully
	}

	freeaddrinfo(res); // free the linked list

	return (sockfd > 0 ? sockfd : -1);
}

/**
 * Set the socket to non-blocking
 * @method set_nonblocking
 * @param  socket          [socket file descriptor]
 */
void set_nonblocking(int socket) {
	int flags;

	flags = fcntl(socket, F_GETFL, 0);
	if (flags != -1) {
		fcntl(socket, F_SETFL, flags | O_NONBLOCK);
	}
}

/**
 * Provide a reset timer
 * @method reset_timer
 * @param  tv          [description]
 */
void reset_timer(struct timeval *tv) {

	logging(__FILE__,__FUNCTION__,__LINE__,"Resetting Timer");

	tv->tv_sec = conf->timeout;
	tv->tv_usec = UTIMEOUT;

}

void logging(const char *file, const char *func, int line, const char *message) {

	time_t current_time;
	struct tm *struct_time;

	time( &current_time);

	struct_time = gmtime( &current_time);

	if (conf->log_fd) {
		fprintf(conf->log_fd,"%d-%02d-%d %02d-%02d-%02d file:%s %s:%i  :%s",
		struct_time->tm_year+1900,
		struct_time->tm_mon+1,
		struct_time->tm_mday,
		struct_time->tm_hour,
		struct_time->tm_min,
		struct_time->tm_sec,
		file,
		func,
		line,
		message);

	fflush(conf->log_fd);
}

	if (conf->verbose) {
		fprintf(stdout,"%d-%02d-%d %02d-%02d-%02d file:%s %s:%i  :%s",
		struct_time->tm_year+1900,
		struct_time->tm_mon+1,
		struct_time->tm_mday,
		struct_time->tm_hour,
		struct_time->tm_min,
		struct_time->tm_sec,
		file,
		func,
		line,
		message);
	}
}

/* Root is required to capture device input */
bool rootCheck() {

	logging(__FILE__,__FUNCTION__,__LINE__,"Checking for root permissions\n");

	if (geteuid() != 0)
	{
		return FALSE;
	}
	return TRUE;
}

/**
 * [get_ip_str description]
 * @method get_ip_str
 * @param  sa         [description]
 * @param  s          [description]
 * @param  maxlen     [description]
 * @return            [description]
 */
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
	switch (sa->sa_family) {
		case AF_INET:
			inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
			break;

		default:
			strncpy(s, "Unknown AF", maxlen);
			return NULL;
	}

	return s;
}

/**
 * Send the stop signal to the server
 * @method stop_video
 * @param  addr       [description]
 * @param  sockfd     [description]
 * @return            [description]
 */
int send_stop(int sockfd) {
	int ret;

	if (conf->verbose)
		printf("Sending stop to server\n");

	ret = send(sockfd, STOP, sizeof(STOP), 0);

	#ifdef HAVE_WIRINGPI
	// switch gpio pin to disable relay


	logging(__FILE__,__FUNCTION__,__LINE__,"LED OFF\n");

	digitalWrite(LED, OFF);

	#endif

	if (ret < 0) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Error sending data!\n\t-STOP");
	} else {
		logging(__FILE__,__FUNCTION__,__LINE__,"Success\n");
	}

	return (ret);
}

/**
 * Send the start signal to the server
 * @method start_video
 * @param  addr        [description]
 * @param  sockfd      [description]
 * @return             [description]
 */
int send_start(int sockfd) {
	int ret;

	logging(__FILE__,__FUNCTION__,__LINE__,"Sending start to server\n");

	ret = send(sockfd, PLAY, sizeof(PLAY), 0);

	#ifdef HAVE_WIRINGPI

	// switch gpio pin to enable relay
	digitalWrite(LED, ON);

	logging(__FILE__,__FUNCTION__,__LINE__,"LED ON\n");

	#endif /* HAVE_WIRINGPI */

	if (ret < 0) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Error sending data!\n\t-PLAY\n");
	} else {
		logging(__FILE__,__FUNCTION__,__LINE__,"success...\n");
	}

	return (ret);
}

/**
 * Opens the keyboard device file
 *
 * @param  deviceFile   the path to the keyboard device file
 * @return              the file descriptor on success, error code on failure
 */
int openDeviceFile(char *deviceFile) {
	int dev_fd = open(deviceFile, O_RDONLY | O_NONBLOCK);

	if (dev_fd == -1) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Could not get device : ");
		if (conf->log_fd) { fprintf(conf->log_fd,"%s\n", strerror(errno)); }
		return 0;
	}

	return (dev_fd);
} /* openKeyboardDeviceFile */

/**
 * convert case to upper inplace
 * @method stoupper
 * @param  s
 */
void stoupper(char s[]) {
	int c = 0;

	while (s[c] != '\0') {
		if (s[c] >= 'a' && s[c] <= 'z') {
			s[c] = s[c] - 32;
		}
		++c;
	}
}

/**
 * convert case to lower inplace
 * @method stolower
 * @param  s
 */
void stolower(char s[]) {
	int c = 0;

	while (s[c] != '\0') {
		if (s[c] >= 'A' && s[c] <= 'Z') {
			s[c] = s[c] + 32;
		}
		++c;
	}
}
