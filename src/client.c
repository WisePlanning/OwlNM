
#include "client.h"
typedef struct client
{
	/* The bufferedevent for this client. */
	struct event_base *base;
	struct bufferevent *client_buf_ev;

	/* The clients socket. */
	evutil_socket_t client_fd;

	struct timeval *event_timer;
	int error_count;

} Client;

/**
 * Called on when there is data to read on the socket
 * @param void*
 * @param bufferevent*
 * @return void
 */
void read_callback(struct bufferevent *bev, void *ctx)
{
	int n;
	char buf[BUF_SIZE];
	char buffer[BUF_SIZE];
	struct evbuffer *input = bufferevent_get_input(bev);

	/* Copy all the data to a buffer */
	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0)
	{
		strcpy(buffer, buf);
	}

	/* process */
	if (strncmp(buffer, PLAY, 4) == 0)
	{
		/* Start the video player */
		if (!play_video())
		{
			LOG_WRITE("Error:  %s", strerror(errno));
		}
	}

	if ((strncmp(buffer, STOP, 4) == 0))
	{
		/* Kill any running video players */
		if (!stop_video())
		{
			LOG_WRITE("Error: %s", strerror(errno));
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
void event_callback(struct bufferevent *bev, short events, void *ctx)
{
	Client *client = (Client *)ctx;
	if (events & BEV_EVENT_CONNECTED)
	{
		LOG_WRITE("Connected\n");
	}
	else if (events & BEV_EVENT_EOF)
	{
		LOG_WRITE("Connection closed.\n");
		bufferevent_disable(bev, EV_READ);

// if there is no server address,
#ifdef HAVE_AVAHI
		if (conf->avahi || NULL == conf->server_address)
		{
			do
			{
				avahi_client();
			} while (NULL == conf->server_address);
		}
#endif

		if (NULL == conf->server_address)
		{
			LOG_WRITE("No server address\n");
			if (conf->log_fd)
			{
				fclose(conf->log_fd);
			}
			exit(EXIT_FAILURE);
		}

		/* Get and connect a socket */
		do
		{
			sleep(1);
			if (client->client_fd)
				close(client->client_fd);
			client->client_fd = get_socket();
		} while (client->client_fd <= 0);

		LOG_WRITE("New network socket is %i\n", client->client_fd);
		bufferevent_setfd(bev, client->client_fd);
		bufferevent_enable(bev, EV_READ);

		++client->error_count;

		// recurring error forces restart
		if (client->error_count > 5)
		{
			LOG_WRITE("Error count > 5 : restarting\n");

			sleep(5);

			/* Exit the current loop */
			event_base_loopbreak(client->base);

			/* Start again */
			control_run_loop();
		}
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOG_WRITE("Got an error on the connection :%s\n", strerror(errno));
		bufferevent_disable(bev, EV_READ);
		sleep(1);

// if there is no server address,
#ifdef HAVE_AVAHI
		if (conf->avahi || NULL == conf->server_address)
		{
			do
			{
				avahi_client();
			} while (NULL == conf->server_address);
		}
#endif

		if (NULL == conf->server_address)
		{
			LOG_WRITE("No server address\n");
			if (conf->log_fd)
			{
				fclose(conf->log_fd);
			}
			exit(EXIT_FAILURE);
		}

		/* Get and connect a socket */
		do
		{
			sleep(1);
			if (client->client_fd)
				close(client->client_fd);
			client->client_fd = get_socket();
		} while (client->client_fd <= 0);

		LOG_WRITE("New network socket is %i\n", client->client_fd);
		bufferevent_setfd(bev, client->client_fd);
		bufferevent_enable(bev, EV_READ);

		++client->error_count;

		// recurring error forces restart
		if (client->error_count > 5)
		{
			LOG_WRITE("Error count > 5 : restarting\n");

			sleep(5);

			/* Exit the current loop */
			event_base_loopbreak(client->base);

			/* Start again */
			control_run_loop();
		}
	}
}

/**
 * The loop for clients
 * @return
 */
int client_run_loop()
{

	LOG_WRITE("Starting Client\n");
	log_config(conf);

	Client *client = malloc(sizeof(Client));
	client->client_fd = 0;
	client->client_buf_ev = NULL;
	client->error_count = 0;
	client->event_timer = NULL;
	client->base = NULL;

	/* Kill any running video players */
	if (!stop_video())
	{
		LOG_WRITE("ERROR : %s", strerror(errno));
	}

	struct event_base *base;
	struct bufferevent *bev = NULL;
	base = NULL;

// if there is no server address,
#ifdef HAVE_AVAHI
	if (conf->avahi || NULL == conf->server_address)
	{
		do
		{
			avahi_client();
		} while (NULL == conf->server_address);
	}
#endif

	if (NULL == conf->server_address)
	{
		LOG_WRITE("No server address\n");
		if (conf->log_fd)
		{
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}

	/* libevent base object */
	base = event_base_new();
	client->base = base;
	if (!base)
	{
		LOG_WRITE("Could not initialize libevent!: %s\n", strerror(errno));
		if (conf->log_fd)
		{
			fclose(conf->log_fd);
		}
		exit(EXIT_FAILURE);
	}

	/* socket file descriptor */
	evutil_socket_t listen_fd = 0;

	/* Get  and connect a socket */
	do
	{
		listen_fd = get_socket();
	} while (listen_fd <= 0);
	client->client_fd = listen_fd;

	/* create the socket */
	bev = bufferevent_socket_new(base, listen_fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev)
		exit(EXIT_FAILURE);
	client->client_buf_ev = bev;

	/* set the callbacks */
	bufferevent_setcb(bev, read_callback, NULL, event_callback, client);

	/* enable reading */
	bufferevent_enable(bev, EV_READ);

	/* Start the event loop */
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	// free the stuff
	bufferevent_free(bev);

	// free the event base
	event_base_free(base);

	// close the listening socket
	close(listen_fd);

	// free the struct
	free(client);

	// exit
	fclose(conf->log_fd);

	exit(EXIT_SUCCESS);
}
