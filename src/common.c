#include "common.h"

/**
 * Get a socket descriptor
 * @method get_socket
 * @return file discriptors
 */
int get_socket()
{

  struct addrinfo *res;
  struct addrinfo hints, *p;
  int status;
  int sockfd;

  /* clear the memory */
  memset(&hints, 0, sizeof(hints));

  /* IPV4 or IPV6 */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(conf->server_address, conf->port, &hints, &res)) !=
      0)
  {
    LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "ERROR: getaddrinfo");
    if (conf->log_fd)
    {
      fprintf(conf->log_fd, "%s", strerror(errno));
    }
    return -1;
  }

  /* loop through all the results and connect to the first we can */
  for (p = res; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "ERROR: socket");
      if (conf->log_fd)
      {
        fprintf(conf->log_fd, "%s", strerror(errno));
      }
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "ERROR: connect");
      if (conf->log_fd)
      {
        fprintf(conf->log_fd, "%s", strerror(errno));
      }
      close(sockfd);
      continue;
    }
    break; // if we get here, we must have connected successfully
  }

  freeaddrinfo(res); // free the linked list

  return (sockfd > 0 ? sockfd : -1);
}

void logging(const char *file, const char *func, int line,
             const char *message)
{

  time_t current_time;
  struct tm *struct_time;

  time(&current_time);

  struct_time = gmtime(&current_time);

  if (conf->log_fd)
  {
    fprintf(conf->log_fd, "\n%d-%02d-%d %02d-%02d-%02d file:%s %s:%i  :%s",
            struct_time->tm_year + 1900, struct_time->tm_mon + 1,
            struct_time->tm_mday, struct_time->tm_hour, struct_time->tm_min,
            struct_time->tm_sec, file, func, line, message);

    fflush(conf->log_fd);
  }

  if (conf->verbose)
  {
    fprintf(stdout, "\n%d-%02d-%d %02d-%02d-%02d file:%s %s:%i  :%s",
            struct_time->tm_year + 1900, struct_time->tm_mon + 1,
            struct_time->tm_mday, struct_time->tm_hour, struct_time->tm_min,
            struct_time->tm_sec, file, func, line, message);
    fflush(stdout);
  }
}

int write_log(const char *file, const char *function, int line, const char *format, ...)
{

  time_t current_time;
  struct tm *struct_time;

  time(&current_time);

  struct_time = gmtime(&current_time);

  va_list arg;
  if (conf->log_fd)
  {
    int rv;
    va_start(arg, format);
    fprintf(conf->log_fd, "\n%d-%02d-%d %02d-%02d-%02d: ",
            struct_time->tm_year + 1900, struct_time->tm_mon + 1,
            struct_time->tm_mday, struct_time->tm_hour, struct_time->tm_min,
            struct_time->tm_sec);
    fprintf(conf->log_fd, "file %s:function %s:line %d:", file, function, __LINE__);
    rv = vfprintf(conf->log_fd, format, arg);
    va_end(arg);
  }

  if (conf->verbose)
  {
    va_start(arg, format);
    vprintf(format, arg);
    va_end(arg);
  }
  return rv;
}

/* Root is required to capture device input */
bool rootCheck()
{

  LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "Checking for root permissions");

  if (geteuid() != 0)
  {
    return FALSE;
  }
  return TRUE;
}

/**
 * Send the stop signal to the server
 * @method stop_video
 * @param  addr       [description]
 * @param  sockfd     [description]
 * @return            [description]
 */
int send_stop(int sockfd)
{
  int ret;

  logging(__FILE__, __FUNCTION__, __LINE__, "Sending stop to server");

  ret = send(sockfd, STOP, sizeof(STOP), 0);

  if (ret < 0)
  {
    LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "Error sending data!\t-STOP");
  }
  else
  {
    LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "Success");
    playing = FALSE;
  }

#ifdef HAVE_WIRINGPI
  // switch gpio pin to disable relay

  LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "LED OFF");

  digitalWrite(LED, OFF);

#endif

  return (ret);
}

/**
 * Opens the keyboard device file
 *
 * @param  deviceFile   the path to the keyboard device file
 * @return              the file descriptor on success, error code on failure
 */
int openDeviceFile(char *deviceFile)
{
  if (!deviceFile)
    return 0;

  int dev_fd = open(deviceFile, O_RDONLY | O_NONBLOCK);

  if (dev_fd == -1)
  {
    LOG_WRITE(__FILE__, __FUNCTION__, __LINE__, "Could not get device : ");
    if (conf->log_fd)
    {
      fprintf(conf->log_fd, "%s", strerror(errno));
    }
    return 0;
  }
  free(deviceFile);
  return (dev_fd);
} /* openKeyboardDeviceFile */

/**
 * convert case to upper inplace
 * @method stoupper
 * @param  s
 */
void stoupper(char s[])
{
  int c = 0;

  while (s[c] != '\0')
  {
    if (s[c] >= 'a' && s[c] <= 'z')
    {
      s[c] = s[c] - 32;
    }
    ++c;
  }
}
