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
 * @file dev/sht21-sensor.c
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
#include "dev/i2c/sht21-sensor.h"

// platform dependent i2c implementation
#include "dev/i2c/i2c-dev.h"

// some std libs
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// debugging
#include "lib/assert.h"

// last temperature value
static int16_t last_temp = 0;

// last humidity value
static int16_t last_rh = 0;

// last humidity over ice value
static int16_t last_rh_ice = 0;


/**
 * SHT21's global memory map.
 * The memory map shows all externally accessible registers
 * needed to operate the SHT21 sensor.
 * @see SHT21 manual.
 */
typedef enum sht21_register_address {
	/// SHT21 Trigger T measurement (hold master): 1110 0011
	temp_meas_hold    = 0xE3,
	/// SHT21 Trigger RH measurement (hold master): 1110 0101
	rh_meas_hold   = 0xE5,
	/// SHT21 Trigger T measurement (no hold master): 1111 0011
	temp_meas_nohold  = 0xF3,
	/// SHT21 Trigger RH measurement (no hold master): 1111 0101
	rh_meas_nohold = 0xF5,
	/// SHT21 Write User Register: 1110 0110
	wr_user_reg    = 0xE6,
	/// SHT21 Read User Register: 1110 0111
	rd_user_reg    = 0xE7,
	/// SHT21 Soft Reset: Rebooting the sensor system: 1111 1110
	softreset      = 0xFE
} sht21_register_address_t;

/**
 * SHT21 sensor provides a CRC-8 checksum for error detection.
 * If error detection is activated, the SHT21 sensor transmit an extra byte
 * which is used to detect errors. The checksum cannot be used to correct
 * an error.
 * @see testChecksum(), read_inHoldMode()
 */
typedef enum sht21_checksum_t {
	NO_CHECKSUM   = false, ///< omit checksum transmission.
	WITH_CHECKSUM = true   ///< try to detect errors with CRC-8 checksum.
} sht21_checksum_t;

/**
 * This enum defines the needed coefficients
 * for relative humidity above ice (RH_i).
 * The RH_i value depends on the
 * relative humidity above water (RH_w) value and the temperature (t).
 *
 * @info The coefficients are multiplied by 100 to get integer values.
 * @see sht21_sensor_convertRH_WaterToIce()
 */
typedef enum sht21_rh_param {
	BETA_W_100   = 1762,  ///< beta_w = 17.62 []
	BETA_I_100   = 2246,  ///< beta_i = 22.46 []
	LAMBDA_W_100 = 24312, ///< lambda_w = 243.12 [°C]
	LAMBDA_I_100 = 27262  ///< lambda_i = 272.62 [°C]
} sht21_rh_param_t;

/// SHT21's error types.
typedef enum ERROR {
	NO_ERROR = true,
	ERROR = false
} error_t;

/// The states of the SHT21
typedef enum sht21_states {
	/// The SHT21 sensor module is deactivated, so
	/// there is no communication to the SHT21 sensor,
	/// even if another contiki process try to read out a sensor value.
	OFF,
	/// The SHT21 sensor moule is activated,
	/// but the checksum test is deactivated.
	/// By default, the sensor switches to this state,
	/// if the sensor is activated.
	ON,
	/// The SHT21 sensor moule is activated
	/// and uses the checksum test to detect errors in sensor values.
	ON_WITH_CHECKSUM_CHECK,
} sht21_states_t;

/// This indicates in which state the SHT21 sensor is.
/// By default, the sensor is deactivated.
static uint8_t state = OFF;

//=============================================================================

static inline bool write(sht21_register_address_t reg_addr,
				const uint8_t *const msblsb, size_t size)
{
	assert(size > 0 && msblsb);
	if (size <= 0 || !msblsb) return false;

	if (i2c_dev_write_data(SHT21_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [SHT21 Sensor]: could not write data!\n");
		return false;
	}
	return true;
}

static inline bool read(sht21_register_address_t reg_addr,
				uint8_t *const msblsb, size_t size)
{
	assert(size > 0 && msblsb);
	if (size <= 0 || !msblsb) return false;

	if (i2c_dev_read_data(SHT21_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [SHT21 Sensor]: could not read data!\n");
		return false;
	}
	return true;
}

static inline bool testChecksum(const uint16_t data, const uint8_t checksum)
{
	// TODO: implement checksum check
	fprintf(stderr, "sht21-sensor: testChecksum is not implemented..\n");
	return true;
}

/**
 * Reads the specified unmetered value in hold master mode.
 * @ingroup i2cApps
 *
 * @param[in] meas TEMP to read the temperature value
 *             or RH to read the relative humidity value
 * @param[out] data pointer to store the unmetered value
 * @param[in] chsum WITH_CHECKSUM if a CRC-8 check is required
 *              else NO_CHECKSUM
 * @return true if successful
 *         else false
 *
 * @todo add crc-8 checksum check to detect errors
 */
static bool read_inHoldMode(const bool readRH, uint16_t *const data, const bool checkChecksum)
{
	assert(data);
	if (!data) return false;

	uint8_t size = (checkChecksum)? 3 : 2;
	uint8_t msblsb[3];
	if (!read( (readRH)? rh_meas_hold:temp_meas_hold, msblsb, size)) return false;

	// Status Bits: 
	// Bit 1: Data is temperature (0) or relative humidity (1)
	// Bit 0: currently not assigned
	uint8_t assumedStatusBits = (readRH)? (1 << 1) : (0 << 1);
	if (assumedStatusBits != bit_is_set(msblsb[1], 1))
	{
		printf("!! [sht21 sensor]: false status bit!\n");
		return false;
	}

	*data = (uint16_t) ((msblsb[0] << 8) | msblsb[1]) & ~( _BV(1) | _BV(0) );

	bool succ = true;
	if (checkChecksum)
	{
		succ = testChecksum(*data, msblsb[2]);
	}

	// remove status bits
	*data &= ~( _BV(0) | _BV(1) );
	return succ;
}

static inline bool readRH_inHoldMode(uint16_t *const rh, const bool checkChecksum)
{
	return read_inHoldMode(true, rh, checkChecksum);
}

static inline bool readTemp_inHoldMode(uint16_t *const temp, const bool checkChecksum)
{
	return read_inHoldMode(false, temp, checkChecksum);
}


/**
 * Resets the SHT21 sensor. 
 * After reset communication, the function waits 15ms until the sensor
 * is ready.
 * @ingroup i2cApps
 *
 * @return true, if soft reset is successful, else false.
 */
static bool doSoftReset(void)
{
	if (i2c_dev_send_cmd(SHT21_SENSOR_I2C_ADDRESS_WRITE, softreset,
					false, false)
			!= SUCCESSFUL)
	{
		printf("!! [SHT21 Sensor]: SoftReset failed.\n");
		return false;
	}

	// The soft reset takes less than 15ms
	i2c_dev_wait_ms(15);

	return true;
}

/**
 * Calculates the temperature in [0.01 °C].
 *
 * @param[in] s_t readed unmetered temperature value from the sensor.
 * @return the temperature in [0.01 °C].
 */
static int16_t calculateTemperature(const uint16_t s_t)
{
	// T [°C] = -46.85 + 175.72 * s_t / 2^16
	// T [0.01 °C] = T [°C] * 100 = -4685 + ((17572 * s_t) >> 16)
	// uint16_t s_t:
	//    s_t = [0, 2^16-1] => T = [-4685, +12887] * 0.01 °C
	int32_t t = (int32_t) s_t;
	t = ((17572 * t) >> 16) - 4685;
	assert(-4685 <= t && t <= 12887);
	return (int16_t) t;
}

/** 
 * Converts the relative humidity data (unmetered measurement)
 * in the relative humidity (RH), no matter which resolution is chosen.
 * The phsical value RH corresponds to the relative humidity
 * above liquid water according to World Meteorological Organization (WMO).
 *
 * @param[in] s_rh the relative humidity data (between 0 and 55575)
 * @return the relative humidity in % (above liquid water).
 */
static int16_t calculateRH(const uint16_t s_rh)
{
	// RH_W [%RH] = -6 + 125 * s_rh / 2^16
	// RH_W [0.01 %RH] = -600 + ((12500 * s_rh) >> 16)
	// uint16_t s_rh:
	//    s_rh = [0, 2^16-1] => RH_W = [-600, +11900[ * 0.01 %RH
	int32_t rh = (int32_t) s_rh;
	rh = ((12500 * rh) >> 16) - 600;
	// assert(-2^15 <= rh && rh <= 2^15-1)
	return (int16_t) rh;
}

/**
 * Converts the relative humidity data (unmetered measurement)
 * in the relative humidity (RH_i), no matter which resolution is chosen.
 * The phsical value RH corresponds to the relative humidity
 * above ice according to World Meteorological Organization (WMO).
 *
 * @param[in] rh_w relative humidity above water in [0.01 %RH]
 * @param[in] t temperature in [0.01 °C]
 * @return relative humidity above ice in [0.01 %RH]
 */
static int16_t convertRH_WaterToIce(const int16_t rh_w, const int16_t t)
{
	// RH_I [%RH] = RH_W [%RH] 
	//              * exp(\beta_W * t / (\lambda_W + t))
	//              / exp(\beta_I * t / (\lambda_I + t))
	int32_t exp_W = (int32_t) t;
	int32_t exp_I = (int32_t) t;
	exp_W *= BETA_W_100;
	exp_W /= (LAMBDA_W_100 + t);
	exp_I *= BETA_I_100;
	exp_I /= (LAMBDA_I_100 + t);
	int32_t rh_i = rh_w * exp_W / exp_I;
	return (int16_t) rh_i;
}


/**
 * Reads the current temperatur value from the sensor.
 * @ingroup sensors sensor_temp
 *
 * @param[out] temp the pointer to store the temperature value in [0.01 °C]
 * @return true if communication with the sensor was successful
 */
static bool getTemperature(int16_t *const temp,
				const bool checkChecksum)
{
	assert(temp);
	if (!temp) return false;

	// read unmetered temperature
	uint16_t ut;
	if (!readTemp_inHoldMode(&ut, checkChecksum))
		return false;

	// calculate real temperature
	*temp = calculateTemperature(ut);
	return true;
}

/**
 * Read the current relative humidity value from the sensor.
 * @ingroup sensors sensor_rh
 *
 * @param[out] rh the relative humidity (above water) in [0.01 %RH]
 * @return true if communication with the sensor was successful; 
 *         else false.
 */
static bool getRelativeHumidity(int16_t *const rh,
				const bool checkChecksum)
{
	assert(rh);
	if (!rh) return false;

	// read unmetered temperature
	uint16_t urh;
	if (!readRH_inHoldMode(&urh, checkChecksum))
		return false;

	// calculate real temperature
	*rh = calculateRH(urh);
	return true;
}

/**
 * Read the current relative humidity value from the sensor.
 * @ingroup sensors sensor_rh sensor_temp
 *
 * @param[out] rh_ice the relative humidity (above ice) in [0.01 %RH]
 * @param[out] rh_water the relative humidity (above water) in [0.01 %RH]
 *                      or NULL [optional]
 * @param[out] temp the temperature in [0.01 °C]
 *                  or NULL [optional]
 * @return true if communication with the sensor was successful; 
 *         else false.
 */
static bool getRelativeHumidityIce(int16_t *const rh_ice,
				int16_t *const rh_water, int16_t *const temp,
				const bool checkChecksum)
{
	assert(rh_ice);
	assert(rh_ice != rh_water);
	if (!rh_ice) return false;
	
	int16_t t;
	if (!getTemperature(&t, checkChecksum))
		return false;

	int16_t rh_w;
	if (!getRelativeHumidity(&rh_w, checkChecksum))
		return false;

	*rh_ice = convertRH_WaterToIce(rh_w, t);
	if (rh_water && rh_ice != rh_water) *rh_water = rh_w;
	if (temp) *temp     = t;
	return true;
}

//=============================================================================

/**
 * Get the current SHT21's type status.
 * @see sht21_sensor_configure_type_t for more informations
 */
static int status(int sensor_type)
{
	switch (sensor_type) {
	case SHT21_SENSOR_ACTIVE: // is the sensor active
	case SHT21_SENSOR_READY:  // or ready ?
		return (state != OFF);
	case SHT21_SENSOR_SOFT_RESET:
		return 1;
	default:
		break;
	}
	return 0; // false, unknown sensor_type
}

/**
 * Reads the temperature or the rlative humidity value from the SHT21.
 * The result is not returned. Instead, it has to read out from the buffer.
 * @see sht21_sensor_value_type_t for more informations
 *
 * @return 1, if no error occurs, else 0.
 */
static int value(int measurement_type)
{
	if (state == OFF)
	{
		fprintf(stderr, "Error: " SHT21_SENSOR " is deactivated!\n");
		return ERROR;
	}


	int16_t temp = 0;
	int16_t rh = 0;
	bool succ = false;
	bool checkChecksum = (state == ON_WITH_CHECKSUM_CHECK);
	// read temp or pressure over i2c/twi interface
	// write the value into a buffer
	switch (measurement_type) {
	case SHT21_SENSOR_TEMP:
		// read current temperature
		printf("SHT21:read:Temp..\n");
		succ = getTemperature(&temp, checkChecksum);
		if (!succ) break;
		// save temp
		printf(SHT21_SENSOR ":temp = %6hd [0.01 °C] = %3hd.%02hd [°C]\n", temp, temp/100, temp%100);
		last_temp = temp;
		return NO_ERROR;
	case SHT21_SENSOR_RELATIVE_HUMIDITY:
		// read current relative humidity
		printf("SHT21:read:RealtiveHumidity..\n");
		succ = getRelativeHumidity(&rh, checkChecksum);
		if (!succ) break;
		// save relative humidity
		last_rh = rh;
		printf(SHT21_SENSOR ":rh(water) = %7hd [0.01 %%RH] = %4hd.%02hd [%%RH]\n", rh, rh/100, rh%100);
		return NO_ERROR;
	case SHT21_SENSOR_RELATIVE_HUMIDITY_ICE:
		// read current relative humidity over ice
		printf("SHT21:read:RealtiveHumidityOverIce..\n");
		succ = getRelativeHumidityIce(&rh, NULL, NULL, checkChecksum);
		if (!succ) break;
		// save relative humidity over ice
		last_rh_ice = rh;
		printf(SHT21_SENSOR ":rh(ice) = %7hd [0.01 %%RH] = %4hd.%02hd [%%RH]\n", rh, rh/100, rh%100);
		return NO_ERROR;
	default:
		break;
	}
	return ERROR; // false, error
}

/**
 * Configures a specific type of the SHT21.
 * @see sht21_sensor_configure_type_t for more informations
 * 
 * @return the return value depends on the given type and value.
 */
static int configure(int sensor_type, int value)
{
	switch (sensor_type) {
	case SHT21_SENSOR_HW_INIT:
		return NO_ERROR;
	case SHT21_SENSOR_ACTIVE:
		if (value && !status(SHT21_SENSOR_ACTIVE)) {
			state = ON;
			printf(SHT21_SENSOR ":conf: Sensor is now activated.\n");
			return configure(SHT21_SENSOR_SOFT_RESET, 1);
		} else {
			state = OFF;
			printf(SHT21_SENSOR ":conf: Sensor is now deactivated.\n");
		}
		return NO_ERROR;
	case SHT21_SENSOR_READY:
		printf(SHT21_SENSOR ":conf: Sensor is ready.\n");
		return NO_ERROR;
	case SHT21_SENSOR_SOFT_RESET:
		if (!doSoftReset())
		{
			state = OFF;
			fprintf(stderr, SHT21_SENSOR ":conf: SoftReset was NOT successfull. Sensor is now deactivated!\n");
			break;
		}
		printf(SHT21_SENSOR ":conf: SoftReset was successfull.\n");
		return NO_ERROR;
	default:
		break;
	}
	return ERROR;
}

/**
 * SHT21 sensor struct.
 * Integrate the SHT21 sensor in the Contiki-OS.
 */
SENSORS_SENSOR(sht21_sensor, SHT21_SENSOR, value, configure, status);

int sht21_sensor_getLastTemperature(char *string, int string_len)
{
	if (!string) return -1;
	// TODO negative temp values
	return sprintf(string, "%3hd.%02hd [°C]\n", last_temp/100, last_temp%100);
}

int sht21_sensor_getLastelativeHumidity(char *string, int string_len)
{
	if (!string) return -1;
	return sprintf(string, "%4hd.%02hd [%%RH]\n", last_rh/100, last_rh%100);
}

