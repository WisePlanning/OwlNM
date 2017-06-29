#include "OwlNM.h"

/**
 * Start the correct run loop based on the configuration taken from the config
 * file and over ridden by the cli switches.
 * if the config file does not ecist, we use hard coded values.
 *
 * @method main
 * @param  argc
 * @param  argv
 * @return int
 */
int main(int argc, char *argv[]) {

  pid_t controller;

  /* cli config input struct */
  Config *cli_flags;

  /* config file input struct */
  Config *conf_file;

  /* clear the global conf */
  conf = clear_config(conf);

  /* override config file with cli switches */
  cli_flags = read_cli_inputs(argc, argv);

  /* read options from config file */
  conf_file = read_conf_file(conf);

  /* Combine */
  conf = combine_config(cli_flags, conf);

  if (conf->verbose)
    display_config();

  /* Choose the run loop for the mode selected */
  switch (conf->mode) {

  /* server mode */
  case SERVER:

    /* fork the program */
    controller = fork();

    if (controller != 0) {

      if (rootCheck()) {

        /* start the controller run loop in the controller fork */
        control_run_loop()
      } else {

        if (conf->verbose) {
          puts("\nNot root, unable to run as controller");
        }
        /* Shut down unused fork */
        exit(EXIT_SUCCESS);
      }
    } else {

      /* Start server loop */
      server_run_loop();
    }
    break;

  case CONTROL:
    /* check for root permissions */
    if (rootCheck()) {

      /* start the control loop */
      control_run_loop();
    } else {
      puts("Controller must be run as Root\n");
    }
    break;

  case CLIENT:
    /* start the client loop */
    client_run_loop();
    break;

  case ERROR:
  default:
    print_usage();
    break;
  } /* switch */

  free(conf);
  exit(EXIT_SUCCESS);
} /* main */
