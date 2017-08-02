
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

/* Libevent. */
/* Required by event.h. */
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/event_struct.h>

/* Libevent. */
void control_event_callback(struct bufferevent *bev, short events, void *ctx) {

  if (events & BEV_EVENT_CONNECTED) {

    puts("Connected\n");
  } else if (events & BEV_EVENT_EOF) {

    puts("Connection closed.\n");

  } else if (events & BEV_EVENT_ERROR) {

    puts("Got an error on the connection ");
  } else if (events & BEV_EVENT_TIMEOUT) {

    bufferevent_enable(bev, EV_WRITE | EV_READ);

    puts("Timeout : Server connection\n");
  }
}

int main() {
  struct event_base *base;
  struct timeval event_timer = {3, 0};

  struct bufferevent *bev = NULL;


  base = NULL;

  base = event_base_new();
  bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
  /* set the callbacks */
  bufferevent_setcb(bev, NULL, NULL, control_event_callback, base);

  /* enable writing */
  bufferevent_enable(bev, EV_WRITE | EV_READ);

  /* set timeout */
  bufferevent_set_timeouts(bev, NULL, &event_timer);

  /* Start the event loop */
  event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

  /* Free the server connection event */
  bufferevent_free(bev);
  event_base_free(base);
  return 0;
}