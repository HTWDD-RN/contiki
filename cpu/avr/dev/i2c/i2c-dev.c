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

// header is in core
#include "dev/i2c/i2c-dev.h"

#include <stdlib.h>
#include <stdio.h>

// Arch Dependent includes
#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#include "lib/assert.h"

#include "contiki.h"


// Debugging
//#define I2C_DEBUG
#ifdef  I2C_DEBUG
#warning "I2C Dev Debugging is activated!"
#endif

#ifndef I2C_MAX_BUFFER_SIZE
#define I2C_MAX_BUFFER_SIZE     20
#endif

#if (I2C_MAX_BUFFER_SIZE <= 1)
#error I2C_MAX_BUFFER_SIZE is too small.
#endif


typedef struct i2c_dev_frame {
	// i2c config
	i2c_dev_frame_types_e frame_type;
	i2c_dev_result_t frame_result;
	uint8_t max_retries;
	bool no_stop;
	// i2c data
	uint8_t dev_addr_rw;
	uint8_t data_size;
	uint8_t data[I2C_MAX_BUFFER_SIZE];
} i2c_dev_frame_t;

static i2c_dev_frame_t                i2c_current;

// check if init function was called
static bool is_inited = false;

static bool is_interrupt_on = false;

//====================================================================
// Private Function Declarations
//====================================================================


/**
 * Sends a START condition to enter a Master mode.
 * The format of the following address packet determines
 * wether Master Transmitter (MT) 
 * or Master Receiver (MR) mode is to be entered.
 * If SLA+W is transmitted, MT mode is entered,
 * if SLA+R is transmitted, MR mode is entered.
 * @ingroup twiBus
 * @note Low-level-function
 */
static void i2c_signal_start(void);

/**
 * Sends a REAPEATED START condition.
 * The 2-wire Serial Interface can access the same Slave again,
 * or a new Slave without transmitting a STOP condition.
 * Repeated START enables the Master to switch between
 * Slaves, Master Transmitter mode and Master Receiver mode
 * without losing control of the bus.
 * @ingroup twiBus
 * @note Low-level-function
 */
static void i2c_signal_restart(void);

/**
 * Send an TWI stop condition in Master mode
 * @ingroup twiBus
 * @note Low-level-function
 */
static void i2c_signal_stop(void);

/**
 * Send no TWI stop condition in Master mode
 * @ingroup twiBus
 * @note Low-level-function
 */
static void i2c_signal_no_stop(void);

/**
 * Send byte over TWI
 * @ingroup twiBus
 * @note Low-level-function
 * @param data (address|R/W) combination or a data byte
 */
static void i2c_send_byte(uint8_t const data);

/**
 * Grabs the next byte from the I2C device.
 * After receiving the next byte the I2C device sends an ACK or a NACK signal.
 * An ACK signal indicates that this was the last byte.
 * 
 * @ingroup twiBus
 * @note Low-level-function
 * @param returnACK true if recevied data should be ACK'ed or
 *                  false if recevied data should be NACK'ed.
 * @see i2c_retrieve_byte()
 */
static void i2c_grab_byte(bool sendACK);

/**
 * Returns the just grabbed byte.
 *
 * @ingroup twiBus
 * @note Low-level-function
 * @return the received byte
 * @see i2c_grab_byte()
 */
static uint8_t i2c_retrieve_byte(void);


//====================================================================
// Private Functions
//====================================================================

static void i2c_signal_start(void)
{
	#ifdef I2C_DEBUG
	printf("next: -(START)->\n");
	#endif

	i2c_current.frame_result = TRANSMISSION_PENDING;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		is_interrupt_on = false;
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA) | _BV(TWIE); // TWSTO = 0
	}
}

static void i2c_signal_restart(void)
{
	#ifdef I2C_DEBUG
	printf("next: -(REPEATED START)->\n");
	#endif

	i2c_current.frame_result = TRANSMISSION_PENDING;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		is_interrupt_on = false;
		// Master Transmitter Mode:
		TWCR = _BV(TWEA) | _BV(TWWC) | _BV(TWIE); // TWEN = TWSTA = 0
		// Master Receiver Mode:
		//TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE); // TWSTO = 0
	}
}

static void i2c_signal_stop(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); // TWSTA = 0
	}

	#ifdef I2C_DEBUG
	printf("-(STOP)->\n");
	#endif
}

static void i2c_signal_no_stop(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		TWCR = _BV(TWINT) | _BV(TWEN); // TWSTA = 0
	}

	#ifdef I2C_DEBUG
	printf("-(NO STOP)->\n");
	#endif
}

static void i2c_send_byte(uint8_t const data_byte)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// save data byte to the TWDR
		TWDR = data_byte;
		// start transmission of data
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE); // TWSTA = TWST0 = 0
	}
}

static void i2c_grab_byte(bool isLastByte_sendNACK)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// begin receive over twi
		if (isLastByte_sendNACK) {
			// NACK the recevied data
			TWCR = _BV(TWINT) | _BV(TWEN) | 
					_BV(TWIE); // TWSTA = TWSTO = TWEA = 0
		} else {
			// ACK the recevied data
			TWCR = _BV(TWINT) | _BV(TWEN) | 
					_BV(TWEA) | _BV(TWIE); // TWSTA = TWSTO = 0
		}
	}
}

static uint8_t i2c_retrieve_byte(void)
{
	uint8_t dr = 0;
	// retieve received data byte from twi TWDR
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		dr = TWDR;
	}
	return dr;
}

// F_SCL = 100 kHz
#ifndef F_SCL
#warning "I2C bus speed is not set! Use default: 100 kHz."
#define F_SCL          100000UL
#endif

// Calculate TWBR value
#define TWBR_VALUE    ( ( (F_CPU / F_SCL) - 16 )/2 )
#if TWBR_VALUE < 10
#error "I2C SCL frequency is too high or CPU frequency is too low!"
#endif

static inline void set_bitrate(void)
{
	// set twi bitrate
	// F_SCL = F_CPU / (16 + 2*TWBR * 4^TWPS)
	// typ.: TWPS = 0, F_SCL <= 0.4 MHz
	TWSR &= ~( _BV(TWPS0) | _BV(TWPS1) ); // TWSR(1:0) = 0b00

	#ifdef I2C_DEBUG
	printf("[I2C-Dev] set TWBR to %lu\n", TWBR_VALUE);
	#endif
	TWBR = TWBR_VALUE;
	TWBR = TWBR_VALUE;
}

static inline void i2c_signal_bus_error(void)
{
	// TWSTA = TWIE = 0, TWSTO = TWINT = 1
	TWCR = _BV(TWSTO) | _BV(TWINT);
}

//===================================================================
// Interrupt Routines
//===================================================================

/** 
 * I2C / TWI Interrupt Routine.
 * Used to check the TWI status register.
 *
 * @ingroup twiBus
 * @note Low-level-function
 */
ISR(TWI_vect)
{
	is_interrupt_on = true;

	// some static 
	static i2c_dev_result_t i2c_result;
	static uint8_t i2c_reg_content_counter = 0;
	static uint8_t i2c_retry;

	// save I2C status
	uint8_t i2c_status;
	i2c_status = TW_STATUS;
	#ifdef I2C_DEBUG
	printf("~~ [I2C-Interrupt] I2C Status: %#02x, Reg-Counter: %d / %d\n",
					i2c_status, i2c_reg_content_counter, i2c_current.data_size);
	#endif

	/// Check Status Register
	switch (i2c_status) {
	case TW_BUS_ERROR:
		// curret: ERROR..
		fprintf(stderr, "Bus error due to illegal START or STOP condition!\n");
		i2c_result = FAILED_BUS_ERROR;
		i2c_retry = 0;
		i2c_reg_content_counter = 0;
		i2c_signal_bus_error();
		break;
	case TW_START:
		// curret: a start condition was successfully send.
		#ifdef I2C_DEBUG
		printf("-(START)->");
		#endif
		// reset message counter
		i2c_result = TRANSMISSION_PENDING;
		// next: transfer address and R/W bit
		#ifdef I2C_DEBUG
		printf(" | next: -(SLA+R/W: %#02x)->\n", i2c_current.dev_addr_rw);
		#endif
		i2c_send_byte(i2c_current.dev_addr_rw);
		break;
	case TW_REP_START:
		// curret: a repeated start condition was successfully send.
		#ifdef I2C_DEBUG
		printf("-(REPEATED START)->");
		#endif
		// reset message counter
		i2c_result = TRANSMISSION_PENDING;
		// next: transfer address and R/W bit
		#ifdef I2C_DEBUG
		printf(" | next: -(SLA+R/W: %#02x)->\n", i2c_current.dev_addr_rw);
		#endif
		i2c_send_byte(i2c_current.dev_addr_rw);
		break;
	case TW_MT_SLA_ACK:
		#ifdef I2C_DEBUG
		printf("-(SLA+W)-> | <-(ACK)-");
		#endif
		i2c_retry = 0;
		i2c_reg_content_counter = 0;
		// current: slave address and write bit successfully transfered to slave
		// next: transfer message to slave
		assert(i2c_current.data_size > 0);
		#ifdef I2C_DEBUG
		printf(" | next: -(Data 1/%u: %#02x)->\n",
				i2c_current.data_size,
				i2c_current.data[0]);
		#endif
		i2c_send_byte(i2c_current.data[0]);
		break;
	case TW_MT_SLA_NACK:
		printf("-(SLA+W)-> | <-(NOT ACK)- | next: -(ABORT/STOP: Failed address)->\n");
		// ERROR! Address does not recognized by any slave device.
		i2c_result = FAILED_ADDR;
		i2c_signal_stop();
		break;
	case TW_MT_DATA_ACK:
		#ifdef I2C_DEBUG
		printf("-(Data)-> | <-(ACK)-");
		#endif
		// current: data byte successfully transfered to slave
		// next: transfer next data byte to slave or send stop/restart
		if (++i2c_reg_content_counter < i2c_current.data_size)
		{
			// send next data byte
			#ifdef I2C_DEBUG
			printf(" | next: -(Data %u/%u: %#02x)->\n",
					i2c_reg_content_counter+1, i2c_current.data_size,
					i2c_current.data[i2c_reg_content_counter]);
			#endif
			i2c_send_byte(i2c_current.data[i2c_reg_content_counter]);
		}
		else
		{
			// info: message transfered
			#ifdef I2C_DEBUG
			printf(" | next: -(STOP / NO STOP)->\n");
			#endif
			// send stop ?
			i2c_result = SUCCESSFUL;
			if (i2c_current.no_stop)
				i2c_signal_no_stop();
			else
				i2c_signal_stop();
		}
		break;
	case TW_MT_DATA_NACK:
		printf("-(Data)-> | <-(NOT ACK)-");
		// ERROR ?
		if (i2c_result == FAILED_ADDR)
		{
			printf(" | next: -(ABORT/STOP: Failed address)->\n");
			i2c_signal_stop();
			break;
		}

		// retry to transmit data
		if (++i2c_retry > i2c_current.max_retries)
		{
			// maximum retries reached
			i2c_result = FAILED_DATA;
			printf(" | next: -(ABORT/STOP: Too many retries)->\n");
			i2c_signal_stop();
		}
		else
		{
			// try again
			printf(" | next: -(Data %u/%u: %#02x, retry %u/%u)->\n",
					i2c_reg_content_counter+1, i2c_current.data_size,
					i2c_current.data[i2c_reg_content_counter],
					i2c_retry, i2c_current.max_retries);
			i2c_send_byte(i2c_current.data[i2c_reg_content_counter]);
		}
		break;
	case TW_MR_ARB_LOST: // == TW_MT_ARB_LOST:
		printf("Arbitration lost in SLA+W or Data || Arbitration lost in SLA+R or NOT ACK bit\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_MR_SLA_ACK:
		#ifdef I2C_DEBUG
		printf("-(SLA+R)-> | <-(ACK)-");
		#endif
		// current: slave address and read bit successfully transfered to slave
		i2c_retry = 0;
		i2c_reg_content_counter = 0;
		// next: receive first byte -- cond=false => Send NACKM (last byte)
		#ifdef I2C_DEBUG
		printf(" | next: <-(Data 1/%u%s)-\n",
				i2c_current.data_size,
				(1 >= i2c_current.data_size)?", last":"");
		#endif
		assert(i2c_current.data_size >= 1);
		i2c_grab_byte(1 >= i2c_current.data_size);
		break;
	case TW_MR_SLA_NACK:
		printf("-(SLA+R)-> | <-(NOT ACK)- | next: -(ABORT/STOP: Failed address)->\n");
		// ERROR! Address does not recognized by any slave device.
		i2c_result = FAILED_ADDR;
		i2c_signal_stop();
		break;
	case TW_MR_DATA_ACK:
		// current: data byte successfully received from slave
		assert(i2c_reg_content_counter < i2c_current.data_size - 1);
		i2c_current.data[i2c_reg_content_counter] = i2c_retrieve_byte();
		#ifdef I2C_DEBUG
		printf("<-(Data %u/%u: %#02x)- | -(ACK)->",
				i2c_reg_content_counter+1, i2c_current.data_size,
				i2c_current.data[i2c_reg_content_counter]);
		#endif
		// next: receive next byte -- cond=false => Send NACKM (last byte)
		i2c_reg_content_counter++;
		#ifdef I2C_DEBUG
		printf(" | next: <-(Data %u/%u%s)-\n",
				i2c_reg_content_counter+1, i2c_current.data_size,
				(i2c_reg_content_counter+1 >= i2c_current.data_size)?
						", last":"");
		#endif
		assert(i2c_reg_content_counter < i2c_current.data_size);
		i2c_grab_byte(i2c_reg_content_counter + 1 >= i2c_current.data_size);
		break;
	case TW_MR_DATA_NACK:
		// current: last data byte successfully received from slave
		assert(i2c_reg_content_counter == i2c_current.data_size - 1);
		i2c_current.data[i2c_reg_content_counter] = i2c_retrieve_byte();
		#ifdef I2C_DEBUG
		printf("<-(Data %u/%u, last: %#02x)- | -(NOT ACK)-> | next: -(STOP / NO STOP)->\n",
				i2c_reg_content_counter+1, i2c_current.data_size,
				i2c_current.data[i2c_reg_content_counter]);
		#endif
		// next: send restart or stop
		i2c_result = SUCCESSFUL;
		if (i2c_current.no_stop)
			i2c_signal_no_stop();
		else
			i2c_signal_stop();
		break;
	case TW_SR_SLA_ACK:
		printf("<-(Own SLA+W)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_ARB_LOST_SLA_ACK:
		printf("Arbitration lost in SLA+R/W as Master | <-(Own SLA+W)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_GCALL_ACK:
		printf("<-(General Call addr)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_ARB_LOST_GCALL_ACK:
		printf("Arbitration lost in SLA+R/W as Master | <-(General Call addr)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_DATA_ACK:
		printf("Previously addressed with own SLA+W | <-(Data)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_DATA_NACK:
		printf("Previously addressed with own SLA+W | <-(Data)- | -(NOT ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_GCALL_DATA_ACK:
		printf("Previously addressed with general call | <-(Data)- | -(ACK)->");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_GCALL_DATA_NACK:
		printf("Previously addressed with general call | <-(Data)- | -(NOT ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_SR_STOP:
		printf("<-(STOP|REPEATED START)- as Slave\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_ST_SLA_ACK:
		printf("<-(Own SLA+R)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_ST_ARB_LOST_SLA_ACK:
		printf("Arbitration lost in SLA-R/W as Master | <-(Own SLA+R)- | -(ACK)->\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_ST_DATA_ACK:
		printf("-(Data in TWDR)-> | <-(ACK)-\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_ST_DATA_NACK:
		printf("-(Data in TWDR)-> | <-(NOT ACK)-\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_ST_LAST_DATA:
		printf("-(Last Data in TWDR)-> (TWEA=0) | <-(ACK)-\n");
		i2c_result = FAILED_NOT_SUPPORTED;
		i2c_signal_stop();
		break;
	case TW_NO_INFO:
		printf("No relevant state Information available, TWINT=0\n");
		i2c_result = FAILED_unknown;
		i2c_signal_stop();
		break;
	default:
		printf("unknown value!\n");
		break;
	}

	if (i2c_result != TRANSMISSION_PENDING)
	{
		#ifdef I2C_DEBUG
		printf("I2C Transmission completed. :) Result-No: %u\n", i2c_result);
		#endif
		i2c_reg_content_counter = 0;
		i2c_retry = 0;
		i2c_current.frame_result = i2c_result;
		if (i2c_result != SUCCESSFUL)
			i2c_dev_wait_ms(1000);
	}
}

//===================================================================
// Public Functions
//===================================================================

void i2c_dev_init(void)
{
	// tr.last_status = PENDING;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// set twi bit rate (depend on schematic)
		set_bitrate();
		// enable TWI
		// 1=TWEN; 0=TWIE=TWINT=TWEA=TWSTA=TWSTO=TWWC
		TWCR = _BV(TWEN);

		// activate interrupts globally
		//sei();
	}
	is_inited = true;

	printf(".. [I2C-Dev] i2c device inited.\n");
}

bool i2c_dev_bus_is_busy(void)
{
	bool is_set = false;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		is_set = bit_is_set(TWCR, TWIE);
	}
	return is_set;
}

bool i2c_dev_wait_until_bus_is_free(void)
{
	assert(is_inited);
	volatile uint32_t i = 0;
	while (i2c_dev_bus_is_busy())
	{
		i = (i+1) % 0xffff;
		if (i == 0 && !is_interrupt_on)
		{
			fprintf(stderr, "!! [I2C-Dev] Interrupt failed! -> ABORT\n");
			return false;
		}
	}
	return true;
}

void i2c_dev_wait_ms(uint16_t ms)
{
	for (; ms > 0; ms--)
		_delay_ms(1);
}

void i2c_dev_wait_us(uint16_t us)
{
	for (; us > 0; us--)
		_delay_us(1);
}

i2c_dev_result_t i2c_dev_send_cmd(const uint8_t device_address_rw,
				const uint8_t register_address_command,
				bool is_repeated_start, bool no_stop)
{
	if (!is_inited) i2c_dev_init();
	assert(is_inited);

	// set frame
	i2c_current.frame_type = SEND_CMD;
	i2c_current.max_retries = 3;
	i2c_current.no_stop = no_stop;

	// set i2c data
	assert(1 <= I2C_MAX_BUFFER_SIZE);
	i2c_current.dev_addr_rw = device_address_rw & ~0;
	i2c_current.data_size = 1;
	i2c_current.data[0] = register_address_command;

	// start transmission
	(!is_repeated_start) ? i2c_signal_start() : i2c_signal_restart();

	// wait until transaction is completed
	if (!i2c_dev_wait_until_bus_is_free()) return FAILED_INTERRUPT;

	assert(i2c_current.frame_result != TRANSMISSION_PENDING);
	return i2c_current.frame_result;
}

i2c_dev_result_t i2c_dev_write_data(const uint8_t device_address_rw,
				const uint8_t register_address,
				const uint8_t *const register_content_data,
				const size_t register_size,
				bool is_repeated_start, bool no_stop)
{
	if (!is_inited) i2c_dev_init();
	assert(is_inited);

	// set i2c config
	i2c_current.frame_type = WRITE_DATA;
	i2c_current.max_retries = 3;
	i2c_current.no_stop = no_stop;

	// set i2c data
	assert(register_size+1 <= I2C_MAX_BUFFER_SIZE);
	assert(register_size >= 1);
	i2c_current.dev_addr_rw = device_address_rw & ~0;
	i2c_current.data_size = register_size+1;
	i2c_current.data[0] = register_address;
	for (size_t i = 0; i < register_size; i++)
	{
		i2c_current.data[i+1] = register_content_data[i];
	}

	// start transmission
	(!is_repeated_start) ? i2c_signal_start() : i2c_signal_restart();

	// wait until transaction is completed
	if (!i2c_dev_wait_until_bus_is_free()) return FAILED_INTERRUPT;

	assert(i2c_current.frame_result != TRANSMISSION_PENDING);
	return i2c_current.frame_result;
}

i2c_dev_result_t i2c_dev_receive_data(const uint8_t device_address_rw,
				uint8_t *const register_content_data,
				const size_t register_size,
				bool is_repeated_start, bool no_stop)
{
	if (!is_inited) i2c_dev_init();
	assert(is_inited);

	// set frame
	i2c_current.frame_type = RECEIVE_DATA;
	i2c_current.max_retries = 3;
	i2c_current.no_stop = no_stop;

	// set i2c data
	assert(register_size+1 <= I2C_MAX_BUFFER_SIZE);
	assert(register_size >= 1);
	i2c_current.dev_addr_rw = device_address_rw | 1;
	i2c_current.data_size = register_size;

	// start transmission
	// (!is_repeated_start) ? i2c_signal_start() : i2c_signal_restart();
	i2c_signal_start();

	// wait until transaction is completed
	if (!i2c_dev_wait_until_bus_is_free()) return FAILED_INTERRUPT;

	for (size_t i = 0; i < register_size; i++)
	{
		register_content_data[i] = i2c_current.data[i];
	}

	assert(i2c_current.frame_result != TRANSMISSION_PENDING);
	return i2c_current.frame_result;
}

i2c_dev_result_t i2c_dev_read_data(const uint8_t device_address_rw,
				const uint8_t register_address,
				uint8_t *const register_content_data,
				const size_t register_size,
				bool is_repeated_start, bool no_stop)
{
	if (!is_inited) i2c_dev_init();
	assert(is_inited);

	i2c_dev_result_t succ = TRANSMISSION_PENDING;
	
	#ifdef I2C_DEBUG
	printf("Initialize I2C Read Data..\n");
	printf("I2C Read Data: 1. Write Command:\n");
	#endif

	succ = i2c_dev_send_cmd(device_address_rw,
				register_address,
				is_repeated_start, true);
	if (succ != SUCCESSFUL) return succ;

	// wait until transaction is completed
	if (!i2c_dev_wait_until_bus_is_free()) return FAILED_INTERRUPT;

	// give the i2c device some time
	i2c_dev_wait_ms(10);

	#ifdef I2C_DEBUG
	printf("I2C Read Data: 2. Receive Data:\n");
	#endif

	succ = i2c_dev_receive_data(device_address_rw,
				register_content_data,
				register_size,
				true, no_stop);
	assert(succ != TRANSMISSION_PENDING);
	return succ;
}

