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
 * @file dev/tsl2561-sensor.c
 * Plattform indepent implementation of the TSL2561 sensor,
 * which uses the plattform dependent I2C-Interface.
 * @author Angelos Drossos <angelos.drossos@gmail.com>
 * @version 0.1
 * @date 10/22/2012
 * @copyright Contiki license (BSD)
 *
 * The TSL2561 sensor can read out the current lux value.
 */
#include "dev/i2c/tsl2561-sensor.h"

// platform dependent i2c implementation
#include "dev/i2c/i2c-dev.h"

// some std libs
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// debugging
#include "lib/assert.h"

//=================================================
// Lux Calculation Macros
//=================================================

/// Lux scale by 2^LUX_SCALE
#define LUX_SCALE     14
/// Scale ratio by 2^RATIO_SCALE
#define RATIO_SCALE   9
/// scale channel values by 2^CH_SCALE
#define CH_SCALE      10
/// scale channel tint0: (322/11 * 2^CH_SCALE) = 0x7517
#define CH_SCALE_TINT0   0x7517
/// scale channel tint1: (322/81 * 2^CH_SCALE) = 0x0fe7
#define CH_SCALE_TINT1   0x0fe7

#ifdef TSL2561_PACKAGE_CS
#include "dev/i2c/tsl2561-package-CS.h"
#else
#include "dev/i2c/tsl2561-package-T.h"
#endif

//=================================================

/**
 * TSL2561's global memory map.
 * The memory map shows all externally accessible registers
 * needed to operate TSL2561.
 * @see TSL2561 manual.
 */
typedef enum tsl2561_register_address_t {
	control               = 0x00,
	timing                = 0x01,
	thresh_low_low        = 0x02,
	thresh_low_high       = 0x03,
	thresh_high_low       = 0x04,
	thresh_high_high      = 0x05,
	interrupt             = 0x06,
	// reserved           = 0x07,
	crc                   = 0x08,
	// reserved           = 0x09,
	id                    = 0x0A,
	// reserved           = 0x0B,
	data_0_low            = 0x0C,
	data_0_high           = 0x0D,
	data_1_low            = 0x0E,
	data_1_high           = 0x0F,
} tsl2561_register_address_t;

#define CMD_REG(CMD_BIT, CLEAR_BIT, WORD_BIT, BLOCK_BIT, ADDRESS) \
		( ((CMD_BIT) << 7) | ((CLEAR_BIT) << 6) | ((WORD_BIT) << 5) \
		  | ((BLOCK_BIT) << 4) | ((ADDRESS) << 0) )

#define COMMAND(ADDRESS) \
		CMD_REG(1, 0, 0, 0, (ADDRESS))

#define READ_VALUE(ADDRESS) \
		CMD_REG(1, 0, 1, 0, (ADDRESS))

/// Control Register: Power modes.
typedef enum control_power {
	control_power_down = 0x0, ///< power down (off, 0.0032 mA, max 0.0150 mA)
	control_power_up   = 0x3  ///< power up   (on,  0.2400 mA, max 0.6000 mA)
} control_power_t;

/// The possible gain modes of sensor.
typedef enum gain_mode {
	g1X  = 0x0, ///< gain mode: 1X
	g16X = 0x1 ///< gain mode: 16X
} gain_mode_t;

/// Timing Control Select: Integration time modes.
typedef enum time_mode {
	t13_7_ms = 0x0, ///< integration time: 13.7 ms
	t101_ms  = 0x1, ///< integration time: 101 ms
	t402_ms  = 0x2, ///< integration time: 402 ms
	tMANUAL  = 0x3  ///< Manual timing control
} time_mode_t;

/// Timing Register.
#define TIME_CONTROL(GAIN_MODE, MANUAL, TIME_MODE) \
		( (0 << 7) | (0 << 6) | (0 << 5) | ((GAIN_MODE) << 4) \
		  | ((MANUAL) << 3) | (0 << 2) | ((TIME_MODE) << 0) )

/// Timing Register with manual timing control.
#define TIME_CONTROL_MANUAL(GAIN_MODE, MANUAL) \
		( TIME_CONTROL(GAIN_MODE, MANUAL, tMANUAL) )

/// Timing Register without manual timing control.
#define TIME_CONTROL_NORMAL(GAIN_MODE, TIME_MODE) \
		( TIME_CONTROL(GAIN_MODE, 0, TIME_MODE) )

/// Interrupt Control Select: Interrupt modes.
typedef enum int_control {
	int_off = 0x00,   ///< Interrupt output disabled
	int_level = 0x08, ///< Level interrupt
	int_smbus = 0x10, ///< SMBAlert compliant
	int_test = 0x18   ///< Test mode: Sets interrupt and functions as #int_smbus
} int_control_t;

/// Interrupt Persistence Select.
typedef enum int_rate {
	int_every_cycle = 0x0, ///< Every ADC cycle generates interrupt
	int_thres_1     = 0x1, ///< Any value outside of threshold range
	int_thres_2     = 0x2, ///< 2 integration time periods out of range
	int_thres_3     = 0x3, ///< 3 integration time periods out of range
	int_thres_4     = 0x4, ///< 4 integration time periods out of range
	int_thres_5     = 0x5, ///< 5 integration time periods out of range
	int_thres_6     = 0x6, ///< 6 integration time periods out of range
	int_thres_7     = 0x7, ///< 7 integration time periods out of range
	int_thres_8     = 0x8, ///< 8 integration time periods out of range
	int_thres_9     = 0x9, ///< 9 integration time periods out of range
	int_thres_10    = 0xA, ///< 10 integration time periods out of range
	int_thres_11    = 0xB, ///< 11 integration time periods out of range
	int_thres_12    = 0xC, ///< 12 integration time periods out of range
	int_thres_13    = 0xD, ///< 13 integration time periods out of range
	int_thres_14    = 0xE, ///< 14 integration time periods out of range
	int_thres_15    = 0xF, ///< 15 integration time periods out of range
} int_rate_t;

/// Interrupt Control Register.
#define INT_CONTROL(INT_CONTROL, INT_RATE) \
		( (0 << 7) | (0 << 6) | ((INT_CONTROL) << 4) | ((INT_RATE) << 0) )

/**
 * ID Register (read-only): provides part number (0: TSL2560, 1: TSL2561)
 * and silicon revision number.
 */
#define DEVICE_ID_REG(PART_NO, REV_NO) \
		( ((PART_NO) << 4) | ((REV_NO) << 0) )

/**
 * BMA180's error types.
 */
enum ERROR {
	NO_ERROR = 1,
	ERROR = 0
};

/// The states of the BMA180
enum tsl2561_states {
	OFF,  ///< Communication to the BMA180 is turned off.
	INIT_ERROR, ///< Shows that the BMA180 could not initialized.
	INIT, ///< The BMA180 is getting this calibration data.
	ON,   ///< The temperature and pressure value can read out.
};

/// This indicates in which state the BMA180 sensor is.
static uint8_t state = OFF;


/// BMA180 sensor struct
const struct sensors_sensor tsl2561_sensor;

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


/**
 * Integrate the BMA180 sensor in the Contiki-OS.
 */
SENSORS_SENSOR(tsl2561_sensor, TSL2561_SENSOR, value, configure, status);

/*---------------------------------------------------------------------------*/

static inline bool write(uint8_t reg_addr,
				const uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_write_data(TSL2561_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [TSL2561 Sensor]: could not write data!\n");
		return false;
	}
	return true;
}

static inline bool read(uint8_t reg_addr,
				uint8_t *const msblsb, size_t size)
{
	assert(msblsb);
	if (i2c_dev_read_data(TSL2561_SENSOR_I2C_ADDRESS_WRITE, reg_addr,
					msblsb, size, false, false)
			!= SUCCESSFUL)
	{
		printf("!! [TSL2561 Sensor]: could not read data!\n");
		return false;
	}
	return true;
}

static inline bool read_command(tsl2561_register_address_t reg_addr,
				uint8_t *const data)
{
	assert(data);
	uint8_t regdata = 0;
	bool succ = read(COMMAND(reg_addr), &regdata, 1);
	if (succ && data)
		*data = regdata;
	return succ;
}

static inline bool read_value(tsl2561_register_address_t reg_addr,
				uint16_t *const value)
{
	assert(value);
	uint8_t regdata[2];
	bool succ = read(READ_VALUE(reg_addr), regdata, 2);
	if (succ && value) {
		*value = (regdata[1] << 8) | regdata[0];
	}
	return succ;
}

static inline bool read_channel_0(uint16_t *const raw_value)
{
	assert(raw_value);
	uint16_t value = 0;
	bool succ = read_value(data_0_low, &value);
	if (succ && raw_value)
		*raw_value = value;
	return succ;
}

static inline bool read_channel_1(uint16_t *const raw_value)
{
	assert(raw_value);
	uint16_t value = 0;
	bool succ = read_value(data_1_low, &value);
	if (succ && raw_value)
		*raw_value = value;
	return succ;
}

static inline bool power_down(void)
{
	control_power_t data = control_power_down;
	return write(control, (uint8_t *) &data, 1);
}

static inline bool power_up(void)
{
	control_power_t data = control_power_up;
	return write(control, (uint8_t *) &data, 1);
}

static inline bool set_interrupt_off(void)
{
	uint8_t data = INT_CONTROL(int_off, int_thres_1);
	return write(interrupt, &data, 1);
}

static inline bool set_interrupt(int_control_t control, int_rate_t rate)
{
	uint8_t data = INT_CONTROL(control, rate);
	return write(interrupt, &data, 1);
}

static inline bool set_integration_time_default(void)
{
	uint8_t data = TIME_CONTROL_NORMAL(g16X, t402_ms);
	return write(interrupt, &data, 1);
}

static inline bool set_integration_time(gain_mode_t gain, time_mode_t time)
{
	uint8_t data = TIME_CONTROL_NORMAL(gain, time);
	return write(interrupt, &data, 1);
}

/**
 * Calculate the approximate illuminance (lux) 
 * given the raw channel values of the sensor.
 * The equation is implemented as a piece-wise linear approximation.
 *
 * The calculation depends on gain mode, integration time
 * and sensor package type.
 *
 * Default gain mode is 16X and default integration time is 402ms.
 *
 * @note Program Code is extracted from TSL2561 manual
 *       (Modification: The type of package is selected by a macro).
 * 
 * @warning The used formulas were obtained by optical testing with fluorescent
 *          and incandescent light sources, and apply only to open-air
 *          applications. Optical apertures (e.g. light pipes) will affect
 *          the incident light on the device. (see manual)
 *
 * @param ch0 raw channel value from channel 0 of sensor
 * @param ch1 raw channel value from channel 1 of sensor
 * @param iGain gain mode of sensor (1X or 16X)
 * @param tInt integration time of sensor (13.7ms, 101ms or 402ms)
 *
 * @return the approximate illuminance (lux) [1 lx = 1 lm/m^2 = 1 cd sr/m^2]
 */
static uint32_t calculateLux(const uint16_t ch0, const uint16_t ch1,
				const gain_mode_t iGain, const time_mode_t tInt)
{
	// first, scale the channel values
	// depending on the gain and integration time
	// 16X, 402mS is nominal.
	// scale if integration time is NOT 402 msec
	uint32_t chScale = 0;
	switch (tInt)
	{
	case t13_7_ms:
		// 13.7 msec
		chScale = CH_SCALE_TINT0;
		break;
	case t101_ms:
		// 101 msec
		chScale = CH_SCALE_TINT1;
		break;
	case t402_ms:
	default:
		// assume no scaling
		chScale = (1 << CH_SCALE);
		break;
	}

	// scale if gain is NOT 16X: scale 1X to 16X
	if (iGain != g16X)
		chScale = (chScale << 4);

	// scale the channel values
	uint32_t channel0 = (ch0 * chScale) >> CH_SCALE;
	uint32_t channel1 = (ch1 * chScale) >> CH_SCALE;

	// find the ratio of the channel values (Channel1/Channel0)
	// protect against divide by zero
	uint32_t ratio1 = 0;
	if (channel0 != 0) ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;

	// round the ratio value
	uint32_t ratio = (ratio1 + 1) >> 1;

	// is ratio <= eachBreak ?
	unsigned int b, m;
	if (ratio <= K1) {
		b = B1;
		m = M1;
	} else if (ratio <= K2) {
		b = B2;
		m = M2;
	} else if (ratio <= K3) {
		b = B3;
		m = M3;
	} else if (ratio <= K4) {
		b = B4;
		m = M4;
	} else if (ratio <= K5) {
		b = B5;
		m = M5;
	} else if (ratio <= K6) {
		b = B6;
		m = M6;
	} else if (ratio <= K7) {
		b = B7;
		m = M7;
	} else if (ratio > K8) {
		b = B8;
		m = M8;
	}

	uint32_t lux = ((channel0 * b) - (channel1 * m));
	// do not allow negative lux value
	if (lux < 0) lux = 0;
	// round lsb (2^(LUX_SCALE - 1))
	lux += (1 << (LUX_SCALE - 1));
	// strip off fractional portion
	lux = lux >> LUX_SCALE;
	return lux;
}

static bool getLux(uint16_t *const lux_value)
{
	assert(lux_value);
	// read channel 0
	uint16_t channel0 = 0;
	if (!read_channel_0(&channel0))
		return false;
	printf("[TSL2561] Channel 0 raw: %hu \n", channel0);

	// read channel 1
	uint16_t channel1 = 0;
	if (!read_channel_1(&channel1))
		return false;
	printf("[TSL2561] Channel 1 raw: %hu \n", channel1);

	// calculate lux
	uint32_t lux = calculateLux(channel0, channel1, g16X, t402_ms);
	assert(lux <= 0xffff);
	if (lux_value)
		*lux_value = (uint16_t) lux;
	return true;
}

/*---------------------------------------------------------------------------*/

static inline bool turn_on()
{
	// after applying Vdd, the TSL2561 device will initially be
	// in the power-down state -> power up
	if (!power_up()) {
		printf("[TSL2561]: Power up FAILED!\n");
		return false;
	}
	printf("[TSL2561]: Power up successful!\n");
	return true;
}

static inline bool turn_off()
{
	// go to power-down state
	if (!power_down()) {
		printf("[TSL2561]: Power down FAILED!\n");
		return false;
	}
	printf("[TSL2561]: Power down successful!\n");
	return true;
}

static inline bool init(void)
{
	if (!set_interrupt_off()) {
		printf("[TSL2561]: Set Interrupt off FAILED!\n");
		return false;
	}
	if (!set_integration_time_default()) {
		printf("[TSL2561]: Set Integration Time+Gain Default FAILED!\n");
		return false;
	}
	return true;
}

/*---------------------------------------------------------------------------*/

static int configure(int sensors_type, int value)
{
	switch (sensors_type) {
	case SENSORS_ACTIVE:
		if (value && !status(SENSORS_ACTIVE)) {
			if (!turn_on()) {
				state = INIT_ERROR;
				return ERROR;
			}
			// no init error
			state = ON;
			if (!init())
				return ERROR;
			printf("[TSL2561]: Initialize successful!\n");
		} else {
			if (!turn_off()) {
				return ERROR;
			}
			state = OFF;
		}
		return NO_ERROR;
	}
	return ERROR;
}

static int status(int sensors_type)
{
	switch (sensors_type) {
	case SENSORS_ACTIVE: // is the sensor active
	case SENSORS_READY:  // or ready ?
		return (state == ON);
	}
	return 0; // false, unknown sensor_type
}

static int value(int measurement_type)
{
	static uint16_t lux = 0;
	bool succ = false;
	// read temp or pressure over i2c/i2c interface
	// write the value into a buffer
	switch (measurement_type) {
	case TSL2561_SENSOR_LUX:
		// read current temperature
		succ = getLux(&lux);
		if (!succ) break;
		// save value
		printf("[TSL2561]: lux = %hu [lx]\n", lux);
		i2c_dev_wait_ms(200);
		break;
	default:
		break;
	}
	return ERROR; // false, error
}

