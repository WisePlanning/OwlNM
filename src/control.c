#include "control.h"

#define EVLOOP_NO_EXIT_ON_EMPTY 0x04
typedef struct server {
	/* The clients socket. */
	int fd;

	/* The bufferedevent for this client. */
	struct bufferevent *buf_ev;
} Server;
/**
 * Get the device for sensor input
 * @method getSensirDeviceFileName
 * @param  position (1|2)
 * @return
 */
char *getSensorDeviceFileName(int position) {

	char buff[20];

	/* get the devicenames of sensors */
	static const char *command = "grep -E 'Handlers|EV' /proc/bus/input/devices |"
	                             "grep -B1 120013 |" // Keyboard identifier
	                             //  "grep -B1 100013 |"  // Sensor identifier
	                             "grep -Eo event[0-9]+ |"
	                             "tr -d '\n'";

	/* device path prefix */
	char path_prefix[20] = "/dev/input/";

	/* Sensor device names */
	char sensor_device_1[10];
	char sensor_device_2[10];

	/* Run the command */
	FILE *cmd = popen(command, "r");

	if (cmd == NULL) {
		logging(__FILE__, __FUNCTION__, __LINE__, "Could not determine sensor device file");
	}
	/* zero the string */
	memset(&buff, 0, 20);

	/* zero the string */
	memset(&sensor_device_1, 0, 10);

	/* zero the string */
	memset(&sensor_device_2, 0, 10);

	/* Read back the info from the command */
	fgets(buff, 20, cmd);

	/* Close the file */
	pclose(cmd);

	if (strlen(buff) > 0) {
		// Assumes the length of the device name is 6 chars long
		// copy the first device name
		strncpy(sensor_device_1, buff, 6);
		logging(__FILE__, __FUNCTION__, __LINE__, sensor_device_1);

		// copy the second device name
		strncpy(sensor_device_2, &buff[6], 6);
		logging(__FILE__, __FUNCTION__, __LINE__, sensor_device_2);
	} else {
		return 0;
	}

	/* return the sensor device */
	if (position == 1) {
		return (strdup(strcat(path_prefix, sensor_device_1)));
	} else if (position == 2) {
		return (strdup(strcat(path_prefix, sensor_device_2)));
	}

	return 0;
}

/**
 * Called on when there is data to read on the sensor
 * @param void*
 * @param bufferevent*
 * @return void
 */
void sensor1_read_callback(evutil_socket_t fd, short what, void *arg) {
	Server *tmp = (Server *)arg;

	if (what & EV_TIMEOUT) {

		sensor1_timedout = TRUE;

		if (sensor2_timedout && playing) {

			logging(__FILE__, __FUNCTION__, __LINE__, "Sending Stop");

			evbuffer_add_printf(bufferevent_get_output(tmp->buf_ev), "%s", STOP);

			#ifdef HAVE_WIRINGPI
			// switch gpio pin to disable relay

			logging(__FILE__, __FUNCTION__, __LINE__, "LED OFF");

			digitalWrite(LED, OFF);

			#endif

			playing = FALSE;
		}

		return;
	}

	sensor1_timedout = FALSE;
	int ret;
	input_event event;
	if ((ret = read(fd, &event, sizeof(input_event))) > 0) { // read from it
		if (event.type == EV_KEY) {
			if (event.value == KEY_PRESS) {

				char *name = getKeyText(event.code, 0);

				if (strcmp(name, UNKNOWN_KEY) != 0) {


					if (!playing) {
						logging(__FILE__, __FUNCTION__, __LINE__, "Sending Play");

						evbuffer_add_printf(bufferevent_get_output(tmp->buf_ev), "%s", PLAY);

						#ifdef HAVE_WIRINGPI
						// switch gpio pin to disable relay

						logging(__FILE__, __FUNCTION__, __LINE__, "LED ON");

						digitalWrite(LED, ON);

						#endif

						playing = TRUE;
					}
				}
			}
		}
	}
}

/**
 * Called on when there is data to read on the sensor
 * @param void*
 * @param bufferevent*
 * @return void
 */
void sensor2_read_callback(evutil_socket_t fd, short what, void *arg) {
	Server *tmp = (Server *)arg;
	if (what & EV_TIMEOUT) {

		sensor2_timedout = TRUE;

		if (sensor1_timedout && playing) {
			logging(__FILE__, __FUNCTION__, __LINE__, "Sending Stop");

			evbuffer_add_printf(bufferevent_get_output(tmp->buf_ev), "%s", STOP);

			#ifdef HAVE_WIRINGPI
			// switch gpio pin to disable relay

			logging(__FILE__, __FUNCTION__, __LINE__, "LED OFF");

			digitalWrite(LED, OFF);

			#endif

			playing = FALSE;
		}

		return;
	}

	sensor2_timedout = FALSE;

	int ret;
	input_event event;
	if ((ret = read(fd, &event, sizeof(input_event))) > 0) { // read from it
		if (event.type == EV_KEY) {
			if (event.value == KEY_PRESS) {

				char *name = getKeyText(event.code, 0);

				if (strcmp(name, UNKNOWN_KEY) != 0) {

					if (!playing) {

						logging(__FILE__, __FUNCTION__, __LINE__, "Sending PLAY");

						evbuffer_add_printf(bufferevent_get_output(tmp->buf_ev), "%s", PLAY);

						#ifdef HAVE_WIRINGPI
						// switch gpio pin to disable relay

						logging(__FILE__, __FUNCTION__, __LINE__, "LED ON");

						digitalWrite(LED, ON);

						#endif

						playing = TRUE;
					}
				}
			}
		}
	}
}


/**
 * Called on socket event
 * @param void*
 * @param short
 * @param bufferevent*
 * @return void
 */
void control_event_callback(struct bufferevent *bev, short events, void *ctx) {
	fprintf(stdout, "event");
	fflush(stdout);
	if (events & BEV_EVENT_CONNECTED) {

		logging(__FILE__, __FUNCTION__, __LINE__, "Connected");

	} else if (events & BEV_EVENT_EOF) {

		logging(__FILE__, __FUNCTION__, __LINE__, "Connection closed.");

		sleep(5);

		/* Exit the current loop */
		event_base_loopbreak(ctx);

		/* Start again */
		control_run_loop();

	} else if (events & BEV_EVENT_ERROR) {

		logging(__FILE__, __FUNCTION__, __LINE__, "Got an error on the connection :");
		fprintf(conf->log_fd, "%s", strerror(errno));

		sleep(5);

		/* Exit the current loop */
		event_base_loopbreak(ctx);

		/* Start again */
		control_run_loop();
	}else if (events & BEV_EVENT_TIMEOUT) {
		logging(__FILE__, __FUNCTION__, __LINE__, "Timeout : Server connection");
	}
}


/**
 * The loop for clients
 * @return
 */
int control_run_loop() {

	logging(__FILE__, __FUNCTION__, __LINE__, "Starting Control - LibEvent");

	log_config(conf);

	sensor1_timedout = TRUE;
	sensor2_timedout = TRUE;
	playing = FALSE;

	Server *server = malloc(sizeof(Server));
	struct event_base *base;

	/* Sensor 1 */
	int sensor_device_1 = 0;
	struct event *sens1 = NULL;

	/* Sensor 2 */
	int sensor_device_2 = 0;
	struct event *sens2 = NULL;

	/* Server connection */
	struct bufferevent *bev = NULL;

	struct timeval event_timer = {conf->timeout, 0};

	base = NULL;

	/* Get the keyboard file descriptos */
	logging(__FILE__, __FUNCTION__, __LINE__, "Getting sensor 1");

	sensor_device_1 = openDeviceFile(getSensorDeviceFileName(1));

	logging(__FILE__, __FUNCTION__, __LINE__, "Sensor 1 = ");

	if (conf->log_fd) {
		fprintf(conf->log_fd, "%d", sensor_device_1);
	}

/* Get the keyboard file descriptos */
logging(__FILE__,__FUNCTION__,__LINE__,"Getting sensor2 ");
sensor_device_2 = openDeviceFile(getSensorDeviceFileName(2));

logging(__FILE__,__FUNCTION__,__LINE__,"Sensor 2 = ");
if (conf->log_fd) {fprintf(conf->log_fd, "%d", sensor_device_2);}

if (!(sensor_device_1 > 0) || !(sensor_device_2 > 0)) {
 logging(__FILE__,__FUNCTION__,__LINE__, "Could not get sensors : ");
 if (conf->log_fd) {fprintf(conf->log_fd, "%s", strerror(errno));}
 if (conf->log_fd) {fclose(conf->log_fd);}
 exit(EXIT_FAILURE);
}

#ifdef HAVE_WIRINGPI
	if (wiringPiSetupGpio() == -1) {
		logging(__FILE__, __FUNCTION__, __LINE__, "Could not open GPIO");
		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}
	// set the pin to output
	pinMode(LED, OUTPUT);

	logging(__FILE__, __FUNCTION__, __LINE__, "LED OFF");

	// switch gpio pin to disable relay
	digitalWrite(LED, OFF);
#endif

// if there is no server address,
#ifdef HAVE_AVAHI
	if (conf->avahi || NULL == conf->server_address) {
		do {
			avahi_client();
		} while (NULL == conf->server_address);
	}
#endif

	if (NULL == conf->server_address) {
		logging(__FILE__, __FUNCTION__, __LINE__, "No server address");
		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}

	/* libevent base object */
	base = event_base_new();

	if (!base) {
		logging(__FILE__, __FUNCTION__, __LINE__, "Could not initialize libevent! :");
		if (conf->log_fd) {
			fprintf(conf->log_fd, "%s", strerror(errno));
		}
		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		return (EXIT_FAILURE);
	}

	/* socket file descriptor */
	int listen_fd = 0;

	/* Get  and connect a socket */
	do {
		listen_fd = get_socket();
	} while (listen_fd <= 0);

	/* Kill any running video players */
	if (!send_stop(listen_fd)) {
		logging(__FILE__, __FUNCTION__, __LINE__, "ERROR :");
		if (conf->log_fd) {
			fprintf(conf->log_fd, "%s", strerror(errno));
		}
	}

	/* create the socket */
	bev = bufferevent_socket_new(base, listen_fd, BEV_OPT_CLOSE_ON_FREE);

	if (!bev) {
		logging(__FILE__, __FUNCTION__, __LINE__, "ERROR : bev");

		exit(EXIT_FAILURE);
	}

	server->fd = listen_fd;
	server->buf_ev = bev;

	/* set the callbacks */
	bufferevent_setcb(bev, NULL, NULL, control_event_callback, base);

	/* enable writing */
	bufferevent_enable(bev, EV_WRITE | EV_READ );

	/* set timeout */
	bufferevent_set_timeouts(bev, NULL, &event_timer);

	/* create the socket */
	sens1 = event_new(base, sensor_device_1, EV_TIMEOUT | EV_READ | EV_PERSIST, sensor1_read_callback, (void *)server);

	event_add(sens1, &event_timer);

	sens2 = event_new(base, sensor_device_2, EV_TIMEOUT | EV_READ | EV_PERSIST, sensor2_read_callback, (void *)server);

	event_add(sens2, &event_timer);

	/* Start the event loop */
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	/* Free the server connection event */
	bufferevent_free(bev);

	/* Free the sensor events */
	event_free(sens1);
	event_free(sens2);

	// free the event base
	event_base_free(base);

	/* Close file descriptors */
	close(sensor_device_1);
	// close(sensor_device_2);
	close(listen_fd);

	// exit
	fclose(conf->log_fd);

	exit(EXIT_SUCCESS);
}
