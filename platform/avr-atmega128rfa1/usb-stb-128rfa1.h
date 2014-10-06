
/* Port definition of USB data port */
#define USB_DATA_PORT         (PORTB)
#define USB_DATA_DDR          (DDRB)
#define USB_DATA_PINS         (PINB)

/* Port definition of USB address decoding port */
// PC6,7 -> PDx on RFA1
#define USB_DECODE_PORT       (PORTD)
#define USB_DECODE_DDR        (DDRD)

#define USB_CS0_PIN           (PD6)
#define USB_CS1_PIN           (PD7)

/* Port definitions which gives indication of reception of byte */
#define USB_RXF_PIN           (PE7)
#define USB_RXF_PORT          (PORTE)
#define USB_RXF_DDR           (DDRE)
#define USB_RXF_PINS          (PINE)

/* Port definitions which gives indication of transmission of byte */
#define USB_TXE_PIN           (PE6)
#define USB_TXE_PORT          (PORTE)
#define USB_TXE_DDR           (DDRE)
#define USB_TXE_PINS          (PINE)

/* Port definitions for write and read access on USB */
#define USB_WR_PIN            (PE4)
#define USB_WR_PORT           (PORTE)
#define USB_WR_DDR            (DDRE)
#define USB_WR_PINS           (PINE)

#define USB_RD_PIN            (PE5)
#define USB_RD_PORT           (PORTE)
#define USB_RD_DDR            (DDRE)
#define USB_RD_PINS           (PINE)


/* === PROTOTYPES ========================================================== */
void usb_init(void);
uint8_t usb_keypressed(void);
int usb_putc_std (char c, FILE* dummy_file);
int usb_getc_std (FILE* dummy_file);
