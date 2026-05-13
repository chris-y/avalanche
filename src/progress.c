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

#include <proto/dos.h> /* FilePart() */
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

#define PROGRESS_MSG_SIZE 100
static char progress_msg[PROGRESS_MSG_SIZE+1];

#ifdef __amigaos4__
void progress_get_area(struct Window *win, ULONG *width, ULONG *height, ULONG *sz_height)
{
	ULONG sz_gad_width = 0;
	ULONG sz_gad_height = 0;

	struct Screen *scrn = LockPubScreen(NULL);
	struct DrawInfo *dri = GetScreenDrawInfo(scrn);

	GetGUIAttrs(NULL, dri,
		GUIA_SizeGadgetWidth, &sz_gad_width,
		GUIA_SizeGadgetHeight, &sz_gad_height,
		TAG_DONE);

	*width = win->Width - scrn->WBorLeft - sz_gad_width;
	*height = sz_gad_height - scrn->WBorBottom;
	*sz_height = sz_gad_height;

	FreeScreenDrawInfo(scrn, dri);
	UnlockPubScreen(NULL, scrn);
}

void progress_set_new_width(struct Window *win, void *frame, void *gauge)
{
	ULONG width, height, sz_height;

	progress_get_area(win, &width, &height, &sz_height);

	SetAttrs((Object *)gauge,
		IA_Width, width,
		TAG_DONE);

	SetGadgetAttrs(frame,
		win, NULL,
		GA_Width, width,
		TAG_DONE);
		
	RefreshGList((struct Gadget *)frame, win, NULL, 1);
}
#endif

void progress_set_file_level(struct Window *win, void *gauge, void *frame, ULONG level, ULONG max, const char *filename)
{
	ULONG percent = (level * 100) / max;
	char *fn = filename;
	if(fn == NULL) fn = "";
	if(strlen(fn) > 50) fn = FilePart(fn);
	
	snprintf(progress_msg, PROGRESS_MSG_SIZE, locale_get_string(MSG_EXTRACTING_FILE), fn, percent);

#ifdef __amigaos4__
	SetGadgetAttrs(frame,
		win, NULL,
		GA_Text, progress_msg,
		GA_Image, gauge,
		TAG_DONE);

	RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
			GA_Text, progress_msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_LEFT,
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
	SetGadgetAttrs(frame,
		win, NULL,
		GA_Text, progress_msg,
		GA_Image, gauge,
		TAG_DONE);

	RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
	SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
		GA_Text, progress_msg,
		FUELGAUGE_Percent, FALSE,
		FUELGAUGE_Justification, FGJ_LEFT,
		FUELGAUGE_Level, 0,
		TAG_DONE);
#endif
}

void progress_set_selected(struct Window *win, void *gauge, void *frame, ULONG selected, ULONG total)
{
	snprintf(progress_msg, PROGRESS_MSG_SIZE, locale_get_string(MSG_SELECTED), selected, total);

#ifdef __amigaos4__
		SetAttrs((Object *)gauge,
				GAUGEIA_Level, 0,
				TAG_DONE);

        SetGadgetAttrs(frame,
                win, NULL,
                GA_Text, progress_msg,
                GA_Image, gauge,
                TAG_DONE);

        RefreshGList((struct Gadget *)frame, win, NULL, 1);
#else
        SetGadgetAttrs((struct Gadget *)gauge, win, NULL,
                GA_Text, progress_msg,
                FUELGAUGE_Percent, FALSE,
                FUELGAUGE_Justification, FGJ_LEFT,
                FUELGAUGE_Level, 0,
                TAG_DONE);
#endif
}

void progress_set_archive_level(struct Window *win, void *gauge, void *frame, ULONG level, ULONG max)
{
#ifdef __amigaos4__
	SetAttrs((Object *)gauge,
			GAUGEIA_Level, level,
			GAUGEIA_Max, max,
			TAG_DONE);

	SetGadgetAttrs(frame,
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


