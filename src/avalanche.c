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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <clib/alib_protos.h>

#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/requester.h>
#include <proto/window.h>

#include <classes/requester.h>
#include <classes/window.h>
#include <gadgets/getfile.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <workbench/startup.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "libs.h"
#include "xad.h"

#include "Avalanche_rev.h"

const char *version = VERSTAG;

enum {
	GID_MAIN = 0,
	GID_ARCHIVE,
	GID_DEST,
	GID_LIST,
	GID_EXTRACT,
	GID_LAST
};

enum {
	WID_MAIN = 0,
	WID_LAST
};

enum {
	OID_MAIN = 0,
	OID_REQ,
	OID_LAST
};

/** Menu **/

struct NewMenu menu[] = {
	{NM_TITLE, "Project",           0,  0, 0, 0,}, // 0
	{NM_ITEM,   "Open...",         "O", 0, 0, 0,}, // 0
	{NM_ITEM,   "About...",        "?", 0, 0, 0,}, // 1
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 2
	{NM_ITEM,   "Quit",            "Q", 0, 0, 0,}, // 3

	{NM_TITLE, "Edit",              0,  0, 0, 0,}, // 1
	{NM_ITEM,   "Select all",      "A", 0, 0, 0,}, // 0
	{NM_ITEM,   "Clear Selection", "Z", 0, 0, 0,}, // 1

	{NM_TITLE, "Settings",              0,  0, 0, 0,}, // 2
	{NM_ITEM,   "Save window position", 0, CHECKIT | MENUTOGGLE | CHECKED, 0, 0,}, // 0
	{NM_ITEM,   "Save settings",        0, NM_ITEMDISABLED, 0, 0,}, // 1

	{NM_END,   NULL,        0,  0, 0, 0,},
};

/** Global config **/
static char *dest;
static char *archive = NULL;

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;

static BOOL save_win_posn = TRUE;

struct List lblist;

/** Useful functions **/

char *strdup(const char *s)
{
  size_t len = strlen (s) + 1;
  char *result = (char*) malloc (len);
  if (result == (char*) 0)
  return (char*) 0;
  return (char*) memcpy (result, s, len);
}

ULONG OpenRequesterTags(Object *obj, struct Window *win, ULONG Tag1, ...)
{
	struct orRequest msg[1];
	msg->MethodID = RM_OPENREQ;
	msg->or_Window = win;	/* window OR screen is REQUIRED */
	msg->or_Screen = NULL;
	msg->or_Attrs = (struct TagItem *)&Tag1;

	return(DoMethodA(obj, (Msg)msg));
}


/** Private functions **/
static void show_error(Object *obj, struct Window *win, long code)
{
	char message[100];

	if(code == -1) {
		sprintf(message, "Unable to open xadmaster.library");
	} else {
		sprintf(message, "%s", xad_error(code));
	}

	if(obj) {
		OpenRequesterTags(obj, win, 
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_ERROR, 
			REQ_BodyText, message,
			REQ_GadgetText, "OK", TAG_DONE);
	} else {
		printf("Unable to open requester to show error;\n%s\n", message);
	}
}

static void free_archive_path(void)
{
	if(archive) FreeVec(archive);
	archive = NULL;
	archive_needs_free = FALSE;
}

static void free_dest_path(void)
{
	if(dest) free(dest);
	dest = NULL;
	dest_needs_free = FALSE;
}

static void addlbnode(char *name, LONG *size, void *userdata)
{
	struct Node *node = AllocListBrowserNode(2,
		LBNA_UserData, userdata,
		LBNA_CheckBox, TRUE,
		LBNA_Checked, TRUE,
		LBNA_Column, 0,
			LBNCA_Text, name,
		LBNA_Column, 1,
			LBNCA_Integer, size,
		TAG_DONE);

	AddTail(&lblist, node);
}

static void *getlbnode(struct Node *node)
{
	void *userdata = NULL;
	ULONG checked = FALSE;

	GetListBrowserNodeAttrs(node,
			LBNA_Checked, &checked,
			LBNA_UserData, &userdata,
		TAG_DONE);

	if(checked == FALSE) return NULL;
	return userdata;
}

static void open_archive_req(struct Window *win, struct Gadget *arc_gad, struct Gadget *list_gad)
{
	if(archive_needs_free) free_archive_path();

	DoMethod(arc_gad, GFILE_REQUEST, win);
	GetAttr(GETFILE_FullFile, arc_gad, &archive);

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);
	FreeListBrowserList(&lblist);

	xad_info(archive, addlbnode);

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, &lblist, TAG_DONE);
}

static void modify_all_list(struct Window *win, struct Gadget *list_gad, BOOL select)
{
	struct Node *node;

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	for(node = lblist.lh_Head; node->ln_Succ; node=node->ln_Succ) {
		SetListBrowserNodeAttrs(node, LBNA_Checked, select, TAG_DONE);
	}

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, &lblist, TAG_DONE);
}

static void gui(void)
{
	struct MsgPort *AppPort;
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];

	struct ColumnInfo lbci[3] = {
		{90, "Name", CIF_WEIGHTED | CIF_DRAGGABLE},
		{10, "Size", CIF_WEIGHTED | CIF_DRAGGABLE},
		{-1, NULL, 0}
	};

	NewList(&lblist);
	if(archive) xad_info(archive, addlbnode);

	if ( AppPort = CreateMsgPort() ) {
		/* Create the window object.
		 */
		objects[OID_MAIN] = WindowObj,
			WA_ScreenTitle, VERS,
			WA_Title, VERS,
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, TRUE,
			WINDOW_NewMenu, menu,
			WINDOW_IconifyGadget, TRUE,
			WINDOW_IconTitle, "Avalanche",
			WINDOW_AppPort, AppPort,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_MAIN] = LayoutVObj,
				LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_EvenSize, TRUE,
					LAYOUT_AddChild, gadgets[GID_ARCHIVE] = GetFileObj,
						GA_ID, GID_ARCHIVE,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, "Select Archive",
						GETFILE_FullFile, archive,
						GETFILE_ReadOnly, TRUE,
					End,
					CHILD_Label, LabelObj,
						LABEL_Text, "_Archive",
					LabelEnd,
					LAYOUT_AddChild, gadgets[GID_DEST] = GetFileObj,
						GA_ID, GID_DEST,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, "Select Destination",
						GETFILE_Drawer, dest,
						GETFILE_DoSaveMode, TRUE,
						GETFILE_DrawersOnly, TRUE,
						GETFILE_ReadOnly, TRUE,
					End,
					CHILD_Label, LabelObj,
						LABEL_Text, "_Destination",
					LabelEnd,
					LAYOUT_AddChild, gadgets[GID_LIST] = ListBrowserObj,
						GA_ID, GID_LIST,
						LISTBROWSER_ColumnInfo, &lbci,
						LISTBROWSER_Labels, &lblist,
						LISTBROWSER_ColumnTitles, TRUE,
					ListBrowserEnd,
					LAYOUT_AddChild, gadgets[GID_EXTRACT] = ButtonObj,
						GA_ID, GID_EXTRACT,
						GA_RelVerify, TRUE,
						GA_Text, "E_xtract",
					ButtonEnd,
					CHILD_WeightedHeight, 0,
				LayoutEnd,
			EndGroup,
		EndWindow;

		objects[OID_REQ] = RequesterObj,
			REQ_TitleText, VERS,
		End;

	 	/*  Object creation sucessful?
	 	 */
		if (objects[OID_MAIN]) {
			/*  Open the window.
			 */
			if (windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN])) {

				ULONG wait, signal, app = (1L << AppPort->mp_SigBit);
				ULONG done = FALSE;
				ULONG result;
				UWORD code;
				long ret = 0;

			 	/* Obtain the window wait signal mask.
				 */
				GetAttr(WINDOW_SigMask, objects[OID_MAIN], &signal);

				/* Input Event Loop
				 */
				while (!done) {
					wait = Wait( signal | SIGBREAKF_CTRL_C | app );

					if ( wait & SIGBREAKF_CTRL_C ) {
						done = TRUE;
					} else {
						while ( (result = RA_HandleInput(objects[OID_MAIN], &code) ) != WMHI_LASTMSG ) {
							switch (result & WMHI_CLASSMASK) {
								case WMHI_CLOSEWINDOW:
									windows[WID_MAIN] = NULL;
									done = TRUE;
									break;

								case WMHI_GADGETUP:
									switch (result & WMHI_GADGETMASK) {
										case GID_ARCHIVE:
											open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST]);
										break;
										
										case GID_DEST:
											if(dest_needs_free) free_dest_path();
											DoMethod(gadgets[GID_DEST], GFILE_REQUEST, windows[WID_MAIN]);
											GetAttr(GETFILE_Drawer, gadgets[GID_DEST], &dest);
										break;

										case GID_EXTRACT:
											if(archive && dest) {
												SetWindowPointer(windows[WID_MAIN],
													WA_BusyPointer, TRUE,
													TAG_DONE);
												ret = xad_extract(archive, dest, &lblist, getlbnode);
												SetWindowPointer(windows[WID_MAIN],
													WA_BusyPointer, FALSE,
													TAG_DONE);

												if(ret != 0) show_error(objects[OID_REQ],
																 windows[WID_MAIN], ret);
											}
										break;
									}
									break;

								case WMHI_ICONIFY:
									RA_Iconify(objects[OID_MAIN]);
									windows[WID_MAIN] = NULL;
									break;

								case WMHI_UNICONIFY:
									windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN]);

									if (windows[WID_MAIN]) {
										GetAttr(WINDOW_SigMask, objects[OID_MAIN], &signal);
									} else {
										done = TRUE;	// error re-opening window!
									}
							 	break;
								 	
								 case WMHI_MENUPICK:
									while((code != MENUNULL) && (done == FALSE)) {
										struct MenuItem *item = ItemAddress(windows[WID_MAIN]->MenuStrip, code);

										switch(MENUNUM(code)) {
											case 0: //project
												switch(ITEMNUM(code)) {
													case 0: //open
														open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST]);
													break;
												
													case 1: //about
														OpenRequesterTags(objects[OID_REQ], windows[WID_MAIN], 
															REQ_Type, REQTYPE_INFO,
															REQ_Image, REQIMAGE_INFO,
															REQ_BodyText, VERS "\nhttps://github.com/chris-y/avalanche",
															REQ_GadgetText, "OK", TAG_DONE);
													break;
												
													case 3: //quit
														done = TRUE;
													break;
												}
											break;
										
											case 1: //edit
												switch(ITEMNUM(code)) {
													case 0: //select all
														modify_all_list(windows[WID_MAIN], gadgets[GID_LIST], TRUE);
													break;
												
													case 1: //clear selection
														modify_all_list(windows[WID_MAIN], gadgets[GID_LIST], FALSE);
													break;
												}
											break;
										
											case 2: //settings
												switch(ITEMNUM(code)) {
													case 0: //save window position
														save_win_posn = !save_win_posn;
														printf("%d\n", save_win_posn);
													break;
												
													case 1: //save settings
													break;
												}
											break;				
										}
										code = item->NextSelect;
									}
								break; //WMHI_MENUPICK
							}
						}
					}
				}
			}

			DisposeObject(objects[OID_MAIN]);
			DisposeObject(objects[OID_REQ]);
		}

		DeleteMsgPort(AppPort);
	}
}

static void gettooltypes(UBYTE **tooltypes)
{
	char *s;
	
	dest = strdup(ArgString(tooltypes, "DEST", "RAM:"));
	dest_needs_free = TRUE;
}

/** Main program **/
int main(int argc, char **argv)
{
	if(libs_open() == FALSE) {
		return 10;
	}

	if(argc == 0) {
		/* Started from WB */
		struct WBStartup *WBenchMsg = (struct WBStartup *)argv;
		struct WBArg *wbarg = NULL;
 
		if(WBenchMsg->sm_NumArgs > 0) {
			/* Started as default tool, get the path+filename of the (first) project */
			wbarg = WBenchMsg->sm_ArgList + 1;

            if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
				if(archive = AllocVec(512, MEMF_CLEAR)) {
					archive_needs_free = TRUE;
					NameFromLock(wbarg->wa_Lock, archive, 512);
					AddPart(archive, wbarg->wa_Name, 512);
				}
			} 
        }
	}
	
	UBYTE **tooltypes = ArgArrayInit(argc, argv);
	gettooltypes(tooltypes);
	ArgArrayDone();

	gui();

	if(archive_needs_free) free_archive_path();
	if(dest_needs_free) free_dest_path();
	
	FreeListBrowserList(&lblist);
	xad_exit();
	libs_close();

	return 0;
}
