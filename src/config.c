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

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>

#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif
#include <dos/dostags.h>
#include <libraries/asl.h>

#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"

#include "Avalanche_rev.h"

enum {
	GID_C_MAIN = 0,
	GID_C_SCAN,
	GID_C_IGNOREFS,
	GID_C_OPENWB,
	GID_C_DEST,
	GID_C_VIEWMODE,
	GID_C_QUIT,
	GID_C_SAVE,
	GID_C_USE,
	GID_C_CANCEL,
	GID_C_LAST
};

enum {
	WID_MAIN = 0,
	WID_LAST
};

enum {
	OID_MAIN = 0,
	OID_LAST
};

static struct Window *windows[WID_LAST];
static struct Gadget *gadgets[GID_C_LAST];
static Object *objects[OID_LAST];
static struct MsgPort *cw_port = NULL;
#ifndef __amigaos4__
static struct HintInfo hi;
#endif

static struct Process *config_process = NULL;
static struct Process *avalanche_process = NULL;

static BOOL config_window_handle_input_events(struct avalanche_config *config, ULONG result, UWORD code);

static void config_window_close(void)
{
	RA_CloseWindow(objects[OID_MAIN]);
	windows[WID_MAIN] = NULL;
	DisposeObject(objects[OID_MAIN]);

	DeleteMsgPort(cw_port);
	cw_port = NULL;
}

static void config_req_dest(void)
{
	DoMethod((Object *)gadgets[GID_C_DEST], GFILE_REQUEST, windows[WID_MAIN]);
}

static void config_save(struct avalanche_config *config)
{
	struct DiskObject *dobj;
	UBYTE **oldtooltypes;
	UBYTE *newtooltypes[22];
	char tt_dest[100];
	char tt_srcdir[100];
	char tt_tmp[100];
	char tt_winx[15];
	char tt_winy[15];
	char tt_winh[15];
	char tt_winw[15];
	char tt_progresssize[20];
	char tt_cxpri[20];
	char tt_cxpopkey[50];
	char tt_modules[40];

	CONFIG_LOCK;

	if(dobj = GetIconTagList(config->progname, NULL)) {
		oldtooltypes = (UBYTE **)dobj->do_ToolTypes;

		if(config->dest && (strcmp("RAM:", config->dest) != 0)) {
			snprintf(tt_dest, 100, "DEST=%s", config->dest);
			newtooltypes[0] = tt_dest;
		} else {
			newtooltypes[0] = "(DEST=RAM:)";
		}

		if(config->viewmode == 1) {
			newtooltypes[1] = "VIEWMODE=LIST";
		} else {
			newtooltypes[1] = "(VIEWMODE=LIST|BROWSER)";
		}

		if(config->disable_asl_hook) {
			newtooltypes[2] = "NOASLHOOK";
		} else {
			newtooltypes[2] = "(NOASLHOOK)";
		}

		if(config->win_x) {
			snprintf(tt_winx, 15, "WINX=%lu", config->win_x);
			newtooltypes[3] = tt_winx;
		} else {
			newtooltypes[3] = "(WINX=0)";
		}

		if(config->win_y) {
			snprintf(tt_winy, 15,"WINY=%lu", config->win_y);
			newtooltypes[4] = tt_winy;
		} else {
			newtooltypes[4] = "(WINY=0)";
		}

		if(config->win_w) {
			snprintf(tt_winw, 15, "WINW=%lu", config->win_w);
			newtooltypes[5] = tt_winw;
		} else {
			newtooltypes[5] = "(WINW=0)";
		}

		if(config->win_h) {
			snprintf(tt_winh, 15, "WINH=%lu", config->win_h);
			newtooltypes[6] = tt_winh;
		} else {
			newtooltypes[6] = "(WINH=0)";
		}

		if(config->progress_size != PROGRESS_SIZE_DEFAULT) {
			snprintf(tt_progresssize, 20, "PROGRESSSIZE=%lu", config->progress_size);
		} else {
			snprintf(tt_progresssize, 20, "(PROGRESSSIZE=%d)", PROGRESS_SIZE_DEFAULT);
		}

		newtooltypes[7] = tt_progresssize;

		if(config->virus_scan) {
			newtooltypes[8] = "VIRUSSCAN";
		} else {
			newtooltypes[8] = "(VIRUSSCAN)";
		}

		switch(config->closeaction) {
			case 1:
				newtooltypes[9] = "CLOSE=QUIT";
			break;
			case 2:
				newtooltypes[9] = "CLOSE=HIDE";
			break;
			case 0:
				newtooltypes[9] = "CLOSE=ASK";
			break;
		}

		if(config->ignorefs) {
			newtooltypes[10] = "IGNOREFS";
		} else {
			newtooltypes[10] = "(IGNOREFS)";
		}

		if(config->tmpdir && (strncmp("T:", config->tmpdir, config->tmpdirlen) != 0)) {
			strcpy(tt_tmp, "TMPDIR=");
			newtooltypes[11] = strncat(tt_tmp, config->tmpdir, config->tmpdirlen);
		} else {
			newtooltypes[11] = "(TMPDIR=T:)";
		}

		if(config->cx_popup == FALSE) {
			newtooltypes[12] = "CX_POPUP=NO";
		} else {
			newtooltypes[12] = "(CX_POPUP=YES)";
		}

		if(config->cx_pri != 0) {
			snprintf(tt_cxpri, 20, "CX_PRIORITY=%d", config->cx_pri);
		} else {
			snprintf(tt_cxpri, 20, "(CX_PRIORITY=0)");
		}
		newtooltypes[13] = tt_cxpri;

		if((config->cx_popkey) && (strcmp(config->cx_popkey, "rawkey ctrl alt a") != 0)) {
			snprintf(tt_cxpopkey, 50, "CX_POPKEY=%s", config->cx_popkey + 7);
		} else {
			snprintf(tt_cxpopkey, 50, "(CX_POPKEY=ctrl alt a)");
		}
		newtooltypes[14] = tt_cxpopkey;

		newtooltypes[15] = "DONOTWAIT";

		if(config->sourcedir && (strcmp("RAM:", config->sourcedir) != 0)) {
			snprintf(tt_srcdir, 100, "SOURCEDIR=%s", config->sourcedir);
			newtooltypes[16] = tt_srcdir;

		} else {
			newtooltypes[16] = "(SOURCEDIR=RAM:)";
		}

		if(config->drag_lock) {
			newtooltypes[17] = "DRAGLOCK";
		} else {
			newtooltypes[17] = "(DRAGLOCK)";
		}

		if(config->aiss) {
			newtooltypes[18] = "AISS";
		} else {
			newtooltypes[18] = "(AISS)";
		}

		if(config->activemodules == (ARC_XAD | ARC_XFD)) {
			snprintf(tt_modules, 40, "(MODULES=XAD|XFD|DEARK)");
		} else {
			snprintf(tt_modules, 40, "MODULES=");
			if(config->activemodules & ARC_XAD) {
				strncat(tt_modules, "XAD", 40);
				if((config->activemodules & ARC_XFD) || (config->activemodules & ARC_DEARK)) {
					strncat(tt_modules, "|", 40);
				}
			}
			if(config->activemodules & ARC_XFD) {
				strncat(tt_modules, "XFD", 40);
				if((config->activemodules & ARC_DEARK)) {
					strncat(tt_modules, "|", 40);
				}
			}
			if(config->activemodules & ARC_DEARK) {
				strncat(tt_modules, "DEARK", 40);
			}
		}
		newtooltypes[19] = tt_modules;

		if(config->openwb) {
			newtooltypes[20] = "OPENWBONEXTRACT";
		} else {
			newtooltypes[20] = "(OPENWBONEXTRACT)";
		}

		newtooltypes[21] = NULL;

		dobj->do_ToolTypes = (STRPTR *)&newtooltypes;
		PutIconTags(config->progname, dobj, NULL);
		dobj->do_ToolTypes = (STRPTR *)oldtooltypes;
		FreeDiskObject(dobj);
	}
	CONFIG_UNLOCK;
}

static void config_window_settings(struct avalanche_config *config, BOOL save)
{
	ULONG data = 0;

	CONFIG_LOCK_EX; /* EXCLUSIVE */

#if 0
	free_dest_path();
	GetAttr(GETFILE_Drawer, gadgets[GID_C_DEST], (APTR)&config->dest);
#endif
	GetAttr(GA_Selected, gadgets[GID_C_SCAN],(ULONG *)&data);
	config->virus_scan = (data ? TRUE : FALSE);

	GetAttr(GA_Selected, gadgets[GID_C_IGNOREFS],(ULONG *)&data);
	config->ignorefs = (data ? TRUE : FALSE);
	
	GetAttr(GA_Selected, gadgets[GID_C_OPENWB],(ULONG *)&data);
	config->openwb = (data ? TRUE : FALSE);
	
	GetAttr(CHOOSER_Selected, gadgets[GID_C_QUIT], (ULONG *)&data);
	config->closeaction = data;

	GetAttr(CHOOSER_Selected, gadgets[GID_C_VIEWMODE], (ULONG *)&data);
	config->viewmode = data;

	CONFIG_UNLOCK;

	if(save) config_save(config);
}

static void config_window_open_internal(struct avalanche_config *config)
{
	BOOL save_disabled = FALSE;
	STRPTR quit_opts[] = {
			locale_get_string(MSG_QUITCFG_ASK),
			locale_get_string(MSG_QUITCFG_QUIT),
			locale_get_string(MSG_QUITCFG_HIDE),
			NULL
	};

	STRPTR viewmode_opts[] = {
			locale_get_string(MSG_VIEWMODEBROWSER),
			locale_get_string(MSG_VIEWMODELIST),
			NULL
	};

	if(CONFIG_GET_LOCK(progname) == NULL) save_disabled = TRUE;
	CONFIG_UNLOCK;

	if(cw_port = CreateMsgPort()) {
		CONFIG_LOCK;
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
			WINDOW_SharedPort, cw_port,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_C_MAIN] = LayoutVObj,
				//LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutVObj,
#if 0
					LAYOUT_AddChild,  gadgets[GID_C_DEST] = GetFileObj,
						GA_ID, GID_C_DEST,
						GA_RelVerify, TRUE,
						GETFILE_TitleText,  locale_get_string( MSG_SELECTDESTINATION ) ,
						GETFILE_Drawer, config->dest,
						GETFILE_DoSaveMode, TRUE,
						GETFILE_DrawersOnly, TRUE,
						GETFILE_ReadOnly, TRUE,
					End,
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text,  locale_get_string( MSG_DESTINATION ) ,
					LabelEnd,
#endif
					LAYOUT_AddChild,  gadgets[GID_C_SCAN] = CheckBoxObj,
						GA_ID, GID_C_SCAN,
						HINTINFO, locale_get_string(MSG_HI_C_SCAN),
						GA_RelVerify, TRUE,
						GA_Disabled, config->disable_vscan_menu,
						GA_Text, locale_get_string( MSG_SCANFORVIRUSES ) ,
						GA_Selected, config->virus_scan,
					End,
					LAYOUT_AddChild,  gadgets[GID_C_IGNOREFS] = CheckBoxObj,
						GA_ID, GID_C_IGNOREFS,
						HINTINFO, locale_get_string(MSG_HI_C_IGNOREFS),
						GA_RelVerify, TRUE,
						GA_Text, locale_get_string( MSG_IGNOREFILESYSTEMS ) ,
						GA_Selected, config->ignorefs,
					End,	
					LAYOUT_AddChild,  gadgets[GID_C_OPENWB] = CheckBoxObj,
						HINTINFO, locale_get_string(MSG_HI_C_OPENWB),
						GA_ID, GID_C_OPENWB,
						GA_RelVerify, TRUE,
						GA_Text, locale_get_string( MSG_OPENWBONEXTRACT ) ,
						GA_Selected, config->openwb,
					End,
					LAYOUT_AddChild, gadgets[GID_C_VIEWMODE] = ChooserObj,
						GA_ID, GID_C_VIEWMODE,
						HINTINFO, locale_get_string(MSG_HI_C_VIEWMODE),
						GA_RelVerify, TRUE,
						CHOOSER_Selected, config->viewmode,
						CHOOSER_PopUp, TRUE,
						CHOOSER_LabelArray, viewmode_opts,
					End,
					CHILD_Label, LabelObj,
						LABEL_Text, locale_get_string(MSG_VIEWMODE),
					LabelEnd,
					LAYOUT_AddChild, gadgets[GID_C_QUIT] = ChooserObj,
						GA_ID, GID_C_QUIT,
						HINTINFO, locale_get_string(MSG_HI_C_QUIT),
						GA_RelVerify, TRUE,
						CHOOSER_Selected, config->closeaction,
						CHOOSER_PopUp, TRUE,
						CHOOSER_LabelArray, quit_opts,
					End,
					CHILD_Label, LabelObj,
						LABEL_Text, locale_get_string(MSG_LASTWINDOWACTION),
					LabelEnd,
					LAYOUT_AddChild,  LayoutHObj,
						LAYOUT_AddChild,  gadgets[GID_C_SAVE] = ButtonObj,
							GA_ID, GID_C_SAVE,
							HINTINFO, locale_get_string(MSG_HI_C_SAVE),
							GA_RelVerify, TRUE,
							GA_Text, locale_get_string( MSG_SAVE ),
							GA_Disabled, save_disabled,
						ButtonEnd,
						LAYOUT_AddChild,  gadgets[GID_C_USE] = ButtonObj,
							GA_ID, GID_C_USE,
							HINTINFO, locale_get_string(MSG_HI_C_USE),
							GA_RelVerify, TRUE,
							GA_Text, locale_get_string( MSG_USE ),
						ButtonEnd,
						LAYOUT_AddChild,  gadgets[GID_C_CANCEL] = ButtonObj,
							GA_ID, GID_C_CANCEL,
							HINTINFO, locale_get_string(MSG_HI_C_CANCEL),
							GA_RelVerify, TRUE,
							GA_Text, locale_get_string( MSG_CANCEL ),
						ButtonEnd,
						CHILD_WeightedHeight, 0,
					LayoutEnd,
				LayoutEnd,
			EndGroup,
		EndWindow;

		CONFIG_UNLOCK;

		if(objects[OID_MAIN]) {
			if(windows[WID_MAIN] = (struct Window *)RA_OpenWindow(objects[OID_MAIN])) {

				ULONG sigbit = (1L << cw_port->mp_SigBit);
				BOOL done = FALSE;

				while(!done) {
					ULONG wait = Wait(sigbit | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

					if(wait & sigbit) {
						UWORD code;
						ULONG result = RA_HandleInput(objects[OID_MAIN], &code);
						done = config_window_handle_input_events(config, result, code);
					} else if (wait & SIGBREAKF_CTRL_C) {
						config_window_close();
						Signal(avalanche_process, SIGF_SINGLE);
						done = TRUE;
					} else if (wait & SIGBREAKF_CTRL_F) {
						WindowToFront(windows[WID_MAIN]);
					}
				}
			}
		}
	}
	
	return;
}

#ifdef __amigaos4__
static void config_window_open_p(void)
#else
static void __saveds config_window_open_p(void)
#endif
{
	config_window_open_internal(get_config());

	config_process = NULL;
}



/* public functions */

void config_window_open(struct avalanche_config *config)
{
	if(config_process != NULL) {
		/* Bring to front */
		Signal(config_process, SIGBREAKF_CTRL_F);
	} else {
		avalanche_process = FindTask(NULL);
		config_process = CreateNewProcTags(NP_Entry, config_window_open_p,
				NP_Name, "Avalanche config process",
			TAG_DONE);
	}
}

void config_window_break(void)
{
	if(config_process == NULL) return;

	SetSignal(0L, SIGF_SINGLE);
	Signal(config_process, SIGBREAKF_CTRL_C);
	Wait(SIGF_SINGLE);
}

static BOOL config_window_handle_input_events(struct avalanche_config *config, ULONG result, UWORD code)
{
	long ret = 0;
	ULONG done = FALSE;

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
			config_window_close();
			done = TRUE;
		break;

		case WMHI_GADGETUP:
			switch (result & WMHI_GADGETMASK) {
				case GID_C_DEST:
					config_req_dest();
				break;
				case GID_C_SAVE:
					config_window_settings(config, TRUE);
					config_window_close();
					done = TRUE;
				break;
				case GID_C_USE:
					config_window_settings(config, FALSE);
					// fall through
				case GID_C_CANCEL:
					config_window_close();
					done = TRUE;
				break;
			}
			break;
	}

	return done;
}

/* Get config if safe to obtain a shared lock */
struct avalanche_config *config_get(void)
{
	struct avalanche_config *config = get_config();
	ObtainSemaphoreShared((struct SignalSemaphore *)config);
	return config;
}
