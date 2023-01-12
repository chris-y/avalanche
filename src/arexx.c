/* Avalanche
 * (c) 2022 Chris Young
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <proto/intuition.h>
#include <proto/exec.h>

#include <proto/arexx.h>
#include <classes/arexx.h>

#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif

#include <reaction/reaction_macros.h>

#include "arexx.h"
#include "avalanche.h"
#include "libs.h"
#include "Avalanche_rev.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
	RX_OPEN=0,
	RX_SHOW,
	RX_VERSION,
};

static Object *arexx_obj = NULL;
STATIC char result[100];
static ULONG event = RXEVT_NONE;
static char *event_param = NULL;

#ifdef __amigaos4__
#define RXHOOKF(func) static VOID func(struct ARexxCmd *cmd, struct RexxMsg *rxm __attribute__((unused)))
#else
#ifndef ASM
#define ASM
#endif

#ifndef REG
#define REG(reg,arg) __reg(#reg) arg
#endif

#define IDoMethod DoMethod
#define IDoMethodA DoMethodA

#define RXHOOKF(func) static ASM VOID func(REG(a0, struct ARexxCmd* cmd), REG(a1, struct RexxMsg* msg))
#endif

RXHOOKF(rx_open);
RXHOOKF(rx_show);
RXHOOKF(rx_version);

STATIC struct ARexxCmd Commands[] =
{
	{"OPEN", RX_OPEN, rx_open, "FILE/A", 		0, 	NULL, 	0, 	0, 	NULL },
	{"SHOW", RX_SHOW, rx_show, NULL, 		0, 	NULL, 	0, 	0, 	NULL },
	{"VERSION", RX_VERSION, rx_version, NULL, 		0, 	NULL, 	0, 	0, 	NULL },
	{ NULL, 		0, 				NULL, 		NULL, 		0, 	NULL, 	0, 	0, 	NULL }
};

BOOL ami_arexx_init(ULONG *rxsig)
{
	if((arexx_obj = ARexxObj,
			AREXX_HostName, "AVALANCHE",
			AREXX_Commands, Commands,
			AREXX_NoSlot, TRUE,
			AREXX_ReplyHook, NULL,
			AREXX_DefExtension, "arx",
			End))
	{
		GetAttr(AREXX_SigMask, arexx_obj, rxsig);
		return TRUE;
	}
	else
	{
/* Create a temporary ARexx port so we can send commands to existing */
		arexx_obj = ARexxObj,
			AREXX_HostName, "AVALANCHE",
			AREXX_Commands, Commands,
			AREXX_NoSlot, FALSE,
			AREXX_ReplyHook, NULL,
			AREXX_DefExtension, "arx",
			End;

		return FALSE;
	}
}

ULONG ami_arexx_handle(void)
{
	RA_HandleRexx(arexx_obj);

	return event;
}

static void ami_arexx_command(const char *cmd, const char *port)
{
	if(arexx_obj == NULL) return;
	IDoMethod(arexx_obj, AM_EXECUTE, cmd, port, NULL, NULL, NULL, NULL);
}

void ami_arexx_send(const char *cmd)
{
	ami_arexx_command(cmd, "AVALANCHE");
}

void ami_arexx_cleanup(void)
{
	if(arexx_obj) DisposeObject(arexx_obj);
}

void arexx_free_event(void)
{
	if(event_param) free(event_param);
	event_param = NULL;
	event = RXEVT_NONE;
}

static void arexx_set_event(ULONG evt, char *param)
{
	if(event_param) {
		arexx_free_event();
		event_param = NULL;
	}
	if(param) event_param = strdup(param);
	event = evt;
}

char *arexx_get_event(void)
{
	return event_param;
}


RXHOOKF(rx_open)
{
	cmd->ac_RC = 0;

	arexx_set_event(RXEVT_OPEN, (char *)cmd->ac_ArgList[0]);
}

RXHOOKF(rx_show)
{
	cmd->ac_RC = 0;

	arexx_set_event(RXEVT_SHOW, NULL);
}

RXHOOKF(rx_version)
{
	static char reply[10];
	cmd->ac_RC = 0;
	snprintf(reply, sizeof(reply), "%d.%d", VERSION, REVISION);
	cmd->ac_Result = reply;
}
