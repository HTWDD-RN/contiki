/*
 * Copyright (c) 2012, Angelos Drossos.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 */


/**
 * @file dev/sht21-sensor.h
 * Plattform indepent implementation of the SHT21 sensor,
 * which uses the plattform dependent I2C-Interface.
 * @author Angelos Drossos <angelos.drossos@gmail.com>
 * @version 0.5
 * @date 10/9/2012
 * @copyright Contiki license
 *
 * The SHT21 sensor can read out the current temperature
 * or current relative humidity.
 *
 * @todo the CRC-8 checksum function has to be implemented
 * @todo the 'no hold mode' mode has to be implemented, use 'hold mode' instead!
 */
#ifndef __SHT21_SENSOR_H__
#define __SHT21_SENSOR_H__

/// Include the Contiki-OS SENSORS interface
#include "lib/sensors.h"

// some std libs
#include <stdbool.h>

/// The I2C device address of the SHT21 sensor.
#undef  SHT21_SENSOR_I2C_ADDRESS_WRITE
#define SHT21_SENSOR_I2C_ADDRESS_WRITE     0x80

/**
 * SHT21 sensor st for the Contiki-OS SENSORS interface.
 * With this struct you can call sht21_sensor.value(..)
 * to read out a specific value of the sensor.
 */
extern const struct sensors_sensor sht21_sensor;

/// The name of the sensor for the Contiki-OS SENSORS interface
#undef  SHT21_SENSOR
#define SHT21_SENSOR "SHT21"

/**
 * The SHT21 sensor has different sensors and this enumeration describes,
 * how this module interacts with the sensor to read out a sensor value.
 * @info the caller should call sht21_sensor.value(..) 
 *       and not sht21_sensor_value(..)!
 */
typedef enum sht21_sensor_value_type {
	/// The SHT21 sensor will read the temperature value.
	SHT21_SENSOR_TEMP,
	/// The SHT21 sensor will read the relative humidity value.
	SHT21_SENSOR_RELATIVE_HUMIDITY,
	/// The SHT21 sensor will read the relative humidity value.
	SHT21_SENSOR_RELATIVE_HUMIDITY_ICE,
	/// The SHT21 sensor will read the battery status.
	SHT21_SENSOR_BATTERY_INDICATOR
} sht21_sensor_value_type_t;

// test the Contiki-OS SENSORS interface for changes
#ifndef SENSORS_HW_INIT
	#error "Contiki-OS SENSORS interface has changed!"
#endif
#ifndef SENSORS_ACTIVE
	#error "Contiki-OS SENSORS interface has changed!"
#endif
#ifndef SENSORS_READY
	#error "Contiki-OS SENSORS interface has changed!"
#endif

/**
 * The SHT21 sensor has different configurations and communication methods
 * and this enumeration describes, how this module can be configured.
 * @info The caller can call the function
 *          'int succ = sht21_sensor.configure(int configure_type, int value)'
 *             to set a specific configuration
 *       or 'int value = sht21_sensor.status(int configure_type)'
 *             to get a specific configuration setting.
 */
typedef enum sht21_sensor_configure_type {
	/// The SHT21 sensor will do a soft reset.
	/// This is a temporary configuration so that the status returns always 1.
	SHT21_SENSOR_SOFT_RESET,

	/// not used.
	SHT21_SENSOR_HW_INIT = SENSORS_HW_INIT,

	/// Set value to 0 to turn the sensor off, to 1 to turn it on.
	/// Alternatively, use the SENSORS macro:
	/// SENSORS_ACTIVATE(sht21_sensor) or SENSORS_DEACTIVATE(sht21_sensor).
	SHT21_SENSOR_ACTIVE = SENSORS_ACTIVE,

	/// Set the sensor in ready state
	SHT21_SENSOR_READY = SENSORS_READY
} sht21_sensor_configure_type_t;


int16_t sht21_sensor_convertRH_WaterToIce(const int16_t rh_w, const int16_t t);

int sht21_sensor_getLastTemperature(char *string, int string_len);
int sht21_sensor_getLastelativeHumidity(char *string, int string_len);

#endif /* __SHT21_SENSOR_H__ */
