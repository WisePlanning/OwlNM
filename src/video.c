#include "video.h"

/**
 * Start the selected video player program
 * @param  video_file [description]
 * @return            [description]
 */

pid_t play_video () {
  int status = 0;
  pid_t pid;
  extern char **environ;
  char play_string[100];

  /* Create the string to call the spawned process */
  sprintf(play_string, "%s --loop %s ", conf->video_player, conf->video_file);

  /* Setup the env for the spawn */
  char *arg[] = {"sh", "-c", play_string, NULL};

  /* Spawn the video player */
  status = posix_spawnp(&pid, "/bin/sh", NULL, NULL, arg, environ);

  if (status < 0)
    perror("Error");

  return (pid);
}

/**
 * Kill the running video player
 * @return int
 */
int stop_video () {
  int status = 0;
  char kill_string[100];

  /* clear the memory */
  memset(&kill_string, 0, sizeof(char) * 100);

  /* Create the killall string*/
  sprintf(kill_string, "killall %s ", conf->video_player);

  /* run the killall */
  status = system(kill_string);

  return status;
}
