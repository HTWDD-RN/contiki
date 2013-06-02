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



#ifndef __BMA180_SENSOR_H__
#define __BMA180_SENSOR_H__

/**
 * @file bma180.h
 * @ingroup sensors sensor_acceleration
 * @brief Read bma180's measurement data.
 * @details TODO
 * These function can read out the data values using the I2C bus system.
 * The application using these functions acts as Master Transmitter/Receiver
 * and the bma180 sensor acts as Slave Transmitter/Receiver.
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
extern const struct sensors_sensor bma180_sensor;

#undef  BMA180_SENSOR
#define BMA180_SENSOR "BMA180"

/// BMA180 Sensor types
typedef enum bma180_sensor_type {
	/// The BMA180 sensor will read the temperature value.
	BMA180_SENSOR_TEMP,
	/// The BMA180 sensor will read the acceleration x-axis value.
	BMA180_SENSOR_ACC_X,
	/// The BMA180 sensor will read the acceleration y-axis value.
	BMA180_SENSOR_ACC_Y,
	/// The BMA180 sensor will read the acceleration z-axis value.
	BMA180_SENSOR_ACC_Z,
} bma180_sensor_type_t;


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
typedef enum bma180_sdo_t {
	/// SDO is conntected to VSS or GND
	BMA180_CONNECTED_VSS = 0,
	/// SDO is conntected to VDDIO
	BMA180_CONNECTED_VDDIO = 1
} bma180_sdo_t;

/**
 * Set SDO line type of the BMA180.
 */
#ifndef BMA180_SDO
	#warning "BMA180 SDO line maybe not defined correctly!"
	#define BMA180_SDO 	(BMA180_CONNECTED_VDDIO)
#endif

/** The I2C device address of th BMA180 sensor.
 * @details Don't care about Read or Write mode.
 * @warning Two BMA180 can be connected to one I2C bus system
 *          at the same time. If you want to use more than two BMA180,
 *          you have to use a hardware switch or something similar.
 */
#undef  BMA180_SENSOR_I2C_ADDRESS_WRITE
#define BMA180_SENSOR_I2C_ADDRESS_WRITE (0x80 | (BMA180_SDO << 1))


/**
 * Convert the unmetered acceleration data to a g-value.
 * The description of the digital signals acc_x, acc_y and acc_z 
 * is "2's complement", based on 14 bits.
 *
 * @param[in] acc unmetered acceleration data (bit 14 and 15 are ignored).
 * @return g-value in [0.000001 g], +2/-2 measurement g-range, e.g.:
 *         @p acc = 0x0000 => 000000 = 0.00000 g;
 *         @p acc = 0x0001 => +000025 = +0.00025 g;
 *         @p acc = 0x1FFF => +199975 = +1.99975 g;
 *         @p acc = 0x2000 => -200000 = -2.00000 g;
 *         @p acc = 0x3FFF => -000025 = -0.00025 g.
 */
int32_t bma180_sensor_convertUnmeteredAcc(const uint16_t acc);

bool bma180_sensor_getTemperature(uint16_t *const unmetered_temperature);

bool bma180_sensor_getUnmeteredAccX(uint16_t *const unmetered_x_acc);
bool bma180_sensor_getUnmeteredAccY(uint16_t *const unmetered_y_acc);
bool bma180_sensor_getUnmeteredAccZ(uint16_t *const unmetered_z_acc);

bool bma180_sensor_getAccX(int32_t *const x_acc);
bool bma180_sensor_getAccY(int32_t *const y_acc);
bool bma180_sensor_getAccZ(int32_t *const z_acc);

/**
 * Get chip ID and chip version.
 */
bool bma180_sensor_getChipID(uint8_t *const id);



#endif /* __BMA180_SENSOR_H__ */
