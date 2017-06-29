#ifndef __config_h_
#define __config_h_

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
#include <unistd.h>

/* Input events */
#ifdef __FreeBSD__
#include "/usr/src/sys/dev/evdev/input.h"
#elif __APPLE__
#ifdef TARGET_OS_MAC
#include <input.h>
#endif
#else
#include <linux/input.h>
#endif /* __FreeBSD__ */

/* Libevent. */
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#ifndef __APPLE__
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/timeval.h>
#include <netdb.h>

#include "service_client.h"
#include "service_server.h"

#endif

#include "client.h"
#include "common.h"
#include "control.h"
#include "key_util.h"
#include "server.h"
#include "video.h"
/**
 * Constants / Defaults
 */
#define BUF_SIZE 1024
#define PLAY "play"
#define STOP "stop"
#define KEY_RELEASE 0
#define KEY_PRESS 1
#define TRUE 1
#define FALSE 0
#define VERSION "0.7"
#define MAX_TRIES 20
#define DEFAULT_TIMEOUT 120
#define UTIMEOUT 0
#define DEFAULT_PORT "4000"
#define SERVICE_NAME "_owlmedia._tcp"
#define DEFAULT_CONFIG_FILE "/usr/local/etc/OwlMedia.conf"
#define DEFAULT_VIDEO "/home/wise/Movies/main.h264"
#define DEFAULT_LOG_FILE "/var/log/OwlMedia.log"
#define DEFAULT_SERVER "192.168.0.101"
#define DEFAULT_PLAYER "hello_video.bin"

#define FOREACH_MODE(MODE)                                                     \
  MODE(ERROR)                                                                  \
  MODE(SERVER)                                                                 \
  MODE(CONTROL)                                                                \
  MODE(CLIENT)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum MODE_ENUM { FOREACH_MODE(GENERATE_ENUM) } MODE;

static const char *MODE_STRING[] = {FOREACH_MODE(GENERATE_STRING)};

typedef struct input_event input_event;

/* Config struct */
typedef struct config_obj {
  int timeout;
  int avahi;
  int verbose;
  FILE *log_fd;
  MODE mode;
  char *config_file;
  char *log_name;
  char *port;
  char *server_address;
  char *video_file;
  char *video_player;
} Config;

/* global config struct */
Config *conf;

/* Read in the config file */
Config *read_conf_file();

/* Set Default options */
void set_defaults();

/* Print the global config flags */
void display_config();

/* Create/clear an empty struct : returns a malloc'd struct */
Config *clear_config();

/* Override the config file with cli */
Config *combine_config(Config *cli, Config *conf);

/* read the cli switches */
Config *read_cli_inputs(int argc, char *argv[]);

/* Free a struct */
void free_config(Config *input);

#endif
