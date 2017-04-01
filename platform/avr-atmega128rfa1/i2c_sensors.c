/**
 * @file sensors.c
 *
 * @brief  Implementation of the TWI sensor interface
 *
 * Implements the API for access to the TWI sensors on deRFnode/gateway boards.
 *
 * $Id: sensors.c,v 1.3 2012/11/14 12:39:05 ele Exp $
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
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "i2c_sensors_interface.h"      // common library interface
#include "i2c_sensors.h"                // device addresses/commands/bitmasks
#include "twi_master.h"             // underlying driver

/* === MACROS / DEFINES ==================================================== */

/* operation mode of the ISL29020 sensor device */
#define MODE_FSR              ISL29020_MODE_FSR_64K
#define MODE_LIGHT            ISL29020_MODE_AMBIENT_LIGHT
#define MODE_RES_TIMER        ISL29020_MODE_INT_16BIT_ADC

/**
 * Define RANGE and COUNT for proper light value (in LUX) calculation
 * (see datasheet for further details)
 */
#if MODE_FSR == ISL29020_MODE_FSR_1K
#define RANGE     (1000UL)
#elif MODE_FSR == ISL29020_MODE_FSR_4K
#define RANGE     (4000UL)
#elif MODE_FSR == ISL29020_MODE_FSR_16K
#define RANGE     (16000UL)
#elif MODE_FSR == ISL29020_MODE_FSR_64K
#define RANGE     (64000UL)
#else
#define RANGE     (1)
#endif

#if MODE_RES_TIMER == ISL29020_MODE_INT_16BIT_ADC
#define COUNT     (65536UL) // 2^16
#elif MODE_RES_TIMER == ISL29020_MODE_INT_12BIT_ADC
#define COUNT     (4096UL) // 2^12
#elif MODE_RES_TIMER == ISL29020_MODE_INT_8BIT_ADC
#define COUNT     (256UL) // 2^8
#elif MODE_RES_TIMER == ISL29020_MODE_INT_4BIT_ADC
#define COUNT     (16UL) // 2^4
#else
#define COUNT     (1)
#endif


/* BMA150 anymotion threshold (the higher the value, the lower the sensitivity) */
#define BMA150_ANYMOTION_THRESHOLD 0x08

/* BMA150 anymotion duration (allowed values are: 0,1,2,3  -> the higher,
 * the less sensitivity) */
#define BMA150_ANYMOTION_DURATION 0x01

/* === TYPES =============================================================== */
/* sensor conversion modes */
typedef enum
{
    ONE_SHOT,
    CONTINUOUS
}  conversion_mode_t;

/* type of registered callback routines */
typedef void (*irq_cb_t)(void);

/* === GLOBALS ============================================================= */
static conversion_mode_t ds620_conv_mode;      // remember the current working mode
static conversion_mode_t tmp102_conv_mode;     // remember the current working mode
static irq_cb_t bma150_isr_cb;                 // callback for BMA150 caused IRQs

/* === PROTOTYPES ========================================================== */
static twi_status_t DS620_ReadConfigRegister(uint8_t* msb, uint8_t* lsb);
static twi_status_t DS620_WriteConfigRegister(uint8_t msb, uint8_t lsb);
static twi_status_t DS620_SendCommand(uint8_t cmd);

static twi_status_t TMP102_ReadConfigRegister(uint8_t* msb, uint8_t* lsb);
static twi_status_t TMP102_WriteConfigRegister(uint8_t msb, uint8_t lsb);

static twi_status_t ISL29020_ReadCommandRegister(uint8_t* cmd);
static twi_status_t ISL29020_WriteCommandRegister(uint8_t cmd);

static twi_status_t BMA150_ReadRegister(uint8_t addr, uint8_t* val);
static twi_status_t BMA150_WriteRegister(uint8_t addr, uint8_t val);

/* === IMPLEMENTATION ====================================================== */

// === interface implementation ===============================================

/**
 * @brief Initializes the TWI-Interface and makes the MCU the busmaster.
 */
twi_status_t TWI_MasterInit(void)
{
    TWI_PULLUP_ENABLE();

    // initialize TWI interface with given clock
    return (twi_status_t) twi_init(TWI_CLK);
}


// --- DS620 ------------------------------------------------------------------

/**
 * @brief Initializes the DS620 temperature sensor with full resolution,
 * operating in single-shot mode. Measurements have to be triggered separately.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_Init(void)
{
    ds620_conv_mode = ONE_SHOT;

    return DS620_WriteConfigRegister(DS620_CFG_RESOLUTION_13BIT | DS620_CFG_1SHOT, 0x00);
}


/**
 * @brief Sets the DS620 temperature sensor to 'one-shot-mode' and triggers
 * one single conversion. Having performed the measurement, the device
 * automatically goes back to sleep state.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_StartOneshotMeasurement(void)
{
    uint8_t cfg_msb, cfg_lsb;

    // read config register
    if(TWI_SUCC != DS620_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;

    // set to one-shot mode
    cfg_msb |= DS620_CFG_1SHOT;
    if(TWI_SUCC != DS620_WriteConfigRegister(cfg_msb, cfg_lsb)) goto fail;

    // send start conversion command
    if(TWI_SUCC != DS620_SendCommand(DS620_CMD_START_CONVERT)) goto fail;

    ds620_conv_mode = ONE_SHOT;
    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Sets the DS620 temperature sensor to 'continous-conversion-mode'
 * and starts the conversion. Having performed the measurement, the device
 * subsequently starts the next one.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_StartContinuousMeasurement(void)
{
    uint8_t cfg_msb, cfg_lsb;

    // read config register
    if(TWI_SUCC != DS620_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;

    // set to continuous conversion mode
    cfg_msb &= ~DS620_CFG_1SHOT;
    if(TWI_SUCC != DS620_WriteConfigRegister(cfg_msb, cfg_lsb)) goto fail;

    // send start conversion command
    if(TWI_SUCC != DS620_SendCommand(DS620_CMD_START_CONVERT)) goto fail;

    ds620_conv_mode = CONTINUOUS;
    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Initiates a software power-on-reset (POR), which stops temperature
 * conversions and resets all registers and logic to their power-up states.
 * The software POR allows the user to simulate cycling the power without
 * actually powering down the device.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_Reset(void)
{
    return DS620_SendCommand(DS620_CMD_RESET);
}


/**
 * @brief Stops temperature conversions when the device is in continuous
 * conversion mode. If the device is in one-shot mode, invoking this procedure
 * is without function.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_StopMeasuring(void)
{
    return DS620_SendCommand(DS620_CMD_STOP_CONVERT);
}


/**
 * @brief Reads the temperature from the sensor. If requested, blocks until
 * measurement available (not available when running continuous conversions).
 * @param   temperature - where to store the read temperature value
 *          blocking_wait - whether to block until measurement done or not
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_GetTemperature(temperature_t* temperature, bool blocking_wait)
{
    uint8_t temp_msb, temp_lsb, cfg_msb, cfg_lsb;
    uint32_t cnt=0;

    if(blocking_wait && (CONTINUOUS != ds620_conv_mode)) {

        // block while polling config register if measurement available
        // (worst case: 200ms at highest sensor resolution)
        do {
            // read config register, check if conversion completed
            if(TWI_SUCC != DS620_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;
            if(cfg_msb & DS620_CFG_DONE)
                break;

        } while(cnt++ < F_CPU/5);

        // loop counter exceeded?
        if(!(cfg_msb & DS620_CFG_DONE))
            goto fail;
    }

    // read out temperature
    if(twi_start(DS620_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(DS620_ADDR_TEMP_MSB)) goto fail;

    // send stop + start condition to start reading from DS620
    twi_stop();
    if(twi_start(DS620_ADDRESS, TWI_READ)) goto fail;

    // read back temperature and configuration register
    temp_msb = twi_readAck();
    temp_lsb = twi_readAck();
    cfg_msb  = twi_readNack();

    // release bus
    twi_stop();

    /*
    * temperature register semantics:
    *  MSB:
    *    BIT   |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    *    VALUE |  S  | 2^7 | 2^6 | 2^5 | 2^4 | 2^3 | 2^2 | 2^1 |
    *
    *  LSB:
    *    BIT   |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    *    VALUE | 2^0 | 2^-1| 2^-2| 2^-3| 2^-4|  0  |  0  |  0  |
    */

    temperature->conversion_done = ((cfg_msb & DS620_CFG_DONE) == 0x80) ? 1 : 0;
    temperature->sign = ((temp_msb & 0x80) == 0x80) ? 1 : 0;
    temperature->integralDigit = ((temp_msb&0x7F) << 1) + ((temp_lsb & 0x80) >> 7);
    temperature->fractionalDigit = (((uint16_t)( (temp_lsb&0x78) >> 3))*25)/4;

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Puts the DS620 to power down mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t DS620_PowerDown(void)
{
    return DS620_StopMeasuring();
}


/**
 * @brief Wakes up the DS620 from power down mode.
 * @return TWI_SUCC
 */
twi_status_t DS620_WakeUp(void)
{
    // Sensor does not require any special wakeup procedure.
    // Measurements may be simply started upon request.

    return TWI_SUCC;
}


/**
 * @brief Helper function reading from the DS620's configuration register.
 * @param   msb - the Configuration Registers MSB
 *          lsb - the Configuration Registers LSB
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t DS620_ReadConfigRegister(uint8_t* msb, uint8_t* lsb)
{
    // set start address which to read from
    if(twi_start(DS620_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(DS620_ADDR_CONFIG_MSB)) goto fail;

    // send stop + start condition to start read from DS620
    twi_stop();
    if(twi_start(DS620_ADDRESS, TWI_READ)) goto fail;

    *msb = twi_readAck();
    *lsb = twi_readNack();

    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Helper function writing to the DS620's configuration register.
 * @param   msb - the Configuration Registers MSB
 *          lsb - the Configuration Registers LSB
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t DS620_WriteConfigRegister(uint8_t msb, uint8_t lsb)
{
    if(twi_start(DS620_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(DS620_ADDR_CONFIG_MSB)) goto fail;
    if(twi_write(msb)) goto fail;
    if(twi_write(lsb)) goto fail;

    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Helper function sending one single command byte to the sensor device.
 * @param cmd - the command byte to send
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t DS620_SendCommand(uint8_t cmd)
{
    if(twi_start(DS620_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(cmd)) goto fail;

    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


// --- TMP102 -----------------------------------------------------------------

/**
 * @brief Initializes the TMP102 temperature sensor. The device initially
 * performs one conversion which may not be avoided. After that, the device
 * is immediately shut down. The result of the first conversion may be read
 * at each time. Further temperature conversions must be triggered explicitly.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_Init(void)
{
    tmp102_conv_mode = ONE_SHOT;

    return TMP102_WriteConfigRegister(TMP102_CFG1_SHUTDOWN_MODE, TMP102_MODE_CONV_RATE_4HZ);
}


/**
 * @brief Starts one single measurement of the TMP102 device. If already
 * measuring in one-shot-mode invoking this function has no effect. When the
 * device is performing continuous measurements, it is switched back to single
 * shot mode and after the last conversion is done, the device is shut down.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_StartOneshotMeasurement(void)
{
    uint8_t cfg_msb, cfg_lsb;

    // read current config register values
    if(TWI_SUCC!=TMP102_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;

    // stop here if one-shot mode already set and currently converting
    if(!(cfg_msb & TMP102_CFG1_ONESHOT_READY)) goto end;

    // enable shutdown mode, set one-shot flag -> trigger single measurement
    cfg_msb |= TMP102_CFG1_SHUTDOWN_MODE | TMP102_CFG1_ONESHOT_READY;

    // write back changed register values
    if(TWI_SUCC!=TMP102_WriteConfigRegister(cfg_msb, cfg_lsb)) goto fail;

end:
    tmp102_conv_mode = ONE_SHOT;
    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Puts the TMP102 device in continuous measuring mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_StartContinuousMeasurement(void)
{
    uint8_t cfg_msb, cfg_lsb;

    // read current register value
    if(TWI_SUCC != TMP102_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;

    // stop here if continuous mode already enabled
    if(!(cfg_msb & TMP102_CFG1_SHUTDOWN_MODE)) goto end;

    // disable shutdown mode
    cfg_msb &= ~TMP102_CFG1_SHUTDOWN_MODE;

    // write back changed register values
    if(TWI_SUCC != TMP102_WriteConfigRegister(cfg_msb, cfg_lsb)) goto fail;

end:
    tmp102_conv_mode = CONTINUOUS;
    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Stops subsequent measurements when operating the TMP102 sensor device
 * in continuous mode, when already operating in one-shot mode this function is
 * without effect since the current conversion is always completed and the device
 * after that automatically enters shutdown mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_StopMeasuring(void)
{
    uint8_t cfg_msb, cfg_lsb;

    // read current register value
    if(TWI_SUCC != TMP102_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;

    // stop here if already converting in one-shot mode / shut down
    if(cfg_msb & TMP102_CFG1_SHUTDOWN_MODE) goto end;

    // set Shutdown flag
    cfg_msb |= TMP102_CFG1_SHUTDOWN_MODE;

    // write back changed register values
    if(TWI_SUCC != TMP102_WriteConfigRegister(cfg_msb, cfg_lsb)) goto fail;

end:
    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Reads the temperature from the sensor. If requested, blocks until
 * measurement available (not available when running continuous conversions).
 * @param   temperature - where to store the read temperature value
 *          blocking_wait - whether to block until measurement done or not
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_GetTemperature(temperature_t* temperature, bool blocking_wait)
{
    uint8_t temp_msb, temp_lsb, cfg_msb, cfg_lsb;
    uint32_t cnt=0;

    if(blocking_wait && (CONTINUOUS != tmp102_conv_mode)) {

        // block while polling config register if measurement available
        // (worst case: 26ms at highest sensor resolution)
        do {
           // read config register, check if conversion completed
           if(TWI_SUCC != TMP102_ReadConfigRegister(&cfg_msb, &cfg_lsb)) goto fail;
           if(cfg_msb & TMP102_CFG1_ONESHOT_READY)
               break;

        } while(cnt++ < F_CPU/32);

        // loop counter exceeded?
        if(!(cfg_msb & TMP102_CFG1_ONESHOT_READY)) goto fail;
    }

    // set start address which to read from
    if(twi_start(TMP102_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(TMP102_ADDR_TEMP)) goto fail;

    // send stop + start condition to start reading
    twi_stop();
    if(twi_start(TMP102_ADDRESS, TWI_READ)) goto fail;

    // read two bytes, MSB is transmitted first
    temp_msb = twi_readAck();
    temp_lsb = twi_readNack();

    // release bus
    twi_stop();

    /*
     * temperature register semantics (assumed normal mode - 12bit resolution,
     * otherwise - 13bit resolution - everything shifted one bit right
     *
     *  MSB:
     *    BIT   |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
     *    VALUE |  S  | 2^6 | 2^5 | 2^4 | 2^3 | 2^2 | 2^1 | 2^0 |
     *
     *  LSB:
     *    BIT   |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
     *    VALUE | 2^-1| 2^-2| 2^-3| 2^-4|  0  |  0  |  0  | mode|
     */

    // convert
    temperature->conversion_done = (temp_msb==0x00 && ((temp_lsb&0xFE)==0x00))?0:1;
    temperature->sign = ((temp_msb & 0x80) == 0x80) ? 1 : 0;

    if(temp_lsb&TMP102_TMP2_EXT_MODE) {
        // extended mode
        temperature->integralDigit   = ((temp_msb&0x7F) << 1) | ((temp_lsb&0x80) >> 7);
        temperature->fractionalDigit =  (((uint16_t)((temp_lsb&0x78) >> 3))*25)/4 ;
    } else {
        // normal mode
        temperature->integralDigit   = (temp_msb&0x7F);
        temperature->fractionalDigit = (((uint16_t)((temp_lsb&0xF0) >> 4))*25)/4 ;
    }

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Puts the TMP102 to power down mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t TMP102_PowerDown(void)
{
    return TMP102_StopMeasuring();
}


/**
 * @brief Wakes up the TMP102 from power down mode.
 * @return TWI_SUCC
 */
twi_status_t TMP102_WakeUp(void)
{
    // Sensor does not require any special wakeup procedure.
    // Measurements may be simply started upon request.

    return TWI_SUCC;
}



/**
 * @brief Helper function reading from the TMP102's Configuration Register.
 * @param   msb - the configuration Registers MSB
 *          lsb - the configuration Registers LSB
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t TMP102_ReadConfigRegister(uint8_t* msb, uint8_t* lsb)
{
    // send address where we want read data from
    if(twi_start(TMP102_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(TMP102_ADDR_CFG)) goto fail;

    // stop connection to restart with read access
    twi_stop();
    if(twi_start(TMP102_ADDRESS, TWI_READ)) goto fail;

    // read back temperature and configuration registers
    *msb = twi_readAck();
    *lsb = twi_readNack();

    // release bus
    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Helper function writing to the TMP102's Configuration Register.
 * @param   msb - the configuration Registers MSB
 *          lsb - the configuration Registers LSB
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t TMP102_WriteConfigRegister(uint8_t msb, uint8_t lsb)
{
    // set start address which to write to
    if(twi_start(TMP102_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(TMP102_ADDR_CFG)) goto fail;

    // write two bytes, MSB is transmitted first
    if(twi_write(msb)) goto fail;
    if(twi_write(lsb)) goto fail;

    // release bus
    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


// --- ISL29020 ---------------------------------------------------------------

/**
 * @brief Initializes the ISL290 luminosity sensor sensing ambient light with
 * sensing type, full scale range and resolution set as given per macros
 * (default: full scale range 64k Lux, internal conversion, 16bit resolution).
 * Conversions must be triggered separately.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_Init(void)
{
    uint8_t cmd_reg;

    // assemble command register
    cmd_reg = MODE_LIGHT | MODE_RES_TIMER | MODE_FSR;

    // write new value
    return ISL29020_WriteCommandRegister(cmd_reg);
}


/**
 * @brief Makes the luminosity sensor perform a single measurement.
 * After completion, the device is automatically powered down.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_StartOneshotMeasurement(void)
{
    uint8_t cmd_reg;

    // read out command register
    if(TWI_SUCC != ISL29020_ReadCommandRegister(&cmd_reg)) goto fail;

    // kill continuous conversion flag, enable device
    cmd_reg &= ~ISL29020_MODE_CONTINUOUS;
    cmd_reg |= ISL29020_MODE_ENABLE;

    // write new value
    if(TWI_SUCC!=ISL29020_WriteCommandRegister(cmd_reg)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Puts the luminosity sensor in continuous conversion mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_StartContinuousMeasurement(void)
{
    uint8_t cmd_reg;

    // read out command register
    if(TWI_SUCC != ISL29020_ReadCommandRegister(&cmd_reg)) goto fail;

    // set continuous conversion flag, enable device
    cmd_reg |= ISL29020_MODE_CONTINUOUS;
    cmd_reg |= ISL29020_MODE_ENABLE;

    // write new value
    if(TWI_SUCC!=ISL29020_WriteCommandRegister(cmd_reg)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Stops luminosity sensing when the device is in continuous conversion
 * mode. Invocation without function if the device is in one-shot mode.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_StopMeasuring(void)
{
    uint8_t cmd_reg;

    // read out command register
    if(TWI_SUCC != ISL29020_ReadCommandRegister(&cmd_reg)) goto fail;

    // kill enable bit in command register
    cmd_reg &= ~ISL29020_MODE_ENABLE;

    // write new value
    if(TWI_SUCC!=ISL29020_WriteCommandRegister(cmd_reg)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Reads the luminosity from the sensor. Blocking until measurement finished
 * is not available for this kind of sensor - its up to the application to
 * wait long enough until valid measurement results are available.
 * (100ms at full resolution, see table 10 in datasheet for details)
 * @param   temperature - where to store the read temperature value
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_GetLuminosity(luminosity_t* luminosity)
{
    uint8_t data_lsb, data_msb;

    // read out luminosity

    // send address where we want read data from
    if(twi_start(ISL29020_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(ISL29020_ADDR_DATA_LSB)) goto fail;

    // stop connection to restart with read access
    twi_stop();
    if(twi_start(ISL29020_ADDRESS, TWI_READ)) goto fail;

    // read back luminosity registers
    data_lsb = twi_readAck();
    data_msb = twi_readNack();

    // release bus
    twi_stop();

    /*
     * Calculation:
     *                                                   RANGE
     *   E (Lux) = a * (data_msb|data_lsb) = a * data = ------- * data
     *                                                   COUNT
     *
     * RANGE depends on Full Scale Range and COUNT depends on ADC resolution
     */

    *luminosity = (uint16_t) ((float)((uint16_t)data_lsb | ((uint16_t)data_msb << 8))
                    * ((float)RANGE/(float)COUNT));

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Powers down the ISL29020 sensor device.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t ISL29020_PowerDown(void)
{
    return ISL29020_StopMeasuring();
}


/**
 * @brief Wakes up the ISL29020 from power down mode.
 * @return TWI_SUCC
 */
twi_status_t ISL29020_WakeUp(void)
{
    // Sensor does not require any special wakeup procedure.
    // Measurements may be simply started upon request.

    return TWI_SUCC;
}


/**
 * @brief Helper function reading from the ISL29020's command register.
 * @param  cmd - the Command Registers content
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t ISL29020_ReadCommandRegister(uint8_t* cmd)
{
    // send address where we want read data from
    if(twi_start(ISL29020_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(ISL29020_ADDR_COMMAND)) goto fail;

    // stop connection to restart with read access
    twi_stop();
    if(twi_start(ISL29020_ADDRESS, TWI_READ)) goto fail;

    // read back temperature and configuration registers
    *cmd = twi_readNack();

    // release bus
    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Helper function writing to the ISL29020's command register.
 * @param  cmd - the Command Registers new content
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t ISL29020_WriteCommandRegister(uint8_t cmd)
{
    // send address of command register and write data to it
    if(twi_start(ISL29020_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(ISL29020_ADDR_COMMAND)) goto fail;
    if(twi_write(cmd)) goto fail;

    // release bus
    twi_stop();
    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


// --- BMA150 -----------------------------------------------------------------

/**
 * @brief Initializes and enables the BMA150 acceleration sensor.
 * Sets anymotion detection parameters to values defined per macros above.
 * Motion detection itself must enabled separately.
 * Once configured, the device has no explicit 'start measurement' command,
 * it consecutively determines the current acceleration and temperature
 * that both may be requested without any timeout to wait before.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_Init(void)
{
    uint8_t tmp;

    // set anymotion duration and threshold
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_ANY_MOTION_THRES, BMA150_ANYMOTION_THRESHOLD)) goto fail;
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_ANY_MOTION_DUR, (BMA150_ANYMOTION_DURATION&0x03) << 6)) goto fail;

    // decrease bandwidth for smaller jitter
    if(TWI_SUCC!=BMA150_ReadRegister(BMA150_ADDR_CTRL_3, &tmp)) goto fail;
    tmp &= ~BMA150_CTRL3_BANDWIDTH;
    tmp |= BMA150_BANDWIDTH_100HZ;
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_CTRL_3, tmp)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief ISR for BMA150-triggered interrupts. Invokes user defined callbacks
 * if there are any. Remember that depending on the set sensivity (motion
 * duration/-threshold) these interrupts may occur with high frequency!
 */
/*
ISR(TIMER0_COMPA_vect)
{
    // if there is an user callback, invoke it
    if(bma150_isr_cb!=NULL)
        bma150_isr_cb();
}
*/

/**
 * @brief Sets or removes a callback routine for anymotion detection based
 * interrupts. Existing callbacks may be removed by invoking with a NULL parameter.
 * @param callback - function pointer to callback routine (NULL for removal)
 */
void BMA150_SetAnymotionIrqCallback(bma150_callback_t callback)
{
    // save callback locally
    bma150_isr_cb = (irq_cb_t)callback;
}


/**
 * @brief Switches the 'anymotion detection'-feature either on or off.
 * If the feature is enabled, the device will throw interrupts if it is moved
 * or shaken more than set thresholds allow.
 * @param enabled - whether to switch anymotion detection on (1) or off (0)
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_ConfigureAnymotionIrq(uint8_t enabled)
{
    uint8_t reg_val, new_val;

	// configure interrupt detection on IRQ line
    if(enabled) {

        TCNT0 = 0;                   // reset counter value
        OCR0A = 1;                   // set compare match value
        TCCR0B = (0x07<<CS10);       // externally clocked, riging edge
        TCCR0A = _BV(WGM01);         // auto reset counter value upon compare match
        TIMSK0 |= _BV(OCIE0A);       // cmp. match IRQ enable
        sei();                       // globally enable interrupts, if not already done

    } else {

        TCCR0B = 0;                  // disable clock source
    }

    // enable advanced interrupt system, if not already enabled
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_CTRL_2, &reg_val)) goto fail;

    // setup new register value
    if(enabled)
        new_val = reg_val | BMA150_MODE_ANYMOTION_IRQ_EN;
    else
        new_val = reg_val & ~BMA150_MODE_ANYMOTION_IRQ_EN;

    // check if change required at all ...
    if(new_val != reg_val) {
        // update register
        if(TWI_SUCC != BMA150_WriteRegister(BMA150_ADDR_CTRL_2, new_val)) goto fail;
    }

    // enable advanced interrupt system, if not already enabled
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_CTRL_4, &reg_val)) goto fail;

    // check if change required at all ...
    if(!(reg_val & BMA150_MODE_ADV_IRQ_EN)) {
        // update register
        reg_val |= BMA150_MODE_ADV_IRQ_EN;
        if(TWI_SUCC != BMA150_WriteRegister(BMA150_ADDR_CTRL_4, reg_val)) goto fail;
    }

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Reads out the current acceleration from the BMA150 sensor device.
 * Since the first measurement after device initialization / wakeup is always
 * uncorrent, it is recommended to perform one dummy read in these situations.
 * @param data - parameter storage destination
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_GetAcceleration(acceleration_t* data)
{
    uint8_t tmp;
    uint16_t acc_z, acc_y, acc_x;

    // Read Z axis acceleration values
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_Z_MSB, &tmp)) goto fail;
    acc_z = (tmp << 2);
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_Z_LSB, &tmp)) goto fail;
    acc_z |= ((tmp&0xA0) >> 6);

    // Calculate integral and fractional digit of z axis acceleration
    if ((acc_z & (1 << 9)) == 0)
    {
        data->acc_z_sign = 0;
        acc_z = (uint16_t) ((float) acc_z * 0.78125);
    } else
    {
        data->acc_z_sign = 1;
        acc_z &= ~(1 << 9);
        acc_z = 400 - (uint16_t) ((float) acc_z * 0.78125);
    }
    data->acc_z_integral = acc_z / 100;
    data->acc_z_fractional = acc_z % 100;

    // Read Y axis acceleration values
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_Y_MSB, &tmp)) goto fail;
    acc_y = (tmp << 2);
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_Y_LSB, &tmp)) goto fail;
    acc_y |= ((tmp&0xA0) >> 6);

    // Calculate integral and fractional digit of y axis acceleration
    if ((acc_y & (1 << 9)) == 0)
    {
        data->acc_y_sign = 0;
        acc_y = (uint16_t) ((float) acc_y * 0.78125);
    } else
    {
        data->acc_y_sign = 1;
        acc_y &= ~(1 << 9);
        acc_y = 400 - (uint16_t) ((float) acc_y * 0.78125);
    }
    data->acc_y_integral = acc_y / 100;
    data->acc_y_fractional = acc_y % 100;

    // Read X axis acceleration values
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_X_MSB, &tmp)) goto fail;
    acc_x = (tmp << 2);
    if(TWI_SUCC != BMA150_ReadRegister(BMA150_ADDR_ACC_X_LSB, &tmp)) goto fail;
    acc_x |= ((tmp&0xA0) >> 6);

    // Calculate integral and fractional digit of x axis acceleration
    if ((acc_x & (1 << 9)) == 0)
    {
        data->acc_x_sign = 0;
        acc_x = (uint16_t) ((float) acc_x * 0.78125);
    } else
    {
        data->acc_x_sign = 1;
        acc_x &= ~(1 << 9);
        acc_x = 400 - (uint16_t) ((float) acc_x * 0.78125);
    }
    data->acc_x_integral = acc_x / 100;
    data->acc_x_fractional = acc_x % 100;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Reads out the temperature from the BMA150 sensor device.
 * @param temperature - parameter storage destination
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_GetTemperature(temperature_t* temperature)
{
    uint8_t temp_raw;
    float help;

    if(TWI_SUCC!= BMA150_ReadRegister(BMA150_ADDR_TEMP, &temp_raw)) goto fail;

    // convert temperature:
    // 0x00 == -30�C, 0.5�C resolution
    help = (((float)temp_raw)/2) - 30;

    // fill data structure
    temperature->conversion_done = 1;
    temperature->sign            = (help>0)?0:1;
    temperature->integralDigit   = (uint8_t) help;
    temperature->fractionalDigit = ((uint8_t)(help*10))%10;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Switches off down the BMA150 sensor device. Switching it on again
 * requires invoking BMA150_Init().
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_PowerDown(void)
{
    uint8_t reg_val;

    // read previous register content
    if(TWI_SUCC!=BMA150_ReadRegister(BMA150_ADDR_CTRL_1, &reg_val)) goto fail;

    // update and write back
    reg_val |= BMA150_MODE_SLEEP_MODE_EN;
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_CTRL_1, reg_val)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Wakes up the BMA150 from power down mode. Its up to application to
 * ensure that accessing the sensor once it is woken up is delayed accord to
 * the timeouts given in datasheet (1ms before first read, 10ms before EEPROM
 * access).
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_WakeUp(void)
{
    uint8_t reg_val;

    // there are 2 possibilities to wakeup the device:
    // either remove the sleep mode bit or perform a soft reset

    // read previous register content
    if(TWI_SUCC!=BMA150_ReadRegister(BMA150_ADDR_CTRL_1, &reg_val)) goto fail;

    // kill sleep mode bit and write back
    reg_val &= ~BMA150_MODE_SLEEP_MODE_EN;
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_CTRL_1, reg_val)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Helper function performing a soft reset on the BMA150 sensor device.
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
twi_status_t BMA150_SoftReset(void)
{
    uint8_t reg_val;

    // read out previous register content
    if(TWI_SUCC!=BMA150_ReadRegister(BMA150_ADDR_CTRL_1, &reg_val)) goto fail;

    // preserve register value, just add soft reset bit, write back
    reg_val |= BMA150_CTRL1_SOFTRESET;
    if(TWI_SUCC!=BMA150_WriteRegister(BMA150_ADDR_CTRL_1, reg_val)) goto fail;

    return TWI_SUCC;

fail:
    return TWI_FAIL;
}


/**
 * @brief Helper function reading from one of the BMA150 registers.
 * @param   regAddr - address of the register to read from
 *          regVal  - the registers content
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t BMA150_ReadRegister(uint8_t addr, uint8_t* val)
{
    if(twi_start(BMA150_ADDRESS, TWI_WRITE)) goto fail;

    // set address which to read from
    if(twi_write(addr)) goto fail;

    // send stop + start condition, then read
    twi_stop();
    if(twi_start(BMA150_ADDRESS, TWI_READ)) goto fail;
    *val = twi_readNack();

    // release bus
    twi_stop();

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/**
 * @brief Helper function writing to one of the BMA150 registers.
 * @param   addr - address of the register to write to
 *          val  - the registers new content
 * @return TWI_SUCC in case of success, otherwise TWI_FAIL.
 */
static twi_status_t BMA150_WriteRegister(uint8_t addr, uint8_t val)
{
    if(twi_start(BMA150_ADDRESS, TWI_WRITE)) goto fail;
    if(twi_write(addr)) goto fail;
    if(twi_write(val)) goto fail;

    // release bus
    twi_stop();

    return TWI_SUCC;

fail:
    twi_stop();
    return TWI_FAIL;
}


/* EOF */
