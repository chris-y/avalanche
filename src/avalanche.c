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
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/pointerclass.h>
#include <workbench/startup.h>

#include <classes/window.h>

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

#define IEVENT_POPUP 1L

/** Global config **/
#define PROGRESS_SIZE_DEFAULT 20

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;

static struct avalanche_config config;

/** Shared variables **/
static char *archive = NULL;
static struct Locale *locale = NULL;

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
static void free_archive_path(void)
{
	if(archive) FreeVec(archive);
	archive = NULL;
	archive_needs_free = FALSE;
}

static void free_dest_path(void)
{
	if(config.dest) free(config.dest);
	config.dest = NULL;
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

	if(dobj = GetIconTagList(config.progname, NULL)) {
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
			GetAttr(WA_Top, win, (APTR)&config.win_x);
			GetAttr(WA_Left, win, (APTR)&config.win_y);
			GetAttr(WA_Width, win, (APTR)&config.win_w);
			GetAttr(WA_Height, win, (APTR)&config.win_h);
		} else {
			newtooltypes[2] = "(SAVEWINPOSN)";
		}

		if(config.win_x) {
			sprintf(tt_winx, "WINX=%lu", config.win_x);
			newtooltypes[3] = tt_winx;
		} else {
			newtooltypes[3] = "(WINX=0)";
		}

		if(config.win_y) {
			sprintf(tt_winy, "WINY=%lu", config.win_y);
			newtooltypes[4] = tt_winy;
		} else {
			newtooltypes[4] = "(WINY=0)";
		}

		if(config.win_w) {
			sprintf(tt_winw, "WINW=%lu", config.win_w);
			newtooltypes[5] = tt_winw;
		} else {
			newtooltypes[5] = "(WINW=0)";
		}

		if(config.win_h) {
			sprintf(tt_winh, "WINH=%lu", config.win_h);
			newtooltypes[6] = tt_winh;
		} else {
			newtooltypes[6] = "(WINH=0)";
		}

		if(config.progress_size != PROGRESS_SIZE_DEFAULT) {
			sprintf(tt_progresssize, "PROGRESSSIZE=%lu", config.progress_size);
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
		PutIconTags(config.progname, dobj, NULL);
		dobj->do_ToolTypes = (STRPTR *)oldtooltypes;
		FreeDiskObject(dobj);
	}
}

ULONG ask_quit(void *awin)
{
	if(config.confirmquit) {
		return ask_quit_req(awin);
	}

	return 1;
}

struct avalanche_config *get_config(void)
{
	return &config;
}

static ULONG vscan(void *awin, char *file, UBYTE *buf, ULONG len)
{
	long res = 0;

	if(config.virus_scan) {
		if(buf == NULL) {
			res = xvs_scan(file, len, awin);
		} else {
			res = xvs_scan_buffer(buf, len, awin);
		}

		if((res == -1) || (res == -3)) {
			config.virus_scan = FALSE;
			window_disable_vscan_menu(awin);
			
		}
	}

	return res;
}

long extract(void *awin, char *archive, char *newdest, struct Node *node)
{
	long ret = 0;

	if(newdest == NULL) newdest = config.dest; /* TODO: should always have dest passed in */

	if(archive && newdest) {
		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
										WA_PointerType, POINTERTYPE_PROGRESS,
										TAG_DONE);
		window_reset_count(awin);
		window_disable_gadgets(awin, TRUE);

		switch(window_get_archiver(awin)) {
			case ARC_XAD:
				if(node == NULL) {
					ret = xad_extract(awin, archive, newdest, window_get_lblist(awin), window_get_lbnode, vscan);
				} else {
					ULONG pud = 0;
					ret = xad_extract_file(awin, archive, newdest, node, window_get_lbnode, vscan, &pud);
				}
			break;
			case ARC_XFD:
				ret = xfd_extract(awin, archive, newdest, vscan);
			break;
		}

		window_disable_gadgets(awin, FALSE);

		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);
	}

	return ret;
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
		newbroker.nb_Pri = config.cx_pri;
		newbroker.nb_Port = CXMP;
		newbroker.nb_ReservedChannel = 0;

		if(broker = CxBroker(&newbroker, NULL)) {
			/* Try to add hotkey */
			if(config.cx_popkey) {
				if(filter = CxFilter(config.cx_popkey)) {
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

static void gui(void)
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

		if(config.cx_popup || archive) {
			/* only create the window object if we are showing the window OR have an archive */
			awin = window_create(&config, archive, winport, AppPort);

			/* Open window */
			if(config.cx_popup) window_open(awin, appwin_mp);

			/* Open initial archive, if there is one. */
			if(archive) window_req_open_archive(awin, &config, TRUE);
		}
		
		
		/* Input Event Loop
		 */
		 
		signal = (1L << winport->mp_SigBit);
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

								if(archive_needs_free) free_archive_path();
								if(archive = AllocVec(512, MEMF_CLEAR)) {
									NameFromLock(wbarg->wa_Lock, archive, 512);
									AddPart(archive, wbarg->wa_Name, 512);
									window_update_archive((void *)appmsg->am_UserData, archive);
									free_archive_path();
									window_req_open_archive(awin, &config, TRUE);
									if(config.progname && (appmsg->am_NumArgs > 1)) {
										for(int i = 1; i < appmsg->am_NumArgs; i++) {
											wbarg++;
											OpenWorkbenchObject(config.progname+8,
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
									if(archive_needs_free) free_archive_path();
									if(archive = AllocVec(512, MEMF_CLEAR)) {
										char *tempdest = NULL;
										NameFromLock(wbarg->wa_Lock, archive, 512);
										tempdest = strdup(archive);
										AddPart(archive, wbarg->wa_Name, 512);
										window_update_archive((void *)appmsg->am_UserData, archive);
										free_archive_path();
										window_req_open_archive(awin, &config, TRUE);
										if(window_get_archiver(awin) != ARC_NONE) {
											ret = extract(awin, archive, tempdest, NULL);
											if(ret != 0) show_error(ret, awin);
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
				while((done == FALSE) && ((result = window_handle_input(awin, &code)) != WMHI_LASTMSG)) {
					done = window_handle_input_events(awin, &config, result, appwin_mp, code);
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
	dest_needs_free = TRUE;

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
			if(config.progname = AllocVec(40, MEMF_CLEAR)) {
				strcpy(config.progname, "PROGDIR:");
				AddPart(config.progname, wbarg->wa_Name, 40);
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

			if(config.progname && (WBenchMsg->sm_NumArgs > 2)) {
				for(int i = 2; i < WBenchMsg->sm_NumArgs; i++) {
					wbarg++;
					OpenWorkbenchObject(config.progname+8,
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

	gui();

	Locale_Close();

	DeleteFile(config.tmpdir);

	if(config.cx_popkey) FreeVec(config.cx_popkey);
	if(config.tmpdir) FreeVec(config.tmpdir);
	if(config.progname != NULL) FreeVec(config.progname);
	if(archive_needs_free) free_archive_path();
	if(dest_needs_free) free_dest_path();
	
	xad_exit();
	xfd_exit();
	libs_close();

	return 0;
}
