#include "service_server.h"

static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static char *name = NULL;
static void create_services(AvahiClient *c);

/**
 * [entry_group_callback description]
 * @method entry_group_callback
 * @param  g                    [description]
 * @param  state                [description]
 * @param  userdata             [description]
 */
static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                 AVAHI_GCC_UNUSED void *userdata) {
	assert(g == group || group == NULL);
	group = g;

	/* Called whenever the entry group state changes */
	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED:

			/* The entry group has been established successfully */
			fprintf(stderr, "Service '%s' successfully established.\n", name);
			break;
		case AVAHI_ENTRY_GROUP_COLLISION: {
			char *n;

			/* A service name collision with a remote service
			 * happened. Let's pick a new name */
			n = avahi_alternative_service_name(name);
			avahi_free(name);
			name = n;
			fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

			/* And recreate the services */
			create_services(avahi_entry_group_get_client(g));
			break;
		}
		case AVAHI_ENTRY_GROUP_FAILURE:
			fprintf(
				stderr, "Entry group failure: %s\n",
				avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

			/* Some kind of failure happened while we were registering our services */
			avahi_simple_poll_quit(simple_poll);
			break;
		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:;
	}
}

/**
 * [create_services description]
 * @method create_services
 * @param  c               [description]
 */
static void create_services(AvahiClient *c) {
	char *n, r[128];
	int ret;
	assert(c);

	/* If this is the first time we're called, let's create a new
	 * entry group if necessary */
	if (!group)
		if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
			fprintf(stderr, "avahi_entry_group_new() failed: %s\n",
			        avahi_strerror(avahi_client_errno(c)));
			goto fail;
		}

	/* If the group is empty (either because it was just created, or
	 * because it was reset previously, add our entries.  */
	if (avahi_entry_group_is_empty(group)) {
		fprintf(stderr, "Adding service '%s'\n", name);

		/* Create some random TXT data */
		snprintf(r, sizeof(r), "random=%i", rand());

		/* Add the service for server */
		if ((ret = avahi_entry_group_add_service(
				 group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0, name, SERVICE_NAME,
				 NULL, NULL, atoi(conf->port), "Media Control Server", r, NULL)) <
		    0) {
			if (ret == AVAHI_ERR_COLLISION)
				goto collision;
			fprintf(stderr, "Failed to add _OwlNM._tcp service: %s\n",
			        avahi_strerror(ret));
			goto fail;
		}

		/* Tell the server to register the service */
		if ((ret = avahi_entry_group_commit(group)) < 0) {
			fprintf(stderr, "Failed to commit entry group: %s\n",
			        avahi_strerror(ret));
			goto fail;
		}
	}
	return;

collision:

	/* A service name collision with a local service happened. Let's
	 * pick a new name */
	n = avahi_alternative_service_name(name);
	avahi_free(name);
	name = n;
	fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);
	avahi_entry_group_reset(group);
	create_services(c);
	return;
fail:
	avahi_simple_poll_quit(simple_poll);
}

/**
 * [client_callback description]
 * @method client_callback
 * @param  c               [description]
 * @param  state           [description]
 * @param  userdata        [description]
 */
static void client_callback(AvahiClient *c, AvahiClientState state,
                            AVAHI_GCC_UNUSED void *userdata) {
	assert(c);

	/* Called whenever the client or server state changes */
	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:

			/* The server has startup successfully and registered its host
			 * name on the network, so it's time to create our services */
			create_services(c);
			break;
		case AVAHI_CLIENT_FAILURE:
			fprintf(stderr, "Client failure: %s\n",
			        avahi_strerror(avahi_client_errno(c)));
			avahi_simple_poll_quit(simple_poll);
			break;
		case AVAHI_CLIENT_S_COLLISION:

		/* Let's drop our registered services. When the server is back
		 * in AVAHI_SERVER_RUNNING state we will register them
		 * again with the new host name. */
		case AVAHI_CLIENT_S_REGISTERING:

			/* The server records are now being established. This
			 * might be caused by a host name change. We need to wait
			 * for our own records to register until the host name is
			 * properly esatblished. */
			if (group)
				avahi_entry_group_reset(group);
			break;
		case AVAHI_CLIENT_CONNECTING:;
	}
}

/**
 * [service_server description]
 * @method service_server
 * @return [description]
 */
int avahi_server() {
	if (!conf->avahi)
		exit(0);

	LOG_WRITE("Starting Avahi broadcasting.\n");

	AvahiClient *client = NULL;
	int error;
	int ret = 1;

	/* Allocate main loop object */
	if (!(simple_poll = avahi_simple_poll_new())) {
		fprintf(stderr, "Failed to create simple poll object.\n");
		goto fail;
	}

	name = avahi_strdup("OwlNM");

	/* Allocate a new client */
	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);

	/* Check wether creating the client object succeeded */
	if (!client) {
		fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
		goto fail;
	}

	/* Run the main loop */
	avahi_simple_poll_loop(simple_poll);
	ret = 0;

fail:

	/* Cleanup things */
	if (client)
		avahi_client_free(client);
	if (simple_poll)
		avahi_simple_poll_free(simple_poll);
	avahi_free(name);
	return ret;
}
