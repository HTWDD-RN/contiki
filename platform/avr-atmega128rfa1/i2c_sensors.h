/**
 * @file sensors.h
 *
 * @brief deRFnode/gateway device specfic sensor definitions
 *
 * This file contains command definitions and register bitmasks for
 * sensors available on deRFnode/gateway devices.
 *
 * $Id: sensors.h,v 1.2 2012/11/14 12:39:05 ele Exp $
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

/* prevent double inclusion */
#ifndef _SENSORS_H
#define _SENSORS_H

/* === INCLUDES ============================================================ */

/* === MACROS / DEFINES ==================================================== */

// ============================================================================
// sensor base addresses (not differing between read/write mode), one bit shifted right
// ============================================================================
#define DS620_ADDRESS                 (0x48)   // 0x90
#define ISL29020_ADDRESS              (0x44)   // 0x88
#define BMA150_ADDRESS                (0x38)   // 0x70
#define TMP102_ADDRESS                (0x48)   // 0x90

// ============================================================================
// register map
// ============================================================================
#define DS620_ADDR_TEMP_MSB           (0xAA)   // temperature register (MSB)
#define DS620_ADDR_TEMP_LSB           (0xAB)   // temperature register (LSB)
#define DS620_ADDR_CONFIG_MSB         (0xAC)   // configuration register (MSB)
#define DS620_ADDR_CONFIG_LSB         (0xAD)   // configuration register (LSB)

#define TMP102_ADDR_TEMP              (0x00)   // Pointer register pointing to temperature register
#define TMP102_ADDR_CFG               (0x01)   // Pointer register pointing to configuration register
#define TMP102_ADDR_TLO               (0x02)   // Pointer register pointing to T_Low limit register
#define TMP102_ADDR_THI               (0x03)   // Pointer register pointing to T_High limit register

#define ISL29020_ADDR_COMMAND         (0x00)   // command register
#define ISL29020_ADDR_DATA_LSB        (0x01)   // data register (LSB)
#define ISL29020_ADDR_DATA_MSB        (0x02)   // data register (MSB)

#define BMA150_ADDR_CHIP_ID           (0x00)   // Chip ID Register
#define BMA150_ADDR_VERSION           (0x01)   // Version Register
#define BMA150_ADDR_ACC_X_LSB         (0x02)   // Acceleration X register (LSB part)
#define BMA150_ADDR_ACC_X_MSB         (0x03)   // Acceleration X register (MSB part)
#define BMA150_ADDR_ACC_Y_LSB         (0x04)   // Acceleration Y register (LSB part)
#define BMA150_ADDR_ACC_Y_MSB         (0x05)   // Acceleration Y register (MSB part)
#define BMA150_ADDR_ACC_Z_LSB         (0x06)   // Acceleration Z register (LSB part)
#define BMA150_ADDR_ACC_Z_MSB         (0x07)   // Acceleration Z register (MSB part)
#define BMA150_ADDR_TEMP              (0x08)   // Temperature register
#define BMA150_ADDR_ANY_MOTION_THRES  (0x10)   // Anymotion threshold register
#define BMA150_ADDR_ANY_MOTION_DUR    (0x11)   // Anymotion duration register
#define BMA150_ADDR_CTRL_1            (0x0A)   // Control Register 1
#define BMA150_ADDR_CTRL_2            (0x0B)   // Control Register 2
#define BMA150_ADDR_CTRL_3            (0x14)   // Control Register 3
#define BMA150_ADDR_CTRL_4            (0x15)   // Control Register 4


// ============================================================================
// configuration register bits/masks
// ============================================================================
#define DS620_CFG_RESOLUTION_10BIT    (0x00)   // configure DS620 with 10Bit resolution (fast but unexact)
#define DS620_CFG_RESOLUTION_11BIT    (0x04)   // configure DS620 with 11Bit resolution
#define DS620_CFG_RESOLUTION_12BIT    (0x08)   // configure DS620 with 12Bit resolution
#define DS620_CFG_RESOLUTION_13BIT    (0x0C)   // configure DS620 with 13Bit resolution (slow but exact)
#define DS620_CFG_1SHOT               (0x01)   // configure DS620 just make a single measurement
#define DS620_CFG_AUTO_CONV           (0x02)   // configure DS620 make continuous measurement
#define DS620_CFG_DONE                (0x80)   // mask bit which indicate if measurement is completed

#define TMP102_CFG1_ONESHOT_READY     (0x80)   // one-shot / conversion ready
#define TMP102_CFG1_CONV_RES          (0x60)   // conversion resolution
#define TMP102_CFG1_FLT_QUEUE         (0x18)   // fault queue
#define TMP102_CFG1_POLARITY          (0x04)   // alarm flag polarity
#define TMP102_CFG1_THERMOSTAT_MODE   (0x02)   // thermostat mode
#define TMP102_CFG1_SHUTDOWN_MODE     (0x01)   // shutdown mode
#define TMP102_CFG2_CONV_RATE         (0xA0)   // conversion rate
#define TMP102_CFG2_ALERT_MODE        (0x20)   // alert mode
#define TMP102_CFG2_EXT_MODE          (0x10)   // extended mode
#define TMP102_TMP2_EXT_MODE          (0x01)   // indicates device operating mode (ext.=1, normal=0)

#define TMP102_MODE_CONV_RATE_025HZ   (0x00)   // converting with 0.25Hz
#define TMP102_MODE_CONV_RATE_1HZ     (0x40)   // converting with 1Hz
#define TMP102_MODE_CONV_RATE_4HZ     (0x80)   // converting with 4Hz
#define TMP102_MODE_CONV_RATE_8HZ     (0xA0)   // converting with 8Hz

#define ISL29020_MODE_ENABLE          (0x80)   // Enable device
#define ISL29020_MODE_CONTINUOUS      (0x40)   // Continuous time measurement
#define ISL29020_MODE_AMBIENT_LIGHT   (0x00)   // Ambient light measurement
#define ISL29020_MODE_INFRARED_LIGHT  (0x20)   // Infrared light measurement
#define ISL29020_MODE_FSR_1K          (0x00)   // Full Scale Range to max. 1.000 Lux
#define ISL29020_MODE_FSR_4K          (0x01)   // Full Scale Range to max. 4.000 Lux
#define ISL29020_MODE_FSR_16K         (0x02)   // Full Scale Range to max. 16.000 Lux
#define ISL29020_MODE_FSR_64K         (0x03)   // Full Scale Range to max. 64.000 Lux
#define ISL29020_MODE_INT_16BIT_ADC   (0x00)   // internal timing with 16Bit ADC
#define ISL29020_MODE_INT_12BIT_ADC   (0x04)   // internal timing with 12Bit ADC
#define ISL29020_MODE_INT_8BIT_ADC    (0x08)   // internal timing with 8Bit ADC
#define ISL29020_MODE_INT_4BIT_ADC    (0x0C)   // internal timing with 4Bit ADC
#define ISL29020_MODE_EXT_ADC         (0x10)   // external timing with ADC data output
#define ISL29020_MODE_EXT_TIMER       (0x14)   // external timing with timer data output

#define BMA150_MODE_SLEEP_MODE_EN     (0x01)   // Sleep Mode enable bit
#define BMA150_MODE_ANYMOTION_IRQ_EN  (0x40)   // Anymotion IRQ enable bit
#define BMA150_MODE_ADV_IRQ_EN        (0x40)   // Advanced IRQ system enable bit

#define BMA150_CTRL3_BANDWIDTH        (0x07)   // bandwith mask in control register 3
#define BMA150_CTRL1_SOFTRESET        (0x02)   // soft reset bit mask
#define BMA150_BANDWIDTH_100HZ        (0x03)   // 100Hz ADC bandwidth setting


// ============================================================================
// commands
// ============================================================================
/**
 * Initiates temperature conversions. If the part is in one-shot mode (1SHOT = 1), only one
 * conversion is performed. In continuous mode (1SHOT = 0), continuous temperature conversions
 * are performed until a Stop Convert command is issued (even if 1SHOT is changed to a 1).
 */
#define DS620_CMD_START_CONVERT     (0x51)

/**
 * Stops temperature conversions when  the device is in continuous conversion mode (1SHOT = 0).
 * This command has no function if the device is in one-shot mode (1SHOT = 1)
 */
#define DS620_CMD_STOP_CONVERT      (0x22)

/**
 * Refreshes SRAM shadow register with EEPROM data.
 */
#define DS620_CMD_RECALL_DATA       (0xB8)

/**
 * Copies data from all SRAM shadow registers to EEPROM.
 * The DS620 must be set to the continuous conversion mode and be actively
 * converting temperature to enable the Copy Data command to function properly.
 */
#define DS620_CMD_COPY_DATA         (0x48)

/**
 * Initiates a software power-on-reset (POR), which stops temperature conversions and resets
 * all registers and logic to their power-up states. The software POR allows the user to
 * simulate cycling the power without actually powering down the device. This command should
 * not be issued while a Copy Data command is in progress.
 */
#define DS620_CMD_RESET             (0x54)


/* === TYPES =============================================================== */

/* === PROTOTYPES ========================================================== */

/* === INLINE FUNCTIONS ==================================================== */

#endif /* _SENSORS_H */

/* EOF */

