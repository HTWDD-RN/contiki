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


#ifndef __BMP085_SENSOR_H__
#define __BMP085_SENSOR_H__

#include "lib/sensors.h"
#include <stdbool.h>

/// Oversampling Setting (note: code is not set up to use other OSS values)
#ifndef BMP085_OSS
#define BMP085_OSS 0
#endif

/// BMP085 sensor struct
extern const struct sensors_sensor bmp085_sensor;

#undef  BMP085_SENSOR
#define BMP085_SENSOR "BMP085"

/// BMP085 Sensor types
typedef enum bmp085_sensor_type {
	/// The BMP085 sensor will read the temperature value.
	BMP085_SENSOR_TEMP,
	/// The BMP085 sensor will read the pressure value.
	BMP085_SENSOR_PRESSURE,
	/// The BMP085 sensor will read the pressure value.
	BMP085_SENSOR_PRESSURE_AND_TEMPERATURE,
	/// The BMP085 sensor will read the battery status.
	BMP085_SENSOR_BATTERY_INDICATOR
} bmp085_sensor_type_t;

/// The I2C device address of th BMP085 sensor.
#undef  BMP085_SENSOR_I2C_ADDRESS_WRITE
#define BMP085_SENSOR_I2C_ADDRESS_WRITE 0xEE

//void bmp085_press(void);
bool bmp085_sensor_getTemperature(int32_t *const temperature);
bool bmp085_sensor_getPressure(int32_t *const pressure, int32_t *const temperature);

#endif /* __BMP085_SENSOR_H__ */
