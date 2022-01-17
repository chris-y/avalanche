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
	{NM_ITEM,	"Hierarchical browser (experimental)", 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 0
	{NM_ITEM,   "Save window position", 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 1
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 2
	{NM_ITEM,   "Save settings",        0,  0, 0, 0,}, // 3

	{NM_END,   NULL,        0,  0, 0, 0,},
};

/** Global config **/
static char *progname = NULL;
static char *dest;
static char *archive = NULL;

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;
static BOOL dir_seen = FALSE;

static BOOL save_win_posn = TRUE;
static BOOL h_browser = FALSE;

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

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, BOOL h)
{
	ULONG flags = 0;
	ULONG gen = 0;
	int i = 0;

	if(h) {
		gen = 1;
		if (dir) {
			dir_seen = TRUE;
			flags = LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN;
		}

		while(name[i]) {
			if(name[i] == '/') gen++;
			i++;
		}

		if((gen > 1) && (dir_seen == FALSE)) {
			/* Probably we have an archive which doesn't have directory nodes */
			gen = 1;
		} else {
			name = FilePart(name);
		}
	}

	struct Node *node = AllocListBrowserNode(2,
		LBNA_UserData, userdata,
		LBNA_CheckBox, TRUE,
		LBNA_Checked, TRUE,
		LBNA_Flags, flags,
		LBNA_Generation, gen,
		LBNA_Column, 0,
			LBNCA_Text, name,
		LBNA_Column, 1,
			LBNCA_Integer, size,
		TAG_DONE);

	AddTail(&lblist, node);
}

static void addlbnode_cb(char *name, LONG *size, BOOL dir, void *userdata)
{
#if 0
	/* This section tries to add directory nodes when they don't exist in the file */
	static LONG zero = 0;
	static char *prev_path = NULL;
	char *path;

	if(h_browser) {
		ULONG path_len = PathPart(name) - name;
		if(path_len > 0) {
			if(path = AllocVec(path_len + 1, MEMF_CLEAR)) {
				strncpy(path, name, path_len);
				if(prev_path == NULL) prev_path = AllocVec(1, MEMF_CLEAR);
				printf("%s prev:%s\n", path, prev_path);
				if(strcmp(path, prev_path) != 0) addlbnode(path, &zero, TRUE, NULL, h_browser);
				FreeVec(prev_path);
				prev_path = path;
			}
		}
	}
#endif

	addlbnode(name, size, dir, userdata, h_browser);
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

static void open_archive_req(struct Window *win, struct Gadget *arc_gad, struct Gadget *list_gad, BOOL refresh_only)
{
	if(archive_needs_free) free_archive_path();
	dir_seen = FALSE;

	if(refresh_only == FALSE) DoMethod((Object *)arc_gad, GFILE_REQUEST, win);
	GetAttr(GETFILE_FullFile, arc_gad, (APTR)&archive);

	SetWindowPointer(win,
					WA_BusyPointer, TRUE,
					TAG_DONE);

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);
	FreeListBrowserList(&lblist);

	xad_info(archive, addlbnode_cb);

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, &lblist, TAG_DONE);

	SetWindowPointer(win,
					WA_BusyPointer, FALSE,
					TAG_DONE);
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

	struct MsgPort *appwin_mp = NULL;
	struct AppWindow *appwin = NULL;
	struct AppMessage *appmsg = NULL;
	ULONG appwin_sig = 0;

	struct ColumnInfo lbci[3] = {
		{90, "Name", CIF_WEIGHTED | CIF_DRAGGABLE},
		{10, "Size", CIF_WEIGHTED | CIF_DRAGGABLE},
		{-1, NULL, 0}
	};

	NewList(&lblist);
	if(archive) xad_info(archive, addlbnode_cb);

	if(h_browser) menu[9].nm_Flags |= CHECKED;
	if(save_win_posn) menu[10].nm_Flags |= CHECKED;
	if(progname == NULL) menu[12].nm_Flags |= NM_ITEMDISABLED;

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
					LAYOUT_AddChild, gadgets[GID_ARCHIVE] = GetFileObj,
						GA_ID, GID_ARCHIVE,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, "Select Archive",
						GETFILE_FullFile, archive,
						GETFILE_ReadOnly, TRUE,
					End,
					CHILD_WeightedHeight, 0,
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
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text, "_Destination",
					LabelEnd,
					LAYOUT_AddChild, gadgets[GID_LIST] = ListBrowserObj,
						GA_ID, GID_LIST,
						LISTBROWSER_ColumnInfo, &lbci,
						LISTBROWSER_Labels, &lblist,
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_Hierarchical, h_browser,
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

				if(appwin_mp = CreateMsgPort()) {
					if(appwin = AddAppWindowA(0, 0, windows[WID_MAIN], appwin_mp, NULL)) {
						appwin_sig = 1L << appwin_mp->mp_SigBit;
					}
				}

				/* Input Event Loop
				 */
				while (!done) {
					wait = Wait( signal | SIGBREAKF_CTRL_C | app | appwin_sig );

					if ( wait & SIGBREAKF_CTRL_C ) {
						done = TRUE;
					} else if(wait & appwin_sig) {
						while(appmsg = (struct AppMessage *)GetMsg(appwin_mp)) {
							struct WBArg *wbarg = appmsg->am_ArgList;
							if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
								if(archive_needs_free) free_archive_path();
								if(archive = AllocVec(512, MEMF_CLEAR)) {
									NameFromLock(wbarg->wa_Lock, archive, 512);
									AddPart(archive, wbarg->wa_Name, 512);
									SetGadgetAttrs(gadgets[GID_ARCHIVE], windows[WID_MAIN], NULL,
													GETFILE_FullFile, archive, TAG_DONE);
									FreeVec(archive);
									open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST], TRUE);		
								}
							} 

							ReplyMsg((struct Message *)appmsg);
						}
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
											open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST], FALSE);
										break;
										
										case GID_DEST:
											if(dest_needs_free) free_dest_path();
											DoMethod((Object *)gadgets[GID_DEST], GFILE_REQUEST, windows[WID_MAIN]);
											GetAttr(GETFILE_Drawer, gadgets[GID_DEST], (APTR)&dest);
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
									RemoveAppWindow(appwin);
									RA_Iconify(objects[OID_MAIN]);
									windows[WID_MAIN] = NULL;
									break;

								case WMHI_UNICONIFY:
									windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN]);

									if (windows[WID_MAIN]) {
										GetAttr(WINDOW_SigMask, objects[OID_MAIN], &signal);
										if(appwin = AddAppWindowA(0, 0, windows[WID_MAIN], appwin_mp, NULL)) {
											appwin_sig = 1L << appwin_mp->mp_SigBit;
										}
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
														open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST], FALSE);
													break;
												
													case 1: //about
														OpenRequesterTags(objects[OID_REQ], windows[WID_MAIN], 
															REQ_Type, REQTYPE_INFO,
															REQ_Image, REQIMAGE_INFO,
															REQ_BodyText, 	VERS " (" DATE ")\n"
																			"(c) 2022 Chris Young\n\33uhttps://github.com/chris-y/avalanche\33n\n\n"
																			"This program is free software; you can redistribute it and/or modify\n"
																			"it under the terms of the GNU General Public License as published by\n"
																			"the Free Software Foundation; either version 2 of the License, or\n"
																			"(at your option) any later version.\n\n"
																			"This program is distributed in the hope that it will be useful,\n"
																			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
																			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
																			"GNU General Public License for more details.",
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
													case 0: //browser mode
														h_browser = !h_browser;
														
														SetGadgetAttrs(gadgets[GID_LIST], windows[WID_MAIN], NULL,
																LISTBROWSER_Hierarchical, h_browser, TAG_DONE);

														open_archive_req(windows[WID_MAIN], gadgets[GID_ARCHIVE], gadgets[GID_LIST], TRUE);
													break;
													
													case 1: //save window position
														save_win_posn = !save_win_posn;
													break;
												
													case 3: //save settings
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

		RemoveAppWindow(appwin);
		if(appwin_mp) DeleteMsgPort(appwin_mp);
		DeleteMsgPort(AppPort);
	}
}

static void gettooltypes(UBYTE **tooltypes)
{
	char *s;
	
	dest = strdup(ArgString(tooltypes, "DEST", "RAM:"));
	dest_needs_free = TRUE;

	if(FindToolType(tooltypes, "HBROWSER")) h_browser = TRUE;
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
		struct WBArg *wbarg = WBenchMsg->sm_ArgList;

		if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
			if(progname = AllocVec(40, MEMF_CLEAR)) {
				strcpy(progname, "PROGDIR:");
				AddPart(progname, wbarg->wa_Name, 40);
			}
		}

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

	if(progname != NULL) FreeVec(progname);
	if(archive_needs_free) free_archive_path();
	if(dest_needs_free) free_dest_path();
	
	FreeListBrowserList(&lblist);
	xad_exit();
	libs_close();

	return 0;
}
