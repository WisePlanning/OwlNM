/***
 * This file is part of avahi.
 * avahi is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 * avahi is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
 * Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with avahi; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 ***/
#include "config.h"

static AvahiSimplePoll *simple_poll = NULL;
/**
 * [resolve_callback description]
 * @method resolve_callback
 * @param  r                [description]
 * @param  interface        [description]
 * @param  protocol         [description]
 * @param  event            [description]
 * @param  name             [description]
 * @param  type             [description]
 * @param  domain           [description]
 * @param  host_name        [description]
 * @param  address          [description]
 * @param  port             [description]
 * @param  txt              [description]
 * @param  flags            [description]
 * @param  userdata         [description]
 */
static void resolve_callback(AvahiServiceResolver *r,
                             AVAHI_GCC_UNUSED AvahiIfIndex interface,
                             AVAHI_GCC_UNUSED AvahiProtocol protocol,
                             AvahiResolverEvent event, const char *name,
                             const char *type, const char *domain,
                             const char *host_name, const AvahiAddress *address,
                             uint16_t port, AvahiStringList *txt,
                             AvahiLookupResultFlags flags,
                             AVAHI_GCC_UNUSED void *userdata) {
	assert(r);

	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			fprintf(stderr,
			        "(Resolver) Failed to resolve service '%s' of type '%s' in domain "
			        "'%s': %s\n",
			        name, type, domain,
			        avahi_strerror(
						avahi_client_errno(avahi_service_resolver_get_client(r))));
			break;

		case AVAHI_RESOLVER_FOUND: {
			char a[AVAHI_ADDRESS_STR_MAX], *t;
			if (conf->verbose) {
				fprintf(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);
			}

			avahi_address_snprint(a, sizeof(a), address);
			t = avahi_string_list_to_string(txt);

			if (conf->verbose)
				fprintf(stderr,
				        "\t%s:%u (%s)\n"
				        "\tTXT=%s\n"
				        "\tcookie is %u\n"
				        "\tis_local: %i\n"
				        "\tour_own: %i\n"
				        "\twide_area: %i\n"
				        "\tmulticast: %i\n"
				        "\tcached: %i\n",
				        host_name, port, a, t, avahi_string_list_get_service_cookie(txt),
				        !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
				        !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
				        !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
				        !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
				        !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

			if (conf->server_address == NULL)
				conf->server_address = malloc(BUF_SIZE);

			memset(conf->server_address, 0, BUF_SIZE);

			strncpy(conf->server_address, a, BUF_SIZE - 1);

			// printf("%s\n", conf->server_address);

			avahi_free(t);
			avahi_simple_poll_quit(simple_poll);
		}
	} /* switch */
	avahi_service_resolver_free(r);
} /* resolve_callback */

/**
 * [browse_callback description]
 * @method browse_callback
 * @param  b               [description]
 * @param  interface       [description]
 * @param  protocol        [description]
 * @param  event           [description]
 * @param  name            [description]
 * @param  type            [description]
 * @param  domain          [description]
 * @param  flags           [description]
 * @param  userdata        [description]
 */
static void browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface,
                            AvahiProtocol protocol, AvahiBrowserEvent event,
                            const char *name, const char *type,
                            const char *domain,
                            AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                            void *userdata) {
	AvahiClient *c = userdata;
	assert(b);

	/* Called whenever a new services becomes available on the LAN or is removed
	 * from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			if (conf->verbose)
				fprintf(stderr, "(Browser) %s\n",
				        avahi_strerror(
							avahi_client_errno(avahi_service_browser_get_client(b))));
			avahi_simple_poll_quit(simple_poll);

			return;

		case AVAHI_BROWSER_NEW:
			if (conf->verbose)
				fprintf(stderr,
				        "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name,
				        type, domain);

			/* We ignore the returned resolver object. In the callback
			 * function we free it. If the server is terminated before
			 * the callback function is called the server will free
			 * the resolver for us. */
			if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain,
			                                 AVAHI_PROTO_UNSPEC, 0, resolve_callback,
			                                 c)))
				fprintf(stderr, "Failed to resolve service '%s': %s\n", name,
				        avahi_strerror(avahi_client_errno(c)));
			break;
		case AVAHI_BROWSER_REMOVE:
			fprintf(stderr,
			        "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n",
			        name, type, domain);
			break;
		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			if (conf->verbose)
				fprintf(stderr, "(Browser) %s\n",
				        event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED"
						: "ALL_FOR_NOW");
			break;
	} /* switch */
} /* browse_callback */

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
	if (state == AVAHI_CLIENT_FAILURE) {
		fprintf(stderr, "Server connection failure: %s\n",
		        avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(simple_poll);
	}
} /* client_callback */

/**
 * [service_client description]
 * @method service_client
 * @return [description]
 */
int avahi_client() {
	if (!conf->avahi)
		return 0;

	LOG_WRITE("Starting Avahi client.\n");

	AvahiClient *client = NULL;
	AvahiServiceBrowser *sb = NULL;
	int error;
	int ret = 1;

	/* Allocate main loop object */
	if (!(simple_poll = avahi_simple_poll_new())) {
		fprintf(stderr, "Failed to create simple poll object.\n");
		goto fail;
	}

	/* Allocate a new client */
	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0,
	                          client_callback, NULL, &error);

	/* Check wether creating the client object succeeded */
	if (!client) {
		fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
		goto fail;
	}

	/* Create the service browser */
	if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC,
	                                     AVAHI_PROTO_UNSPEC, SERVICE_NAME, NULL,
	                                     0, browse_callback, client))) {
		fprintf(stderr, "Failed to create service browser: %s\n",
		        avahi_strerror(avahi_client_errno(client)));
		goto fail;
	}

	/* Run the main loop */
	avahi_simple_poll_loop(simple_poll);
	ret = 0;

fail:
	/* Cleanup things */
	if (sb)
		avahi_service_browser_free(sb);
	if (client)
		avahi_client_free(client);
	if (simple_poll)
		avahi_simple_poll_free(simple_poll);
	return ret;
} /* main */
