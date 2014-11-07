/*
 * @author    dresden elektronik ingenieurtechnik gmbh: http://www.dresden-elektronik.de
 * @author    Support email: wireless@dresden-elektronik.de
 *
 * Copyright (c) 2011, dresden elektronik ingenieurtechnik gmbh. All rights reserved.
 *
 * Licensed under dresden elektronik's Limited License Agreement --> deEULA.txt
 */


/* Prevent double inclusion */
#ifndef IO_ACCESS_H_
#define IO_ACCESS_H_

/* === INCLUDES ============================================================ */

/* === MACROS / DEFINES ==================================================== */

/******************************************************************************
 * GLOBAL DEFINITIONS - VALID FOR ALL PLATFORMS
 ******************************************************************************/

/** All state mode's which available for a LED */
typedef enum led_mode_tag
{
    LED_ON,
    LED_OFF,
    LED_TOGGLE
} led_mode_t;

/** All state mode's which available for a Button */
typedef enum button_state_tag
{
   BUTTON_PRESSED,
   BUTTON_RELEASED
} button_state_t;


/******************************************************************************
 * PLATFORM SPECIFIC DEFINITIONS
 ******************************************************************************/

/******************************************************************************
 * specify button and LED definitions for DE_RF_NODE
 ******************************************************************************/

/* specify all LEDs available on this platform */
typedef enum led_number_tag
{
    LED_0,
    LED_1,
    LED_2
} led_number_t;

/*
 * PORT, DDR and PIN where LEDs are connected
 */
#define LED_0_PIN          (PG5)
#define LED_0_PORT         (PORTG)
#define LED_0_DDR          (DDRG)

#define LED_1_PIN          (PE3)
#define LED_1_PORT         (PORTE)
#define LED_1_DDR          (DDRE)

#define LED_2_PIN          (PE4)
#define LED_2_PORT         (PORTE)
#define LED_2_DDR          (DDRE)

/* specify all BUTTON's available on this platform */
typedef enum button_number_tag
{
    BUTTON_0,
    BUTTON_1
} button_number_t;

/*
 * PORT, DDR, PIN and INPUTS defined where Button is connected
 */
#define BUTTON_0_PIN          (PB7)
#define BUTTON_0_PORT         (PORTB)
#define BUTTON_0_DDR          (DDRB)
#define BUTTON_0_PINS         (PINB)

#define BUTTON_1_PIN          (PD7)
#define BUTTON_1_PORT         (PORTD)
#define BUTTON_1_DDR          (DDRD)
#define BUTTON_1_PINS         (PIND)


/* === TYPES =============================================================== */

/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif


void led_set(led_number_t led_nr, led_mode_t led_mode);
void io_init(void);
button_state_t button_pressed(button_number_t button_nr);


#ifdef __cplusplus
} /* extern "C" */
#endif

/* === INLINE FUNCTIONS ==================================================== */

#endif /* IO_ACCESS_H_ */

/* EOF */

/* the following code does not belong to dresden elektronik */

int leds_onboard_get(void);
