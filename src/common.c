#include "common.h"

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
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	/* loop through all the results and connect to the first we can */
	for (p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
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
	struct timeval new;

	new.tv_sec = conf->timeout;
	new.tv_usec = UTIMEOUT;

	*tv = new;
}

/* Root is required to capture device input */
bool rootCheck() {
	if (geteuid() != 0) {
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
		puts("Sending stop to server\n");

	ret = send(sockfd, STOP, sizeof(STOP), 0);

	#ifdef HAVE_WIRINGPI
	// switch gpio pin to disable relay
	digitalWrite(LED, OFF);
	#endif

	if (ret < 0) {

		printf("Error sending data!\n\t-%s", PLAY);

	} else {

		if (conf->verbose)
			puts("success...");
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

	if (conf->verbose)
		puts("Sending start to server\n");

	ret = send(sockfd, PLAY, sizeof(PLAY), 0);

	#ifdef HAVE_WIRINGPI
	// switch gpio pin to enable relay
	digitalWrite(LED, ON);
	#endif

	if (ret < 0) {

		printf("Error sending data!\n\t-%s", PLAY);

	} else {

		if (conf->verbose)
			puts("success...");
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
	int dev_fd = open(deviceFile, O_RDONLY);

	if (dev_fd == -1) {
		perror("Could not get device\n");
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
