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

#include <proto/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/getfile.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>

#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/pointerclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <utility/date.h>
#include <workbench/startup.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "req.h"
#include "libs.h"
#include "locale.h"
#include "win.h"
#include "xad.h"
#include "xfd.h"
#include "xvs.h"

#include "Avalanche_rev.h"

#ifndef __amigaos4__
#define IsMinListEmpty(L) (L)->mlh_Head->mln_Succ == 0
#endif

const char *version = VERSTAG;

enum {
	ARC_NONE = 0,
	ARC_XAD,
	ARC_XFD
};

#define GID_EXTRACT_TEXT  locale_get_string(MSG_EXTRACT)
#define IEVENT_POPUP 1L

/** Global config **/
#define PROGRESS_SIZE_DEFAULT 20

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;
static BOOL dir_seen = FALSE;

static struct avalanche_config config;

/** Shared variables **/
static struct Locale *locale = NULL;
static ULONG current_item = 0;
static ULONG total_items = 0;

/** Useful functions **/
#ifndef __amigaos4__
struct Node *GetHead(struct List *list)
{
	struct Node *res = NULL;

	if ((NULL != list) && (NULL != list->lh_Head->ln_Succ))
	{
		res = list->lh_Head;
	}
	return res;
}

struct Node *GetPred(struct Node *node)
{
	if (node->ln_Pred->ln_Pred == NULL) return NULL;
	return node->ln_Pred;
}

struct Node *GetSucc(struct Node *node)
{
	if (node->ln_Succ->ln_Succ == NULL) return NULL;
	return node->ln_Succ;
}
#endif

char *strdup(const char *s)
{
  size_t len = strlen (s) + 1;
  char *result = (char*) malloc (len);
  if (result == (char*) 0)
  return (char*) 0;
  return (char*) memcpy (result, s, len);
}

/** Private functions **/
static void free_archive_path(char *archive)
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

void savesettings(Object *win)
{
	struct DiskObject *dobj;
	UBYTE **oldtooltypes;
	UBYTE *newtooltypes[17];
	char tt_dest[100];
	char tt_tmp[100];
	char tt_winx[15];
	char tt_winy[15];
	char tt_winh[15];
	char tt_winw[15];
	char tt_progresssize[20];
	char tt_cxpri[20];
	char tt_cxpopkey[50];

	if(dobj = GetIconTagList(progname, NULL)) {
		oldtooltypes = (UBYTE **)dobj->do_ToolTypes;

		if(config.dest && (strcmp("RAM:", config.dest) != 0)) {
			strcpy(tt_dest, "DEST=");
			newtooltypes[0] = strcat(tt_dest, config.dest);
		} else {
			newtooltypes[0] = "(DEST=RAM:)";
		}

		if(config.h_browser) {
			newtooltypes[1] = "HBROWSER";
		} else {
			newtooltypes[1] = "(HBROWSER)";
		}

		if(config.save_win_posn) {
			newtooltypes[2] = "SAVEWINPOSN";

			/* fetch current win posn */
			GetAttr(WA_Top, win, (APTR)&win_x);
			GetAttr(WA_Left, win, (APTR)&win_y);
			GetAttr(WA_Width, win, (APTR)&win_w);
			GetAttr(WA_Height, win, (APTR)&win_h);
		} else {
			newtooltypes[2] = "(SAVEWINPOSN)";
		}

		if(win_x) {
			sprintf(tt_winx, "WINX=%lu", win_x);
			newtooltypes[3] = tt_winx;
		} else {
			newtooltypes[3] = "(WINX=0)";
		}

		if(win_y) {
			sprintf(tt_winy, "WINY=%lu", win_y);
			newtooltypes[4] = tt_winy;
		} else {
			newtooltypes[4] = "(WINY=0)";
		}

		if(win_w) {
			sprintf(tt_winw, "WINW=%lu", win_w);
			newtooltypes[5] = tt_winw;
		} else {
			newtooltypes[5] = "(WINW=0)";
		}

		if(win_h) {
			sprintf(tt_winh, "WINH=%lu", win_h);
			newtooltypes[6] = tt_winh;
		} else {
			newtooltypes[6] = "(WINH=0)";
		}

		if(progress_size != PROGRESS_SIZE_DEFAULT) {
			sprintf(tt_progresssize, "PROGRESSSIZE=%lu", progress_size);
		} else {
			sprintf(tt_progresssize, "(PROGRESSSIZE=%d)", PROGRESS_SIZE_DEFAULT);
		}

		newtooltypes[7] = tt_progresssize;

		if(config.virus_scan) {
			newtooltypes[8] = "VIRUSSCAN";
		} else {
			newtooltypes[8] = "(VIRUSSCAN)";
		}

		if(config.confirmquit) {
			newtooltypes[9] = "CONFIRMQUIT";
		} else {
			newtooltypes[9] = "(CONFIRMQUIT)";
		}

		if(config.ignorefs) {
			newtooltypes[10] = "IGNOREFS";
		} else {
			newtooltypes[10] = "(IGNOREFS)";
		}

		if(config.tmpdir && (strncmp("T:", config.tmpdir, config.tmpdirlen) != 0)) {
			strcpy(tt_tmp, "TMPDIR=");
			newtooltypes[11] = strncat(tt_tmp, config.tmpdir, config.tmpdirlen);
		} else {
			newtooltypes[11] = "(TMPDIR=T:)";
		}

		if(config.cx_popup == FALSE) {
			newtooltypes[12] = "CX_POPUP=NO";
		} else {
			newtooltypes[12] = "(CX_POPUP=YES)";
		}

		if(config.cx_pri != 0) {
			sprintf(tt_cxpri, "CX_PRIORITY=%d", config.cx_pri);
		} else {
			sprintf(tt_cxpri, "(CX_PRIORITY=0)");
		}
		newtooltypes[13] = tt_cxpri;

		if((config.cx_popkey) && (strcmp(config.cx_popkey, "rawkey ctrl alt a") != 0)) {
			sprintf(tt_cxpopkey, "CX_POPKEY=%s", config.cx_popkey + 7);
		} else {
			sprintf(tt_cxpopkey, "(CX_POPKEY=ctrl alt a)");
		}
		newtooltypes[14] = tt_cxpopkey;

		newtooltypes[15] = "DONOTWAIT";

		newtooltypes[16] = NULL;

		dobj->do_ToolTypes = (STRPTR *)&newtooltypes;
		PutIconTags(progname, dobj, NULL);
		dobj->do_ToolTypes = (STRPTR *)oldtooltypes;
		FreeDiskObject(dobj);
	}
}

static ULONG ask_quit(void *awin)
{
	if(confirmquit) {
		return ask_quit_req(awin);
	}

	return 1;
}

struct avalanche_config *get_config(void)
{
	return &config;
}

static void *getlbnode(struct Node *node)
{
	void *userdata = NULL;
	ULONG checked = FALSE;
	char msg[20];

	GetListBrowserNodeAttrs(node,
			LBNA_Checked, &checked,
			LBNA_UserData, &userdata,
		TAG_DONE);

	current_item++;
	sprintf(msg, "%lu/%lu", current_item, total_items);

	SetGadgetAttrs(gadgets[GID_PROGRESS], windows[WID_MAIN], NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);

	if(checked == FALSE) return NULL;
	return userdata;
}

/* Activate/disable menus related to an open archive */
static void menu_activation(BOOL enable)
{
	if(windows[WID_MAIN] == NULL) return;

	if(enable) {
		OnMenu(windows[WID_MAIN], FULLMENUNUM(0,2,0));
		OnMenu(windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OnMenu(windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OnMenu(windows[WID_MAIN], FULLMENUNUM(1,2,0));
	} else {
		OffMenu(windows[WID_MAIN], FULLMENUNUM(0,2,0));
		OffMenu(windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OffMenu(windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OffMenu(windows[WID_MAIN], FULLMENUNUM(1,2,0));
	}
}

static void modify_all_list(struct Window *win, struct Gadget *list_gad, ULONG select)
{
	struct Node *node;
	BOOL selected;

	/* select: 0 = deselect all, 1 = select all, 2 = toggle all */
	if(select == 0) selected = FALSE;
	if(select == 1) selected = TRUE;

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	for(node = lblist.lh_Head; node->ln_Succ; node=node->ln_Succ) {

		if(select == 2) {
			ULONG current;
			GetListBrowserNodeAttrs(node, LBNA_Checked, &current, TAG_DONE);
			selected = !current;
		}

		SetListBrowserNodeAttrs(node, LBNA_Checked, selected, TAG_DONE);
	}

	SetGadgetAttrs(list_gad, win, NULL,
				LISTBROWSER_Labels, &lblist,
			TAG_DONE);
}

static void disable_gadgets(BOOL disable)
{
	if(disable) {
		SetGadgetAttrs(gadgets[GID_EXTRACT], windows[WID_MAIN], NULL,
				GA_Text,  locale_get_string( MSG_STOP ) ,
			TAG_DONE);
	} else {
		SetGadgetAttrs(gadgets[GID_EXTRACT], windows[WID_MAIN], NULL,
				GA_Text, GID_EXTRACT_TEXT,
			TAG_DONE);
	}

	SetGadgetAttrs(gadgets[GID_ARCHIVE], windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(gadgets[GID_DEST], windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(gadgets[GID_LIST], windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
}

static ULONG vscan(char *file, UBYTE *buf, ULONG len)
{
	long res = 0;

	if(virus_scan) {
		if(buf == NULL) {
			res = xvs_scan(file, len);
		} else {
			res = xvs_scan_buffer(buf, len);
		}

		if((res == -1) || (res == -3)) {
			virus_scan = FALSE;
			if(windows[WID_MAIN]) OffMenu(windows[WID_MAIN], FULLMENUNUM(2,0,0));
		}
	}

	return res;
}

static long extract(void *awin, char *newdest, struct Node *node)
{
	long ret = 0;

	if(newdest == NULL) newdest = dest;

	if(archive && newdest) {
		if(windows[WID_MAIN]) SetWindowPointer(windows[WID_MAIN],
										WA_PointerType, POINTERTYPE_PROGRESS,
										TAG_DONE);
		current_item = 0;

		disable_gadgets(TRUE);

		switch(archiver) {
			case ARC_XAD:
				if(node == NULL) {
					ret = xad_extract(awin, archive, newdest, &lblist, getlbnode, vscan);
				} else {
					ULONG pud = 0;
					ret = xad_extract_file(awin, archive, newdest, node, getlbnode, vscan, &pud);
				}
			break;
			case ARC_XFD:
				ret = xfd_extract(archive, newdest, vscan);
			break;
		}

		disable_gadgets(FALSE);

		if(windows[WID_MAIN]) SetWindowPointer(windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);
	}

	return ret;
}

#ifdef __amigaos4__
static ULONG aslfilterfunc(struct Hook *h, struct FileRequester *fr, struct AnchorPathOld *ap)
#else
static ULONG __saveds aslfilterfunc(__reg("a0") struct Hook *h, __reg("a2") struct FileRequester *fr, __reg("a1") struct AnchorPath *ap)
#endif
{
	BOOL found = FALSE;
	char fullfilename[256];

	if(ap->ap_Info.fib_DirEntryType > 0) return(TRUE); /* Drawer */

	strcpy(fullfilename, fr->fr_Drawer);
	AddPart(fullfilename, ap->ap_Info.fib_FileName, 256);

	found = xad_recog(fullfilename);
	if(found == FALSE) found = xfd_recog(fullfilename);

	return found;
}

/* Check if abort button is pressed - only called from xad hook */
BOOL check_abort(void)
{
	ULONG result;
	UWORD code;

	while((result = RA_HandleInput(objects[OID_MAIN], &code)) != WMHI_LASTMSG ) {
		switch (result & WMHI_CLASSMASK) {
			case WMHI_GADGETUP:
				switch (result & WMHI_GADGETMASK) {
					case GID_EXTRACT:
						return TRUE;
					break;
				}
			break;

			case WMHI_RAWKEY:
				switch(result & WMHI_GADGETMASK) {
					case RAWKEY_ESC:
						return TRUE;
					break;
				}
			break;
		}
	}
	return FALSE;
}

static void UnregisterCx(CxObj *CXBroker, struct MsgPort *CXMP)
{
	CxMsg *msg;

	DeleteCxObjAll(CXBroker);
	while(msg = (CxMsg *)GetMsg(CXMP))
		ReplyMsg((struct Message *)msg);

	DeletePort(CXMP);
}

static struct MsgPort *RegisterCx(CxObj **CXBroker)
{
	struct MsgPort *CXMP = NULL;
	struct NewBroker newbroker;
	CxObj *broker = NULL;
	CxObj *filter = NULL;
	CxObj *sender = NULL;
	CxObj *translate = NULL;

	if(CXMP = CreateMsgPort()) {
		newbroker.nb_Version = NB_VERSION;
		newbroker.nb_Name = "Avalanche";
		newbroker.nb_Title = VERS;
		newbroker.nb_Descr = locale_get_string(MSG_CXDESCRIPTION);
		newbroker.nb_Unique = 0;
		newbroker.nb_Flags = COF_SHOW_HIDE;
		newbroker.nb_Pri = cx_pri;
		newbroker.nb_Port = CXMP;
		newbroker.nb_ReservedChannel = 0;

		if(broker = CxBroker(&newbroker, NULL)) {
			/* Try to add hotkey */
			if(cx_popkey) {
				if(filter = CxFilter(cx_popkey)) {
					AttachCxObj(broker, filter);
					if(sender = CxSender(CXMP, IEVENT_POPUP)) {
						AttachCxObj(filter, sender);
						if(translate = CxTranslate(NULL)) {
							AttachCxObj(filter, translate);
						}
					}
				}
			}
			ActivateCxObj(broker, 1L);
			*CXBroker = broker;
		} else {
			DeleteMsgPort(CXMP);
			return NULL;
		}
	}

	return CXMP;
}

static void gui(char *archive)
{
	struct MsgPort *cx_mp = NULL;
	CxObj *cx_broker = NULL;
	ULONG cx_signal = 0;
	struct MsgPort *AppPort = NULL;
	struct MsgPort *winport = NULL;
	struct MsgPort *appwin_mp = NULL;
	struct AppMessage *appmsg = NULL;
	struct AppMenuItem *appmenu = NULL;
	ULONG appwin_sig = 0;

	ULONG wait, signal, app;
	ULONG done = FALSE;
	ULONG result;
	UWORD code;
	long ret = 0;
	ULONG tmp = 0;
	struct Node *node;
	
	void *awin = NULL;

	if((AppPort = CreateMsgPort()) && (winport = CreateMsgPort())) {

		/* Register CX */
		if(cx_mp = RegisterCx(&cx_broker)) {
			cx_signal = (1 << cx_mp->mp_SigBit);
		}
		
		/* Register in Tools menu */
		if(appwin_mp = CreateMsgPort()) {
			if(appmenu = AddAppMenuItem(0, 0, locale_get_string(MSG_APPMENU_EXTRACTHERE), appwin_mp,
							WBAPPMENUA_CommandKeyString, "X",
							TAG_DONE)) {
			}
			appwin_sig = 1L << appwin_mp->mp_SigBit;
		}

		if(cx_popup || archive) {
			/* only create the window object if we are showing the window OR have an archive */
			awin = window_create(&config, archive, winport, AppPort);

			/* Open window */
			if(cx_popup) window_open(awin, appwin_mp);

			/* Open initial archive, if there is one. */
			if(archive) window_req_open_archive(awin, &config, TRUE);
		}
		
		
		/* Input Event Loop
		 */
		 
		signal = (1L << winport << winport->mp_SigBit);
		app = (1L << AppPort->mp_SigBit);
		 
		while (!done) {
			wait = Wait( signal | app | appwin_sig | cx_signal );

			if(wait & cx_signal) {
				ULONG cx_msgid, cx_msgtype;
				CxMsg *cx_msg;

				while(cx_msg = (CxMsg *)GetMsg(cx_mp)) {
					cx_msgid = CxMsgID(cx_msg);
					cx_msgtype = CxMsgType(cx_msg);
					ReplyMsg((struct Message *)cx_msg);

					switch(cx_msgtype) {
						case CXM_COMMAND:
							switch(cx_msgid) {
								case CXCMD_KILL:
									done = TRUE;
								break;
								case CXCMD_APPEAR:
									window_open(awin, appwin_mp);
								break;
								case CXCMD_UNIQUE:
									//not unique, ignore
								break;
								case CXCMD_DISAPPEAR:
									window_close(awin, FALSE);
								break;

								case CXCMD_ENABLE:
									ActivateCxObj(cx_broker, 1L);
								break;
								case CXCMD_DISABLE:
									ActivateCxObj(cx_broker, 0L);
								break;
								default:
								break;
							}
						break;
						case CXM_IEVENT:
							switch(cx_msgid) {
								case IEVENT_POPUP:
									window_open(awin, appwin_mp);
								break;
							}
						default:
						break;
					}
				}
			} else if(wait & appwin_sig) {
				while(appmsg = (struct AppMessage *)GetMsg(appwin_mp)) {
					struct WBArg *wbarg = appmsg->am_ArgList;
					switch(appmsg->am_Type) {
						case AMTYPE_APPWINDOW:
							if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {

								/* TODO: manage archive within window structure */
								if(archive_needs_free) free_archive_path();
								if(archive = AllocVec(512, MEMF_CLEAR)) {
									NameFromLock(wbarg->wa_Lock, archive, 512);
									AddPart(archive, wbarg->wa_Name, 512);
									window_update_archive(appmsg->am_UserData, archive);

									window_req_open_archive(awin, &config, TRUE);
									if(progname && (appmsg->am_NumArgs > 1)) {
										for(int i = 1; i < appmsg->am_NumArgs; i++) {
											wbarg++;
											OpenWorkbenchObject(progname+8,
												WBOPENA_ArgLock, wbarg->wa_Lock,
												WBOPENA_ArgName, wbarg->wa_Name,
												TAG_DONE);
										}
									}
								}
							}
						break;
						case AMTYPE_APPMENUITEM:
							for(int i=0; i<appmsg->am_NumArgs; i++) {
								if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
									/* TODO: manage archive within window structure */
									if(archive_needs_free) free_archive_path();
									if(archive = AllocVec(512, MEMF_CLEAR)) {
										char *tempdest = NULL;
										NameFromLock(wbarg->wa_Lock, archive, 512);
										tempdest = strdup(archive);
										AddPart(archive, wbarg->wa_Name, 512);
										window_update_archive(appmsg->am_UserData, archive);
										window_req_open_archive(awin, &config, TRUE);
										if(archiver != ARC_NONE) {
											ret = extract(awin, tempdest, NULL);
											if(ret != 0) show_error(ret);
										}
									}
								}
								wbarg++;
							}
						break;
						default:
						break;
					}
					ReplyMsg((struct Message *)appmsg);
				}
			} else {
				while((done == FALSE) && ((result = RA_HandleInput(window_get_object(awin), &code) ) != WMHI_LASTMSG)) {
					switch (result & WMHI_CLASSMASK) {
						case WMHI_CLOSEWINDOW:
							if(ask_quit(awin)) {
								window_close(awin, FALSE);
								done = TRUE;
							}
						break;

						case WMHI_GADGETUP:
							switch (result & WMHI_GADGETMASK) {
								case GID_ARCHIVE:
									window_req_open_archive(awin, &config, FALSE);
								break;
									
								case GID_DEST:
									window_req_dest(awin);
								break;

								case GID_EXTRACT:
									ret = extract(awin, NULL, NULL);
									if(ret != 0) show_error(ret);
								break;

								case GID_LIST:
									window_list_handle(awin, config.tmpdir);
								break;
							}
							break;

						case WMHI_RAWKEY:
							switch(result & WMHI_GADGETMASK) {
								case RAWKEY_ESC:
									if(ask_quit(awin)) {
										done = TRUE;
									}
								break;

								case RAWKEY_RETURN:
									ret = extract(awin, NULL, NULL);
									if(ret != 0) show_error(ret, awin);
								break;
							}
						break;

						case WMHI_ICONIFY:
							window_close(awin, TRUE);
						break;

						case WMHI_UNICONIFY:
							window_open(awin);
						break;
								
						 case WMHI_MENUPICK:
							while((code != MENUNULL) && (done == FALSE)) {
								if(windows[WID_MAIN] == NULL) continue;
								struct MenuItem *item = ItemAddress(windows[WID_MAIN]->MenuStrip, code);

								switch(MENUNUM(code)) {
									case 0: //project
										switch(ITEMNUM(code)) {
											case 0: //open
												window_req_open_archive(awin, &config, FALSE);
											break;
											
											case 2: //info
												switch(archiver) {
													case ARC_XAD:
														xad_show_arc_info(awin);
													break;
													case ARC_XFD:
														xfd_show_arc_info(awin);
													break;
												}
											break;

											case 3: //about
												show_about(awin);
											break;
											
											case 5: //quit
												if(ask_quit(awin)) {
													done = TRUE;
												}
											break;
										}
									break;
									
									case 1: //edit
										switch(ITEMNUM(code)) {
											case 0: //select all
												modify_all_list(windows[WID_MAIN], gadgets[GID_LIST], 1);
											break;
											
											case 1: //clear selection
												modify_all_list(windows[WID_MAIN], gadgets[GID_LIST], 0);
											break;

											case 2: //invert selection
												modify_all_list(windows[WID_MAIN], gadgets[GID_LIST], 2);
											break;
										}
									break;
									
									case 2: //settings
										switch(ITEMNUM(code)) {
											case 0: //virus scan
												virus_scan = !virus_scan;
											break;

											case 1: //browser mode
												h_browser = !h_browser;
													
												window_toggle_hbrowser(awin, h_browser);
												window_req_open_archive(awin, &config, TRUE);
											break;
												
											case 2: //save window position
												save_win_posn = !save_win_posn;
											break;

											case 3: //save window position
												confirmquit = !confirmquit;
											break;

											case 4: //ignore fs
												ignorefs = !ignorefs;
												window_req_open_archive(awin, &config, TRUE);
											break;

											case 6: //save settings
												savesettings(window_get_object(awin));
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

	window_dispose(awin);
	if(cx_broker && cx_mp) UnregisterCx(cx_broker, cx_mp);

	RemoveAppMenuItem(appmenu);
	if(appwin_mp) DeleteMsgPort(appwin_mp);
	if(winport) DeleteMsgPort(winport);
	DeleteMsgPort(AppPort);
}

static void gettooltypes(UBYTE **tooltypes)
{
	char *s;

	s = ArgString(tooltypes, "CX_POPUP", "YES");
	if(MatchToolValue(s, "NO")) {
		config.cx_popup = FALSE;
	}

	config.cx_pri = ArgInt(tooltypes, "CX_PRIORITY", 0);

	s = ArgString(tooltypes, "CX_POPKEY", "ctrl alt a");
	if(config.cx_popkey = AllocVec(strlen("rawkey ") + strlen(s) + 1, MEMF_ANY)) {
		strcpy(config.cx_popkey, "rawkey ");
		strcat(config.cx_popkey, s);
	}

	config.dest = strdup(ArgString(tooltypes, "DEST", "RAM:"));
	config.dest_needs_free = TRUE;

	if(config.tmpdir) {
		strncpy(config.tmpdir, ArgString(tooltypes, "TMPDIR", "T:"), 90);
		config.tmpdirlen = strlen(config.tmpdir);
	}

	if(FindToolType(tooltypes, "HBROWSER")) config.h_browser = TRUE;
	if(FindToolType(tooltypes, "VIRUSSCAN")) config.virus_scan = TRUE;
	if(FindToolType(tooltypes, "SAVEWINPOSN")) config.save_win_posn = TRUE;
	if(FindToolType(tooltypes, "CONFIRMQUIT")) config.confirmquit = TRUE;
	if(FindToolType(tooltypes, "IGNOREFS")) config.ignorefs = TRUE;
	if(FindToolType(tooltypes, "DEBUG")) config.debug = TRUE;

	config.progress_size = ArgInt(tooltypes, "PROGRESSSIZE", PROGRESS_SIZE_DEFAULT);

	config.win_x = ArgInt(tooltypes, "WINX", 0);
	config.win_y = ArgInt(tooltypes, "WINY", 0);
	config.win_w = ArgInt(tooltypes, "WINW", 0);
	config.win_h = ArgInt(tooltypes, "WINH", 0);
}

/** Main program **/
int main(int argc, char **argv)
{
	char *tmp = NULL;
	char *archive = NULL;

	if(libs_open() == FALSE) {
		return 10;
	}


	/* Initialise config default values */
	config.progname = NULL;
	config.dest = NULL;
	config.save_win_posn = FALSE;
	config.h_browser = FALSE;
	config.virus_scan = FALSE;
	config.debug = FALSE;
	config.confirmquit = FALSE;
	config.ignorefs = FALSE;

	config.win_x = 0;
	config.win_y = 0;
	config.win_w = 0;
	config.win_h = 0;
	config.progress_size = PROGRESS_SIZE_DEFAULT;

	config.cx_pri = 0;
	config.cx_popup = TRUE;
	config.cx_popkey = NULL;

	config.tmpdir = NULL;
	config.tmpdirlen = 0;


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

			if(progname && (WBenchMsg->sm_NumArgs > 2)) {
				for(int i = 2; i < WBenchMsg->sm_NumArgs; i++) {
					wbarg++;
					OpenWorkbenchObject(progname+8,
						WBOPENA_ArgLock, wbarg->wa_Lock,
						WBOPENA_ArgName, wbarg->wa_Name,
						TAG_DONE);
				}
			}
        }
	}

	config.tmpdir = AllocVec(100, MEMF_CLEAR);
	
	UBYTE **tooltypes = ArgArrayInit(argc, (CONST_STRPTR *) argv);
	gettooltypes(tooltypes);
	ArgArrayDone();

	Locale_Open("avalanche.catalog", 0, 0);

	fill_menu_labels();

	if(tmp = AllocVec(20, MEMF_CLEAR)) {
		sprintf(tmp, "Avalanche.%lx", GetUniqueID());
		AddPart(config.tmpdir, tmp, 100);
		UnLock(CreateDir(config.tmpdir));
		FreeVec(tmp);
	}

	gui(archive);

	Locale_Close();

	DeleteFile(tmpdir);

	if(config.cx_popkey) FreeVec(cx_popkey);
	if(config.tmpdir) FreeVec(tmpdir);
	if(config.progname != NULL) FreeVec(progname);
	if(archive_needs_free) free_archive_path(archive);
//	if(dest_needs_free) free_dest_path();
	
	xad_exit();
	xfd_exit();
	libs_close();

	return 0;
}
