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
 * @file dev/bma180-sensor.c
 * Plattform indepent implementation of the BMA180 sensor,
 * which uses the plattform dependent I2C-Interface.
 * @author Angelos Drossos <angelos.drossos@gmail.com>
 * @version 0.5
 * @date 10/9/2012
 * @copyright Contiki license
 *
 * The BMA180 sensor can read out the current temperature or pressure.
 *
 */
#include "dev/i2c/bma180-sensor.h"

// platform dependent i2c implementation
#include "dev/i2c/i2c-dev.h"

// some std libs
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// debugging
#include "lib/assert.h"

/**
 * BMA180's global memory map.
 * The memory map shows all externally accessible registers
 * needed to operate BMA180.
 * @see BMA180 manual.
 */
typedef enum bma180_register_address_t {
	chip_id               = 0x00,
	version               = 0x01,
	acc_x_lsb             = 0x02,
	acc_x_msb             = 0x03,
	acc_y_lsb             = 0x04,
	acc_y_msb             = 0x05,
	acc_z_lsb             = 0x06,
	acc_z_msb             = 0x07,
	temp                  = 0x08,
	status_reg1           = 0x09,
	status_reg2           = 0x0A,
	status_reg3           = 0x0B,
	status_reg4           = 0x0C,
	ctrl_reg0             = 0x0D,
	ctrl_reg1             = 0x0E,
	ctrl_reg2             = 0x0F,
	reset                 = 0x10,

	bw_tcs                = 0x20,
	ctrl_reg3             = 0x21,
	ctrl_reg4             = 0x22,
	hy                    = 0x23,
	slope_tapsens_info    = 0x24,
	high_low_info         = 0x25,
	low_dur               = 0x26,
	high_dur              = 0x27,
	tapsens_th            = 0x28,
	low_th                = 0x29,
	high_th               = 0x2A,
	slope_th              = 0x2B,
	cd1                   = 0x2C,
	cd2                   = 0x2D,
	tco_x                 = 0x2E,
	tco_y                 = 0x2F,
	tco_z                 = 0x30,
	gain_t                = 0x31,
	gain_x                = 0x32,
	gain_y                = 0x33,
	gain_z                = 0x34,
	offset_lsb1           = 0x35,
	offset_lsb2           = 0x36,
	offset_t              = 0x37,
	offset_x              = 0x38,
	offset_y              = 0x39,
	offset_z              = 0x3A,

	ee_bw_tcs             = 0x40,
	ee_ctrl_reg3          = 0x41,
	ee_ctrl_reg4          = 0x42,
	ee_hy                 = 0x43,
	ee_slope_tapsens_info = 0x44,
	ee_high_low_info      = 0x45,
	ee_low_dur            = 0x46,
	ee_high_dur           = 0x47,
	ee_tapsens_th         = 0x48,
	ee_low_th             = 0x49,
	ee_high_th            = 0x4A,
	ee_slope_th           = 0x4B,
	ee_cd1                = 0x4C,
	ee_cd2                = 0x4D,
	ee_tco_x              = 0x4E,
	ee_tco_y              = 0x4F,
	ee_tco_z              = 0x50,
	ee_gain_t             = 0x51,
	ee_gain_x             = 0x52,
	ee_gain_y             = 0x53,
	ee_gain_z             = 0x54,
	ee_offset_lsb1        = 0x55,
	ee_offset_lsb2        = 0x56,
	ee_offset_t           = 0x57,
	ee_offset_x           = 0x58,
	ee_offset_y           = 0x59,
	ee_offset_z           = 0x5A,
	ee_crc                = 0x5B
} bma180_register_address_t;

/**
 * BMA180's error types.
 */
enum ERROR {
	NO_ERROR = 1,
	ERROR = 0
};

/// The states of the BMA180
enum bma180_states {
	OFF,  ///< Communication to the BMA180 is turned off.
	INIT_ERROR, ///< Shows that the BMA180 could not initialized.
	INIT, ///< The BMA180 is getting this calibration data.
	ON,   ///< The temperature and pressure value can read out.
};

/// This indicates in which state the BMA180 sensor is.
static uint8_t state = OFF;


/// BMA180 sensor struct
const struct sensors_sensor bma180_sensor;

/**
 * Get the current BMA180's type status.
 *
 * \param type ...
 * \return ...
 */
static int status(int sensors_type);

/**
 * Configures a specific type of the BMA180.
 * 
 * \param type ...
 * \param value ...
 * \return ...
 */
static int configure(int sensors_type, int value);

/**
 * Reads the temperature or the pressure value (unmetered)
 * from the BMA180.
 *
 * \return 1 if no error occurs else 0.
 */
static int value(int measurement_type);

static void turn_off();
static inline bool init(void);


/**
 * Integrate the BMA180 sensor in the Contiki-OS.
 */
SENSORS_SENSOR(bma180_sensor, BMA180_SENSOR, value, configure, status);

/*---------------------------------------------------------------------------*/
static int value(int measurement_type)
{
	static uint16_t temp = 0;
	static int32_t accX = 0;
	static int32_t accY = 0;
	static int32_t accZ = 0;
	bool succ = false;
	// read temp or pressure over i2c/i2c interface
	// write the value into a buffer
	switch (measurement_type) {
	case BMA180_SENSOR_TEMP:
		// read current temperature
		succ = bma180_sensor_getTemperature(&temp);
		if (!succ) break;
		// save temp
		break;
	case BMA180_SENSOR_ACC_X:
		// read current pressure
		succ = bma180_sensor_getAccX(&accX);
		if (!succ) break;
		// save pressure
		break;
	case BMA180_SENSOR_ACC_Y:
		// read current pressure
		succ = bma180_sensor_getAccY(&accY);
		if (!succ) break;
		// save pressure
		break;
	case BMA180_SENSOR_ACC_Z:
		// read current pressure
		succ = bma180_sensor_getAccZ(&accZ);
		if (!succ) break;
		// save pressure
		break;
	default:
		break;
	}
	return ERROR; // false, error
}
/*---------------------------------------------------------------------------*/
static int configure(int sensors_type, int value)
{
	switch (sensors_type) {
	case SENSORS_ACTIVE:
		if (value && !status(SENSORS_ACTIVE)) {
			if (!init()) {
				state = INIT_ERROR;
				return ERROR;
			}
			// no init error
			state = ON;
		} else {
			turn_off();
			state = OFF;
		}
		return NO_ERROR;
	}
	return ERROR;
}
/*---------------------------------------------------------------------------*/
static int status(int sensors_type)
{
	switch (sensors_type) {
	case SENSORS_ACTIVE: // is the sensor active
	case SENSORS_READY:  // or ready ?
		return (state == ON);
	}
	return 0; // false, unknown sensor_type
}
/*---------------------------------------------------------------------------*/
static inline bool init(void)
{
	return true;
}
/*---------------------------------------------------------------------------*/
static void turn_off()
{

}
/*---------------------------------------------------------------------------*/

static inline bool write(bma180_register_address_t reg_addr,
				const uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_write_data(BMA180_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [BMA180 Sensor]: could not write data!\n");
		return false;
	}
	return true;
}

static inline bool read(bma180_register_address_t reg_addr,
				uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_read_data(BMA180_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [BMA180 Sensor]: could not read data!\n");
		return false;
	}
	return true;
}


static bool readTemp(int8_t *const tempdata)
{
	assert(tempdata);
	if (!tempdata) return false;

	uint8_t data;
	if (!read(temp, &data, 1)) return false;

	// 0x80 is lowest temperature
	*tempdata = data;
	return true;
}

static bool readAcceleration(const bma180_register_address_t acc_lsb,
		uint16_t *const data)
{
	assert(data);
	if (!data) return false;

	uint8_t msblsb[2];
	if (!read(acc_lsb, msblsb, 2)) return false;

	*data = msblsb[1] << 6;
	*data |= msblsb[0] >> 2;
	return true;
}

bool bma180_sensor_getTemperature(uint16_t *const ut)
{
	assert(ut);
	if (!ut) return false;
	
	int8_t utp;
	if (!readTemp(&utp)) return false;

	*ut = utp;
	return true;
}

bool bma180_sensor_getUnmeteredAccX(uint16_t *const x)
{
	assert(x);
	if (!x) return false;
	return readAcceleration(acc_x_lsb, x);
}

bool bma180_sensor_getUnmeteredAccY(uint16_t *const y)
{
	assert(y);
	if (!y) return false;
	return readAcceleration(acc_y_lsb, y);
}

bool bma180_sensor_getUnmeteredAccZ(uint16_t *const z)
{
	assert(z);
	if (!z) return false;
	return readAcceleration(acc_z_lsb, z);
}

int32_t bma180_sensor_convertUnmeteredAcc(const uint16_t acc)
{
	// calculate positive sign
	int32_t gval = 0;
	gval = acc & ~(_BV(15)|_BV(14)|_BV(13));
	gval *= 25;
	// is negative bit set?
	if ((acc & _BV(13)) == _BV(13))
	{
		// negative sign: add -2g to acc
		gval -= 2000000;
	}
	return gval;
}

bool bma180_sensor_getAccX(int32_t *const x)
{
	assert(x);
	if (!x) return false;

	uint16_t ua;
	if (!bma180_sensor_getUnmeteredAccX(&ua)) return false;

	*x = bma180_sensor_convertUnmeteredAcc(ua);
	return true;
}

bool bma180_sensor_getAccY(int32_t *const y)
{
	assert(y);
	if (!y) return false;

	uint16_t ua;
	if (!bma180_sensor_getUnmeteredAccY(&ua)) return false;

	*y = bma180_sensor_convertUnmeteredAcc(ua);
	return true;
}

bool bma180_sensor_getAccZ(int32_t *const z)
{
	assert(z);
	if (!z) return false;

	uint16_t ua;
	if (!bma180_sensor_getUnmeteredAccZ(&ua)) return false;

	*z = bma180_sensor_convertUnmeteredAcc(ua);
	return true;
}

bool bma180_sensor_getChipID(uint8_t *const id)
{
	assert(id);
	if (!id) return false;

	uint8_t msblsb;
	if (!read(chip_id, &msblsb, 1)) return false;

	*id = msblsb & (_BV(0)|_BV(1)|_BV(2));
	return true;
}

