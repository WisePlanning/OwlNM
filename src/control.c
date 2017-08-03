#include "control.h"


typedef struct server
{
	/* The bufferedevent for this client. */
	struct event_base *base;
	struct bufferevent *server_buf_ev;
	struct bufferevent *sensor1_buf_ev;
	struct bufferevent *sensor2_buf_ev;

	int playing;
	evutil_socket_t sensor1_fd;
	evutil_socket_t sensor2_fd;
	/* The clients socket. */
	int server_fd;
	struct timeval *event_timer;
	int error_count;
} Server;

/**
 * Get the device for sensor input
 * @method getSensirDeviceFileName
 * @param  position (1|2)
 * @return
 */
char *getSensorDeviceFileName(int position)
{

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
		LOG_WRITE("Could not determine sensor device file\n");
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
		if (strlen(buff) > 6) {
			strncpy(sensor_device_2, &buff[6], 6);
		}
	}else {
		return 0;
	}

	/* return the sensor device */
	if (position == 1) {
		if (strlen(sensor_device_1) > 1)
			return (strdup(strcat(path_prefix, sensor_device_1)));
	}else if (position == 2) {
		if (strlen(sensor_device_2) > 1)
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
void sensor_read(struct bufferevent *bev, void *ctx){
	Server *server = (Server *)ctx;
	LOG_WRITE("Sensor read called\n");

	// input_event event;

	// evbuffer_remove(bufferevent_get_input(bev), &event, sizeof(input_event));

	if (server->playing == 0) {
		evbuffer_add_printf(bufferevent_get_output(server->server_buf_ev), "%s", PLAY);
		server->playing = 1;
	#ifdef HAVE_WIRINGPI

		LOG_WRITE("LED ON\n");

		// switch gpio pin to disable relay
		digitalWrite(LED, ON);

	#endif
	} else {
		evbuffer_add_printf(bufferevent_get_output(server->server_buf_ev), "c");
	}

}

/*
 * Called after writing the message to the network
 */
void message_sent(struct bufferevent *bev, void *ctx){
	Server *server = (Server *)ctx;

	LOG_WRITE("Resetting Timer\n");
	bufferevent_set_timeouts(bev, server->event_timer, server->event_timer);
}

/**
 * called on an event on the socket
 *
 *
 */
void sensor_event(struct bufferevent *bev, short events, void *ctx)
{
	Server *server = (Server *)ctx;

	if (events & BEV_EVENT_CONNECTED) {
		LOG_WRITE("Sensor Connected\n");
	} else if (events & BEV_EVENT_EOF) {
		LOG_WRITE("Sensor Connection closed.\n");

	} else if (events & BEV_EVENT_ERROR) {
		LOG_WRITE("Sensor got an error on the connection :%s\n", strerror(errno));

		do {
			if (server->sensor1_fd!=0) close(server->sensor1_fd);
			if (server->sensor2_fd!=0) close(server->sensor2_fd);

			sleep(1);

			/* Get the keyboard file descriptos */
			server->sensor1_fd = openDeviceFile(getSensorDeviceFileName(1));
			LOG_WRITE("Sensor 1 = %d\n", server->sensor1_fd);

			/* Get the keyboard file descriptos */
			server->sensor2_fd = openDeviceFile(getSensorDeviceFileName(2));
			LOG_WRITE("Sensor 2 = %d\n", server->sensor2_fd);

		} while(!(server->sensor1_fd > 0) || !(server->sensor2_fd > 0));

		bufferevent_disable(server->sensor1_buf_ev, EV_WRITE | EV_READ);
		bufferevent_disable(server->sensor2_buf_ev, EV_WRITE | EV_READ);

		bufferevent_flush(server->sensor1_buf_ev, EV_WRITE | EV_READ,BEV_FLUSH);
		bufferevent_flush(server->sensor2_buf_ev, EV_WRITE | EV_READ,BEV_FLUSH);

		bufferevent_setfd(server->sensor1_buf_ev, server->sensor1_fd);
		bufferevent_setfd(server->sensor2_buf_ev, server->sensor2_fd);

		bufferevent_enable(server->sensor1_buf_ev, EV_WRITE | EV_READ);
		bufferevent_enable(server->sensor2_buf_ev, EV_WRITE | EV_READ);

		++server->error_count;
		// recurring error forces restart
		if (server->error_count > 5) {
		LOG_WRITE("Error count > 5 : restarting\n");

			sleep(5);

			/* Exit the current loop */
			event_base_loopbreak(server->base);

		/* Start again */
			control_run_loop();
		}
	} else if (events & BEV_EVENT_TIMEOUT) {
		LOG_WRITE("Timeout : Sensor connection\n");
		evbuffer_add_printf(bufferevent_get_output(server->server_buf_ev), " ");
		bufferevent_enable(bev, EV_WRITE | EV_READ);
	}
}
/**
 * Called on socket event
 * @param void*
 * @param short
 * @param bufferevent*
 * @return void
 */
void control_event_callback(struct bufferevent *bev, short events, void *ctx)
{
	Server *server = (Server *)ctx;
	if (events & BEV_EVENT_CONNECTED) {
		LOG_WRITE("Server Connected\n");
	}else if (events & BEV_EVENT_EOF) {
		LOG_WRITE("Server Connection closed.\n");

		bufferevent_disable(server->server_buf_ev, EV_WRITE | EV_READ);

		++server->error_count;
		// recurring error forces restart

		if (server->error_count > 5) {
		LOG_WRITE("Error count > 5 : restarting\n");

			sleep(5);
			/* Exit the current loop */
			event_base_loopbreak(server->base);

		/* Start again */
			control_run_loop();
		}

		sleep(1);

// if there is no server address,
#ifdef HAVE_AVAHI
		if (conf->avahi || NULL == conf->server_address) {
			do
			{
				avahi_client();
			} while (NULL == conf->server_address);
		}
#endif

		if (NULL == conf->server_address) {
			LOG_WRITE("No server address\n");
			if (conf->log_fd) {
				fclose(conf->log_fd);
			}
			exit(EXIT_FAILURE);
		}

		/* Get and connect a socket */
		do
		{ if (server->server_fd) close(server->server_fd);
		  server->server_fd = get_socket();} while (server->server_fd <= 0);

		bufferevent_setfd(server->server_buf_ev, server->server_fd);
		bufferevent_enable(server->server_buf_ev, EV_WRITE | EV_READ);


	}else if (events & BEV_EVENT_ERROR) {
		LOG_WRITE("Server Got an error on the connection :%s\n", strerror(errno));

		bufferevent_disable(server->server_buf_ev, EV_WRITE | EV_READ);

		++server->error_count;
		// recurring error forces restart

		if (server->error_count > 5) {
		LOG_WRITE("Error count > 5 : restarting\n");

			sleep(5);

			/* Exit the current loop */
			event_base_loopbreak(server->base);

		/* Start again */
			control_run_loop();
		}

		sleep(1);

// if there is no server address,
#ifdef HAVE_AVAHI
		if (conf->avahi || NULL == conf->server_address) {
			do
			{
				avahi_client();
			} while (NULL == conf->server_address);
		}
#endif

		if (NULL == conf->server_address) {
			LOG_WRITE("No server address\n");
			if (conf->log_fd) {
				fclose(conf->log_fd);
			}
			exit(EXIT_FAILURE);
		}

		/* Get and connect a socket */
		do
		{ if (server->server_fd) close(server->server_fd);
		  server->server_fd = get_socket();} while (server->server_fd <= 0);
		LOG_WRITE("New network socket is %i\n", server->server_fd);
		bufferevent_setfd(server->server_buf_ev, server->server_fd);
		bufferevent_enable(server->server_buf_ev, EV_WRITE | EV_READ);


	}else if (events & BEV_EVENT_TIMEOUT) {
		LOG_WRITE("Timeout : Server connection\n");
		evbuffer_add_printf(bufferevent_get_output(bev), "%s", STOP);

#ifdef HAVE_WIRINGPI

		LOG_WRITE("LED OFF\n");

		// switch gpio pin to disable relay
		digitalWrite(LED, OFF);

#endif
		server->playing = 0;
		bufferevent_enable(bev, EV_WRITE | EV_READ);
	}
}


/**
 * The loop for clients
 * @return
 */
int control_run_loop()
{

	LOG_WRITE("Starting Control - LibEvent\n");

	log_config(conf);

	Server *server = malloc(sizeof(Server));
	server->base = NULL;
	server->sensor1_buf_ev = NULL;
	server->sensor2_buf_ev = NULL;
	server->server_buf_ev = NULL;
	server->event_timer = NULL;
	server->error_count = 0;
	struct event_base *base = NULL;
	struct bufferevent *server_buf_ev = NULL;
	struct bufferevent *sensor1_buf_ev = NULL;
	struct bufferevent *sensor2_buf_ev= NULL;

	/* The clients socket. */
	server->server_fd = 0;
	evutil_socket_t server_fd = 0;
	evutil_socket_t sensor_device_1 = 0;
	evutil_socket_t sensor_device_2 = 0;

	/*playing flag*/
	server->playing = 0;

	/* Sensor 1 */
	struct timeval event_timer = {conf->timeout, 0};
	server->event_timer = &event_timer;

// if there is no server address,
#ifdef HAVE_AVAHI
	if (conf->avahi || NULL == conf->server_address) {
		do
		{
			avahi_client();
		} while (NULL == conf->server_address);
	}
#endif

	if (NULL == conf->server_address) {
		LOG_WRITE("No server address\n");
		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}

	/* libevent base object */
	base = event_base_new();
	server->base = base;

	if (!server->base) {
		LOG_WRITE("Could not initialize libevent! :%s\n", strerror(errno));

		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		return (EXIT_FAILURE);
	}

	/* socket file descriptor */
	server->server_fd = 0;

	/* Get and connect a socket */
	do
	{
		server_fd = get_socket();
	} while (server_fd <= 0);

	server->server_fd = server_fd;

	do {
		sleep(1);

		/* Get the keyboard file descriptos */
		sensor_device_1 = openDeviceFile(getSensorDeviceFileName(1));
		LOG_WRITE("Sensor 1 = %d\n", sensor_device_1);

		/* Get the keyboard file descriptos */
		sensor_device_2 = openDeviceFile(getSensorDeviceFileName(2));
		LOG_WRITE("Sensor 2 = %d\n", sensor_device_2);
	} while(!(sensor_device_1 > 0) || !(sensor_device_2 > 0));

	server->sensor1_fd = sensor_device_1;
	server->sensor2_fd = sensor_device_2;

	/* create the socket */
	server_buf_ev = bufferevent_socket_new(base, server_fd, BEV_OPT_CLOSE_ON_FREE);
	server->server_buf_ev = server_buf_ev;

	if (!server_buf_ev ) {
		LOG_WRITE("ERROR :server_buf_ev\n");

		exit(EXIT_FAILURE);
	}
	/* set the callbacks */
	bufferevent_setcb(server_buf_ev, NULL, message_sent, control_event_callback, server);
	bufferevent_enable(server_buf_ev, EV_WRITE | EV_READ);
	bufferevent_set_timeouts(server_buf_ev, &event_timer, &event_timer);
	// LOG_WRITE("Sending Initial STOP\n");
	// evbuffer_add_printf(bufferevent_get_output(server_buf_ev), "%s", STOP);

#ifdef HAVE_WIRINGPI
	if (wiringPiSetupGpio() == -1) {
		LOG_WRITE("Could not open GPIO\n");
		if (conf->log_fd) {
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}
	// set the pin to output
	pinMode(LED, OUTPUT);

	// LOG_WRITE("LED OFF\n");

	// // switch gpio pin to disable relay
	// digitalWrite(LED, OFF);
	// playing = 0;
	server->playing = 0;
#endif

	/*Sensor 1*/
	sensor1_buf_ev = bufferevent_socket_new(base, sensor_device_1, BEV_OPT_CLOSE_ON_FREE);
	if (!sensor1_buf_ev ) {
		LOG_WRITE("ERROR : sensor1_buf_ev\n");

		exit(EXIT_FAILURE);
	}
	server->sensor1_buf_ev = sensor1_buf_ev;
	bufferevent_setcb(sensor1_buf_ev, sensor_read, NULL, sensor_event, server);
	bufferevent_enable(sensor1_buf_ev, EV_WRITE | EV_READ);

	/*Sensor 2*/
	sensor2_buf_ev = bufferevent_socket_new(base, sensor_device_2, BEV_OPT_CLOSE_ON_FREE);
	if (!sensor2_buf_ev ) {
		LOG_WRITE("ERROR : sensor2_buf_ev\n");

		exit(EXIT_FAILURE);
	}
	server->sensor2_buf_ev = sensor2_buf_ev;
	bufferevent_setcb(sensor2_buf_ev, sensor_read, NULL, sensor_event, server);
	bufferevent_enable(sensor2_buf_ev, EV_WRITE | EV_READ);

	/* Start the event loop */
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	free(server);

	// exit
	fclose(conf->log_fd);

	return(EXIT_SUCCESS);
}
