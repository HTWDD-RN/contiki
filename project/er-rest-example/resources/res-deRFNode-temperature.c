/**
 * \file
 *      Observable "on-change" temperature resource based on example code
 *      from Matthias Kovatsch and Cristiano De Alti (see res-temperature.c).
 * \author
 *      Alexander Graeb <s74742@htw-dresden.de>
 */

#include "contiki.h"

#define DEBUG 0
#if DEBUG
	#include <stdio.h>
	#define PRINTF(...) printf(__VA_ARGS__)
#else
	#define PRINTF(...)
#endif

#if DE_RF_NODE

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "rest-engine.h"
#include "i2c_sensors_interface.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

#define MAX_AGE      60

/*
 * BUG: Due to a bug, querying the TMP102 sensor as periodic ressource (needed
 * for CoAP subscriptions) will cause the node to get stuck after a while. The
 * node will not respond to any request (CoAP, ping, etc.) then, until system
 * reset. Must to do something with sleep mode, because it's working then sleep
 * mode is disabled (comment out #define RDC_CONF_MCU_SLEEP 1 in platform/
 * avr-atmega128rfa1/contiki-conf.h).
 * Therefor we set up a non-periodic ressource (res_periodic_handler (see
 * above) is never called). This simply means to disable subsscriptions for this
 * ressource as work around.
 */
// Setup non-periodic ressource (no subsscription handliing).
RESOURCE(res_derfnode_tmp102,
         "title=\"Temperature\";rt=\"Temperature\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL);

// Setup periodic ressource for subscription handling.
/*PERIODIC_RESOURCE(res_derfnode_tmp102,
         "title=\"Temperature\";rt=\"Temperature\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         60*CLOCK_SECOND, // Call res_periodic_handler every minute.
         res_periodic_handler);*/

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by REST.notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */

	PRINTF("Get handler for temperature sensor called.\n");  // For debugging.

	// >>>
	/* temperature sensor value */
	temperature_t temp;

	/* measure temperature */
	TMP102_StartOneshotMeasurement();
	TMP102_GetTemperature(&temp, true);
	// <<<

  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%c%d.%02d [°C]\n", (temp.sign ? '-' : '+'), temp.integralDigit, temp.fractionalDigit);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == REST.type.APPLICATION_JSON) {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'temperature':%c%d.%02d [°C]}", (temp.sign ? '-' : '+'), temp.integralDigit, temp.fractionalDigit);

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
	PRINTF("Periodic handler for temperature sensor called.\n");  // For debugging.
	REST.notify_subscribers(&res_derfnode_tmp102); //Notify the registered observers which will trigger the res_get_handler to create the response.
}
#endif /* DE_RF_NODE */
