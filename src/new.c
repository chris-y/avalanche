/* Avalanche
 * (c) 2023 Chris Young
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
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>

#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif
#include <libraries/asl.h>

#include <proto/button.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "new.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
#include "module.h"
#include "win.h"

#include "Avalanche_rev.h"

enum {
	GID_N_MAIN = 0,
	GID_N_ARCHIVE,
	GID_N_TYPE,
	GID_N_CREATE,
	GID_N_CANCEL,
	GID_N_LAST
};

enum {
	WID_MAIN = 0,
	WID_LAST
};

enum {
	OID_MAIN = 0,
	OID_LAST
};

STRPTR type_opts[] = {
		"LhA",
#ifdef __amigaos4__
		"Zip",
#endif
		NULL
};

static struct Window *windows[WID_LAST];
static struct Gadget *gadgets[GID_N_LAST];
static Object *objects[OID_LAST];
static struct MsgPort *naw_port = NULL;
static void *newarc_parent = NULL;
#ifndef __amigaos4__
static struct HintInfo hi;
#endif


static void newarc_window_close(void)
{
	RA_CloseWindow(objects[OID_MAIN]);
	windows[WID_MAIN] = NULL;
	DisposeObject(objects[OID_MAIN]);

	DeleteMsgPort(naw_port);
	naw_port = NULL;

	newarc_parent = NULL;
}

static void newarc_req_archive(void)
{
	ULONG res;

	if(res = DoMethod((Object *)gadgets[GID_N_ARCHIVE], GFILE_REQUEST, windows[WID_MAIN])) {
		SetGadgetAttrs(gadgets[GID_N_CREATE], windows[WID_MAIN], NULL,
			GA_Disabled, FALSE,
		TAG_DONE);
	}
}

static char *newarc_add_ext(const char *archive, const char *ext)
{
	ULONG max_arc_size = strlen(archive) + strlen(ext) + 2;
	char *arc_corrected = AllocVec(max_arc_size, MEMF_CLEAR);
	char *dot = strrchr(archive, '.');

	if(arc_corrected) {
		if(((dot != NULL) && (strcmp(dot + 1, ext) != 0)) || (dot == NULL)) {
			snprintf(arc_corrected, max_arc_size, "%s.%s", archive, ext);
		} else {
			strcpy(arc_corrected, archive);
		}
	} else {
		return NULL;
	}
	
	return arc_corrected;
}

static void newarc_create(void)
{
	BOOL ret = FALSE;
	ULONG data;
	char *arc_type;
	char *archive, *arc_r;

	GetAttr(CHOOSER_Selected, gadgets[GID_N_TYPE], (ULONG *)&data);
	arc_type = type_opts[data];
	
	GetAttr(GETFILE_FullFile, gadgets[GID_N_ARCHIVE], (APTR)&archive);
	
	if(archive == NULL) return;

	switch(data) {
		case 0: // LhA
			if(arc_r = newarc_add_ext(archive, "lha")) {
				ret = mod_lha_new(newarc_parent, arc_r);
			} else {
				/* Use the original name */
				ret = mod_lha_new(newarc_parent, archive);
			}
		break;

		case 1: // Zip
			if(arc_r = newarc_add_ext(archive, "zip")) {
				ret = mod_zip_new(newarc_parent, arc_r);
			} else {
				/* Use the original name */
				ret = mod_zip_new(newarc_parent, archive);
			}
		break;
	}

	if(ret) {
		if(arc_r) {
			window_update_archive(newarc_parent, arc_r);
			FreeVec(arc_r);
		} else {
			window_update_archive(newarc_parent, archive);
		}
		window_req_open_archive(newarc_parent, get_config(), TRUE);
	}

}

/* Public functions */
void newarc_window_open(void *awin)
{
	if(windows[WID_MAIN]) { // already open
		WindowToFront(windows[WID_MAIN]);
		return;
	}

	if(naw_port = CreateMsgPort()) {
		/* Create the window object */
		objects[OID_MAIN] = WindowObj,
			WA_ScreenTitle, VERS,
			WA_Title, VERS,
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, FALSE,
			/* Enable HintInfo */
			WINDOW_GadgetHelp, TRUE,
#ifndef __amigaos4__
			WINDOW_HintInfo, &hi,
#endif
			WINDOW_SharedPort, naw_port,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_N_MAIN] = LayoutVObj,
				//LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild,  LayoutVObj,
						LAYOUT_AddChild,  gadgets[GID_N_ARCHIVE] = GetFileObj,
							GA_ID, GID_N_ARCHIVE,
							HINTINFO, locale_get_string(MSG_HI_N_ARCHIVE),
							GA_RelVerify, TRUE,
							GETFILE_TitleText,  locale_get_string( MSG_SELECTARCHIVE ) ,
							GETFILE_DoSaveMode, TRUE,
							GETFILE_DrawersOnly, FALSE,
							GETFILE_ReadOnly, TRUE,
						End,
						CHILD_WeightedHeight, 0,
						CHILD_Label, LabelObj,
							LABEL_Text,  locale_get_string( MSG_ARCHIVE ) ,
						LabelEnd,

						LAYOUT_AddChild,  gadgets[GID_N_TYPE] = ChooserObj,
							GA_ID, GID_N_TYPE,
							HINTINFO, locale_get_string(MSG_HI_N_TYPE),
							GA_RelVerify, TRUE,
							GA_Selected, 0,
							CHOOSER_PopUp, TRUE,
							CHOOSER_LabelArray, type_opts,
						End,
						CHILD_Label, LabelObj,
							LABEL_Text,  locale_get_string(MSG_TYPE),
						LabelEnd,
					LayoutEnd,
					LAYOUT_AddChild,  LayoutHObj,
						LAYOUT_AddChild,  gadgets[GID_N_CREATE] = ButtonObj,
							GA_ID, GID_N_CREATE,
							HINTINFO, locale_get_string(MSG_HI_N_CREATE),
							GA_RelVerify, TRUE,
							GA_Text, locale_get_string( MSG_CREATE ),
							GA_Disabled, TRUE,
						ButtonEnd,
						LAYOUT_AddChild,  gadgets[GID_N_CANCEL] = ButtonObj,
							GA_ID, GID_N_CANCEL,
							HINTINFO, locale_get_string(MSG_HI_N_CANCEL),
							GA_RelVerify, TRUE,
							GA_Text, locale_get_string( MSG_CANCEL ),
						ButtonEnd,
						CHILD_WeightedHeight, 0,
					LayoutEnd,
				LayoutEnd,
			EndGroup,
		EndWindow;

		if(objects[OID_MAIN]) {
			windows[WID_MAIN] = (struct Window *)RA_OpenWindow(objects[OID_MAIN]);

			newarc_parent = awin;
		}
	}

	return;
}

ULONG newarc_window_get_signal(void)
{
	if(naw_port) {
		return (1L << naw_port->mp_SigBit);
	} else {
		return 0;
	}
}

ULONG newarc_window_handle_input(UWORD *code)
{
	return RA_HandleInput(objects[OID_MAIN], code);
}

BOOL newarc_window_handle_input_events(ULONG result, UWORD code)
{
	long ret = 0;
	ULONG done = FALSE;

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
			newarc_window_close();
			done = TRUE;
		break;

		case WMHI_GADGETUP:
			switch (result & WMHI_GADGETMASK) {
				case GID_N_ARCHIVE:
					newarc_req_archive();
				break;
				case GID_N_CREATE:
					newarc_create();
					newarc_window_close();
					done = TRUE;
				break;
				case GID_N_CANCEL:
					newarc_window_close();
					done = TRUE;
				break;
			}
			break;
	}

	return done;
}

void newarc_window_close_if_associated(void *awin)
{
	if(awin == newarc_parent) newarc_window_close();
}
