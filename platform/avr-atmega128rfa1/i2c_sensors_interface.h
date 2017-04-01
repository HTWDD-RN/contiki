/**
 * @file sensors_interface.h
 *
 * @brief header file for sensors.c.
 *
 * Defines an API for accessing the TWI sensors on deRFnode/gateway boards.
 *
 * $Id: sensors_interface.h,v 1.2 2012/11/14 12:39:05 ele Exp $
 *
 */
/*
 * @author    dresden elektronik ingenieurtechnik gmbh: http://www.dresden-elektronik.de
 * @author    Support email: wireless@dresden-elektronik.de
 *
 * Copyright (c) 2011, dresden elektronik ingenieurtechnik gmbh. All rights reserved.
 *
 * Licensed under dresden elektronik's Limited License Agreement --> deEULA.txt
 */

/* Prevent double inclusion */
#ifndef _SENSORS_INTERFACE_H
#define _SENSORS_INTERFACE_H

/* === INCLUDES ============================================================= */
#include <inttypes.h>
#include <stdbool.h>

/* === MACROS / DEFINES ==================================================== */

// necessary for deRFmega128-based boards - change pullups
#define TWI_PULLUP_ENABLE()   do {  \
    DDRD |= (1<<PD6);               \
    PORTD &= ~(1<<PD6);             \
} while(0);

#define TWI_PULLUP_DISABLE()  do {  \
    DDRD |= (1<<PD6);               \
    PORTD |= (1<<PD6);              \
} while(0);

#define TWI_CLK             (100000)

/* === TYPES ================================================================ */

/* common function return code */
typedef enum {
    TWI_SUCC = 0x00,
    TWI_FAIL = 0x01
} twi_status_t;

/* temperature type */
typedef struct
{
    uint8_t conversion_done;      // (1) - conversion completed; (0) - conversion uncompleted
    uint8_t sign;                 // (0) <-> >=0, (1) <-> <0 (like 2's complement)
    uint8_t integralDigit;        // integral part of temperature
    uint8_t fractionalDigit;      // fractional part of temperature in 1/100 parts

} temperature_t;

/* luminosity type */
typedef uint16_t luminosity_t;

/* three-axial acceleration type */
typedef struct
{
    uint8_t  acc_x_sign;          // is acc_x value positive (0) or negative (1)?
    uint8_t  acc_x_integral;      // integral digit of acc_x
    uint8_t  acc_x_fractional;    // fractional digit of acc_x in 1/100 parts
    uint8_t  acc_y_sign;          // is acc_y value positive (0) or negative (1)?
    uint8_t  acc_y_integral;      // integral digit of acc_y
    uint8_t  acc_y_fractional;    // fractional digit of acc_y in 1/100 parts
    uint8_t  acc_z_sign;          // is acc_z value positive (0) or negative (1)?
    uint8_t  acc_z_integral;      // integral digit of acc_z
    uint8_t  acc_z_fractional;    // fractional digit of acc_z in 1/100 parts
} acceleration_t;

/* callbacks called from ISRs */
typedef void* bma150_callback_t;


/* === PROTOTYPES =========================================================== */

// --- generic interface ------------------------------------------------------
twi_status_t TWI_MasterInit(void);

// --- DS620 ------------------------------------------------------------------
twi_status_t DS620_Init(void);
twi_status_t DS620_StartOneshotMeasurement(void);
twi_status_t DS620_StartContinuousMeasurement(void);
twi_status_t DS620_Reset(void);
twi_status_t DS620_StopMeasuring(void);
twi_status_t DS620_GetTemperature(temperature_t* temperature, bool blocking_wait);
twi_status_t DS620_PowerDown(void);
twi_status_t DS620_WakeUp(void);

// --- TMP102 -----------------------------------------------------------------
twi_status_t TMP102_Init(void);
twi_status_t TMP102_StartOneshotMeasurement(void);
twi_status_t TMP102_StartContinuousMeasurement(void);
twi_status_t TMP102_StopMeasuring(void);
twi_status_t TMP102_GetTemperature(temperature_t* temperature, bool blocking_wait);
twi_status_t TMP102_PowerDown(void);
twi_status_t TMP102_WakeUp(void);

// --- ISL29020 ---------------------------------------------------------------
twi_status_t ISL29020_Init(void);
twi_status_t ISL29020_StartOneshotMeasurement(void);
twi_status_t ISL29020_StartContinuousMeasurement(void);
twi_status_t ISL29020_StopMeasuring(void);
twi_status_t ISL29020_GetLuminosity(luminosity_t* luminosity);
twi_status_t ISL29020_PowerDown(void);
twi_status_t ISL29020_WakeUp(void);

// --- BMA150 -----------------------------------------------------------------
twi_status_t BMA150_Init(void);
// no 'start', 'stop', 'oneshoot', 'continuous', ...  functions available for this sensor
void BMA150_SetAnymotionIrqCallback(bma150_callback_t callback);
twi_status_t BMA150_ConfigureAnymotionIrq(uint8_t enabled);
twi_status_t BMA150_GetAcceleration(acceleration_t* acceleration);
twi_status_t BMA150_GetTemperature(temperature_t* temperature);
twi_status_t BMA150_PowerDown(void);
twi_status_t BMA150_WakeUp(void);

// --- helper functions -------------------------------------------------------

/* === INLINE FUNCTIONS ==================================================== */

#endif /* _SENSORS_INTERFACE_H */

/* EOF */
