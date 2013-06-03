### Project SN-Sensoransteuerung
###    This project should only test I2C devices
###    like SHT21, BMP085, TLS2561, BMA180.
### Copyright (c) 2012, Angelos Drossos <angelos.drossos@gmail.com>
### Contiki BSD license

# Contiki Project Name
CONTIKI_PROJECT = sn-sensoransteuerung
# PROJECTDIRS = .

#=======================================================================
# TARGETs
#=======================================================================

# tested targets
ifndef TARGET
TARGET=avr-atmega128rfa1
# TARGET=deRFmega128
# TARGET=rcb128rfa1-stb
endif

# activate DEBUG
CFLAGS += -DDEBUG=1

#=======================================================================
# IPv6 Config
#=======================================================================

# variable for root Makefile.include
WITH_UIP6=1

# for some platforms
UIP_CONF_IPV6=1

# Programm address (comment out to use default addresses)
#CFLAGS += -DEUI64_ADDRESS="{0x02, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x03}"

#=======================================================================
# I2C/Sensors
#=======================================================================

include Makefile.sensors.include

#=======================================================================
# RS232 / UART Settings
#=======================================================================

# set rs232 port for debug prints (0: USART0, 1: USART1)
# rs232 port for Sensor Terminal Board: USART1
#CFLAGS += -DRS232_PRINT_PORT=RS232_PORT_0
CFLAGS += -DRS232_PRINT_PORT=RS232_PORT_1

# set rs232 BAUD rate for debug prints (see cpu/avr/dev/rs232_atmega128rfa1.h)
CFLAGS += -DRS232_PRINT_BAUD=USART_BAUD_9600
#CFLAGS += -DRS232_PRINT_BAUD=USART_BAUD_115200


# set rs232 port for slip (0: USART0, 1: USART1)
# rs232 port for Sensor Terminal Board: USART1
# uncomment to deactivate SLIP feature
# Warning: check if slip port is not print port!
#CFLAGS += -DSLIP_PORT=RS232_PORT_1
CFLAGS += -DSLIP_PORT=RS232_PORT_0

# set rs232 BAUD rate for slip (see cpu/avr/dev/rs232_atmega128rfa1.h)
CFLAGS += -DSLIP_BAUD=USART_BAUD_9600
#CFLAGS += -DSLIP_BAUD=USART_BAUD_115200

#=======================================================================
# Process SN Sensoransteuerung
#=======================================================================

# activate Process SN Sensoransteuerung Debug Prints
CFLAGS += -DSN_SENSORANSTEUERUNG_DEBUG=1

#=======================================================================
# Project: rs232_project
#=======================================================================

CONTIKI_SOURCEFILES += rs232_project.c

#=======================================================================
# Network/Internet Settings
#=======================================================================

# Flags
#UIP_CONF_IPV6=1

#=======================================================================
# AVRDUDE Settings
#=======================================================================

# custom avrdude command
#AVRDUDE = sudo avrdude
#AVRDUDE_PORT = usb
#AVRDUDE_PROGRAMMER = dragon_jtag


#=======================================================================
# Make Configurations: Hide Output and use compile colors
#=======================================================================
# comment to deactivate

MAKEFILE_CONF_HIDE_OUTPUT = 1
MAKEFILE_CONF_USE_COMPILE_COLORS = 1

#=======================================================================
# RULES
#=======================================================================

# make all makes the project
all: $(CONTIKI_PROJECT)

# Contiki main folder and makefile
CONTIKI = ../..
include $(CONTIKI)/Makefile.include

#=======================================================================
# RULES: make u | make eu | make program
#=======================================================================

# shortcuts to download the project to the microcontroller board
u: $(CONTIKI_PROJECT).$(TARGET).u
.PHONY: u
eu: $(CONTIKI_PROJECT).$(TARGET).eu
.PHONY: eu
program: u
.PHONY: program

# setup rs232 port in linux with cat
# use: make cat TTY=/dev/ttyUSB0 BAUD=9600
# or change default values and use: make cat
ifndef $(TTY)
TTY = /dev/ttyUSB0
endif
ifndef $(BAUD)
BAUD = 9600
endif
cat:
	@echo "TTY: $(TTY)    BAUD: $(BAUD)"
	@stty -F $(TTY) raw
	@stty -F $(TTY) -echo -echoe -echok
	@stty -F $(TTY) $(BAUD)
	@cat $(TTY)
.PHONY: cat
