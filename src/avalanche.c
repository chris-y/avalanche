/* Avalanche
 * (c) 2022-5 Chris Young
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
#include <workbench/startup.h>

#include <classes/window.h>

#include "arexx.h"
#include "avalanche.h"
#include "config.h"
#include "req.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
#include "module.h"
#include "new.h"
#include "win.h"

#include "Avalanche_rev.h"

const char *version = VERSTAG;
const ULONG zero = 0;

#define IEVENT_POPUP 1L

/** Global config **/
static BOOL dest_needs_free = FALSE;

static struct avalanche_config config;

/** Shared variables **/
static struct Locale *locale = NULL;
static struct MinList win_list;
ULONG window_count = 0;

void free_dest_path(void)
{
	if(dest_needs_free) {
		CONFIG_LOCK_EX;
		if(config.dest) FreeVec(config.dest);
		config.dest = NULL;
		dest_needs_free = FALSE;
		CONFIG_UNLOCK;
	}
}

ULONG ask_quit(void *awin)
{
	return ask_yesno_req(awin, locale_get_string( MSG_AREYOUSUREYOUWANTTOEXIT ));
}

ULONG ask_quithide(void *awin)
{
	if(CONFIG_GET_LOCK(closeaction) == 0) {
		CONFIG_UNLOCK;
		return ask_quithide_req();
	}
	CONFIG_UNLOCK;
	return config.closeaction;
}

struct avalanche_config *get_config(void)
{
	return &config;
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
		newbroker.nb_Unique = NBU_UNIQUE;
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

void add_to_window_list(void *awin)
{
	struct Node *node = (struct Node *)awin;

	if(node) {
		AddTail((struct List *)&win_list, (struct Node *)node);
		window_count++;
	}
}

void del_from_window_list(void *awin)
{
	struct Node *node = (struct Node *)awin;

	if(node) {
		Remove((struct Node *)node);
		window_count--;
	}
}

static void show_hide_all(struct MsgPort *appwin_mp, struct MsgPort *winport, struct MsgPort *AppPort, BOOL show, BOOL dispose)
{
	void *awin;

	if(IsMinListEmpty((struct MinList *)&win_list) == FALSE) {
		awin = (void *)GetHead((struct List *)&win_list);
		struct Node *nnode;

		do {
			nnode = (struct Node *)GetSucc((struct Node *)awin);
			if(show) {
				window_open(awin, appwin_mp);
			} else {
				window_close(awin, FALSE);
				if(dispose) window_dispose(awin);
			}
		} while(awin = (void *)nnode);
	} else {
		if(show) {
			/* No windows, create one */
			awin = window_create(&config, NULL, winport, AppPort);
			if(awin == NULL) return;
			window_open(awin, appwin_mp);
		}
	}
}

static void close_all_windows()
{
	show_hide_all(NULL, NULL, NULL, FALSE, TRUE);
}

/* Do not call this directly! */
static BOOL open_archive_from_wbarg(void *awin, struct WBArg *wbarg, BOOL new_window, BOOL arexx,
				struct MsgPort *win_port, struct MsgPort *app_port, struct MsgPort *appwin_mp)
{
	if(wbarg->wa_Lock) {
		if(*wbarg->wa_Name) {
			char *appwin_archive = NULL;
			if(appwin_archive = AllocVec(512, MEMF_CLEAR)) {
				NameFromLock(wbarg->wa_Lock, appwin_archive, 512);
				AddPart(appwin_archive, wbarg->wa_Name, 512);
				if(arexx) {
					char cmd[1024];
					snprintf(cmd, 1024, "OPEN \"%s\"", appwin_archive);
					FreeVec(appwin_archive);
					ami_arexx_send(cmd);
					return TRUE;
				}
				if(new_window == FALSE) {
					window_update_archive(awin, appwin_archive);
				} else {
					awin = window_create(&config, appwin_archive, win_port, app_port);
				}
				FreeVec(appwin_archive);
				if(awin) {
					/* Ensure our window is open to avoid confusion */
					window_open(awin, appwin_mp);
					window_req_open_archive(awin, &config, TRUE);
				}
			}

			return TRUE;
		} else {
			if(arexx) return FALSE;
			if(awin == NULL) return FALSE;

			char *appwin_dir = NULL;
			if(appwin_dir = AllocVec(512, MEMF_CLEAR)) {
				NameFromLock(wbarg->wa_Lock, appwin_dir, 512);
				window_update_sourcedir(awin, appwin_dir);
				FreeVec(appwin_dir);
			}
		}
	}

	return FALSE;
}

static BOOL open_archive_from_wbarg_new(struct WBArg *wbarg, struct MsgPort *win_port, struct MsgPort *app_port, struct MsgPort *appwin_mp)
{
	return open_archive_from_wbarg(NULL, wbarg, TRUE, FALSE, win_port, app_port, appwin_mp);
}

static BOOL open_archive_from_wbarg_existing(void *awin, struct WBArg *wbarg)
{
	return open_archive_from_wbarg(awin, wbarg, FALSE, FALSE, NULL, NULL, NULL);
}

static BOOL open_archive_from_wbarg_arexx(struct WBArg *wbarg)
{
	return open_archive_from_wbarg(NULL, wbarg, FALSE, TRUE, NULL, NULL, NULL);
}

static void gui(struct WBStartup *WBenchMsg, ULONG rxsig, char *initial_archive)
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

	ULONG wait, signal, app, cw_sig, na_sig;
	ULONG done = WIN_DONE_OK;
	ULONG result;
	UWORD code;
	ULONG tmp = 0;

	void *awin = NULL;

	NewMinList(&win_list);

	if((AppPort = CreateMsgPort()) && (winport = CreateMsgPort())) {
		BOOL window_is_open = FALSE;

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

		if(WBenchMsg) {
			if(WBenchMsg->sm_NumArgs > 1) {
				/* Started as default tool, get the path+filename of the (first) project */
				struct WBArg *wbarg = WBenchMsg->sm_ArgList;

				for(int i = 1; i < WBenchMsg->sm_NumArgs; i++) {
					wbarg++;
					open_archive_from_wbarg_new(wbarg, winport, AppPort, appwin_mp);
					window_is_open = TRUE;
				}
			}
		} else if(initial_archive) {
			awin = window_create(&config, initial_archive, winport, AppPort);
			FreeVec(initial_archive);
			if(awin == NULL) return;
			window_open(awin, appwin_mp);
			window_req_open_archive(awin, &config, TRUE);
			window_is_open = TRUE;
		}

		if(window_is_open == FALSE) {
			/* If we wanted cx_popup but there's no window yet, open one */
			if(CONFIG_GET_LOCK(cx_popup)) {
				CONFIG_UNLOCK;
				awin = window_create(&config, NULL, winport, AppPort);
				if(awin == NULL) return;
				window_open(awin, appwin_mp);
			}
		}

		/* Input Event Loop
		 */
		 
		signal = (1L << winport->mp_SigBit);
		app = (1L << AppPort->mp_SigBit);

		while (done != WIN_DONE_QUIT) {
			done = WIN_DONE_OK;
			na_sig = newarc_window_get_signal();

			wait = Wait( signal | app | appwin_sig | cx_signal | rxsig | na_sig);
			
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
									done = WIN_DONE_QUIT;
								break;
								case CXCMD_APPEAR:
									show_hide_all(appwin_mp, winport, AppPort, TRUE, FALSE);
								break;
								case CXCMD_UNIQUE:
									//not unique, ignore
								break;
								case CXCMD_DISAPPEAR:
									show_hide_all(appwin_mp, winport, AppPort, FALSE, FALSE);
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
									show_hide_all(appwin_mp, winport, AppPort, TRUE, FALSE);
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
							if(open_archive_from_wbarg_existing((void *)appmsg->am_UserData, wbarg)) {
								if(appmsg->am_NumArgs > 1) {
									for(int i = 1; i < appmsg->am_NumArgs; i++) {
										wbarg++;
										open_archive_from_wbarg_new(wbarg, winport, AppPort, appwin_mp);
									}
								}
							}
						break;
						case AMTYPE_APPWINDOWZONE:
							switch(appmsg->am_ID) {
								case 0: // full window
									if(open_archive_from_wbarg_existing((void *)appmsg->am_UserData, wbarg)) {
										if(appmsg->am_NumArgs > 1) {
											for(int i = 1; i < appmsg->am_NumArgs; i++) {
												wbarg++;
												open_archive_from_wbarg_new(wbarg, winport, AppPort, appwin_mp);
											}
										}
									}
								break;

								case 1: // listbrowser
									if(module_has_add((void *)appmsg->am_UserData)) {
										for(int i = 0; i < appmsg->am_NumArgs; i++) {
											BOOL ret = TRUE;
											if(wbarg->wa_Lock) {
												char *file = NULL;
												if(file = AllocVec(1024, MEMF_CLEAR)) {
													NameFromLock(wbarg->wa_Lock, file, 1024);
													if(*wbarg->wa_Name) {
														AddPart(file, wbarg->wa_Name, 1024);
														ret = window_edit_add((void *)appmsg->am_UserData, file, NULL);
													} else {
#ifdef __amigaos4__
														if(object_is_dir(file)) {
															recursive_scan((void *)appmsg->am_UserData, file, file);
														} else {
															ret = window_edit_add(awin, file, NULL);
														}
#else
														if(object_is_dir(wbarg->wa_Lock)) {
															recursive_scan((void *)appmsg->am_UserData, wbarg->wa_Lock, file);
														} else {
															ret = window_edit_add(awin, file, NULL);
														}
#endif
													}
													window_req_open_archive((void *)appmsg->am_UserData, get_config(), TRUE);
													FreeVec(file);
												}
											}
											wbarg++;
											if(ret == FALSE) break;
										}
									} else {
										if(open_archive_from_wbarg_existing((void *)appmsg->am_UserData, wbarg)) {
											if(appmsg->am_NumArgs > 1) {
												for(int i = 1; i < appmsg->am_NumArgs; i++) {
													wbarg++;
													open_archive_from_wbarg_new(wbarg, winport, AppPort, appwin_mp);
												}
											}
										}
									}
								break;
							}
						break;
						case AMTYPE_APPMENUITEM:
							for(int i=0; i<appmsg->am_NumArgs; i++) {
								if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
									char *am_archive = NULL;
									if(am_archive = AllocVec(512, MEMF_CLEAR)) {
										NameFromLock(wbarg->wa_Lock, am_archive, 512);
										char *tempdest = strdup_vec(am_archive);
										AddPart(am_archive, wbarg->wa_Name, 512);
										
										/* Create a new window for our AppMenu to use */
										struct avalanche_window *appmenu_awin = window_create(&config, am_archive, winport, AppPort);
										if(appmenu_awin) {
											window_open(appmenu_awin, appwin_mp);
											window_req_open_archive(appmenu_awin, &config, TRUE);
											if(window_get_archiver(appmenu_awin) != ARC_NONE) {
												long ret = extract(appmenu_awin, am_archive, tempdest, NULL);
												FreeVec(tempdest);
												if(ret != 0) show_error(ret, awin);
											}
											window_dispose(appmenu_awin);
										}
										FreeVec(am_archive);
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
			} else if(wait & rxsig) {
				/* ARexx messages */
				ULONG evt = ami_arexx_handle();
				
				switch(evt) {
					case RXEVT_OPEN: /* Archive set on OPEN */
						{
							void *arexx_awin = window_create(&config, arexx_get_event(), winport, AppPort);
							if(arexx_awin) {
								window_open(arexx_awin, appwin_mp);
								window_req_open_archive(arexx_awin, &config, TRUE);
							}
						}
					break;
					
					case RXEVT_SHOW:
						show_hide_all(appwin_mp, winport, AppPort, TRUE, FALSE);
					break;
				}
				arexx_free_event();
			} else if(na_sig && (wait & na_sig)) {
				BOOL na_done = FALSE;
				while((na_done == FALSE) && ((result = newarc_window_handle_input(&code)) != WMHI_LASTMSG)) {
					na_done = newarc_window_handle_input_events(result, code);
				}
			} else {
				if(IsMinListEmpty((struct MinList *)&win_list) == FALSE) {
					awin = (void *)GetHead((struct List *)&win_list);
					struct Node *nnode;

					do {
						nnode = (struct Node *)GetSucc((struct Node *)awin);

						while((done == WIN_DONE_OK) && ((result = window_handle_input(awin, &code)) != WMHI_LASTMSG)) {
							done = window_handle_input_events(awin, &config, result, appwin_mp, code, winport, AppPort);
						}
					} while((done == WIN_DONE_OK) && (awin = (void *)nnode));
				}
			}
			if(done == WIN_DONE_CLOSED) {
				if(window_count == 1) {
					ULONG ret = ask_quithide(NULL);
					/* Last window closed */
					if(ret) {
						window_close(awin, FALSE);
						if(ret == 1) {
							window_dispose(awin);
							done = WIN_DONE_QUIT;
						}
					}
				} else {
					window_close(awin, FALSE);
					window_dispose(awin);
				}
			}
		} // while
	}

	close_all_windows();
	config_window_break();

	if(cx_broker && cx_mp) UnregisterCx(cx_broker, cx_mp);

	RemoveAppMenuItem(appmenu);
	if(appwin_mp) DeleteMsgPort(appwin_mp);
	if(winport) DeleteMsgPort(winport);
	DeleteMsgPort(AppPort);
}

static void gettooltypes(struct WBArg *wbarg)
{
	struct DiskObject *dobj;
	STRPTR *toolarray;
	char *s;

	if((*wbarg->wa_Name) && (dobj = GetDiskObject(wbarg->wa_Name))) {
		toolarray = (STRPTR *)dobj->do_ToolTypes;

		if(s = (char *)FindToolType(toolarray, "CX_POPUP")) {
			if(MatchToolValue(s, "NO")) {
				config.cx_popup = FALSE;
			} else {
				config.cx_popup = TRUE;
			}
		}

		if(s = (char *)FindToolType(toolarray,"CX_PRIORITY")) config.cx_pri = atoi(s);

		s = (char *)FindToolType(toolarray, "CX_POPKEY");
		if(s == NULL) s = "ctrl alt a";

		int len = strlen("rawkey ") + strlen(s) + 1;
		if(config.cx_popkey = AllocVec(len, MEMF_ANY | MEMF_CLEAR)) {
			strncpy(config.cx_popkey, "rawkey ", len);
			strncat(config.cx_popkey, s, len);
		}

		if(s = (char *)FindToolType(toolarray, "SOURCEDIR")) {
			config.sourcedir = strdup_vec(s);
		} else {
			config.sourcedir = strdup_vec("RAM:");
		}

		if(s = (char *)FindToolType(toolarray, "DEST")) {
			config.dest = strdup_vec(s);
		} else {
			config.dest = strdup_vec("RAM:");
		}

		dest_needs_free = TRUE;

		s = (char *)FindToolType(toolarray, "TMPDIR");
		if(s == NULL) s = "T:";

		if(config.tmpdir) {
			strncpy(config.tmpdir, s, 90);
			config.tmpdirlen = strlen(config.tmpdir);
		}

		if(s = (char *)FindToolType(toolarray, "VIEWMODE")) {
			if(MatchToolValue(s, "LIST")) {
				config.viewmode = 1;
			}
		}
		if(FindToolType(toolarray, "VIRUSSCAN")) config.virus_scan = TRUE;
		if(FindToolType(toolarray, "NOASLHOOK")) config.disable_asl_hook = TRUE;
		if(FindToolType(toolarray, "IGNOREFS")) config.ignorefs = TRUE;
		if(FindToolType(toolarray, "DEBUG")) config.debug = TRUE;
		if(FindToolType(toolarray, "DRAGLOCK")) config.drag_lock = TRUE;
		if(FindToolType(toolarray, "AISS")) config.aiss = TRUE;
		if(FindToolType(toolarray, "OPENWBONEXTRACT")) config.openwb = TRUE;
		
		if(s = (char *)FindToolType(toolarray, "CLOSE")) {
			if(MatchToolValue(s, "HIDE")) {
				config.closeaction = 2;
			} else if(MatchToolValue(s, "QUIT")) {
				config.closeaction = 1;
			} else {
				config.closeaction = 0;
			}
		}

		if(s = (char *)FindToolType(toolarray, "MODULES")) {
			config.activemodules = 0;

			if(MatchToolValue(s, "XAD")) {
				config.activemodules |= ARC_XAD;
			}

			if(MatchToolValue(s, "XFD")) {
				config.activemodules |= ARC_XFD;
			}

			if(MatchToolValue(s, "DEARK")) {
				config.activemodules |= ARC_DEARK;
			}
		}

		if(s = (char *)FindToolType(toolarray,"PROGRESSSIZE")) config.progress_size = atoi(s);

		if(s = (char *)FindToolType(toolarray,"WINX")) config.win_x = atoi(s);
		if(s = (char *)FindToolType(toolarray,"WINY")) config.win_y = atoi(s);
		if(s = (char *)FindToolType(toolarray,"WINW")) config.win_w = atoi(s);
		if(s = (char *)FindToolType(toolarray,"WINH")) config.win_h = atoi(s);

		FreeDiskObject(dobj);
	}
}

/** Main program **/
int main(int argc, char **argv)
{
	char *tmp = NULL;
	struct WBStartup *WBenchMsg = NULL;
	char *archive = NULL; /* Archive provided on command line */
	ULONG rxsig = 0;
	LONG rarray[] = {0};
	struct RDArgs *args;
	STRPTR template = "FILE";

	enum
	{
		A_FILE
	};

	if(libs_open() == FALSE) {
		return RETURN_ERROR;
	}


	/* Initialise config default values */
	config.progname = NULL;
	config.dest = NULL;
	config.disable_asl_hook = FALSE;
	config.viewmode = 0;
	config.virus_scan = FALSE;
	config.debug = FALSE;
	config.ignorefs = FALSE;
	config.disable_vscan_menu = FALSE;
	config.closeaction = 0; // Ask
	config.drag_lock = FALSE;
	config.aiss = FALSE;

	config.activemodules = ARC_XAD | ARC_XFD; /* ARC_DEARK disabled by default */

	config.win_x = 0;
	config.win_y = 0;
	config.win_w = 0;
	config.win_h = 0;
	config.progress_size = PROGRESS_SIZE_DEFAULT;

	config.cx_pri = 0;
	config.cx_popup = TRUE;
	config.cx_popkey = NULL;

	config.tmpdir = AllocVec(100, MEMF_CLEAR);
	config.tmpdirlen = 0;

	InitSemaphore((struct SignalSemaphore *)&config);

	if(argc == 0) {
		int i;
		/* Started from WB */
		WBenchMsg = (struct WBStartup *)argv;
		struct WBArg *wbarg = WBenchMsg->sm_ArgList;

		if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
			if(config.progname = AllocVec(40, MEMF_CLEAR)) {
				strncpy(config.progname, "PROGDIR:", 40);
				AddPart(config.progname, wbarg->wa_Name, 40);
			}
		}

		for(i=0, wbarg=WBenchMsg->sm_ArgList; i<WBenchMsg->sm_NumArgs; i++, wbarg++) {
			BPTR olddir =-1;
			if((wbarg->wa_Lock)&&(*wbarg->wa_Name))
				olddir = CurrentDir(wbarg->wa_Lock);

			gettooltypes(wbarg);
			if(olddir !=-1) CurrentDir(olddir);
		}
	} else {
		if(config.tmpdir) {
			strcpy(config.tmpdir, "T:");
			config.tmpdirlen = strlen(config.tmpdir);
		}

		args = ReadArgs(template, rarray, NULL);

		if(args) {
			if(rarray[A_FILE]) {
				archive = strdup_vec((char *)rarray[A_FILE]);
			}

			FreeArgs(args);
		} else {
			return RETURN_ERROR; /* TODO: Will never get here, but if we add required args
					      * then will need to do proper cleanup */
		}
	}

	Locale_Open("avalanche.catalog", 0, 0);

	fill_menu_labels();

	if(tmp = AllocVec(20, MEMF_CLEAR)) {
		snprintf(tmp, 20, "Avalanche.%lx", GetUniqueID());
		AddPart(config.tmpdir, tmp, 100);
		UnLock(CreateDir(config.tmpdir));
		FreeVec(tmp);
	}

	if(ami_arexx_init(&rxsig)) {
		/* ARexx port did not already exist */
		gui(WBenchMsg, rxsig, archive);
	} else {
		BOOL arc_opened = FALSE;
		if(WBenchMsg) {
			if(WBenchMsg->sm_NumArgs > 1) {
				/* Started as default tool, get the path+filename of the (first) project */
				struct WBArg *wbarg = WBenchMsg->sm_ArgList + 1;

				if(open_archive_from_wbarg_arexx(wbarg)) {
					if(WBenchMsg->sm_NumArgs > 2) {
						for(int i = 2; i < WBenchMsg->sm_NumArgs; i++) {
							wbarg++;
							open_archive_from_wbarg_arexx(wbarg);
						}
					}
					arc_opened = TRUE;
				}
			}
		} else if(archive) {
			char cmd[1024];
			snprintf(cmd, 1024, "OPEN \"%s\"", archive);
			FreeVec(archive);
			ami_arexx_send(cmd);
			arc_opened = TRUE;
		}
		if(arc_opened == FALSE) ami_arexx_send("SHOW");
	}

	ami_arexx_cleanup();
	Locale_Close();

	DeleteFile(config.tmpdir);

	CONFIG_LOCK;

	if(config.cx_popkey) FreeVec(config.cx_popkey);
	if(config.tmpdir) FreeVec(config.tmpdir);
	if(config.sourcedir) FreeVec(config.sourcedir);
	if(config.progname != NULL) FreeVec(config.progname);

	CONFIG_UNLOCK;

	if(dest_needs_free) free_dest_path();

	module_exit();
	libs_zip_exit();
	libs_close();

	return RETURN_OK;
}
