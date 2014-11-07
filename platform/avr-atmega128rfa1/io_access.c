/*
 * @author    dresden elektronik ingenieurtechnik gmbh: http://www.dresden-elektronik.de
 * @author    Support email: wireless@dresden-elektronik.de
 *
 * Copyright (c) 2011, dresden elektronik ingenieurtechnik gmbh. All rights reserved.
 *
 * Licensed under dresden elektronik's Limited License Agreement --> deEULA.txt
 */

/* === INCLUDES ============================================================ */
#include <stdbool.h>          // include bool definition
#include <avr/io.h>           // include io definitions

#include "io_access.h"

/* === TYPES =============================================================== */

/* === MACROS / DEFINES ==================================================== */

/* === GLOBALS ============================================================= */

/* status register for actual LEDs state */
static uint8_t status_led = 0;

/* === PROTOTYPES ========================================================== */

uint8_t read_io_byte(void);
void write_io_byte(uint8_t data);

/* === IMPLEMENTATION ====================================================== */

/**
 * @brief Control LED status.
 *
 * All LEDs are 'LOW ACTIVE' (RCB, STB and deRFnode).
 *
 * @param   led_nr   number of LED
 * @param   led_mode mode which should be set (LED_ON, LED_OFF, LED_TOGGLE)
 *
 */
void led_set(led_number_t led_nr, led_mode_t led_mode)
{
   switch (led_mode)
   {
   case LED_ON:
      status_led &= ~_BV(led_nr);
      break;
   case LED_OFF:
      status_led |= _BV(led_nr);
      break;
   case LED_TOGGLE:
      if(status_led & _BV(led_nr))
          status_led &= ~_BV(led_nr);
      else
          status_led |= _BV(led_nr);
      break;
   default:
      break; // do nothing
   }

   switch (led_nr)
   {
   case LED_0:
      if(status_led & _BV(LED_0)) {LED_0_PORT |= _BV(LED_0_PIN);} else{LED_0_PORT &= ~_BV(LED_0_PIN);}
      LED_0_DDR  |=  _BV(LED_0_PIN);
      break;
   case LED_1:
      if(status_led & _BV(LED_1)) {LED_1_PORT |= _BV(LED_1_PIN);} else{LED_1_PORT &= ~_BV(LED_1_PIN);}
      LED_1_DDR  |=  _BV(LED_1_PIN);
      break;
   case LED_2:
      if(status_led & _BV(LED_2)) {LED_2_PORT |= _BV(LED_2_PIN);} else{LED_2_PORT &= ~_BV(LED_2_PIN);}
      LED_2_DDR  |=  _BV(LED_2_PIN);
      break;
   default:
      break; // do nothing
   }
}


/**
 * @brief LED initialization function.
 *
 * This function initializes the led hardware.
 *
 */
void led_init(void)
{
   led_set(LED_0, LED_OFF);
   led_set(LED_1, LED_OFF);
   led_set(LED_2, LED_OFF);
}

/**
 * @brief Initialization function for IO hardware.
 *
 * This function initializes the hardware.
 *
 */
void io_init(void)
{
   /* initialize buttons */
   BUTTON_0_DDR  &= ~_BV(BUTTON_0_PIN);
   BUTTON_0_PORT |=  _BV(BUTTON_0_PIN);
   BUTTON_1_DDR  &= ~_BV(BUTTON_1_PIN);
   BUTTON_1_PORT |=  _BV(BUTTON_1_PIN);
   /* initialize LEDs */
   led_init();
}

/**
 * @brief Gives back the button's state
 *
 * IMPORTANT: Button(s) are 'LOW ACTIVE' (on RCB, STB and deRFnode)
 *
 * @param button_nr  button to select
 *
 * @return button state - BUTTON_PRESSED, BUTTON_RELEASED
 *
 */
button_state_t button_pressed(button_number_t button_nr)
{
   switch(button_nr)
   {
   case BUTTON_0:
     return (BUTTON_0_PINS & _BV(BUTTON_0_PIN)) ? BUTTON_RELEASED : BUTTON_PRESSED;
     break;

   case BUTTON_1:
      return (BUTTON_1_PINS & _BV(BUTTON_1_PIN)) ? BUTTON_RELEASED : BUTTON_PRESSED;
      break;
   }

   return BUTTON_RELEASED;
}

/* EOF */

/* the following code does not belong to dresden elektronik */

int leds_onboard_get(void)
{
    return status_led;
}
