/*
 * Copyright (c) 2012, HTWDD, Angelos Drossos.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

#include "rs232_project.h"

#include <stdlib.h>
#include <stdio.h>

// include cpu/avr rs232.h
#include "dev/rs232.h"

// define RS232 project port
#ifndef RS232_PROJECT_PORT
#define RS232_PROJECT_PORT  RS232_PORT_0
#endif

void rs232_project_init(int (*rs232_project_getchar)(unsigned char))
{
	rs232_init(RS232_PROJECT_PORT, USART_BAUD_9600,
			USART_DATA_BITS_8 | USART_STOP_BITS_1 | USART_MODE_ASYNC);
	rs232_double_transmission_speed(RS232_PROJECT_PORT, 1);
	// rs232_redirect_stdout(RS232_PROJECT_PORT);
	rs232_set_input(RS232_PROJECT_PORT, rs232_project_getchar);
	// rs232_print(RS232_PROJECT_PORT, "\nrs232 project inited!\n");
}

static int rs232_project_putc(char c, FILE *stream)
{
	rs232_send(RS232_PROJECT_PORT, c);
	return c;
}

static FILE rs232_project = FDEV_SETUP_STREAM(rs232_project_putc,
		NULL,
		_FDEV_SETUP_WRITE);

FILE *rs232_project_stream = &rs232_project;

