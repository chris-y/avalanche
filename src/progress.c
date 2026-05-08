/* Avalanche
 * (c) 2022-6 Chris Young
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

#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#ifdef __amigaos4__
#include <intuition/gui.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#else
#include <gadgets/fuelgauge.h>
#endif

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "locale.h"
#include "progress.h"

#define PROGRESS_MSG_SIZE 50
static char progress_msg[PROGRESS_MSG_SIZE+1];

void progress_set_text(struct Window *win, void *gauge, void *frame, ULONG current, ULONG total)
{
	snprintf(progress_msg, PROGRESS_MSG_SIZE, "%d/%lu", current, total);

#ifdef __amigaos4__
	RefreshSetGadgetAttrs(frame,
		win, NULL,
		GA_Text, progress_msg,
		GA_Image, gauge,
		TAG_DONE);
		
	RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			GA_Text, progress_msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);
#endif

}

void progress_set_scanning(struct Window *win, void *gauge, void *frame, ULONG total)
{
	if(total != 0) {
		snprintf(progress_msg, PROGRESS_MSG_SIZE, "%s %lu", locale_get_string(MSG_SCANNING), total);
	} else {
		snprintf(progress_msg, PROGRESS_MSG_SIZE, "%s", locale_get_string(MSG_SCANNING));
	}
	
#ifdef __amigaos4__
	RefreshSetGadgetAttrs(frame,
		win, NULL,
		GA_Text, progress_msg,
		GA_Image, gauge,
		TAG_DONE);

	RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
		GA_Text, progress_msg,
		FUELGAUGE_Percent, FALSE,
		FUELGAUGE_Justification, FGJ_CENTER,
		FUELGAUGE_Level, 0,
		TAG_DONE);
#endif

}

void progress_set_level(struct Window *win, void *gauge, void *frame, ULONG level, ULONG max)
{
#ifdef __amigaos4__
	SetGadgetAttrs(frame,
		win, NULL,
		GA_Image, NULL,
		TAG_DONE);

	SetAttrs((Object *)gauge,
			GAUGEIA_Level, level,
			GAUGEIA_Max, max,
			TAG_DONE);

	RefreshSetGadgetAttrs(frame,
		win, NULL,
		GA_Image, gauge,
		TAG_DONE);
		
	RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			FUELGAUGE_Max, max,
			FUELGAUGE_Level, level,
			TAG_DONE);
#endif

}


