/*
 * Copyright (c) 2012, Matthias Kovatsch and other contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP Sensornode made from Erbium (Er) REST Engine example (with CoAP-specific code)
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 *      Thomas Kuehnel <kuehnelth@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"

/* Define which resources to include to meet memory constraints. */
#define REST_RES_EVENT 0
#define REST_RES_LEDS 1
#define REST_RES_TOGGLE 1
#define REST_RES_SHT21 1
#define REST_RES_BMP085 1

#if !UIP_CONF_IPV6_RPL && !defined (CONTIKI_TARGET_MINIMAL_NET) && !defined (CONTIKI_TARGET_NATIVE)
#warning "Compiling with static routing!"
#include "static-routing.h"
#endif

#include "erbium.h"


#if defined (PLATFORM_HAS_BUTTON)
#include "dev/button-sensor.h"
#endif
#if defined (PLATFORM_HAS_LEDS)
#include "dev/leds.h"
#endif
#if defined (PLATFORM_HAS_SHT21)
#include "dev/i2c/sht21-sensor.h"
#endif
#if defined (PLATFORM_HAS_BMP085)
#include "dev/i2c/bmp085-sensor.h"
#endif


/* For CoAP-specific example: not required for normal RESTful Web service. */
#if WITH_COAP == 3
#include "er-coap-03.h"
#elif WITH_COAP == 7
#include "er-coap-07.h"
#elif WITH_COAP == 12
#include "er-coap-12.h"
#elif WITH_COAP == 13
#include "er-coap-13.h"
#else
#warning "Erbium example without CoAP-specifc functionality"
#endif /* CoAP-specific example */

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/******************************************************************************/
#if REST_RES_EVENT && defined (PLATFORM_HAS_BUTTON)
/*
 * Example for an event resource.
 * Additionally takes a period parameter that defines the interval to call [name]_periodic_handler().
 * A default post_handler takes care of subscriptions and manages a list of subscribers to notify.
 */
EVENT_RESOURCE(event, METHOD_GET, "sensors/button", "title=\"Event demo\";obs");

void
event_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  /* Usually, a CoAP server would response with the current resource representation. */
  const char *msg = "It's eventful!";
  REST.set_response_payload(response, (uint8_t *)msg, strlen(msg));

  /* A post_handler that handles subscriptions/observing will be called for periodic resources by the framework. */
}

/* Additionally, a handler function named [resource name]_event_handler must be implemented for each PERIODIC_RESOURCE defined.
 * It will be called by the REST manager process with the defined period. */
void
event_event_handler(resource_t *r)
{
  static uint16_t event_counter = 0;
  static char content[12];

  ++event_counter;

  PRINTF("TICK %u for /%s\n", event_counter, r->url);

  /* Build notification. */
  coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
  coap_init_message(notification, COAP_TYPE_CON, REST.status.OK, 0 );
  coap_set_payload(notification, content, snprintf(content, sizeof(content), "EVENT %u", event_counter));

  /* Notify the registered observers with the given message type, observe option, and payload. */
  REST.notify_subscribers(r, event_counter, notification);
}
#endif /* PLATFORM_HAS_BUTTON */

/******************************************************************************/
#if defined (PLATFORM_HAS_LEDS)
/******************************************************************************/
#if REST_RES_LEDS
/*A simple actuator example, depending on the color query parameter and post variable mode, corresponding led is activated or deactivated*/
RESOURCE(leds, METHOD_POST | METHOD_PUT , "actuators/leds", "title=\"LEDs: ?color=r|g|b, POST/PUT mode=on|off\";rt=\"Control\"");

void
leds_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t len = 0;
  const char *color = NULL;
  const char *mode = NULL;
  uint8_t led = 0;
  int success = 1;
  int changed = 0;

  if ((len=REST.get_query_variable(request, "color", &color))) {
    PRINTF("color %.*s\n", len, color);

    if (strncmp(color, "r", len)==0) {
      led = LEDS_RED;
    } else if(strncmp(color,"g", len)==0) {
      led = LEDS_GREEN;
    } else if (strncmp(color,"y", len)==0) {
      led = LEDS_YELLOW;
    } else {
      success = 0;
    }
  } else {
    success = 0;
  }

  if (success && (len=REST.get_post_variable(request, "mode", &mode))) {
    PRINTF("mode %s\n", mode);

    if (strncmp(mode, "on", len)==0) {
      changed = (leds_get() & led) == 0;
      leds_on(led);
    } else if (strncmp(mode, "off", len)==0) {
      changed = (leds_get() & led) == led;
      leds_off(led);
    } else {
      success = 0;
    }
  } else {
    success = 0;
  }

  if (!success) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    if (changed)
      REST.set_response_status(response, REST.status.CHANGED);
  }
}
#endif

/******************************************************************************/
#if REST_RES_TOGGLE
/* A simple actuator example. Toggles the red led */
RESOURCE(toggle, METHOD_POST, "actuators/toggle", "title=\"Red LED\";rt=\"Control\"");
void
toggle_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  leds_toggle(LEDS_RED);
}
#endif
#endif /* PLATFORM_HAS_LEDS */

/******************************************************************************/
#if REST_RES_SHT21 && defined (PLATFORM_HAS_SHT21)
/* A simple getter example. Returns the reading from sht21 sensor with a simple etag */
PERIODIC_RESOURCE(sht21_temperature, METHOD_GET, "sensors/sht21_temperature", "title=\"Temperature in [°C] (supports JSON)\";rt=\"Temperature-C\";if=\"core.s\";obs", 60*CLOCK_SECOND);
void
sht21_temperature_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char tempstr[20] = "ERROR";
  if (sht21_sensor.value(SHT21_SENSOR_TEMP) == 1)
    sht21_sensor_getLastTemperature(tempstr, sizeof(tempstr));
  if (strcmp(tempstr, "ERROR") == 0)
    REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);

  const uint16_t *accept = NULL;
  int num = REST.get_header_accept(request, &accept);

  if ((num==0) || (num && accept[0]==REST.type.TEXT_PLAIN))
  {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s", tempstr);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_XML))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "<temperature value=\"%s\"/>", tempstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_JSON))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'temperature':'%s'}", tempstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else
  {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain, application/xml, and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }
}

void
sht21_temperature_periodic_handler(resource_t *r)
{
  static uint16_t obs_counter = 0;
  char tempstr[20] = "ERROR";
  if (sht21_sensor.value(SHT21_SENSOR_TEMP) == 1)
    sht21_sensor_getLastTemperature(tempstr, sizeof(tempstr));
  /* Build notification. */
  coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
  coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
  coap_set_payload(notification, tempstr, strlen(tempstr));
  if (strcmp(tempstr, "ERROR") == 0)
    coap_set_status_code(notification, REST.status.INTERNAL_SERVER_ERROR);

  /* Notify the registered observers with the given message type, observe option, and payload. */
  REST.notify_subscribers(r, obs_counter, notification);
}

PERIODIC_RESOURCE(sht21_humidity, METHOD_GET, "sensors/sht21_humidity", "title=\"Relative Humidity in [%RH] (supports JSON)\";rt=\"Humidity-rel\";if=\"core.s\";obs", 60*CLOCK_SECOND);
void
sht21_humidity_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char humstr[20] = "ERROR";
  if (sht21_sensor.value(SHT21_SENSOR_RELATIVE_HUMIDITY) == 1)
    sht21_sensor_getLastelativeHumidity(humstr, sizeof(humstr));
  if (strcmp(humstr, "ERROR") == 0)
    REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);

  const uint16_t *accept = NULL;
  int num = REST.get_header_accept(request, &accept);

  if ((num==0) || (num && accept[0]==REST.type.TEXT_PLAIN))
  {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s", humstr);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_XML))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "<humidity relative=\"%s\"/>", humstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_JSON))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'humidity': {'relative':'%s'}}", humstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else
  {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain, application/xml, and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }
}

void
sht21_humidity_periodic_handler(resource_t *r)
{
  static uint16_t obs_counter = 0;
  char humstr[20] = "ERROR";
  if (sht21_sensor.value(SHT21_SENSOR_RELATIVE_HUMIDITY) == 1)
    sht21_sensor_getLastelativeHumidity(humstr, sizeof(humstr));
  /* Build notification. */
  coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
  coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
  coap_set_payload(notification, humstr, strlen(humstr));
  if (strcmp(humstr, "ERROR") == 0)
    coap_set_status_code(notification, REST.status.INTERNAL_SERVER_ERROR);

  /* Notify the registered observers with the given message type, observe option, and payload. */
  REST.notify_subscribers(r, obs_counter, notification);
}

#endif /* PLATFORM_HAS_SHT21 */

/******************************************************************************/
#if REST_RES_BMP085 && defined (PLATFORM_HAS_BMP085)
/* A simple getter example. Returns the reading from bmp085 sensor with a simple etag */
PERIODIC_RESOURCE(bmp085, METHOD_GET, "sensors/bmp085", "title=\"Air pressure\";rt=\"Pressure-hPa\";if=\"core.s\";obs", 60*CLOCK_SECOND);
void
bmp085_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char tempstr[20] = "ERROR";
  int32_t p, t;
  if (bmp085_sensor_getPressure(&p, &t) == true)
    snprintf(tempstr, 20, "%4ld.%02ld hPa", p/100, p%100);
  if (strcmp(tempstr, "ERROR") == 0)
    REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);

  const uint16_t *accept = NULL;
  int num = REST.get_header_accept(request, &accept);

  if ((num==0) || (num && accept[0]==REST.type.TEXT_PLAIN))
  {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s", tempstr);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_XML))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "<pressure value=\"%s\"/>", tempstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else if (num && (accept[0]==REST.type.APPLICATION_JSON))
  {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'pressure':'%s'}", tempstr);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  }
  else
  {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain, application/xml, and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }
}

void
bmp085_periodic_handler(resource_t *r)
{
  static uint16_t obs_counter = 0;
  char tempstr[20] = "ERROR";
  int32_t p, t;
  if (bmp085_sensor_getPressure(&p, &t) == true)
    snprintf(tempstr, 20, "%4ld.%02ld hPa", p/100, p%100);
  printf("%s\n", tempstr);

  /* Build notification. */
  coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
  coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
  coap_set_payload(notification, tempstr, strlen(tempstr));
  if (strcmp(tempstr, "ERROR") == 0)
    coap_set_status_code(notification, REST.status.INTERNAL_SERVER_ERROR);

  /* Notify the registered observers with the given message type, observe option, and payload. */
  REST.notify_subscribers(r, obs_counter, notification);
}
#endif /* PLATFORM_HAS_BMP085 */




PROCESS(rest_server_example, "CoAP Sensornode");
AUTOSTART_PROCESSES(&rest_server_example);

PROCESS_THREAD(rest_server_example, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Starting CoAP Sensornode\n");

#ifdef RF_CHANNEL
  PRINTF("RF channel: %u\n", RF_CHANNEL);
#endif
#ifdef IEEE802154_PANID
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);
#endif

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

/* if static routes are used rather than RPL */
#if !UIP_CONF_IPV6_RPL && !defined (CONTIKI_TARGET_MINIMAL_NET) && !defined (CONTIKI_TARGET_NATIVE)
  set_global_address();
  configure_routing();
#endif

  /* Initialize the REST engine. */
  rest_init_engine();

  /* Activate the application-specific resources. */
#if defined (PLATFORM_HAS_BUTTON) && REST_RES_EVENT
  SENSORS_ACTIVATE(button_sensor);
  rest_activate_event_resource(&resource_event);
#endif
#if defined (PLATFORM_HAS_LEDS)
#if REST_RES_LEDS
  rest_activate_resource(&resource_leds);
#endif
#if REST_RES_TOGGLE
  rest_activate_resource(&resource_toggle);
#endif
#endif /* PLATFORM_HAS_LEDS */
#if defined (PLATFORM_HAS_SHT21) && REST_RES_SHT21
  int result_sht21 = SENSORS_ACTIVATE(sht21_sensor);  
  if(result_sht21 == 1) { // ok
    rest_activate_periodic_resource(&periodic_resource_sht21_temperature);
    rest_activate_periodic_resource(&periodic_resource_sht21_humidity);
  } else { // error
    PRINTF("SHT21 not connected\n");
  }
#endif
#if defined (PLATFORM_HAS_BMP085) && REST_RES_BMP085
  int result_bmp085 = SENSORS_ACTIVATE(bmp085_sensor);
  if(result_bmp085 == 1) { // ok
    rest_activate_periodic_resource(&periodic_resource_bmp085);
  } else { // error
    PRINTF("BMP085 not connected\n");
  }
#endif
  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
#if defined (PLATFORM_HAS_BUTTON)
    if (ev == sensors_event && data == &button_sensor) {
      PRINTF("BUTTON\n");
#if REST_RES_EVENT
      /* Call the event_handler for this application-specific event. */
      event_event_handler(&resource_event);
#endif
    }
#endif /* PLATFORM_HAS_BUTTON */
  } /* while (1) */

  PROCESS_END();
}
