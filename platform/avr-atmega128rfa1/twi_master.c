/**
 * @file twi_master.c
 *
 * @brief Implementation of a TWI (Two Wire Interface) master.
 *
 * This provides functions to access any TWI device, when the MCU should
 * be acting as master and the device as slave.
 *
 * $Id: twi_master.c,v 1.3 2012/11/14 12:39:05 ele Exp $
 *
 */
/*
 * @author    dresden elektronik ingenieurtechnik gmbh: http://www.dresden-elektronik.de
 * @author    Support email: wireless@dresden-elektronik.de
 *
 * Copyright (c) 2011, dresden elektronik ingenieurtechnik gmbh. All rights reserved.
 *
 * Licensed under dresden elektronik's Limited License Agreement --> deEULA.txt
 */

/* === INCLUDES ============================================================ */

#include <stdint.h>
#include <avr/io.h>

#include "twi_master.h"

/* === TYPES =============================================================== */

/* === MACROS / DEFINES ==================================================== */

/* === PROTOTYPES ========================================================== */

/* === GLOBALS ============================================================= */

/* === IMPLEMENTATION ====================================================== */

/**
 * @brief Initialize TWI Master (Set TWI speed)
 *
 * @param   twi_bitrate bit rate from TWI Master interface in Hz
 * @return  1 if bitrate is too high, else 0
 */
uint8_t twi_init(uint32_t twi_bitrate)
{
   // reset TWI register set, this is necessary for proper initialization
   // after wakeup from sleep mode
   TWCR = TWSR = TWDR = TWAR = TWAMR = 0;

   TWBR = (((F_CPU / twi_bitrate) - 16) / 2);
   if (TWBR < 11) return 1;

   return 0;
}

/**
 * @brief start the TWI interface by enable connection to target device
 *
 * @param   address  TWI address of device
 * @param   mode     TWI mode (READ or WRITE)
 *
 * @return  0 if TWI connection is established, 1 if error occured
 */
uint8_t twi_start(uint8_t address, uint8_t mode)
{
   uint8_t     status;

   /* Send START condition */
   TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

   /* Wait for TWINT Flag set. This indicates that the START condition has been transmitted. */
   while (!(TWCR & (1 << TWINT)));

   /* Check value of TWI Status Register. Mask prescaler bits. */
   status = TWSR & 0xF8;
   if ((status != TWI_START) && (status != TWI_REP_START)) return 1;

   /* Send device address */
   TWDR = (address << 1) + mode;
   TWCR = (1 << TWINT) | (1 << TWEN);

   /* Wait until transmission completed and ACK/NACK has been received */
   while (!(TWCR & (1 << TWINT)));

   /* Check value of TWI Status Register. Mask prescaler bits. */
   status = TWSR & 0xF8;
   if ((status != TWI_MT_SLA_ACK) && (status != TWI_MR_SLA_ACK)) return 1;

   return 0;
}

/**
 * @brief Stop TWI interface connection.
 */
void twi_stop(void)
{
   /* Send stop condition */
   TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

   /* Wait until stop condition is executed and bus released */
   while (TWCR & (1 << TWINT));
}

/**
 * @brief Write a byte to the slave.
 *
 * @param   byte  Byte that should be written
 *
 * @return        0 if transmission was successful and 1 if any error occured
 */
uint8_t twi_write(uint8_t byte)
{
   uint8_t   twst;

   /* send data to the previously addressed device */
   TWDR = byte;
   TWCR = (1 << TWINT) | (1 << TWEN);

   /* Wait until transmission completed */
   while (!(TWCR & (1 << TWINT)));

   /* Check value of TWI Status Register. Mask prescaler bits */
   twst = TWSR & 0xF8;
   if (twst != TWI_MT_DATA_ACK) return 1;

   return 0;
}

/**
 * @brief Read a byte from the slave and request next byte.
 *
 * @return  byte read from slave
 */
uint8_t twi_readAck(void)
{
   TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

   /* Wait until transmission completed */
   while (!(TWCR & (1 << TWINT)));

   return TWDR;
}

/**
 * @brief Read the last byte of the slave device.
 *
 * @return  byte read from slave
 */
uint8_t twi_readNack(void)
{
   TWCR = (1 << TWINT) | (1 << TWEN);

   /* Wait until transmission completed */
   while(!(TWCR & (1 << TWINT)));

   return TWDR;
}

/* EOF */
