

/* === INCLUDES ============================================================ */
#include <avr/io.h>           // include io definitions
#include <util/delay.h>       // include delay definitions
#include <stdio.h>            // include standard io definitions
#include <stdbool.h>          // include bool definition

#include "usb.h"              // include USB specific definitions


// Setup a File stream with putchar and getchar functionality over USB
FILE usb_stream = FDEV_SETUP_STREAM(usb_putc_std, usb_getc_std,_FDEV_SETUP_RW);

/* === PROTOTYPES ========================================================== */

uint8_t read_usb_byte(void);
void write_usb_byte(uint8_t data);

/* === IMPLEMENTATION ====================================================== */

/**
 * @brief USB interface initialization function.
 *
 * This function initializes the USB hardware.
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 */
void usb_init(void){

   /* Make sure USB_RXF and USB_TXE are inputs */
   USB_TXE_DDR  &= ~_BV(USB_TXE_PIN);
   USB_TXE_PORT |=  _BV(USB_TXE_PIN);

   USB_RXF_DDR  &= ~_BV(USB_RXF_PIN);
   USB_RXF_PORT |=  _BV(USB_RXF_PIN);

   stdout = &usb_stream;                            // init standard output over USB
	 stdin  = &usb_stream;                            // init standard input over USB
}


/**
 * @brief Checks if a new character received.
 *
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 * 
 * @return  Returns '1' if a new character is available on the FTDI USB chip
 *
 */
uint8_t usb_keypressed(void)
{
#if (PLATFORM == SENS_TERM_BOARD) || (PLATFORM == DE_RF_NODE)

   return (!(USB_RXF_PINS & _BV(USB_RXF_PIN))) ? 1 : 0;

#endif // (PLATFORM == SENS_TERM_BOARD) || (PLATFORM == DE_RF_NODE)
   return 0;
}

/**
 * @brief Reads a character via USB.
 *
 * This function waits for character available on the USB chip and gives it back.
 * Blocking function.
 * This function is the standard I/O implementation which can be used for std. I/O facilities.
 *
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 *
 * @param     dummy_file  not used
 * @return    character on the USB chip
 *
 */
int usb_getc_std(FILE* dummy_file)
{
   /* wait until new char received */
   while (USB_RXF_PINS & _BV(USB_RXF_PIN)){;}
   /* return the data */
   return(read_usb_byte());
}

/**
 * @brief Writes a character via USB.
 *
 * This function waits for empty buffer and then writes a character to USB.
 * This function is the standard I/O implementation which can be used for std. I/O facilities.
 *
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 *
 * @param     c           the character to write
 * @param     dummy_file  not used
 *
 * @return    char: character written to USB
 *
 */
int usb_putc_std(char c, FILE* dummy_file)
{
   /* Wait for empty transmit buffer */
   while (USB_TXE_PINS & _BV(USB_TXE_PIN)){;}
   /* write the byte into the USB FIFO */
   write_usb_byte(c);
   /* return the char */
   return(c);
}

/**
 * @brief Helper for usb byte input.
 *
 * This function reads a byte from USB interface.
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 *
 * @return    the character read from USB
 *
 */
uint8_t read_usb_byte(void)
{
   uint8_t data = 0;
   /* prepare data -> all bits are inputs */
   USB_DATA_DDR = 0x00;
   /* prepare chip select for usb */
   USB_DECODE_PORT &= ~_BV(USB_CS0_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS0_PIN);
   USB_DECODE_PORT &= ~_BV(USB_CS1_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS1_PIN);

   /* generate a single low / hi edge RD */
   USB_RD_PORT |=  _BV(USB_RD_PIN);
   USB_RD_DDR  |=  _BV(USB_RD_PIN);
   USB_RD_PORT &= ~_BV(USB_RD_PIN);
   /* give chip a chance to put out the data */
   _delay_us(1);
   /* save data */
   data = USB_DATA_PINS;
   /* and now back ... */
   USB_RD_PORT |= _BV(USB_RD_PIN);

   /* disable chip select */
   USB_DECODE_PORT &= ~_BV(USB_CS0_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS0_PIN);
   USB_DECODE_PORT &= ~_BV(USB_CS1_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS1_PIN);
   return(data);
}


/**
 * @brief Helper for usb byte output.
 *
 * This function writes a byte to USB interface.
 * This is only working if platform is Sensor Terminal Board or deRFnode.
 *
 * @param   data  data which should be written
 *
 */
void write_usb_byte(uint8_t data)
{
   /* prepare data */
   USB_DATA_PORT = data;
   /* all bits are outputs */
   USB_DATA_DDR = 0xff;

   /* prepare chip select for usb */
   USB_DECODE_PORT &= ~_BV(USB_CS0_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS0_PIN);
   USB_DECODE_PORT &= ~_BV(USB_CS1_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS1_PIN);

   /* generate a single low / hi edge WR */
   USB_WR_PORT |= _BV(USB_WR_PIN);
   USB_WR_DDR  |= _BV(USB_WR_PIN);
   /* and now back ... */
   USB_WR_PORT &= ~_BV(USB_WR_PIN);
   USB_WR_PORT |=  _BV(USB_WR_PIN);

   /* disable chip select */
   USB_DECODE_PORT &= ~_BV(USB_CS0_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS0_PIN);
   USB_DECODE_PORT &= ~_BV(USB_CS1_PIN);
   USB_DECODE_DDR  |=  _BV(USB_CS1_PIN);
}



