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


#include "sensor-control.h"

// Debugging
//#define SENSOR_CONTROL_DEBUG
#ifdef SENSOR_CONTROL_DEBUG
	#warning "Sensor Control Process: Debug is activated!"
#endif

// SENSORS interface
#include "lib/sensors.h"

// Sensors
#include "dev/i2c/i2c-dev.h"
#include "dev/i2c/bmp085-sensor.h"
#include "dev/i2c/sht21-sensor.h"
#include "dev/i2c/bma180-sensor.h"
#include "dev/i2c/tsl2561-sensor.h"

#include <stdlib.h>
#include <stdio.h>

static const char *const pIdent = "[sensor_control] ";

PROCESS(sensor_control_process, "Sensor control process");


/// Sensors on the platform
//SENSORS(&pir_sensor, &vib_sensor, &button_sensor);
SENSORS(&bmp085_sensor, &sht21_sensor, &bma180_sensor, &tsl2561_sensor);

static inline bool sensor_init_tsl2561(void)
{
	if (!SENSORS_ACTIVATE(tsl2561_sensor)) {
		fprintf(stderr, "!! %s", pIdent);
		fprintf(stderr, "Error: TSL2561 cannot be activated!\n");
		return false;
	}
	printf("__ %s", pIdent);
	printf("TSL2561 was initialized.\n");
	return true;
}

static inline bool sensor_init_sht21(void)
{
	if (!SENSORS_ACTIVATE(sht21_sensor)) {
		fprintf(stderr, "!! %s", pIdent);
		fprintf(stderr, "Error: SHT21 cannot be activated!\n");
		return false;
	}
	printf("__ %s", pIdent);
	printf("SHT21 was initialized.\n");
	return true;
}

static inline bool sensor_init_bmp085(void)
{
	if (!SENSORS_ACTIVATE(bmp085_sensor)) {
		fprintf(stderr, "!! %s", pIdent);
		fprintf(stderr, "Error: BMP085 cannot be activated!\n");
		return false;
	}
	printf("__ %s", pIdent);
	printf("BMP085 was initialized.\n");
	return true;
}

static inline bool sensor_init_bma180(void)
{
	if (!SENSORS_ACTIVATE(bma180_sensor)) {
		fprintf(stderr, "!! %s", pIdent);
		fprintf(stderr, "Error: BMA180 cannot be activated!\n");
		return false;
	}
	printf("__ %s", pIdent);
	printf("BMA180 was initialized.\n");
	return true;
}

static bool is_init_tsl2561 = false;
static bool is_init_sht21   = false;
static bool is_init_bmp085  = false;
static bool is_init_bma180  = false;

static void sensor_init(void)
{
	#ifdef SENSOR_CONTROL_DEBUG
	printf("__ %s", pIdent);
	printf("initialize i2c device..\n");
	#endif

	i2c_dev_init();

	#ifdef SENSOR_CONTROL_DEBUG
	printf("__ %s", pIdent);
	printf("initialize sensors..\n");
	#endif

	// is_init_tsl2561 = sensor_init_tsl2561();
	is_init_sht21   = sensor_init_sht21();
	is_init_bmp085  = sensor_init_bmp085();
	// is_init_bma180  = sensor_init_bma180();
}

static void sensor_readings(void)
{
	#ifdef SENSOR_CONTROL_DEBUG
	printf("__ %s", pIdent);
	printf("Read sensors..\n");
	#endif

	if (is_init_tsl2561) {
		#ifdef SENSOR_CONTROL_DEBUG
		printf("__ %s", pIdent);
		printf("Read TSL2561 sensor..\n");
		#endif
		tsl2561_sensor.value(TSL2561_SENSOR_LUX);
	}

	if (is_init_sht21) {
		#ifdef SENSOR_CONTROL_DEBUG
		printf("__ %s", pIdent);
		printf("Read SHT21 sensor..\n");
		#endif
		sht21_sensor.value(SHT21_SENSOR_TEMP);
		sht21_sensor.value(SHT21_SENSOR_RELATIVE_HUMIDITY);
		// sht21_sensor.value(SHT21_SENSOR_RELATIVE_HUMIDITY_ICE);
	}

	if (is_init_bmp085) {
		#ifdef SENSOR_CONTROL_DEBUG
		printf("__ %s", pIdent);
		printf("Read BMP085 sensor..\n");
		#endif
		bmp085_sensor.value(BMP085_SENSOR_TEMP);
		bmp085_sensor.value(BMP085_SENSOR_PRESSURE_AND_TEMPERATURE);
	}

	if (is_init_bma180) {
		#ifdef SENSOR_CONTROL_DEBUG
		printf("__ %s", pIdent);
		printf("Read BMA180 sensor..\n");
		#endif
		bma180_sensor.value(BMA180_SENSOR_ACC_X);
		bma180_sensor.value(BMA180_SENSOR_ACC_Y);
		bma180_sensor.value(BMA180_SENSOR_ACC_Z);
	}
}

static void pollhandler()
{
	printf("PH %s", pIdent);
	printf("is polled..\n");
}

static void exithandler()
{
	printf("XH %s", pIdent);
	printf("is exiting..\n");
}


PROCESS_THREAD(sensor_control_process, ev, data)
{
	PROCESS_POLLHANDLER(pollhandler())
	PROCESS_EXITHANDLER(exithandler())
	
	PROCESS_BEGIN();

	// init sensors
	sensor_init();

	while (1)
	{
		sensor_readings();

		// pause
		PROCESS_PAUSE();
	}
	PROCESS_END();
}

