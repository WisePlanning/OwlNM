#include "client.h"

#define EVLOOP_NO_EXIT_ON_EMPTY 0x04

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
	                             "grep -B1 120013 |"  // Keyboard identifier
	                            //  "grep -B1 100013 |"  // Sensor identifier
	                             "grep -Eo event[0-9]+ |"
	                             "tr -d '\\n'";

	/* device path prefix */
	char path_prefix[20] = "/dev/input/";

	/* Sensor device names */
	char sensor_device_1[10];
	char sensor_device_2[10];

	/* Run the command */
	FILE *cmd = popen(command, "r");

	if (cmd == NULL) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Could not determine sensor device file\n");
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

		// copy the second device name
		strncpy(sensor_device_2, &buff[6], 6);
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
 * Called on when there is data to read on the socket
 * @param void*
 * @param bufferevent*
 * @return void
 */
void control_read_callback(struct bufferevent *bev, void *ctx) {
	int n;
	char buf[BUF_SIZE];
	char buffer[BUF_SIZE];
	struct evbuffer *input = bufferevent_get_input(bev);

	/* Copy all the data to a buffer */
	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
		strcpy(buffer, buf);
	}

	/* process */
	if (strncmp(buffer, PLAY, 4) == 0) {
		/* Start the video player */
		if (!play_video()) {
			logging(__FILE__,__FUNCTION__,__LINE__,"Error: ");
			if (conf->log_fd){
				fprintf(conf->log_fd,"%s\n", strerror(errno));
			}
		}
	}

	if ((strncmp(buffer, STOP, 4) == 0)) {
		/* Kill any running video players */
		if (!stop_video()) {
			logging(__FILE__,__FUNCTION__,__LINE__,"Error: ");
			if (conf->log_fd){
				fprintf(conf->log_fd,"%s\n", strerror(errno));
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
	if (events & BEV_EVENT_CONNECTED) {

		logging(__FILE__,__FUNCTION__,__LINE__,"Connected\n");

	} else if (events & BEV_EVENT_EOF) {

		logging(__FILE__,__FUNCTION__,__LINE__, "Connection closed.\n");

		sleep(5);

		/* cleanup */
		bufferevent_free(bev);

		/* Exit the current loop */
		event_base_loopexit(ctx, NULL);

		/* Start again */
		client_run_loop();
	} else if (events & BEV_EVENT_ERROR) {
		if (conf->verbose)
			printf("Got an error on the connection: %s\n", strerror(errno));

		logging(__FILE__,__FUNCTION__,__LINE__, "Got an error on the connection :");
		fprintf(conf->log_fd,"%s\n",strerror(errno));

		sleep(5);

		/* cleanup */
		bufferevent_free(bev);

		/* Exit the current loop */
		event_base_loopexit(ctx, NULL);

		/* Start again */
		client_run_loop();
	}
}

/**
 * The loop for clients
 * @return
 */
int control_run_loop() {

	logging(__FILE__,__FUNCTION__,__LINE__, "Starting Client - LibEvent\n");
	log_config(conf);

	/* Kill any running video players */
	if (!stop_video()) {
		logging(__FILE__,__FUNCTION__,__LINE__, "ERROR :");
		if (conf->log_fd) {fprintf(conf->log_fd,"%s\n",strerror(errno));}
	}

	struct event_base *base;
	struct bufferevent *bev = NULL;
	struct bufferevent *bev1 = NULL;
	struct bufferevent *bev2 = NULL;
	base = NULL;

	/* Get the keyboard file descriptos */
	logging(__FILE__,__FUNCTION__,__LINE__,"Getting sensor 1\n");

	int sensor_device_1 = openDeviceFile(getSensorDeviceFileName(1));

	logging(__FILE__,__FUNCTION__,__LINE__,"Sensor 1 = ");
	if (conf->log_fd) {fprintf(conf->log_fd, "%d\n", sensor_device_1);}

	// /* Get the keyboard file descriptos */
	// logging(__FILE__,__FUNCTION__,__LINE__,"Getting sensor2 \n");
	// int sensor_device_2 = openDeviceFile(getSensorDeviceFileName(2));


	// logging(__FILE__,__FUNCTION__,__LINE__,"Sensor 2 = ");
	// if (conf->log_fd) {fprintf(conf->log_fd, "%d\n", sensor_device_2);}

	// if (!(sensor_device_1 > 0) || !(sensor_device_2 > 0)) {
	// 	logging(__FILE__,__FUNCTION__,__LINE__, "Could not get sensors : ");
	// 	if (conf->log_fd) {fprintf(conf->log_fd, "%s\n", strerror(errno));}
	// 	if (conf->log_fd) {fclose(conf->log_fd);}
	// 	exit(EXIT_FAILURE);
	// }

#ifdef HAVE_WIRINGPI
	if (wiringPiSetupGpio () == -1) {
		logging(__FILE__,__FUNCTION__,__LINE__, "Could not open GPIO\n");
		if (conf->log_fd) {fclose(conf->log_fd);}
		exit(EXIT_FAILURE);
	}
	//set the pin to output
	pinMode (LED, OUTPUT);

	logging(__FILE__,__FUNCTION__,__LINE__,"LED OFF\n");

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
		logging(__FILE__,__FUNCTION__,__LINE__, "No server address\n");
		if (conf->log_fd) {fclose(conf->log_fd);}
		exit(EXIT_FAILURE);
	}

	/* libevent base object */
	base = event_base_new();

	if (!base) {
		logging(__FILE__,__FUNCTION__,__LINE__, "Could not initialize libevent! :");
		if (conf->log_fd){fprintf(conf->log_fd,"%s\n", strerror(errno));}
		if (conf->log_fd) {fclose(conf->log_fd);}
		return (EXIT_FAILURE);
	}

	/* socket file descriptor */
	int listen_fd = 0;

	/* Get  and connect a socket */
	do {
		listen_fd = get_socket();
	} while (listen_fd <= 0);

	logging(__FILE__,__FUNCTION__,__LINE__,"Connected\n");

	/* create the socket */
	bev = bufferevent_socket_new(base, listen_fd, BEV_OPT_CLOSE_ON_FREE);

	if (!bev)
		exit(EXIT_FAILURE);
	/* set the callbacks */
	bufferevent_setcb(bev, control_read_callback, NULL, control_event_callback, base);

	/* enable reading */
	bufferevent_enable(bev, EV_WRITE | EV_PERSIST);

	/* create the socket */
	bev1 = bufferevent_socket_new(base, sensor_device_1, BEV_OPT_CLOSE_ON_FREE);

	if (!bev1)
		exit(EXIT_FAILURE);
	/* set the callbacks */
	bufferevent_setcb(bev1, control_read_callback, NULL, control_event_callback, base);

	/* enable reading */
	bufferevent_enable(bev1, EV_READ | EV_PERSIST);
		/* create the socket */
	// bev2 = bufferevent_socket_new(base, sensor_device_2, BEV_OPT_CLOSE_ON_FREE);

	// if (!bev2)
	// 	exit(EXIT_FAILURE);
	// /* set the callbacks */
	// bufferevent_setcb(bev2, control_read_callback, NULL, control_event_callback, base);

	// /* enable reading */
	// bufferevent_enable(bev2, EV_READ | EV_PERSIST);

	/* Start the event loop */
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	// free the stuff
	bufferevent_free(bev);
	bufferevent_free(bev1);
	bufferevent_free(bev2);

	// free the event base
	event_base_free(base);

	// exit
	fclose(conf->log_fd);

	exit (EXIT_FAILURE);
}
