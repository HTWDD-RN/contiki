/*
 * @author    dresden elektronik ingenieurtechnik gmbh: http://www.dresden-elektronik.de
 * @author    Support email: wireless@dresden-elektronik.de
 *
 * Copyright (c) 2011, dresden elektronik ingenieurtechnik gmbh. All rights reserved.
 *
 * Licensed under dresden elektronik's Limited License Agreement --> deEULA.txt
 */

/* Prevent double inclusion */
#ifndef USB_H_
#define USB_H_

/* === INCLUDES ============================================================ */
#include <stdio.h>      // FILE* type

/* === MACROS / DEFINES ==================================================== */

/*
 * The following definitions are for deRFnode platform
 *
 * Due to backward compability of radio modules the USB pins are not located at one single port
 * They are connected to different pins on radio modules, therefore another definition is required
 */

/* Port definitions which gives indication of reception of byte */
#define USB_RXF_PIN                 (PE2)
#define USB_RXF_PORT                (PORTE)
#define USB_RXF_DDR                 (DDRE)
#define USB_RXF_PINS                (PINE)

/* Port definitions which gives indication of transmission of byte */
#define USB_TXE_PIN                 (PB5)
#define USB_TXE_PORT                (PORTB)
#define USB_TXE_DDR                 (DDRB)
#define USB_TXE_PINS                (PINB)

/* Port definitions for write and read access on USB */
#define USB_WR_PIN                  (PD3)
#define USB_WR_PORT                 (PORTD)
#define USB_WR_DDR                  (DDRD)
#define USB_WR_PINS                 (PIND)

#define USB_RD_PIN                  (PD2)
#define USB_RD_PORT                 (PORTD)
#define USB_RD_DDR                  (DDRD)
#define USB_RD_PINS                 (PIND)

/* Port definitions of data port from USB interface */
#define USB_D0_BIT                  (0)
#define USB_D0_PIN                  (PB0)
#define USB_D0_PORT                 (PORTB)
#define USB_D0_DDR                  (DDRB)
#define USB_D0_PINS                 (PINB)

#define USB_D1_BIT                  (1)
#define USB_D1_PIN                  (PF2)
#define USB_D1_PORT                 (PORTF)
#define USB_D1_DDR                  (DDRF)
#define USB_D1_PINS                 (PINF)

#define USB_D2_BIT                  (2)
#define USB_D2_PIN                  (PD5)
#define USB_D2_PORT                 (PORTD)
#define USB_D2_DDR                  (DDRD)
#define USB_D2_PINS                 (PIND)

#define USB_D3_BIT                  (3)
#define USB_D3_PIN                  (PG2)
#define USB_D3_PORT                 (PORTG)
#define USB_D3_DDR                  (DDRG)
#define USB_D3_PINS                 (PING)

#define USB_D4_BIT                  (4)
#define USB_D4_PIN                  (PE6)
#define USB_D4_PORT                 (PORTE)
#define USB_D4_DDR                  (DDRE)
#define USB_D4_PINS                 (PINE)

#define USB_D5_BIT                  (5)
#define USB_D5_PIN                  (PB4)
#define USB_D5_PORT                 (PORTB)
#define USB_D5_DDR                  (DDRB)
#define USB_D5_PINS                 (PINB)

#define USB_D6_BIT                  (6)
#define USB_D6_PIN                  (PE7)
#define USB_D6_PORT                 (PORTE)
#define USB_D6_DDR                  (DDRE)
#define USB_D6_PINS                 (PINE)

#define USB_D7_BIT                  (7)
#define USB_D7_PIN                  (PB6)
#define USB_D7_PORT                 (PORTB)
#define USB_D7_DDR                  (DDRB)
#define USB_D7_PINS                 (PINB)

/* === TYPES =============================================================== */

/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

void usb_io_init(void);
uint8_t usb_keypressed(void);
int usb_putc_std (char c, FILE* dummy_file);
int usb_getc_std (FILE* dummy_file);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* === INLINE FUNCTIONS ==================================================== */


#endif /* USB_H_ */

/* EOF */
