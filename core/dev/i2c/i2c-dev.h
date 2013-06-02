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


#ifndef I2C_DEV_H_
#define I2C_DEV_H_

#include <stdbool.h>
#include <stdlib.h>

#include "contiki.h"

/// 
typedef enum i2c_result_t {
	SUCCESSFUL = 0,
	TRANSMISSION_PENDING,
	FAILED_unknown,
	FAILED_ALLOC,
	FAILED_TOO_MUCH_DATA,
	FAILED_BUS_ERROR,
	FAILED_ADDR,
	FAILED_DATA,
	FAILED_NOT_SUPPORTED,
	FAILED_INTERRUPT
} i2c_dev_result_t;

typedef enum {
	SEND_CMD,
	WRITE_DATA,
	RECEIVE_DATA,
} i2c_dev_frame_types_e;

/**
 * Initialize the I2C bus device.
 * @ingroup i2c_bus
 */
void i2c_dev_init(void);

/**
 * Checks if the I2C bus is busy.
 * Use this function to poll the I2C bus.
 * The function returns the state of the interrupt flag (TWIE).
 * @warning Don't use this function in an endless loop without break condition!
 *          Instead, use the function i2c_dev_wait_until_bus_is_free()
 * @ingroup i2c_bus
 *
 * @return true if the bus is busy else false.
 */
bool i2c_dev_bus_is_busy(void);

/**
 * Waits until the I2C bus is free or a timeout was reached (busy wait!).
 * @return true, if the bus is free,
 *         else false to indicate that the I2C interrupt was not started.
 * @ingroup i2c_bus
 */
bool i2c_dev_wait_until_bus_is_free(void);

/**
 * Waits @p ms milliseconds.
 *
 * @param ms number of milliseconds.
 */
void i2c_dev_wait_ms(uint16_t ms);

/**
 * Waits @p us microseconds.
 *
 * @param us number of microseconds.
 */
void i2c_dev_wait_us(uint16_t us);

/**
 * Starts a new transmission
 */
i2c_dev_result_t i2c_dev_send_cmd(const uint8_t device_address_rw,
				const uint8_t register_address_command,
				bool is_repeated_start, bool no_stop);

i2c_dev_result_t i2c_dev_write_data(const uint8_t device_address_rw,
				const uint8_t register_address,
				const uint8_t *const register_content_data,
				const size_t register_size,
				bool is_repeated_start, bool no_stop);

i2c_dev_result_t i2c_dev_read_data(const uint8_t device_address_rw,
				const uint8_t register_address,
				uint8_t *const register_content_data,
				const size_t register_size,
				bool is_repeated_start, bool no_stop);


#endif /* I2C_DEV_H_ */
