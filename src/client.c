#include "client.h"

#if __amd64__ || __i386__

#define EVLOOP_NO_EXIT_ON_EMPTY 0x04
/**
 * Called on when there is data to read on the socket
 * @param void*
 * @param bufferevent*
 * @return void
 */
void read_callback(struct bufferevent *bev, void *ctx) {
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
    if (conf->verbose)
      puts("Play\n");

    /* Start the video player */
    if (!play_video())
      perror("Error");
  }

  if ((strncmp(buffer, STOP, 4) == 0)) {
    if (conf->verbose)
      puts("Kill");

    /* Kill any running video players */
    if (!stop_video())
      perror("Error");
  }
}

/**
 * Called on socket event
 * @param void*
 * @param short
 * @param bufferevent*
 * @return void
 */
void event_callback(struct bufferevent *bev, short events, void *ctx) {
  if (events & BEV_EVENT_CONNECTED) {
    if (conf->verbose)
      puts("connected");
  } else if (events & BEV_EVENT_EOF) {
    if (conf->verbose)
      puts("Connection closed.");

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
int client_run_loop() {

  if (conf->verbose)
    puts("Starting Client - libevent");

  struct event_base *base;
  struct sockaddr sin;
  struct bufferevent *bev = NULL;
  int status = 0;
  base = NULL;

  // if there is no server address,
  if (conf->avahi || NULL == conf->server_address) {
    do {
      avahi_client();
    } while (NULL == conf->server_address);
  }

  /* Socked address structures */
  struct addrinfo *res;
  struct addrinfo hints, *p;

  /* clear the memory */
  memset(&hints, 0, sizeof(hints));

  /* IPV4 or IPV6 */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(conf->server_address, conf->port, &hints, &res)) !=
      0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return (EXIT_FAILURE);
  }

  /* libevent base object */
  base = event_base_new();

  if (!base) {
    perror("Could not initialize libevent!\n");
    return (EXIT_FAILURE);
  }

  /* loop through all the results and connect to the first we can */
  for (p = res; p != NULL; p = p->ai_next) {

    /* make a copy of the address */
    memcpy(&sin, p->ai_addr, sizeof(&p->ai_addr));

    /* create the socket */
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    /* set the callbacks */
    bufferevent_setcb(bev, read_callback, NULL, event_callback, base);

    /* connect to the server */
    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) <
        0) {
      perror("Could not connect!");
      bufferevent_free(bev);
      return (EXIT_FAILURE);
    }

    /* enable reading */
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

    /* Free the linked list */
    freeaddrinfo(res);

    /* Start the event loop */
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

    break; // if we get here, we must have connected successfully and exited
           // program
  }
  // free the stuff
  bufferevent_free(bev);

  // free the event base
  event_base_free(base);

  // exit
  return (EXIT_SUCCESS);
}

#elif __arm__

/* Lib event bug causes hang on arm so we use select instead */
int client_run_loop() {

  // timeout struct
  struct timeval tv;

  fd_set master;
  fd_set read_set;

  // buffer for recv
  char buffer[BUF_SIZE];

  // max number for select
  int fd_max;

  // socket connected to the server
  int listen_fd;

  // use avahi to find the server
  // if there is no server address,
  if (conf->avahi || NULL == conf->server_address) {
    do {
      avahi_client();
    } while (NULL == conf->server_address);
  }

  do {
    listen_fd = get_socket();
  } while (listen_fd <= 0);

  reset_timer(&tv);

  FD_ZERO(&master);
  FD_SET(listen_fd, &master);

  /* Wait for timeout or person to leave */
  while (TRUE) {
    read_set = master;
    fd_max = listen_fd + 1;

    int ret = select(fd_max, &read_set, NULL, NULL, &tv);
    if (ret == -1) {
      perror("Error reading data!\n");
      do {
        /* reconnect */
        shutdown(listen_fd, 2);

        if (conf->avahi)
          avahi_client();

        listen_fd = get_socket();
        sleep(5);

      } while (listen_fd <= 0);

      FD_ZERO(&master);
      FD_SET(listen_fd, &master);

      reset_timer(&tv);
    } else if (ret == 0) {
      reset_timer(&tv);
      if (conf->verbose) {
        puts("timeout");
      }

    } else {

      /* Check the link with the server */
      if (FD_ISSET(listen_fd, &read_set)) {
        ret = read(listen_fd, &buffer, BUF_SIZE);

        /* if disconnected, try to reconnect */
        if (ret == 0) {
          perror("Connection Lost. Reconnecting.\n");
          do {
            /* reconnect */
            shutdown(listen_fd, 2);

            if (conf->avahi)
              avahi_client();

            listen_fd = get_socket();
            sleep(5);

          } while (listen_fd <= 0);

          FD_ZERO(&master);
          FD_SET(listen_fd, &master);

          reset_timer(&tv);
        } else {

          /* Check for play */
          if (strncmp(buffer, PLAY, 4) == 0) {
            if (conf->verbose)
              puts("Play");

            /* Start the video player */
            if (!play_video())
              perror("Error");
          }

          /* Check for stop */
          if ((strncmp(buffer, STOP, 4) == 0)) {
            if (conf->verbose)
              puts("Kill");

            /* Kill any running video players */
            if (!stop_video())
              perror("Error");
          }
        }
      }
    }
  }
  return 0;
} /* control_run_loop */

#endif
