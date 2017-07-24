#include "config.h"

/**
 * print usage description]
 * @method print_usage
 */
void print_usage() {
  printf("Version %d.%d\n", MAJOR_VERSION, MINOR_VERSION);
  printf("-v : Verbose\n");
  printf("-s : Server mode\n");
  printf("-m : Control mode\n");
  printf("-c : Client mode\n");
  printf("-a : Server address (If not set, will use Zeroconf to find the "
         "server)\n");
  printf("-t : Video Timeout (seconds)\n");
  printf("-h : help\n");
}

/**
 * Print the input config
 * @method display_config
 * @param  conf           [description]
 */
void display_config(Config *co) {

  if (co->server_address != FALSE) {
    printf("Server address:\t%s\n", co->server_address);
  }
  printf("Port:\t\t%s\n", co->port);
  printf("Mode:\t\t%s\n", MODE_STRING[co->mode]);
  printf("Video timeout:\t%d\n", co->timeout);
  printf("Video file:\t%s\n", co->video_file);
  printf("Config file:\t%s\n", co->config_file);
  printf("Log file:\t%s\n", co->log_name);

  if (co->avahi)
    puts("Use Avahi:\tTrue");
  else
    puts("Use Avahi:\tFalse");
}

/**
 * Empty a config struct and return a malloced empty struct
 * @method clear_config
 * @param  in           Config struct pointer
 * @return              Config struct pointer (malloced)
 */
Config *clear_config(Config *in) {
  if (in != NULL)
    free_config(in);

  /* create a new malloc'd struct */
  in = malloc(sizeof(Config));
	in->server_address = NULL;
	in->verbose = 0;
  in->playing = FALSE;
  in->mode = 0;
  in->port = NULL;
  in->timeout = 0;
	in->video_file = NULL;
	in->video_player = NULL;
	in->config_file = NULL;
  in->log_open = false;
	in->log_name = NULL;
  in->log_path = NULL;
	in->avahi = 0;
  return in;
}

/**
 * Free memory used by a Config struct properly
 * @method free_config
 * @param  input       Config struct pointer
 */
void free_config(Config *input) {
  if (input == NULL)
    return;

  if (NULL != input->port)
    free(input->port);
  if (NULL != input->video_file)
    free(input->video_file);
  if (NULL != input->video_player)
    free(input->video_player);
  if (NULL != input->config_file)
    free(input->config_file);
  if (NULL != input->log_name)
    free(input->log_name);
      if (NULL != input->log_path)
    free(input->log_path);
  if (NULL != input->server_address)
    free(input->server_address);

  free(input);

  return;
}

/**
 * Read the config file selected
 * @method read_conf_file
 * @param  settings  Config to store input
 * @return           Config struct (malloced)
 */
Config *read_conf_file(Config *settings, Config *cli_flags) {
  FILE *fp;
  char bufr[BUF_SIZE];
  char val[50];
  char var[50];

  if ((access(cli_flags->config_file, R_OK) == -1)) {
    if (conf->verbose)
      puts("\nConfig file does not exist\nUsing defaults.");
    return NULL;
  }

  Config *tmp = malloc(sizeof(Config));

  if ((fp = fopen(cli_flags->config_file, "r")) != NULL) {
    while (!feof(fp)) {
      fgets(bufr, BUF_SIZE, fp);
      sscanf(bufr, "%s %s", val, var);
      stoupper(val);

      if (strncmp(val, "#", 1) == 0)
        continue;
      /* Log file */
      if (strcmp(val, "LOG_FILE") == 0) {
        tmp->log_name = malloc(BUF_SIZE);
        strcpy(tmp->log_name, var);
      }

      /* sensor timeout time */
      if (strncmp(val, "TIMEOUT", 7) == 0) {
        tmp->timeout = atoi(var);
      }

      /* daemon run mode */
      if (strncmp(val, "MODE", 4) == 0) {
        if (strncmp("SERVER", var, 6) == 0) {
          tmp->mode = SERVER;
        }
        if (strncmp("CONTROL", var, 7) == 0) {
          tmp->mode = CONTROL;
        }
        if (strncmp("CLIENT", var, 6) == 0) {
          tmp->mode = CLIENT;
        }
      }

      /* video file to play */
      if (strncmp(val, "VIDEO_FILE", 10) == 0) {
        tmp->video_file = malloc(BUF_SIZE);
        strcpy(tmp->video_file, var);
      }

      /* Server communication port */
      if (strncmp(val, "PORT", 4) == 0) {
        tmp->port = malloc(BUF_SIZE);
        strcpy(tmp->port, var);
      }

      /* Media player */
      if (strncmp(val, "PLAYER", 6) == 0) {
        tmp->video_player = malloc(BUF_SIZE);
        strcpy(tmp->video_player, var);
      }

      /* Server communication port */
      if (strncmp(val, "ENABLE_AVAHI", 12) == 0) {
        if (strncmp(var, "YES", 3) == 0) {
          tmp->avahi = TRUE;
        }
        if (strncmp(var, "NO", 2) == 0) {
          tmp->avahi = FALSE;
        }
      }

      /* do stuff */
      memset(bufr, '\0', BUF_SIZE - 1);
      memset(val, '\0', 50);
      memset(var, '\0', 50);
    }
    fclose(fp);
  } else {
    puts("unable to open config file");
    /* error processing, couldn't open file */
  }
  return tmp;
}

/**
 * [combine_config description]
 * @method combine_config
 * @param  cli_flags      [description]
 * @param  conf_file      [description]
 * @return                [description]
 */
Config *combine_config(Config *cli_flags, Config *conf_file) {

  Config *tmp = NULL;

  tmp = clear_config(tmp);

  tmp->server_address = malloc(BUF_SIZE);
	if (cli_flags->server_address != NULL) {
    strcpy(tmp->server_address, cli_flags->server_address);
  } else {
		if (conf_file != NULL && conf_file->server_address != NULL) {
      strcpy(tmp->server_address, conf_file->server_address);
    } else {
      strcpy(tmp->server_address, DEFAULT_SERVER);
    }
  }

  // Verbose
  if (cli_flags->verbose) {
    tmp->verbose = cli_flags->verbose;
  } else {
		if (conf_file != NULL && conf_file->verbose) {
			tmp->verbose = conf_file->verbose;
    } else {
      tmp->verbose = 0;
    }
  }

  // Loop type
  if (cli_flags->mode != ERROR) {
    tmp->mode = cli_flags->mode;
  } else {
		if (conf_file != NULL && conf_file->mode) {
			tmp->mode = conf_file->mode;
    } else {
      tmp->mode = CLIENT;
    }
  }

  // Port
  tmp->port = malloc(BUF_SIZE);
  if (cli_flags->port != 0) {
    strcpy(tmp->port, cli_flags->port);
  } else {
		if (conf_file != NULL && conf_file->port != NULL) {
      strcpy(tmp->port, conf_file->port);
    } else {
      strcpy(tmp->port, DEFAULT_PORT);
    }
  }

  // Video Timeout
  if (cli_flags->timeout != 0) {
    tmp->timeout = cli_flags->timeout;
  } else {
		if (conf_file != NULL && conf_file->timeout != 0) {
      tmp->timeout = conf_file->timeout;
    } else {
      tmp->timeout = DEFAULT_TIMEOUT;
    }
  }

  // Video file
  tmp->video_file = malloc(BUF_SIZE);
	if (cli_flags->video_file != NULL) {
    strcpy(tmp->video_file, cli_flags->video_file);
  } else {
		if (conf_file != NULL && conf_file->video_file != NULL) {
      strcpy(tmp->video_file, conf_file->video_file);
    } else {
      strcpy(tmp->video_file, DEFAULT_VIDEO);
    }
  }

  // Video player
  tmp->video_player = malloc(BUF_SIZE);
	if (cli_flags->video_player != NULL) {
    strcpy(tmp->video_player, cli_flags->video_player);
  } else {
		if (conf_file != NULL && conf_file->video_player != NULL) {
      strcpy(tmp->video_player, conf_file->video_player);
    } else {
      strcpy(tmp->video_player, DEFAULT_PLAYER);
    }
  }

  // config file
  tmp->config_file = malloc(BUF_SIZE);
	if (cli_flags->config_file != NULL) {
    strcpy(tmp->config_file, cli_flags->config_file);
  } else {
		if (conf_file != NULL && conf_file->config_file != NULL) {
      strcpy(tmp->config_file, conf_file->config_file);
    } else {
      strcpy(tmp->config_file, DEFAULT_CONFIG_FILE);
    }
  }

  // Logname
  tmp->log_name = malloc(BUF_SIZE);
	if (cli_flags->log_name != NULL) {
    strcpy(tmp->log_name, cli_flags->log_name);
  } else {
		if (conf_file != NULL && conf_file->log_name != NULL) {
      strcpy(tmp->log_name, conf_file->log_name);
    } else {
      strcpy(tmp->log_name, DEFAULT_LOG_FILE);
    }
  }

  // Logname
  tmp->log_path = malloc(BUF_SIZE);
	if (cli_flags->log_path != NULL) {
    strcpy(tmp->log_path, cli_flags->log_path);
  } else {
		if (conf_file != NULL && conf_file->log_path != NULL) {
      strcpy(tmp->log_path, conf_file->log_path);
    } else {
      strcpy(tmp->log_path, DEFAULT_LOG_PATH);
    }
  }

  // Avahi
  if (cli_flags->avahi) {
    tmp->avahi = cli_flags->avahi;
  } else {
		if (conf_file != NULL && conf_file->avahi) {
      tmp->avahi = conf_file->avahi;
    } else {
      tmp->avahi = FALSE;
    }
  }

  // free_config(cli_flags);
	if(conf_file != NULL)
  free_config(conf_file);

  // Return the config
  return tmp;
}

/**
 * Read cli flags
 * @method read_cli_inputs
 * @param  argc            [description]
 * @param  argv            [description]
 * @return                 [description]
 */
Config *read_cli_inputs(int argc, char *argv[]) {
  int c;

  /* RetFurn struct */
  Config *tmp = NULL;
  tmp = clear_config(tmp);

  while (1) {
    struct option long_options[] = {

        /* These options set a flag. */
        {"verbose", no_argument, 0, 'v'},
        {"enable-avahi", no_argument, 0, 'A'},

        /* These options don’t set a flag. We distinguish them by their
           indices. */
        {"help", no_argument, 0, 'h'},
        {"server", no_argument, 0, 's'},
        {"client", no_argument, 0, 'c'},
        {"control", no_argument, 0, 'm'},
        {"timeout", required_argument, 0, 't'},
        {"address", required_argument, 0, 'a'},
        {"port", required_argument, 0, 'p'},
        {"video", required_argument, 0, 'V'},
        {"log", required_argument, 0, 'l'},
        {"config", required_argument, 0, 'g'},
			{0, 0, 0, 0}
		};

    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "hvAscma:t:g:l:p:V:", long_options,
                    &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;

    case 'h':
      print_usage();
      exit(EXIT_SUCCESS);
      break;

    case 'v':
      tmp->verbose = TRUE;
      conf->verbose = TRUE;
      break;

    case 'A':
      tmp->avahi = TRUE;
      break;

    case 's':
      tmp->mode = SERVER; // echo server
      break;

    case 'm':
      tmp->mode = CONTROL; // Sensor input controller
      break;

    case 'c':
      tmp->mode = CLIENT; // player
      break;

    case 'a':
      tmp->server_address = optarg;
      break;

    case 't':
      tmp->timeout = atoi(optarg);
      break;

    case 'V':
      tmp->video_file = optarg;
      break;

    case 'l':
      tmp->log_name = optarg;
      break;

    case 'p':
      tmp->port = optarg;
      break;

    case 'g':
      tmp->config_file = optarg;
      break;

    case '?':
      /* getopt_long already printed an error message. */
      break;

    default:
      abort();
    }
  }

  /* Instead of reporting  E-verbose E
     and  E-brief Eas they are encountered,
     we report the final status resulting from them. */
  if (conf->verbose)
    puts("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    putchar('\n');
  }

  if (tmp->config_file == NULL) {
    tmp->config_file = malloc(BUF_SIZE);
    strcpy(tmp->config_file, DEFAULT_CONFIG_FILE);
  }
  return tmp;
}
