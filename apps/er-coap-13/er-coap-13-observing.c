/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *      CoAP module for observing resources
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <string.h>

#include "er-coap-13-observing.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif


MEMB(observers_memb, coap_observer_t, COAP_MAX_OBSERVERS);
LIST(observers_list);

/*-----------------------------------------------------------------------------------*/
#ifdef CONDITION				/* Conditiona Observe supported*/
coap_observer_t *
coap_add_observer(uip_ipaddr_t *addr, uint16_t port, const uint8_t *token, size_t token_len, const char *url, coap_condition_t *condition) // Conditional observe
{
  /* Remove existing observe relationship, if any. */
	coap_remove_observer_by_url(addr, port, url);

	coap_observer_t *o = memb_alloc(&observers_memb);

	if (o)
	{
		o->url = url;
		uip_ipaddr_copy(&o->addr, addr);
		o->port = port;
		o->token_len = token_len;
		memcpy(o->token, token, token_len);
		o->last_mid = 0;

		stimer_set(&o->refresh_timer, COAP_OBSERVING_REFRESH_INTERVAL);

		o->last_notification_time = clock_seconds();

		o->last_notified_value = 0;

		/* Conditional observe */
		if(condition)
		{
			o->condition.cond_type = condition->cond_type;
			o->condition.reliability_flag = condition->reliability_flag;
			o->condition.value_type = condition->value_type;
			o->condition.value = condition->value;

			o->cond_observe_flag = 1; /* Conditional Observation*/ 

			PRINTF("Adding conditional observer for /%s [0x%02X%02X] condition=%d\n", o->url, o->token[0], o->token[1], o->condition.value);
		}
		else
		{
			o->cond_observe_flag = 0; /* Normal Observation*/

			PRINTF("Adding normal observer for /%s [0x%02X%02X] \n", o->url, o->token[0], o->token[1]);
		}

		list_add(observers_list, o);
	}

	return o;
}
/*-----------------------------------------------------------------------------------*/
#else					/* Conditional Observe Not Supported */
coap_observer_t *
coap_add_observer(uip_ipaddr_t *addr, uint16_t port, const uint8_t *token, size_t token_len, const char *url)
{
  /* Remove existing observe relationship, if any. */
  coap_remove_observer_by_url(addr, port, url);

  coap_observer_t *o = memb_alloc(&observers_memb);

  if (o)
  {
    o->url = url;
    uip_ipaddr_copy(&o->addr, addr);
    o->port = port;
    o->token_len = token_len;
    memcpy(o->token, token, token_len);
    o->last_mid = 0;

    stimer_set(&o->refresh_timer, COAP_OBSERVING_REFRESH_INTERVAL);

    PRINTF("Adding observer for /%s [0x%02X%02X]\n", o->url, o->token[0], o->token[1]);
    list_add(observers_list, o);
  }

  return o;
}
#endif
/*-----------------------------------------------------------------------------------*/
void
coap_remove_observer(coap_observer_t *o)
{
  PRINTF("Removing observer for /%s [0x%02X%02X]\n", o->url, o->token[0], o->token[1]);

  memb_free(&observers_memb, o);
  list_remove(observers_list, o);
}

int
coap_remove_observer_by_client(uip_ipaddr_t *addr, uint16_t port)
{
  int removed = 0;
  coap_observer_t* obs = NULL;

  for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
  {
    PRINTF("Remove check client ");
    PRINT6ADDR(addr);
    PRINTF(":%u\n", port);
    if (uip_ipaddr_cmp(&obs->addr, addr) && obs->port==port)
    {
      coap_remove_observer(obs);
      removed++;
    }
  }
  return removed;
}

int
coap_remove_observer_by_token(uip_ipaddr_t *addr, uint16_t port, uint8_t *token, size_t token_len)
{
  int removed = 0;
  coap_observer_t* obs = NULL;

  for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
  {
    PRINTF("Remove check Token 0x%02X%02X\n", token[0], token[1]);
    if (uip_ipaddr_cmp(&obs->addr, addr) && obs->port==port && obs->token_len==token_len && memcmp(obs->token, token, token_len)==0)
    {
      coap_remove_observer(obs);
      removed++;
    }
  }
  return removed;
}

int
coap_remove_observer_by_url(uip_ipaddr_t *addr, uint16_t port, const char *url)
{
  int removed = 0;
  coap_observer_t* obs = NULL;

  for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
  {
    PRINTF("Remove check URL %p\n", url);
    if ((addr==NULL || (uip_ipaddr_cmp(&obs->addr, addr) && obs->port==port)) && (obs->url==url || memcmp(obs->url, url, strlen(obs->url))==0))
    {
      coap_remove_observer(obs);
      removed++;
    }
  }
  return removed;
}

int
coap_remove_observer_by_mid(uip_ipaddr_t *addr, uint16_t port, uint16_t mid)
{
  int removed = 0;
  coap_observer_t* obs = NULL;

  for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
  {
    PRINTF("Remove check MID %u\n", mid);
    if (uip_ipaddr_cmp(&obs->addr, addr) && obs->port==port && obs->last_mid==mid)
    {
      coap_remove_observer(obs);
      removed++;
    }
  }
  return removed;
}
/*-----------------------------------------------------------------------------------*/
#ifdef CONDITION	/* Conditional observe supported */
void
coap_notify_observers(resource_t *resource, uint16_t obs_counter, void *notification, uint32_t cond_value)
{
	coap_packet_t *const coap_res = (coap_packet_t *) notification;

	coap_observer_t* obs = NULL;

	uint8_t preferred_type = coap_res->type;

	PRINTF("Observing: Notification from %s value: <%u>\n", resource->url, cond_value);

	/* Iterate over observers. */
	for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
	{
		if (obs->url==resource->url) 			/* using RESOURCE url pointer as handle. */
		{
			coap_transaction_t *transaction = NULL;

			if (obs->cond_observe_flag) 				/* Conditional observe */
			{
				if (!satisfies_condition(obs, cond_value))
				{
					PRINTF("Condition not satisfied. Skipping this observer\n");
					continue;
				} 
				else 
				{
  			  preferred_type = obs->condition.reliability_flag;
					PRINTF("Condition Satisfied. Sending Notification...\n");
				}
			}
			else 											/* Normal Observe */
			{
				if  (obs->last_notified_value != cond_value  || 
            (obs->last_notified_value == cond_value && (clock_seconds() - obs->last_notification_time) >= coap_res->max_age))
				{
					PRINTF("Sending Normal Observation Notification... %u\n", cond_value);
				}
				else
				{
					PRINTF("Value not changed. Skipping this observer %u\n", cond_value);
					continue;
				}
			}
			
			/*TODO implement special transaction for CON, sharing the same buffer to allow for more observers. */
			
			if ( (transaction = coap_new_transaction(coap_get_mid(), &obs->addr, obs->port)) )
			{
				PRINTF("           Observer ");
				PRINT6ADDR(&obs->addr);
				PRINTF(":%u\n", obs->port);

				/* Update last MID for RST matching. */
				obs->last_mid = transaction->mid;
			
				/* Prepare response */
				coap_res->mid = transaction->mid;
				if (obs_counter >= 0) coap_set_header_observe(coap_res, obs_counter);

				coap_set_header_token(coap_res, obs->token, obs->token_len);
			
				/* Use CON to check whether client is still there/interested after COAP_OBSERVING_REFRESH_INTERVAL. */
				if (stimer_expired(&obs->refresh_timer))
				{
					PRINTF("           Refreshing with CON\n");
					coap_res->type = COAP_TYPE_CON;
					stimer_restart(&obs->refresh_timer);
				}
				else
				{
					coap_res->type = preferred_type; 
				}
				transaction->packet_len = coap_serialize_message(coap_res, transaction->packet);

				coap_send_transaction(transaction);

				obs->last_notified_value = cond_value;

				obs->last_notification_time = clock_seconds();

			} /*if transaction*/
		}/*if obs_url*/
	} /*for */
}
/*-----------------------------------------------------------------------------------*/
#else 				/*Conditional Observe Not Supported */
void
coap_notify_observers(resource_t *resource, int32_t obs_counter, void *notification)
{
  coap_packet_t *const coap_res = (coap_packet_t *) notification;
  coap_observer_t* obs = NULL;
  uint8_t preferred_type = coap_res->type;

  PRINTF("Observing: Notification from %s\n", resource->url);

  /* Iterate over observers. */
  for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
  {
    if (obs->url==resource->url) /* using RESOURCE url pointer as handle */
    {
      coap_transaction_t *transaction = NULL;

      /*TODO implement special transaction for CON, sharing the same buffer to allow for more observers. */

      if ( (transaction = coap_new_transaction(coap_get_mid(), &obs->addr, obs->port)) )
      {
        PRINTF("           Observer ");
        PRINT6ADDR(&obs->addr);
        PRINTF(":%u\n", obs->port);

        /* Update last MID for RST matching. */
        obs->last_mid = transaction->mid;

        /* Prepare response */
        coap_res->mid = transaction->mid;
        if (obs_counter>=0) coap_set_header_observe(coap_res, obs_counter);
        coap_set_header_token(coap_res, obs->token, obs->token_len);

        /* Use CON to check whether client is still there/interested after COAP_OBSERVING_REFRESH_INTERVAL. */
        if (stimer_expired(&obs->refresh_timer))
        {
          PRINTF("           Refreshing with CON\n");
          coap_res->type = COAP_TYPE_CON;
          stimer_restart(&obs->refresh_timer);
        }
        else
        {
          coap_res->type = preferred_type;
        }

        transaction->packet_len = coap_serialize_message(coap_res, transaction->packet);

        coap_send_transaction(transaction);
      }
    }
  }
}
#endif
/*-----------------------------------------------------------------------------------*/
#ifdef CONDITION		/* Conditional Observe Supported */
void
coap_observe_handler(resource_t *resource, void *request, void *response)
{
	coap_packet_t *const coap_req = (coap_packet_t *) request;
	coap_packet_t *const coap_res = (coap_packet_t *) response;

	static char content[16];

	if (coap_req->code==COAP_GET && coap_res->code<128) /* GET request and response without error code */
	{
		if (IS_OPTION(coap_req, COAP_OPTION_OBSERVE))
		{
			if(IS_OPTION(coap_req, COAP_OPTION_CONDITION)) 		/*Conditional Observe */
			{
				coap_condition_t cond; // Conditional Observe
				if (coap_decode_condition(&cond, coap_req->condition, coap_req->condition_len))
				{
					if(coap_add_observer(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, coap_req->token, coap_req->token_len, resource->url, &cond))
					{
						coap_set_header_observe(coap_res, 0);
						coap_set_payload(coap_res, content, snprintf(content, sizeof(content), "Added with condition%u/%u", list_length(observers_list), COAP_MAX_OBSERVERS));
						
					} /*if (add observer)*/
				}/*if(decode condition)*/
			}/*is (option condition)*/
			
			else						/* Normal Observe */ 
			{
				if (coap_add_observer(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, coap_req->token, coap_req->token_len, resource->url, NULL))
				{
					coap_set_header_observe(coap_res, 0);

				}/* if (added observer) */
				else
				{
					coap_res->code = SERVICE_UNAVAILABLE_5_03;
					coap_set_payload(coap_res, "TooManyObservers", 16);
				} /* if(cant add observer)*/

			} /* if (normal observe) */
		}
		else /* if (observe) */
		{
			/* Remove client if it is currently observing. */
			coap_remove_observer_by_url(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, resource->url);
		} /* if (observe) */
	} /* if (GET) */
}
/*-----------------------------------------------------------------------------------*/
#else					/* Conditional Observe Not Supported */
void
coap_observe_handler(resource_t *resource, void *request, void *response)
{
  coap_packet_t *const coap_req = (coap_packet_t *) request;
  coap_packet_t *const coap_res = (coap_packet_t *) response;

  static char content[16];

  if (coap_req->code==COAP_GET && coap_res->code<128) /* GET request and response without error code */
  {
    if (IS_OPTION(coap_req, COAP_OPTION_OBSERVE))
    {

      if (coap_add_observer(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, coap_req->token, coap_req->token_len, resource->url))
      {
        coap_set_header_observe(coap_res, 0);
        /*
         * For demonstration purposes only. A subscription should return the same representation as a normal GET.
         * TODO: Comment the following line for any real application.
         */
        coap_set_payload(coap_res, content, snprintf(content, sizeof(content), "Added %u/%u", list_length(observers_list), COAP_MAX_OBSERVERS));
      }
      else
      {
        coap_res->code = SERVICE_UNAVAILABLE_5_03;
        coap_set_payload(coap_res, "TooManyObservers", 16);
      } /* if (added observer) */
    }
    else /* if (observe) */
    {
      /* Remove client if it is currently observing. */
      coap_remove_observer_by_url(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, resource->url);
    } /* if (observe) */
  }
}
#endif
/*------------------------------------------------------------------------------------*/
#ifdef CONDITION	/*Functions specific to Conditional Observation */
int
coap_encode_condition(coap_condition_t *cond, uint8_t *encoded_cond, uint8_t *condition_len)
{
	uint32_t temp = 0;
	uint32_t value = 0;
	uint8_t i = 1;
	
	/*Encode the header*/
	encoded_cond[0] = (uint8_t) (cond->cond_type) << 3;
	
	encoded_cond[0] = encoded_cond[0] | ((uint8_t) (cond->reliability_flag)) << 2;
	
	encoded_cond[0] = encoded_cond[0] | ((uint8_t) (cond->value_type));
	
	/*Encode the value (upto 4 bytes) */
  
	temp = cond->value;

	while (temp > 0) {
		value = temp << 24;

		value = value >> 24;

		encoded_cond[i] = (uint8_t) value;

		temp = temp >> 8;

		i++;
	}

	*condition_len = i;

	PRINTF("Encoded: %u [0x%02x%02x%02x%02x]\n",*condition_len, encoded_cond[0], encoded_cond[1], encoded_cond[2], encoded_cond[3]);

	return 1;
}
/*------------------------------------------------------------------------------------------*/
int 
coap_decode_condition (coap_condition_t *cond, uint8_t *encoded_cond, uint8_t condition_len)
{
	uint8_t temp;
	uint8_t i;
	uint32_t value = 0;

	temp = encoded_cond[0];

	cond->cond_type = temp>>3;

	temp =  temp << 5;

	cond->reliability_flag = temp >> 7;
  
	temp = temp << 1;

	cond->value_type = temp >> 6;

	for(i = 1; i < condition_len; i++) {

		value = value << 8;

		value = value | encoded_cond[i];

		i++;
	}
	cond->value = value;
	PRINTF("Decoded: Type: %u value: %u\n", cond->cond_type, cond->value);

	return 1;
}
/*-----------------------------------------------------------------------------------*/
int 
satisfies_condition(coap_observer_t *obs, uint32_t cond_value) 
{

	uint8_t satisfied = 0;
	if (obs->condition.cond_type == CONDITION_TIME_SERIES)
	{
		if (obs->last_notified_value != cond_value)
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		} 
	}
	else if (obs->condition.cond_type == CONDITION_MIN_RESP_TIME )
	{
		/*Notify if the value has changed and the MinRT is >= Time Delta */
		if ((obs->last_notified_value != cond_value) && \
			(obs->condition.value) <= ( clock_seconds() - obs->last_notification_time))
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_MAX_RESP_TIME)
	{
		if ((obs->last_notified_value != cond_value) ||
		    ((obs->last_notified_value == cond_value) && 
		    ( (obs->condition.value) <= ( clock_seconds() - obs->last_notification_time))) ) 
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_STEP )
	{
		uint32_t delta =cond_value > obs->last_notified_value ?
						cond_value - obs->last_notified_value :
						obs->last_notified_value - cond_value;
		if (delta >= obs->condition.value)
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_ALLVALUES_LESS)
	{
		if ((cond_value < obs->condition.value) && (cond_value != obs->last_notified_value))
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_ALLVALUES_GREATER)
	{
		if ((cond_value > obs->condition.value) && (cond_value != obs->last_notified_value))
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_VALUE_EQUAL )
	{
		if (cond_value == obs->condition.value)
		{
			satisfied = 1;
		}
		else {
				return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_VALUE_NOT_EQUAL )
	{
		if (((obs->last_notified_value < obs->condition.value) && (obs->condition.value <cond_value))||
		   ((obs->last_notified_value > obs->condition.value) && (obs->condition.value > cond_value))) {
  				satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	else if (obs->condition.cond_type == CONDITION_PERIODIC )
	{

		if ((clock_seconds() - obs->last_notification_time) >= obs->condition.value)
		{
			satisfied = 1;
		}
		else
		{
			return 0;
		}
	}
	return satisfied; // if satisfied = 0, then condition is not supported yet;
}
//---------------------------------------------------------------------------------------------------------------
int
coap_set_header_condition(void *packet, uint8_t *condition, uint8_t condition_len)
{
	coap_packet_t *const coap_pkt = (coap_packet_t *) packet;
  
	coap_pkt->condition_len = MIN(COAP_MAX_CONDITION_LEN, condition_len);
  
	memcpy(coap_pkt->condition, condition, coap_pkt->condition_len);
  
	SET_OPTION(coap_pkt, COAP_OPTION_CONDITION);

	return coap_pkt->condition_len;
}

int
coap_get_header_condition(void *packet, uint8_t **condition)
{
	coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

	if (!IS_OPTION(coap_pkt, COAP_OPTION_CONDITION) ) return 0;

	*condition = coap_pkt->condition;
  
	return coap_pkt->condition_len;
}
#endif /* CONDITION */
//---------------------------------------------------------------------------------------------------------------
int 
coap_reset_observations() 
{
	coap_observer_t* obs = NULL, *nxt=NULL;

	PRINTF("Resetting all Observation Relationships...\n");

	for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
	{
			nxt = obs->next;
			coap_remove_observer(obs);
			obs->next = nxt;
	}
	return 1;
}
/*----------------------------------------------------------------------------------------------*/
int 
coap_list_observations(char *res_url, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
	size_t strpos = 0; /* position in overall string (which is larger than the buffer) */
	size_t bufpos = 0; /* position within buffer (bytes written) */
	size_t tmplen = 0;
	coap_observer_t* obs = NULL;

	static char tmpval[42];
	uip_ipaddr_t *addr;

	for (obs = (coap_observer_t*)list_head(observers_list); obs; obs = obs->next)
	{
      if (strpos>0)
      {
        if (strpos >= *offset && bufpos < preferred_size)
        {
          buffer[bufpos++] = '\n';
        }
        ++strpos;
      }
      if (strpos >= *offset && bufpos < preferred_size)
      {
        buffer[bufpos++] = '<';
      }
      ++strpos;

			if (strpos >= *offset && bufpos < preferred_size)
      {
        buffer[bufpos++] = '/';
      }
      ++strpos;

			/* add the resource being observed to the buffer*/
      tmplen = strlen(obs->url);
      if (strpos+tmplen > *offset)
      {
        bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
                         "%s", obs->url + ((*offset-(int32_t)strpos > 0) ? (*offset-(int32_t)strpos) : 0));
                                                          /* minimal-net requires these casts */
        if (bufpos >= preferred_size)
        {
          break;
        }
      }
      strpos += tmplen;

      if (strpos >= *offset && bufpos < preferred_size)
      {
        buffer[bufpos++] = '>';
      }
      ++strpos;

			if (strpos >= *offset && bufpos < preferred_size)
			{
				buffer[bufpos++] = ';';
      }
      ++strpos;

			/*add 'ipaddr=' */
			strcpy(tmpval, "ipaddr=");

      tmplen = strlen(tmpval);

      if (strpos+tmplen-1 > *offset)
      {
          bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
                         "%s", tmpval + (*offset-((int32_t)strpos) > 0 ? *offset-((int32_t)strpos) : 0));

          if (bufpos >= preferred_size)
          {
            break;
          }
      }
		
			strpos +=tmplen;
			
			/* add ip address of observer*/
			addr = &(obs->addr);

	 			 snprintf(tmpval, 42,"[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", 									
								((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], 
								((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], 
								((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], 
								((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15]);

			tmplen = strlen(tmpval);

			if (strpos+tmplen-1 > *offset)
      {		
          bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
                         "%s", tmpval + (*offset-(int32_t)strpos > 0 ? *offset-(int32_t)strpos : 0));

          if (bufpos >= preferred_size)
          {
            break;
          }
        }
        strpos += tmplen;
#ifdef CONDITION
			/*Add condition type, and value */
			if (obs->cond_observe_flag) 
			{
				if (strpos >= *offset && bufpos < preferred_size)
        {
          buffer[bufpos++] = ';';
        }
        ++strpos;

				/*add Condition_Type*/
				snprintf(tmpval, 42, "T=");

				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{				

					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;

				snprintf(tmpval, 42, "%u", obs->condition.cond_type);

				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{
					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;
			
				if (strpos >= *offset && bufpos < preferred_size)
        {
          buffer[bufpos++] = ';';
        }
        ++strpos;

				/* add condition value*/
				snprintf(tmpval, 42, "V=");

				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{				

					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;

				snprintf(tmpval, 42, "%u", obs->condition.value);

				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{
					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;

				if (strpos >= *offset && bufpos < preferred_size)
        {
          buffer[bufpos++] = ';';
        }
        ++strpos;

				/*add reliability flag*/
				snprintf(tmpval, 42, "R=");
				
				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{
					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;
						 
				if (obs->condition.reliability_flag == NON) 
				{
					sprintf(tmpval, "NON");
				}
				else 
				{
					sprintf(tmpval, "CON");
				}
				tmplen = strlen(tmpval);

				if (strpos + tmplen-1 > *offset) 
				{
					bufpos += snprintf((char *) buffer + bufpos, preferred_size - bufpos + 1,
											"%s", ((char *) tmpval) + (*offset-(int32_t)strpos > 0 ? *offset -
															(int32_t)strpos : 0 ));
					if (bufpos >= preferred_size)
					{
						break;
					}
				}
				strpos += tmplen;				

			}/*if condition */
#endif
      /* buffer full, but resource not completed yet; or: do not break if resource exactly fills buffer. */
      if (bufpos >= preferred_size && strpos-bufpos > *offset)
      {
        PRINTF("Obs: BREAK at %s (%p)\n", obs->url, obs);
        break;
      }
    }/*for*/
			
		if (bufpos>=0) {
			if (bufpos > 0) {
	      coap_set_payload(response, buffer, bufpos );
			}	
      PRINTF("BUF %d: %*s\n", bufpos, bufpos, (char *) buffer);
		  coap_set_header_content_type(response, APPLICATION_LINK_FORMAT);
    }
    else if (strpos>0)
    {
      coap_set_status_code(response, BAD_OPTION_4_02);
      coap_set_payload(response, "BlockOutOfScope", 15);
    }

    if (obs==NULL) {
      PRINTF("List: DONE\n");
      *offset = -1;
    }
    else
    {
      PRINTF("res: MORE at %s (%p)\n", obs->url, obs);
      *offset += preferred_size;
    }
		return 1;
}

