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


#ifndef __TSL2561_SENSOR_H__
#define __TSL2561_SENSOR_H__

/**
 * @file tsl2561-sensor.h
 * @ingroup sensors sensor_acceleration
 * @brief Read bma180's measurement data.
 * @details TODO
 * These function can read out the data values using the I2C bus system.
 * The application using these functions acts as Master Transmitter/Receiver
 * and the tsl2561 sensor acts as Slave Transmitter/Receiver.
 *
 * @author Angelos Drossos <angelos.drossos@gmail.com>
 * @version 0.3
 * @date 4/16/2012
 * @copyright Contiki License (BSD)
 *
 * @pre First initialize the I2C bus system: i2cInit().
 * @warning Improper use can block the I2C bus system.
 *
 * @note The I2C (Two-wire Serial Interface) also referred to as 
 * I2C (Inter-Integrated Circuit).
 *
 */

#include "lib/sensors.h"
#include <stdbool.h>

/// BMA180 sensor struct
extern const struct sensors_sensor tsl2561_sensor;

#undef  TSL2561_SENSOR
#define TSL2561_SENSOR "TSL2561"

/// BMA180 Sensor types
typedef enum tsl2561_sensor_type {
	/// The TSL2561 sensor will read the lux value.
	TSL2561_SENSOR_LUX,
} tsl2561_sensor_type_t;


#ifndef SENSORS_HW_INIT
	#error "Sensors Interface has changed!"
#endif
#ifndef SENSORS_ACTIVE
	#error "Sensors Interface has changed!"
#endif
#ifndef SENSORS_READY
	#error "Sensors Interface has changed!"
#endif


/**
 * @brief SDO connection type.
 */
typedef enum tsl2561_addr_t {
	/// ADDR is conntected to GND
	TSL2561_ADDR_CONNECTED_GND = 0x52,
	/// ADDR is not connected
	TSL2561_ADDR_FLOAT = 0x72,
	/// ADDR is not connected
	TSL2561_ADDR_CONNTECTED_VDD = 0x92
} tsl2561_addr_t;

/**
 * Set SDO line type of the TSL2561.
 */
#ifndef TSL2561_ADDR
	#warning "TSL2561 ADDR line maybe not defined correctly!"
	#define TSL2561_ADDR 	(TSL2561_ADDR_FLOAT)
#endif

/** The I2C device address of th BMA180 sensor.
 * @details Don't care about Read or Write mode.
 * @warning Two BMA180 can be connected to one I2C bus system
 *          at the same time. If you want to use more than two BMA180,
 *          you have to use a hardware switch or something similar.
 */
#undef  TSL2561_SENSOR_I2C_ADDRESS_WRITE
#define TSL2561_SENSOR_I2C_ADDRESS_WRITE (TSL2561_ADDR)


#endif /* __TSL2561_SENSOR_H__ */
