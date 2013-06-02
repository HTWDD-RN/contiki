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
 * This file is not part of the Contiki operating system.
 *
 * $Id: sn-sensoransteuerung.c,v 0.1 2012/05/04 22:38:00 Angelos Drossos $
 */

/**
 * \file
 *         A simple Contiki application communicating with some sensors.
 * \author
 *         Angelos Drossos <angelos.drossos@informatik.htw-dresden.de>
 */

#include <stdio.h> /* For printf() */

#include "contiki.h"

#include "test-sn.h"

static int pollhandler(int sn, int ns);
static void exithandler();

/*---------------------------------------------------------------------------*/

PROCESS(test_sn_process, "[SN] Test process");
// Es darf nur ein AUTOSTART_PROCESSES __im Projekt__ geben!
// Alle weiteren Deklarationen werden ignoriert. Es gibt keinen Fehler..

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_sn_process, ev, data)
{
	static int sn1 = 0;
	static int sn2 = 0;
	static int sn3 = 0;
	PROCESS_POLLHANDLER(sn2 = pollhandler(sn1, sn3))
	PROCESS_EXITHANDLER(exithandler())
	PROCESS_BEGIN();

	printf("[test_sn] "
			"here is the process test_sn.\n");

	sn1 = 0;
	sn2 = 0;
	
	while (1)
	{
		printf("[test_sn] Hello, user.. (Event=0x%X, sn=%d, sn2=%d, sn3=%d)\n",
						ev, sn1, sn2, sn3);
		sn1++;
		sn3++;
		PROCESS_WAIT_EVENT();

		printf("[test_sn] Hello, again.. (Event=0x%X, sn=%d, sn2=%d, sn3=%d)\n",
						ev, sn1, sn2, sn3);
		sn1++;
		sn3++;
		PROCESS_WAIT_EVENT();
	}
	
	printf("[test_sn] Bye bye, user! ===========================\n");

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static int pollhandler(int sn, int ns)
{
	printf("[test_sn] is polled (sn=%d,ns=%d)..\n", sn, ns);
	return ((sn+1) % 5);
}

static void exithandler()
{
	printf("[test_sn] is exiting.. ---------------------------------\n");
}

