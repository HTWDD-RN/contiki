/**
 * \file
 *      Observable ATmega128RFA1 internal temperature sensor resource
 *      based on example code from Matthias Kovatsch and Cristiano De
 *      Alti (see res-temperature.c).
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

#if PLATFORM_TYPE == ATMEGA128RFA1

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "rest-engine.h"
#include "dev/adc.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

#define MAX_AGE      60
#define INTERVAL_MIN 5
#define INTERVAL_MAX (MAX_AGE - 1)
#define CHANGE       0

static int32_t interval_counter = INTERVAL_MIN;
static int temperature_old = 0;

PERIODIC_RESOURCE(res_atmega128rfa1_temp,
         "title=\"ATmega128RFA1 internal temperature sensor\";rt=\"CPU temperature\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         60*CLOCK_SECOND,
         res_periodic_handler);

/* Called on get request. */     
static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	// Temperatur auf 2 Nachkommastellen genau (festkomma arithmetik)
	int temperature = readInternalTemp();
	
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);

	if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
		REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
		snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%3d.%02d [°C]\n", temperature/100, temperature%100);

		REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
	} else if(accept == REST.type.APPLICATION_JSON) {
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'temperature':%3d.%02d [°C]}", temperature/100, temperature%100);

		REST.set_response_payload(response, buffer, strlen((char *)buffer));
	} else {
		REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
		const char *msg = "Supporting content-types text/plain and application/json";
		REST.set_response_payload(response, msg, strlen(msg));
	}

	REST.set_header_max_age(response, MAX_AGE);
}

/* Called frequently. See periode above. */
static void
res_periodic_handler()
{
	int temperature = readInternalTemp();

	++interval_counter;

	// Notify observers only if sensor value has changed enough or a maximum period of time elapsed.
	if((abs(temperature - temperature_old) >= CHANGE && interval_counter >= INTERVAL_MIN) || interval_counter >= INTERVAL_MAX) {
		interval_counter = 0;
		temperature_old = temperature;
		/* Notify the registered observers which will trigger the res_get_handler to create the response. */
		REST.notify_subscribers(&res_atmega128rfa1_temp);
	}
}

#endif /* PLATFORM_TYPE == ATMEGA128RFA1 */
