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
#else
#include <gadgets/fuelgauge.h>
#endif

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "locale.h"
#include "progress.h"

void progress_set_text(struct Window *win, void *gauge, ULONG current, ULONG total)
{
	char msg[20];
	snprintf(msg, 19, "%d/%lu", current, total);

#ifdef __amigaos4__
	RefreshSetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			IA_Label, msg,
			IA_Justification, IJ_CENTER,
			GAUGEIA_Level, 0,
			TAG_DONE);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);
#endif

}

void progress_set_scanning(struct Window *win, void *gauge, ULONG total)
{
	ULONG len = strlen(locale_get_string(MSG_SCANNING)) + 10;
	char *msg = AllocVec(len, MEMF_PRIVATE | MEMF_CLEAR);
	if(msg) {
		if(total != 0) {
			snprintf(msg, len, "%s %lu", locale_get_string(MSG_SCANNING), total);
		} else {
			snprintf(msg, len, "%s", locale_get_string(MSG_SCANNING));
		}
		
#ifdef __amigaos4__
		RefreshSetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			IA_Label, msg,
			IA_Justification, IJ_CENTER,
			GAUGEIA_Level, 0,
			TAG_DONE);
			
		RefreshWindowFrame(win);
#else
		SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);
#endif

		FreeVec(msg);
	}


}

void progress_set_level(struct Window *win, void *gauge, ULONG level, ULONG max)
{
#ifdef __amigaos4__
	RefreshSetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			GAUGEIA_Level, level,
			GAUGEIA_Max, max,
			TAG_DONE);
			
	RefreshWindowFrame(win);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			FUELGAUGE_Max, max,
			FUELGAUGE_Level, level,
			TAG_DONE);
#endif

}


