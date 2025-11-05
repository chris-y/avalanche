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

#include <stdio.h>
#include <string.h>

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
#include <images/glyph.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "Avalanche_rev.h"

#include "arexx.h"
#include "config.h"
#include "http.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
#include "req.h"
#include "update.h"
#include "win.h"

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

static Object *objects[OID_U_LAST];
static struct MsgPort *uw_port = NULL;
static struct Window *windows[WID_U_LAST];
static struct Gadget *gadgets[GID_U_LAST];
static struct ColumnInfo *ci;
static struct List list;
#ifndef __amigaos4__
static struct HintInfo hi;
#endif

/* returns FALSE on error */
static BOOL update_update(struct avalanche_version_numbers *vn)
{
	char message[101];
	void *sslctx;

	snprintf(message, 100, locale_get_string(MSG_NEWVERSIONDL), vn->latest_version, vn->latest_revision);
	if(ask_yesno_req(NULL, message)) {
		// download
		APTR buffer = AllocVec(4096, MEMF_PRIVATE);
		if(buffer == NULL) return FALSE;
		char *tmpdir = CONFIG_GET_LOCK(tmpdir);
		ULONG fn_len = strlen(tmpdir) + strlen(FilePart(vn->download_url)) + 5;
		char *dl_filename = AllocVec(fn_len, MEMF_PRIVATE);
		if(dl_filename == NULL) {
			CONFIG_UNLOCK;
			FreeVec(buffer);
			return FALSE;
		}

		strncpy(dl_filename, tmpdir, fn_len);
		CONFIG_UNLOCK;
		AddPart(dl_filename, FilePart(vn->download_url), fn_len);
		BPTR fh = Open(dl_filename, MODE_NEWFILE);
		if(fh) {
			sslctx = http_open_socket_libs();
			BOOL ok = http_get_url(vn->download_url, sslctx, buffer, 4096, fh);
			http_ssl_free_ctx(sslctx);
			Close(fh);

			if(ok) {

				/* We are running as a separate process, pass a message to the parent using ARexx */
				ULONG cmd_len = fn_len + 22;
				char *cmd = AllocVec(cmd_len, MEMF_PRIVATE);
				if(cmd) {
					snprintf(cmd, cmd_len, "OPEN \"%s\" DELETEONCLOSE", dl_filename);
					ami_arexx_send(cmd);
					FreeVec(cmd);
				} else {
					open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), NULL);
				}
			} else {
				open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), NULL);
			}
		} else {
			open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), NULL);
		}

		if(dl_filename) FreeVec(dl_filename);
		if(buffer) FreeVec(buffer);
	}
	return TRUE;
}

void update_close(void)
{
	if(objects[OID_U_MAIN] == NULL) return;
	
	RA_CloseWindow(objects[OID_U_MAIN]);
	windows[WID_U_MAIN] = NULL;
	DisposeObject(objects[OID_U_MAIN]);
	objects[OID_U_MAIN] = NULL;

	FreeLBColumnInfo(ci);
	FreeListBrowserList(&list);

	DeleteMsgPort(uw_port);
	uw_port = NULL;
}

ULONG update_get_signal(void)
{
	if(uw_port == NULL) return 0;
		else return(1L << uw_port->mp_SigBit);
}

BOOL update_to_front(void)
{
	if(windows[WID_U_MAIN] == NULL) return FALSE;

	WindowToFront(windows[WID_U_MAIN]);
	return TRUE;
}

BOOL update_handle_events(void)
{
	if(objects[OID_U_MAIN] == NULL) return FALSE;
	
	UWORD code;
	ULONG result;
	BOOL done = FALSE;

	while(((result = RA_HandleInput(objects[OID_U_MAIN], &code)) != WMHI_LASTMSG) && (done == FALSE)) {

		switch (result & WMHI_CLASSMASK) {
			case WMHI_CLOSEWINDOW:
				done = TRUE;
			break;

			case WMHI_GADGETUP:
				switch (result & WMHI_GADGETMASK) {
					ULONG list_event = 0;
					struct Node *node = NULL;
					struct avalanche_version_numbers *vn = NULL;
					case GID_U_LIST:
						GetAttr(LISTBROWSER_RelEvent, gadgets[GID_U_LIST], (APTR)&list_event);
						switch(list_event) {
							case LBRE_DOUBLECLICK:
								GetAttr(LISTBROWSER_SelectedNode, gadgets[GID_U_LIST], (APTR)&node);
								GetListBrowserNodeAttrs(node, LBNA_UserData, (APTR)&vn, TAG_DONE);

								if(vn->update_available) {
									SetWindowPointer(windows[WID_U_MAIN],
												WA_BusyPointer, TRUE,
												TAG_DONE);
									if(update_update(vn) == FALSE) {
										open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), NULL);
									}
									SetWindowPointer(windows[WID_U_MAIN],
												WA_BusyPointer, FALSE,
												TAG_DONE);
								}
							break;
						}
					break;
				}
			break;
		}
	}
	
	return done;
	
}

void update_gui(struct avalanche_version_numbers avn[])
{
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
				LBCIA_Title,  locale_get_string(MSG_INSTALLED_VER),
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			LBCIA_Column, 2,
				LBCIA_Title,  locale_get_string(MSG_LATEST_VER),
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			LBCIA_Column, 3,
				LBCIA_Title,  locale_get_string(MSG_UPDATE_AVAILABLE),
				LBCIA_Weight, 30,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Sortable, FALSE,
			TAG_DONE);

		NewList(&list);

		for(int i = 0; i < ACHECKVER_MAX; i++) {
			char installed_version[10];
			char latest_version[10];
			ULONG update_glyph = avn[i].update_available ? GLYPH_CHECKMARK : AVALANCHE_GLYPH_NONE;
			if((avn[i].current_version == 0) && (avn[i].current_revision == 0)) {
				strncpy(installed_version, locale_get_string(MSG_NONE), 9);
				installed_version[9] = '\0';
			} else {
				snprintf(installed_version, 9, "%d.%d", avn[i].current_version, avn[i].current_revision);
			}

			if((avn[i].latest_version == 0) && (avn[i].latest_revision == 0)) {
				strncpy(latest_version, locale_get_string(MSG_NONE), 9);
				latest_version[9] = '\0';
			} else {
				snprintf(latest_version, 9, "%d.%d", avn[i].latest_version, avn[i].latest_revision);
			}
			struct Node *node = AllocListBrowserNode(4,
				LBNA_UserData, &avn[i],
				LBNA_Column, 0,
					LBNCA_Text, avn[i].name,
				LBNA_Column, 1,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, installed_version,
				LBNA_Column, 2,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, latest_version,
				LBNA_Column, 3,
					LBNCA_Image, get_glyph(update_glyph),
				TAG_DONE);

			AddTail(&list, node);
		}

		/* Create the window object */
		objects[OID_U_MAIN] = WindowObj,
			WA_ScreenTitle, VERS,
			WA_Title, VERS,
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, TRUE,
			/* Enable HintInfo */
			WINDOW_GadgetHelp, TRUE,
#ifndef __amigaos4__
			WINDOW_HintInfo, &hi,
#endif
			WINDOW_SharedPort, uw_port,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_U_MAIN] = LayoutVObj,
				//LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild, gadgets[GID_U_LIST] = ListBrowserObj,
						GA_ID, GID_U_LIST,
						HINTINFO, locale_get_string(MSG_UPDATE_INFO),
						GA_RelVerify, TRUE,
						LISTBROWSER_AutoFit, TRUE,
						LISTBROWSER_ColumnInfo, ci,
						LISTBROWSER_Labels, &list,
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_TitleClickable, TRUE,
						LISTBROWSER_SortColumn, 0,
						LISTBROWSER_Striping, LBS_ROWS,
						LISTBROWSER_FastRender, TRUE,
						LISTBROWSER_HorizontalProp, TRUE,
					ListBrowserEnd,
					LAYOUT_AddImage, LabelObj,
						LABEL_Text, locale_get_string(MSG_UPDATE_INFO),
					LabelEnd,
				LayoutEnd,
			LayoutEnd,
		EndWindow;

		if(objects[OID_U_MAIN]) {
			windows[WID_U_MAIN] = (struct Window *)RA_OpenWindow(objects[OID_U_MAIN]);
		}

#ifndef __amigaos4__
		return;
#endif

		ULONG sigbit = update_get_signal();
		BOOL done = FALSE;

		if(windows[WID_U_MAIN] == NULL) done = TRUE;

		while(!done) {
			ULONG wait = Wait(sigbit | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

			if(wait & sigbit) {
				done = update_handle_events();

			} else if(wait & SIGBREAKF_CTRL_C) {
				done = TRUE;
			} else if(wait & SIGBREAKF_CTRL_F) {
				update_to_front();
			}
		}

		update_close();
	}
}

void update_break(void)
{
	struct Process *check_ver_proc = http_get_process_check_version();
	if(check_ver_proc == NULL) return;
	Signal(check_ver_proc, SIGBREAKF_CTRL_C);
}
