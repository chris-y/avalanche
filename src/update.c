/* Avalanche
 * (c) 2025 Chris Young
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

#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "Avalanche_rev.h"

#include "libs.h"
#include "locale.h"
#include "update.h"

enum {
	GID_U_MAIN = 0,
	GID_U_LIST,
	GID_U_LAST
};

enum {
	WID_U_MAIN = 0,
	WID_U_LAST
};

enum {
	OID_U_MAIN = 0,
	OID_U_LAST
};




void update_gui(struct avalanche_version_numbers avn[], void *ssl_ctx)
{
	for(int i = 0; i < ACHECKVER_MAX; i++) {
#ifdef __amigaos4__
		DebugPrintF("%d: Current: %d.%d, New: %d.%d\n", i, avn[i].current_version, avn[i].current_revision, avn[i].latest_version, avn[i].latest_revision);
#endif
	}

	struct Window *windows[WID_U_LAST];
	struct Gadget *gadgets[GID_U_LAST];
	Object *objects[OID_U_LAST];
	struct MsgPort *uw_port;
	struct ColumnInfo *ci;
	struct List list;

	if(uw_port = CreateMsgPort()) {

		ci = AllocLBColumnInfo(4, 
			LBCIA_Column, 0,
				LBCIA_Title, locale_get_string(MSG_NAME),
				LBCIA_Weight, 50,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, TRUE,
				LBCIA_SortArrow, TRUE,
				LBCIA_AutoSort, TRUE,
			LBCIA_Column, 1,
				LBCIA_Title,  locale_get_string(MSG_NAME),
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			LBCIA_Column, 2,
				LBCIA_Title,  locale_get_string(MSG_NAME),
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			LBCIA_Column, 3,
				LBCIA_Title,  locale_get_string(MSG_NAME),
				LBCIA_Weight, 30,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			TAG_DONE);

		NewList(&list);

		/* Create the window object */
		objects[OID_U_MAIN] = WindowObj,
			WA_ScreenTitle, VERS,
			WA_Title, VERS,
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, TRUE,
			WINDOW_SharedPort, uw_port,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_U_MAIN] = LayoutVObj,
				//LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild, gadgets[GID_U_LIST] = ListBrowserObj,
						GA_ID, GID_U_LIST,
						GA_RelVerify, TRUE,
						LISTBROWSER_ColumnInfo, ci,
						LISTBROWSER_Labels, &list,
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_TitleClickable, TRUE,
						LISTBROWSER_SortColumn, 1,
						LISTBROWSER_Striping, LBS_ROWS,
						LISTBROWSER_FastRender, TRUE,
					ListBrowserEnd,
				LayoutEnd,
			LayoutEnd,
		EndWindow;
				
		if(objects[OID_U_MAIN]) {
			windows[WID_U_MAIN] = (struct Window *)RA_OpenWindow(objects[OID_U_MAIN]);
		}
	}
	
	
	ULONG sigbit = 1L << uw_port->mp_SigBit;
	BOOL done = FALSE;
	
	while(!done) {
		ULONG wait = Wait(sigbit | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
		
		if(wait & sigbit) {
			UWORD code;
			
			ULONG result = RA_HandleInput(objects[OID_U_MAIN], &code);
			
			switch (result & WMHI_CLASSMASK) {
				case WMHI_CLOSEWINDOW:
					done = TRUE;
				break;
			}
		}
	}
	
	RA_CloseWindow(objects[OID_U_MAIN]);
	windows[WID_U_MAIN] = NULL;
	DisposeObject(objects[OID_U_MAIN]);

	DeleteMsgPort(uw_port);
	uw_port = NULL;
	
		
}
