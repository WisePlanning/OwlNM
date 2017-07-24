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

  /* Cli configuration struct */
  Config *cli_flags;

  /* Conf file struct */
  Config *conf_file;

  /* Clear the global conf */
  conf = clear_config(conf);

  /* override config file with cli switches */
  cli_flags = read_cli_inputs(argc, argv);

  /* read options from config file */
  conf_file = read_conf_file(conf,cli_flags);

  /* Combine */
  conf = combine_config(cli_flags, conf_file);

  /* Show the config in use */
  if (conf->verbose)
    display_config(conf);

  /* Choose the run loop for the mode selected */
  switch (conf->mode) {

  /* server mode */
  case SERVER:

    /* Start server loop */
      server_run_loop();
    break;

  case CONTROL:
    /* check for root permissions */
    if (rootCheck()) {

      /* start the control loop */
      control_run_loop();
    } else {

      fprintf(stderr,"Controller must be run as Root\n");
      free(conf);
      exit(EXIT_FAILURE);
    }
    break;

  case CLIENT:
    /* start the client loop */
    client_run_loop();
    break;

  case ERROR:
    print_usage();
    break;

  default:
    print_usage();
    break;
  } /* switch */

  free(conf);
  exit(EXIT_SUCCESS);
} /* main */
