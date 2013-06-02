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

/**
 * \file
 *         A simple Contiki application communicating with some sensors.
 * \author
 *         Angelos Drossos <angelos.drossos@gmail.com>
 */

// Debugging
//#define SN_SENSORANSTEUERUNG_DEBUG
#ifdef SN_SENSORANSTEUERUNG_DEBUG
	#warning "SN-Sensoransteurerung Process Debug is activated!"
#endif

// some standard library functions
#include <stdio.h> /* For printf() */

// some avr libc functions
#include <util/delay.h>
#include <avr/io.h>

// include contiki header
#include "contiki.h"

// project processes and functions
#include "test-sn.h"
#include "sensor-control.h"
#include "rs232_project.h"

// name of this process for debugging
static const char *const pIdent = "[sn_sensoransteuerung] ";

static void pollhandler();
static void exithandler();

/*---------------------------------------------------------------------------*/

// main project process
PROCESS(sn_sensoransteuerung_process, "[Project] Sensoransteuerung process");

// autostart process list
AUTOSTART_PROCESSES(
	&sn_sensoransteuerung_process
	);

/*---------------------------------------------------------------------------*/

#include "rs232_project.h"

int rs232_project_test(unsigned char c)
{
	// do something
	printf("rs232 project: %c was received.\n", c);
	return c;
}

PROCESS_THREAD(sn_sensoransteuerung_process, ev, data)
{
	PROCESS_POLLHANDLER(pollhandler())
	PROCESS_EXITHANDLER(exithandler())
	
	PROCESS_BEGIN();
	
	printf(".. %s", pIdent);
	printf("here is the process sn_sensoransteuerung.\n");

	// init the rs232 connection
	//rs232_project_init(rs232_project_test);
	//fprintf(rs232_project_stream, "rs232 project connection test...\n");

	printf(".. %s", pIdent);
	printf("Start sensor control process..\n");
	process_start(&sensor_control_process, NULL);

	static int k = 0;
	while (1)
	{
		#ifdef SN_SENSORANSTEUERUNG_DEBUG
		printf("__ %s", pIdent);
		_delay_ms(300);
		printf("Hello, user.. (Event=0x%X)\n", ev);
		#endif

		PROCESS_PAUSE();

		//fprintf(rs232_project_stream, "rs232 project periodic test...\n");

		#ifdef SN_SENSORANSTEUERUNG_DEBUG
		printf(">> %s", pIdent);
		_delay_ms(300);
		printf("Continue event..\n");
		#endif

		//if (k >= 11) break;
		k = (k+1) % 12;
	}
	// unreachable code
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static void pollhandler()
{
	printf("PH [sn_sensoransteuerung] is polled..\n");
}

static void exithandler()
{
	printf("XH [sn_sensoransteuerung] is exiting.. ---------------------------------\n");
}

