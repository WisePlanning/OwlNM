/*
 * The MIT License (MIT)
 * Copyright (c) 2011 Jason Ish
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * A simple chat server using libevent.
 *
 * @todo Check API usage with libevent2 proper API usage.
 * @todo IPv6 support - using libevent socket helpers, if any.
 * https://github.com/jasonish/libevent-examples/blob/master/chat-server/chat-server.c
 */

#include "server.h"
#define BACKLOG 10 // how many pending connections queue will hold

/**
 * [sigchld_handler description]
 * @method sigchld_handler
 * @param  s               [description]
 */
void sigchld_handler(int s) {

  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

int clients_connected = 0;

/* The libevent event base.  In libevent 1 you didn't need to worry
 * about this for simple programs, but its used more in the libevent 2
 * API. */
static struct event_base *evbase;

/**
 * A struct for client specific data.
 *
 * This also includes the tailq entry item so this struct can become a
 * member of a tailq - the linked list of all connected clients.
 */
struct client {
  /* The clients socket. */
  int fd;

  /* The bufferedevent for this client. */
  struct bufferevent *buf_ev;

  /*
   * This holds the pointers to the next and previous entries in
   * the tail queue.
   */
  TAILQ_ENTRY(client)
  entries;
};

/**
 * The head of our tailq of all connected clients.  This is what will
 * be iterated to send a received message to all connected clients.
 */
TAILQ_HEAD(, client)
client_tailq_head;

/**
 * [setnonblock description]
 * @method setnonblock
 * @param  fd          [description]
 * @return             [description]
 */
int setnonblock(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL);
  if (flags < 0)
    return flags;
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0)
    return -1;

  return 0;
}

/**
 * Called by libevent when there is data to read
 * @method buffered_on_read
 * @param  bev              [description]
 * @param  arg              [description]
 */
void buffered_on_read(struct bufferevent *bev, void *arg) {
  struct client *this_client = arg;
  struct client *client;
  uint8_t data[8192];
  size_t n;

  /* Read 8k at a time and send it to all connected clients. */
  for (;;) {
    n = bufferevent_read(bev, data, sizeof(data));
    if (n <= 0) {
      /* Done. */
      break;
    }

    /* Send data to all connected clients except for the
     * client that sent the data. */
    TAILQ_FOREACH(client, &client_tailq_head, entries) {
      if (client != this_client) {
        bufferevent_write(client->buf_ev, data, n);
      }
    }
  }
}

/**
 * Called by libevent when there is an error on the underlying socket
 * descriptor.
 * [buffered_on_error description]
 * @method buffered_on_error
 * @param  bev               [description]
 * @param  what              [description]
 * @param  arg               [description]
 */
void buffered_on_error(struct bufferevent *bev, short what, void *arg) {
  struct client *client = (struct client *)arg;

  if (what & BEV_EVENT_EOF) {
    /* Client disconnected, remove the read event and the
     * free the client structure. */
    printf("Client disconnected.\n");
  } else {
    warn("Client socket error, disconnecting.\n");
  }

  /* Remove the client from the tailq. */
  TAILQ_REMOVE(&client_tailq_head, client, entries);
  clients_connected--;
  bufferevent_free(client->buf_ev);
  close(client->fd);
  free(client);
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 * @method on_accept
 * @param  fd        [description]
 * @param  ev        [description]
 * @param  arg       [description]
 */
void on_accept(int fd, short ev, void *arg) {
  int client_fd;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  struct client *client;
  char s[INET6_ADDRSTRLEN];

  client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    warn("accept failed");
    return;
  }

  /* Set the client socket to non-blocking mode. */
  if (setnonblock(client_fd) < 0)
    warn("failed to set client socket non-blocking");

  /* We've accepted a new client, create a client object. */
  client = calloc(1, sizeof(*client));
  if (client == NULL)
    err(1, "malloc failed");
  client->fd = client_fd;

  client->buf_ev = bufferevent_socket_new(evbase, client_fd, 0);
  bufferevent_setcb(client->buf_ev, buffered_on_read, NULL, buffered_on_error,
                    client);

  /* We have to enable it before our callbacks will be
   * called. */
  bufferevent_enable(client->buf_ev, EV_READ);

  /* Add the new client to the tailq. */
  TAILQ_INSERT_TAIL(&client_tailq_head, client, entries);
  clients_connected++;

  printf("Accepted connection from %s: %d clients connected.\n",
         inet_ntop(client_addr.sin_family,
                   get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s)),
         clients_connected);
}

/**
*
*/
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * [server_run_loop description]
 * @method server_run_loop
 * @return [description]
 */
int server_run_loop() {
  if (conf->verbose)
    puts("Starting Server");

  struct addrinfo hints, *servinfo, *p;
  struct sigaction sa;
  int yes = 1;
  int rv;
  int listen_fd;
  struct event ev_accept;
  pid_t pid = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

  /* Input events */
// #ifdef __FreeBSD__
  hints.ai_family = AF_INET;
// #else
//   hints.ai_family = AF_UNSPEC;
// #endif /* __FreeBSD__ */

  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, conf->port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(listen_fd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL) {
    perror( "server: failed to bind\n");
    exit(1);
  }

  if (listen(listen_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  /* Initialize libevent. */
  evbase = event_base_new();

  /* Initialize the tailq. */
  TAILQ_INIT(&client_tailq_head);

  // Fork off the avahi service advertiser
  pid = fork();
  if (pid == 0) {
    avahi_server();
    return 0;
  }
  /* We now have a listening socket, we create a read event to
   * be notified when a client connects. */
  event_assign(&ev_accept, evbase, listen_fd, EV_READ | EV_PERSIST, on_accept,
               NULL);
  event_add(&ev_accept, NULL);

  /* Start the event loop. */
  event_base_dispatch(evbase);

  return 0;
}
