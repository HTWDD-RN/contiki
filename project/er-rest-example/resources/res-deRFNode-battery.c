/**
 * \file
 *      Observable "on-change" battery resource based on example code
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
#include "dev/adc.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

#define MAX_AGE      60
#define INTERVAL_MIN 5
#define INTERVAL_MAX (MAX_AGE - 1)
#define CHANGE       0 

static int32_t interval_counter = INTERVAL_MIN;
static int voltage_old = INT_MIN;

PERIODIC_RESOURCE(res_derfnode_battery,
         "title=\"Battery voltage\";rt=\"Battery\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         60*CLOCK_SECOND,
         res_periodic_handler);

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	PRINTF("Get handler for battery sensor called.\n");  // For debugging.

	// >>>
	/* Battery sensor value */
	int adcVal = readADC(PF0);
	// gemessene Spannung an ADC0 in V auf 2 nachkommastellen genau (festkomma arithmetik), ref Spannung = 1.6V
	unsigned long int adc_in_v = ((uint32_t)adcVal * 160) / 1023;
	// Spannungsteilerformel nach Vin umgestellt mit R1=820kOhm, R2=220kOhm
	unsigned int voltage = (adc_in_v * 1040) / 220;
	// <<<

  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%2d.%02d [V]\n", voltage/100, voltage%100);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == REST.type.APPLICATION_JSON) {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
	snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'temperature':%2d.%02d [V]}", voltage/100, voltage%100);

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
	/*
	 * Periodic handler does not work yet. Leads node to not respond to requests (coap, ping)
	 * anymore within a few seconds until reset the node.
	 * Must do something with sleep mode, because it's working then sleep mode disabled (comment out
	 * #define RDC_CONF_MCU_SLEEP 1 in platform/avr-atmega128rfa1/contiki-conf.h).
	 */

	PRINTF("Periodic handler for battery sensor called.\n");  // For debugging.

	// >>>
	/* Battery sensor value */
	int adcVal = readADC(PF0);
	// gemessene Spannung an ADC0 in V auf 2 nachkommastellen genau (festkomma arithmetik), ref Spannung = 1.6V
	unsigned long int adc_in_v = ((uint32_t)adcVal * 160) / 1023;
	// Spannungsteilerformel nach Vin umgestellt mit R1=820kOhm, R2=220kOhm
	unsigned int voltage = (adc_in_v * 1040) / 220;
	// <<<

  ++interval_counter;

  if((abs(voltage - voltage_old) >= CHANGE && interval_counter >= INTERVAL_MIN) || 
     interval_counter >= INTERVAL_MAX) {
     interval_counter = 0;
     voltage_old = voltage;
    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    REST.notify_subscribers(&res_derfnode_battery);
  }
}
#endif /* DE_RF_NODE */
