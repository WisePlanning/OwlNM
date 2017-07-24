#include "control.h"

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
	                            //  "grep -B1 120013 |"  // Keyboard identifier
	                             "grep -B1 100013 |"  // Sensor identifier
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
		printf("Could not determine sensor device file\n");

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

	if (conf->verbose) {
		printf("%s\n", sensor_device_1);
		printf("%s\n", sensor_device_2);
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
 * Controller loop
 * @method control_run_loop
 * @return boolean
 */
int control_run_loop() {

	if (conf->verbose)
		printf("Starting Controller\n");

	logging(__FILE__,__FUNCTION__,__LINE__,"Starting Controller\n");
	log_config(conf);

	struct timeval tv;

	uint8_t shift_pressed = 0;

	input_event event;

	fd_set master;
	fd_set read_set;

	bool playing = FALSE;
	bool end = FALSE;

	int fd_max = 0;
	int listen_fd = 0;

	/* Get the keyboard file descriptos */
	if (conf->verbose)
		printf("Getting sensor1 \n");

	if (conf->log_fd) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Getting sensor1");
	}

	int sensor_device_1 = openDeviceFile(getSensorDeviceFileName(1));

	if (conf->log_fd) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Sensor 1 = ");
		fprintf(conf->log_fd, "%d\n", sensor_device_1);
	}

	// /* Get the keyboard file descriptos */
	if (conf->verbose)
		printf("Getting sensor 2\n");

	if (conf->log_fd) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Getting sensor2 \n");
	}

	int sensor_device_2 = openDeviceFile(getSensorDeviceFileName(2));

	if (conf->log_fd) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Sensor 2 = ");
		fprintf(conf->log_fd, "%d\n", sensor_device_2);
			}

	if (!(sensor_device_1 > 0) || !(sensor_device_2 > 0)) {
		perror("Could not get sensors");
		if (conf->log_fd) {
			logging(__FILE__,__FUNCTION__,__LINE__, "Could not get sensors : ");
			fprintf(conf->log_fd, "%s\n", strerror(errno));
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}

#ifdef HAVE_WIRINGPI
	if (wiringPiSetupGpio () == -1) {
		fclose(conf->log_fd);
		logging(__FILE__,__FUNCTION__,__LINE__, "Could not open GPIO\n");
		exit(EXIT_FAILURE);
	}
	//set the pin to output
	pinMode (LED, OUTPUT);

	if (conf->verbose)
		printf("LED OFF\n");
		logging(__FILE__,__FUNCTION__,__LINE__,"LED OFF\n");

	// switch gpio pin to disable relay
	digitalWrite(LED, OFF);
#endif

#ifdef HAVE_AVAHI
	// use avahi to find the server
	// if there is no server address,
	if (conf->avahi || NULL == conf->server_address) {
		do {
			logging(__FILE__,__FUNCTION__,__LINE__, "Starting Avahi client\n");
			avahi_client();
		} while (NULL == conf->server_address);
	}
#endif

  if (NULL == conf->server_address) {
	printf("No server address\n");
	logging(__FILE__,__FUNCTION__,__LINE__,"No server address\n");
	fclose(conf->log_fd);
	exit(EXIT_FAILURE);
  }

	do {
		listen_fd = get_socket();
	} while (listen_fd <= 0);

	if (conf->log_fd) {
		logging(__FILE__,__FUNCTION__,__LINE__,"Server Connection Socket = ");
		fprintf(conf->log_fd, "%d\n", listen_fd);
	}
	reset_timer(&tv);

	/* Setup the select device file array */
	FD_ZERO(&master);
	FD_SET(listen_fd, &master);     // connection to server
	FD_SET(sensor_device_1, &master); // second sensor
	FD_SET(sensor_device_2, &master); // first sensor

	/* copy of the file descriptors for the sensors */
	int fds[2];
	memset(&fds, 0, 2);

	fds[0] = sensor_device_1;
	fds[1] = sensor_device_2;

	/* Wait for timeout or person to leave */
	while (TRUE) {

		read_set = master;
		fd_max = sensor_device_2 + 1;

		int ret = select(fd_max, &read_set, NULL, NULL, &tv);

		if (ret == -1) {

			perror("Error reading data!\n");

			if (conf->log_fd) {
				logging(__FILE__,__FUNCTION__,__LINE__,"Error reading data!\n");
			}

			FD_ZERO(&master);

			/* Close the sensor device file descriptors */
			shutdown(sensor_device_1, 2);
			shutdown(sensor_device_2, 2);

			/* Get the file descriptors */
			int sensor_device_1 = openDeviceFile(getSensorDeviceFileName(1));
			int sensor_device_2 = openDeviceFile(getSensorDeviceFileName(2));

			FD_SET(listen_fd, &master);     // connection to server
			FD_SET(sensor_device_1, &master); // second sensor
			FD_SET(sensor_device_2, &master); // first sensor

		} else if (ret == 0) {
			/* Nothing happened */

			reset_timer(&tv);

			if (conf->verbose) {
				printf("timeout\n");
			}

			/* Send the stop signal when the last sensor was END and the video was
			 * playing  */
				if (conf->verbose) {
					printf("Stopping playback\n");
				}

				/* Send the stop signal */
				send_stop(listen_fd);

				playing = FALSE;
				end = FALSE;
		} else {

			/* Check the link with the server */
			if (FD_ISSET(listen_fd, &read_set)) {

				ret = read(listen_fd, &event, sizeof(input_event));

				/* if disconnected, try to reconnect */
				if (ret == 0) {
					perror("Connection to server Lost. Reconnecting.\n");
					logging(__FILE__,__FUNCTION__,__LINE__, "Connection to server Lost. Reconnecting.\n");
					do
					{
						/* reconnect */
						shutdown(listen_fd, 2);

					#ifdef HAVE_AVAHI
						if (conf->avahi)
							avahi_client();
					#endif

						listen_fd = get_socket();
						sleep(5);

					} while (listen_fd <= 0);

					FD_ZERO(&master);
					FD_SET(sensor_device_1, &master);
					FD_SET(sensor_device_2, &master);
					FD_SET(listen_fd, &master);

					reset_timer(&tv);
				}
			}

			/* loop through the sensor devices */
			for (int i = 0; i < 2; ++i) {
				if (FD_ISSET(fds[i], &read_set)) {
					if ((ret = read(fds[i], &event, sizeof(input_event))) > 0) {
						if (event.type == EV_KEY) {
							if (event.value == KEY_PRESS) {
								if (isShift(event.code)) {
									shift_pressed++;
								}

								char *name = getKeyText(event.code, shift_pressed);

								if (strcmp(name, UNKNOWN_KEY) != 0) {
									if (0 == strcmp(name, ENTER)) {
										if (!playing) {
											if (0 > send_start(listen_fd)) {
												playing = FALSE;
											}

											if (conf->verbose) {
												printf("Entered the area, starting playback\n");
											}
											playing = TRUE;
											reset_timer(&tv);

											end = FALSE;
										} else {
											reset_timer(&tv);
											if (conf->verbose) {
												printf("Someone else entered\n");
											}
											end = FALSE;
										}
									}

									if (0 == strcmp(name, LEAVE)) {
										if (conf->verbose) {
											printf("Leaving the area, starting timer\n");
										}
										reset_timer(&tv);
										end = TRUE;
									}
								}
							} else if (event.value == KEY_RELEASE) {
								if (isShift(event.code)) {
									shift_pressed--;
								}
							}
						}
					}
				}
			}
		}
	}

	fclose(conf->log_fd);
	return 0;
} /* control_run_loop */
