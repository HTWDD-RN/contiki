/**
 * \file
 *      RSSI monitor resource based on example code from Matthias Kovatsch
 *      and Cristiano De Alti (see res-temperature.c).
 * \author
 *      Alexander Graeb <s74742@htw-dresden.de>
 */
 
 #include "contiki.h"
 
#if DE_RF_NODE

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "rest-engine.h"
#include "core/dev/radio.h"
#include "radio/rf230bb/rf230bb.h"

#define MAX_AGE 60

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

#define MAX_AGE      60

// Setup periodic ressource for subscription handling.
PERIODIC_RESOURCE(res_rf230_rssi,
         "title=\"RSSI statistic\";rt=\"RSSI\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         10*CLOCK_SECOND, // Call res_periodic_handler every 10 seconds.
         res_periodic_handler);

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "RSSI: %i [dBm]\n", rf230_last_rssi-90);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == REST.type.APPLICATION_JSON) {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{RSSI':%i [dBm]}\n", rf230_last_rssi-90);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }

  REST.set_header_max_age(response, MAX_AGE);

  /* The REST.subscription_handler() will be called for observable resources by the REST framework. */
}

/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
	REST.notify_subscribers(&res_rf230_rssi); //Notify the registered observers which will trigger the res_get_handler to create the response.
}
#endif /* DE_RF_NODE */
