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
 * @file dev/bmp085-sensor.c
 * Plattform indepent implementation of the BMP085 sensor,
 * which uses the plattform dependent I2C-Interface.
 * @author Angelos Drossos <angelos.drossos@gmail.com>
 * @version 0.5
 * @date 10/9/2012
 * @copyright Contiki license
 *
 * The BMP085 sensor can read out the current temperature or pressure.
 *
 */
#include "dev/i2c/bmp085-sensor.h"

// platform dependent i2c implementation
#include "dev/i2c/i2c-dev.h"

// some std libs
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// debugging
#include "lib/assert.h"

/**
 * BMP085's global memory map.
 * The memory map shows all externally accessible registers
 * needed to operate BMA180.
 * @see BMP085 manual.
 */
typedef enum bmp085_register_address {
	AC1_msb = 0xAA,
	AC1_lsb = 0xAB,
	AC2_msb = 0xAC,
	AC2_lsb = 0xAD,
	AC3_msb = 0xAE,
	AC3_lsb = 0xAF,
	AC4_msb = 0xB0,
	AC4_lsb = 0xB1,
	AC5_msb = 0xB2,
	AC5_lsb = 0xB3,
	AC6_msb = 0xB4,
	AC6_lsb = 0xB5,
	B1_msb  = 0xB6,
	B1_lsb  = 0xB7,
	B2_msb  = 0xB8,
	B2_lsb  = 0xB9,
	MB_msb  = 0xBA,
	MB_lsb  = 0xBB,
	MC_msb  = 0xBC,
	MC_lsb  = 0xBD,
	MD_msb  = 0xBE,
	MD_lsb  = 0xBF,

	start_measurement     = 0xF4,
	read_measurement_msb  = 0xF6,
	read_measurement_lsb  = 0xF7,
	read_measurement_xlsb = 0xF8
} bmp085_register_address_t;

/**
 * BMP085's measurement type.
 */
typedef enum bmp085_start_measurement {
	/// Start temperature measurement.
	temp = 0x2E,
	/// Start pressure measurement
	pressure = (0x34 + (BMP085_OSS << 6))
} bmp085_start_measurement_t;

/**
 * BMP085's error types.
 */
enum ERROR {
	NO_ERROR = 1,
	ERROR = 0
};

/// maximum number of last values (must be a power of two)
#ifndef BMP085_BUFFER_SIZE
#define BMP085_BUFFER_SIZE (2^4)
#endif

/// The states of the BMP085
enum bmp085_states {
	OFF,  ///< Communication to the BMP085 is turned off.
	INIT_ERROR, ///< Shows that the BMP085 could not initialized.
	INIT, ///< The BMP085 is getting this calibration data.
	ON,   ///< The temperature and pressure value can read out.
};

/// This indicates in which state the BMP085 sensor is.
static uint8_t state = OFF;

typedef struct bmp085_data {
	uint8_t unmetered;
	uint8_t converted;
	uint8_t time;
} bmp085_data_t;

/// BMP085 sensor struct
const struct sensors_sensor bmp085_sensor;

/**
 * Get the current BMP085's type status.
 *
 * \param type ...
 * \return ...
 */
static int status(int sensors_type);

/**
 * Configures a specific type of the BMP085.
 * 
 * \param type ...
 * \param value ...
 * \return ...
 */
static int configure(int sensors_type, int value);

/**
 * Reads the temperature or the pressure value (unmetered)
 * from the BMP085.
 *
 * \return 1 if no error occurs else 0.
 */
static int value(int measurement_type);

static void turn_off();
static inline bool init(void);


/**
 * Integrate the BMP085 sensor in the Contiki-OS.
 */
SENSORS_SENSOR(bmp085_sensor, BMP085_SENSOR, value, configure, status);

/*---------------------------------------------------------------------------*/
/*
void bmp085_press(void)
{
	sensors_changed(&button_sensor);
}
*/
/*---------------------------------------------------------------------------*/
static int value(int measurement_type)
{
	static int32_t temp = 0;
	static int32_t pressure = 0;
	bool succ = false;
	// read temp or pressure over i2c/twi interface
	// write the value into a buffer
	switch (measurement_type) {
	case BMP085_SENSOR_TEMP:
		// read current temperature
		succ = bmp085_sensor_getTemperature(&temp);
		if (!succ) break;
		// save temp
		printf("readed Temp:     %7ld [0.1 ?C] = %3ld.%02ld [?C]\n",
						temp, temp/10, (temp%10)*10);
		break;
	case BMP085_SENSOR_PRESSURE:
		// read current pressure
		succ = bmp085_sensor_getPressure(&pressure, NULL);
		if (!succ) break;
		// save pressure
		printf("readed Pressure: %7ld [0.1 hPa] = %5ld.%02ld [hPa]\n",
						pressure, pressure/100, (pressure%100)*100);
		break;
	case BMP085_SENSOR_PRESSURE_AND_TEMPERATURE:
		// read current pressure and temperature
		succ = bmp085_sensor_getPressure(&pressure, &temp);
		if (!succ) break;
		// save pressure and temp
		printf("readed Temp:     %7ld [0.1 ?C] = %3ld.%02ld [?C]\n",
						temp, temp/10, (temp%10)*10);
		printf("readed Pressure: %7ld [0.1 hPa] = %5ld.%02ld [hPa]\n",
						pressure, pressure/10, (pressure%10)*10);
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
			// activate sensor
			if (!init()) {
				state = INIT_ERROR;
				return ERROR;
			}
			// no init error
			state = ON;
			// For for about (t0) ms before the sensor can be used.
			// rtimer_clock_t t0 = RTIMER_NOW();
			// while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RTIMER_SECOND / 100)); // FIXME: endless loop!
			return NO_ERROR;
		} else {
			// deactivate sensor
			state = OFF;
			return NO_ERROR;
		}
	default:
		break;
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

static inline bool write(bmp085_register_address_t reg_addr,
				const uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_write_data(BMP085_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [BMP085 Sensor]: could not write data!\n");
		return false;
	}
	return true;
}

static inline bool read(bmp085_register_address_t reg_addr,
				uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_read_data(BMP085_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [BMP085 Sensor]: could not read data!\n");
		return false;
	}
	return true;
}

static bool read_16u(bmp085_register_address_t reg_addr, uint16_t *const data)
{
	uint8_t msblsb[2];
	if (!read(reg_addr, msblsb, 2)) return false;
	assert(data);
	*data = (uint16_t) ((msblsb[0] << 8) | msblsb[1]);
	return true;
}

static bool read_16s(bmp085_register_address_t reg_addr, int16_t *const data)
{
	uint8_t msblsb[2];
	if (!read(reg_addr, msblsb, 2)) return false;
	assert(data);
	*data = (int16_t) ((msblsb[0] << 8) | msblsb[1]);
	return true;
}

#if (BMP085_OSS > 0)
static bool read_upressure(bmp085_register_address_t reg_addr, int32_t *const data)
{
	if (!data) return false;

	uint8_t msblsbx[3];
	if (!read(reg_addr, msblsbx, 3)) return false;
	*data = (int32_t) msblsbx[0] << 16;
	*data |= (int32_t) msblsbx[1] << 8;
	*data |= msblsbx[2];
	*data >>= (8 - BMP085_OSS);
	return true;
}
#endif

static bool read_temp(int32_t *const tp)
{
	if (!tp) return false;
	uint8_t sm = temp;
	if (!write(start_measurement, &sm, 1)) return false;
	i2c_dev_wait_ms(10);
	int16_t t = 0;
	if (!read_16s(read_measurement_msb, &t)) return false;
	*tp = (int32_t) t;
	return true;
}

static bool read_pressure(int32_t *const pr)
{
	if (!pr) return false;
	uint8_t sm = pressure;
	if (!write(start_measurement, &sm, 1)) return false;
	i2c_dev_wait_ms(10);
	#if (BMP085_OSS == 0)
	int16_t p;
	if (!read_16s(read_measurement_msb, &p)) return false;
	*pr = (int32_t) p;
	#else
	if (!read_upressure(read_measurement_msb, pr)) return false;
	#endif
	return true;
}


/** 
 * Calibration coefficients of the bmp 085 sensor system.
 * Every Sensor has individual coefficients.
 * None of the coefficients has the value 0x0000 or 0xFFFF.
 */
typedef struct bmp085_calibration_coefficients_t {
	int16_t  ac1;	///< example: +408
	int16_t  ac2;	///< example: -72
	int16_t  ac3;	///< example: -14383
	uint16_t ac4;	///< example: 32741
	uint16_t ac5;	///< example: 32757
	uint16_t ac6;	///< example: 23153
	int16_t  b1;	///< example: +6190
	int16_t  b2;	///< example: +4
	int16_t  mb;	///< example: -32768
	int16_t  mc;	///< example: -8711
	int16_t  md;	///< example: +2868
} bmp085_cal_t;

static inline bool calibration(bmp085_cal_t *cal)
{
	assert(cal);
	if (!cal) return false;

	if (!read_16s(AC1_msb, &cal->ac1)) return false;
	if (!read_16s(AC2_msb, &cal->ac2)) return false;
	if (!read_16s(AC3_msb, &cal->ac3)) return false;
	if (!read_16u(AC4_msb, &cal->ac4)) return false;
	if (!read_16u(AC5_msb, &cal->ac5)) return false;
	if (!read_16u(AC6_msb, &cal->ac6)) return false;
	if (!read_16s(B1_msb,  &cal->b1))  return false;
	if (!read_16s(B2_msb,  &cal->b2))  return false;
	if (!read_16s(MB_msb,  &cal->mb))  return false;
	if (!read_16s(MC_msb,  &cal->mc))  return false;
	if (!read_16s(MD_msb,  &cal->md))  return false;
	return true;
}

/// bmp085 Calibration Coefficients
static bmp085_cal_t cal;

static inline bool init(void)
{
	return calibration(&cal);
}

static inline int32_t calculate_temp(const int32_t ut, int32_t *const b5)
{
	assert(b5);
	int32_t x1 = (((int32_t) ut - (int32_t) cal.ac6) 
			* (int32_t) cal.ac5) >> 15;
	int32_t x2 = ((int32_t) cal.mc << 11) / (x1 + (int32_t) cal.md);
	*b5 = x1 + x2;

	int32_t realtemp = (*b5 + 8) >> 4; // in [0.1 °C]
	return realtemp;
}

static inline int32_t calculate_pressure(const int32_t up, const int32_t b5)
{
	int32_t b6 = b5 - 4000;
	int32_t x1 = (cal.b2 * ((b6 * b6) >> 12)) >> 11;
	int32_t x2 = (cal.ac2 * b6) >> 11;
	int32_t x3 = x1 + x2;
	int32_t b3 = (((((int32_t) cal.ac1) * 4 + x3) << BMP085_OSS) + 2) * 4;

	x1 = cal.ac3 * b6 >> 13;
	x2 = (cal.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;

	uint32_t b4 = (cal.ac4 * (uint32_t) (x3 + 32768)) >> 15;
	uint32_t b7 = ((uint32_t) up - b3) * (50000 >> BMP085_OSS);
	int32_t  p  = (b7 < 0x80000000) ? (b7 * 2) / b4 : (b7 / b4) * 2;

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;

	int32_t pressure = p + ((x1 + x2 + 3791) >> 4);
	return pressure * 2;
}

static bool getTemperature(int32_t *const temperature, int32_t *const b5)
{
	assert(temperature);
	assert(b5);
	if (!temperature || !b5) return false;

	// some bug here, have to read twice to get good data
	int32_t ut = 0;
	if (!read_temp(&ut)) return false;
	if (!read_temp(&ut)) return false;

	*temperature = calculate_temp(ut, b5);
	return true;
}

bool bmp085_sensor_getTemperature(int32_t *const temperature)
{
	int32_t b5;
	return getTemperature(temperature, &b5);
}

bool bmp085_sensor_getPressure(int32_t *const pressure, int32_t *const temperature)
{
	assert(pressure);
	if (!pressure) return false;

	int32_t temp = 0;
	int32_t b5 = 0;
	if (!getTemperature(&temp, &b5)) return false;

	// some bug here, have to read twice to get good data
	int32_t up = 0;
	if (!read_pressure(&up)) return false;
	if (!read_pressure(&up)) return false;

	*pressure = calculate_pressure(up, b5);
	if (temperature) *temperature = temp;
	return true;
}

