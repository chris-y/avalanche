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
#include <stdlib.h>
#include <string.h>

#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#include <dos/dostags.h>

#include <intuition/icclass.h>
#include <intuition/pointerclass.h>

#include <libraries/asl.h>
#include <libraries/gadtools.h>

#include <proto/button.h>
#include <proto/clicktab.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/clicktab.h>
#ifndef __amigaos4__
#include <gadgets/fuelgauge.h>
#endif
#include <gadgets/listbrowser.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "glyph.h"
#include "http.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
#include "module.h"
#include "new.h"
#include "progress.h"
#include "req.h"
#include "tab.h"
#include "win.h"

#include "deark.h"
#include "xad.h"
#include "xfd.h"

#include "Avalanche_rev.h"

enum {
	GID_MAIN = 0,
	GID_ARCHIVE,
	GID_OPENWB,
	GID_BROWSERLAYOUT,
	GID_TREELAYOUT,
	GID_TREE,
	GID_LIST,
	GID_EXTRACT,
	GID_PROGRESS,
	GID_PROGRESSFR,
	GID_ABORT,
	GID_TABS,
	GID_LAST
};

enum {
	WID_MAIN = 0,
	WID_LAST
};

enum {
	OID_MAIN = 0,
	OID_LAST
};

#define AVALANCHE_AUTOFIT TRUE
#define AVALANCHE_DROPZONES 2
#define TITLE_MAX_SIZE 100

struct avalanche_window {
	struct MinNode node;
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];
	struct NewMenu *menu;
	struct ColumnInfo *lbci;
	struct ColumnInfo *dtci;
	struct Hook aslfilterhook;
	struct Hook appwindzhook;
	struct Hook lbsorthook;
	struct MsgPort *appwin_mp;
	struct AppWindow *appwin;
	struct AppWindowDropZone *appwindz[AVALANCHE_DROPZONES];
	BOOL flat_mode;
	BOOL drag_lock;
	BOOL iconified;
	BOOL has_dz;
	char title[TITLE_MAX_SIZE];
	struct List tab_list;
	struct Node *tab_node; /* current tab */
	ULONG tab_count;
#ifndef __amigaos4__
	struct HintInfo hi;
	struct Hook idcmphook;
	BOOL tab_closed;
#else
	ULONG p_width;
	ULONG p_height;
	ULONG p_sz_height;
#endif
};

static struct List winlist;

#ifndef __amigaos4__
#define fr_NumArgs rf_NumArgs
#define fr_ArgList rf_ArgList

#define EXDF_HOLD FIBF_HOLD
#define EXDF_SCRIPT FIBF_SCRIPT
#define EXDF_PURE FIBF_PURE
#define EXDF_ARCHIVE FIBF_ARCHIVE
#define EXDF_NO_READ FIBF_READ
#define EXDF_NO_WRITE FIBF_WRITE
#define EXDF_NO_EXECUTE FIBF_EXECUTE
#define EXDF_NO_DELETE FIBF_DELETE

#define P_WIDTH 0
#define P_HEIGHT 0
#else
#define P_WIDTH aw->p_width
#define P_HEIGHT aw->p_height
#endif

#define ANY_SHIFT (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT)

/** Extract process **/

struct avalanche_extract_userdata {
	void *awin;
	const char *archive;
	const char *newdest;
	struct Node *node;
	struct Node *tab_node;
	ULONG sig;
};

/** Menu **/

#ifdef __amigaos4__
#define MIF_SHIFTCOMMSEQ 0
#define AVALANCHE_MS_SPLIT 0
#else
#define AVALANCHE_MS_SPLIT "O"
#endif

static struct NewMenu menu[] = {
	{NM_TITLE,  NULL,           0,  0, 0, 0,}, // 0 Project
		{NM_ITEM,   NULL,         "N", 0, 0, 0,}, // 0 New archive
		{NM_ITEM,   NULL,         "T", 0, 0, 0,}, // 1 New tab
		{NM_ITEM,   NULL,         0, 0, 0, 0,}, // 2 Open
			{NM_SUB,   NULL,        "O", 0, 0, 0,}, // 0 Archive
			{NM_SUB,   NULL,        AVALANCHE_MS_SPLIT, MIF_SHIFTCOMMSEQ, 0, 0,}, // 1 Split
		{NM_ITEM,   NULL,         "X", 0, 0, 0,}, // 3 Extract
		{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 4
		{NM_ITEM,   NULL , "!", NM_ITEMDISABLED, 0, 0,}, // 5 Archive Info
		{NM_ITEM,   NULL ,        "?", 0, 0, 0,}, // 6 About
		{NM_ITEM,   NULL ,        0, 0, 0, 0,}, // 7 Check version

		{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 8
		{NM_ITEM,   NULL,         "Q", 0, 0, 0,}, // 9 Quit

	{NM_TITLE,  NULL,               0,  0, 0, 0,}, // 1 Edit
		{NM_ITEM,   NULL,       "A", NM_ITEMDISABLED, 0, 0,}, // 0 Select All
		{NM_ITEM,   NULL ,  "Z", NM_ITEMDISABLED, 0, 0,}, // 1 Clear Selection
		{NM_ITEM,   NULL , "I", NM_ITEMDISABLED, 0, 0,}, // 2 Invert
		{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 3
		{NM_ITEM,   NULL,       ".", NM_ITEMDISABLED, 0, 0,}, // 4 Add files
		{NM_ITEM,   NULL,       0, NM_ITEMDISABLED, 0, 0,}, // 5 Delete files
		{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 6
		{NM_ITEM,   NULL , "L", CHECKIT | MENUTOGGLE, 0, 0,}, // 7 Toggle drag lock

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Window
		{NM_ITEM,   NULL,         "W", 0, 0, 0,}, // 0 New window
		{NM_ITEM,	NULL , 0, 0, 0, 0,}, // 1 View mode
			{NM_SUB,	NULL , 0, CHECKIT, ~1, 0,}, // 0 Browser
			{NM_SUB,	NULL , 0, CHECKIT, ~2, 0,}, // 1 List
		{NM_ITEM, NULL , 0, 0, 0, 0,}, // 2 Dir tree
			{NM_SUB,  NULL , "-", 0, 0, 0,}, // 0 Collapse all
			{NM_SUB,  NULL , "=", 0, 0, 0,}, // 1 Expand all
		{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 2
		{NM_ITEM, NULL , "K", 0, 0, 0,}, // 4 Close

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 3 Settings
		{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 0 Snapshot
		{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 1 Preferences

	{NM_END,   NULL,        0,  0, 0, 0,},
};

#define MENU_DRAGLOCK 21
#define MENU_FLATMODE 25
#define MENU_LISTMODE 26

#ifdef __amigaos4__
#define CLICKTAB_MinorLabelChange TAG_IGNORE
#define TNAHintInfo TNA_HintInfo
#else
#define TNAHintInfo TNA_HelpText
#endif

/* Private functions */
#ifdef __amigaos4__
static ULONG aslfilterfunc(struct Hook *h, struct FileRequester *fr, struct AnchorPathOld *ap)
#else
static ULONG __saveds aslfilterfunc(__reg("a0") struct Hook *h, __reg("a2") struct FileRequester *fr, __reg("a1") struct AnchorPath *ap)
#endif
{
	BOOL found = FALSE;
	char fullfilename[256];

	if(ap->ap_Info.fib_DirEntryType > 0) return(TRUE); /* Drawer */

	strncpy(fullfilename, fr->fr_Drawer, 255);
	fullfilename[255] = 0;
	AddPart(fullfilename, ap->ap_Info.fib_FileName, 256);

	found = module_recog(fullfilename);

	return found;
}

#ifdef __amigaos4__
static LONG appwindzhookfunc(struct Hook *h, APTR reserved, struct AppWindowDropZoneMsg *awdzm)
#else
static LONG __saveds appwindzhookfunc(__reg("a0") struct Hook *h, __reg("a2") APTR reserved, __reg("a1") struct AppWindowDropZoneMsg *awdzm)
#endif
{
	struct avalanche_window *aw = (struct avalanche_window *)awdzm->adzm_UserData;

	/* Only change if we are able to add files */
	if(module_has_add(aw->tab_node) == FALSE) return 0;

	/* This hook function does some very basic drawing to highlight the listbrowser
	 * in event that writing is allowed and the user is dragging a file over the listbrowser */

	ULONG left, top, width, height;
	
	GetAttr(GA_Top, aw->gadgets[GID_LIST], &top);
	GetAttr(GA_Left, aw->gadgets[GID_LIST], &left);
	GetAttr(GA_Width, aw->gadgets[GID_LIST], &width);
	GetAttr(GA_Height, aw->gadgets[GID_LIST], &height);

	switch(awdzm->adzm_Action) {
		case ADZMACTION_Enter:
			SetAPen(awdzm->adzm_RastPort, 3);
			Move(awdzm->adzm_RastPort, left -1, top - 1);
			Draw(awdzm->adzm_RastPort, left + width + 1, top - 1);
			Draw(awdzm->adzm_RastPort, left + width + 1, top + height + 1);
			Draw(awdzm->adzm_RastPort, left - 1, top + height + 1);
			Draw(awdzm->adzm_RastPort, left - 1, top - 1);
		break;
		
		case ADZMACTION_Leave:
			SetAPen(awdzm->adzm_RastPort, 0);
			Move(awdzm->adzm_RastPort, left -1, top - 1);
			Draw(awdzm->adzm_RastPort, left + width + 1, top - 1);
			Draw(awdzm->adzm_RastPort, left + width + 1, top + height + 1);
			Draw(awdzm->adzm_RastPort, left - 1, top + height + 1);
			Draw(awdzm->adzm_RastPort, left - 1, top - 1);
		break;
	}

	return 0;
}

static BOOL window_tab_is_current(struct avalanche_window *aw, struct Node *tab_node)
{
	if(tab_node == aw->tab_node) return TRUE;
	
	return FALSE;
}

static struct List *window_get_dirtree_list(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return tab_get_dirtree_list(aw->tab_node);
}

static void window_remove_dropzones(struct avalanche_window *aw)
{
	if(aw->has_dz == FALSE) return;

	if(aw->appwindz) {
		for(int i=0; i<AVALANCHE_DROPZONES; i++) {
			RemoveAppWindowDropZone(aw->appwin, aw->appwindz[i]);
			aw->appwindz[i] = NULL;
		}
	}
	aw->has_dz = FALSE;
}

static void window_add_dropzones(struct avalanche_window *aw, BOOL edit)
{
	BOOL ndz = CONFIG_GET_LOCK(no_dropzones);
	CONFIG_UNLOCK;

	if(ndz) return;

	if(aw->has_dz) window_remove_dropzones(aw);

	if(aw->appwin) {
		aw->appwindzhook.h_Entry = appwindzhookfunc;
		aw->appwindzhook.h_SubEntry = NULL;
		aw->appwindzhook.h_Data = NULL;
		
		if(edit) {
			/* listbrowser */
			ULONG left, top, width, height;
		
			GetAttr(GA_Top, aw->gadgets[GID_LIST], &top);
			GetAttr(GA_Left, aw->gadgets[GID_LIST], &left);
			GetAttr(GA_Width, aw->gadgets[GID_LIST], &width);
			GetAttr(GA_Height, aw->gadgets[GID_LIST], &height);

			aw->appwindz[1] = AddAppWindowDropZone(aw->appwin, 1, (ULONG)aw,
										WBDZA_Left, left,
										WBDZA_Top, top, 
										WBDZA_Width, width,
										WBDZA_Height, height,						
										WBDZA_Hook, &aw->appwindzhook,
									TAG_END);
		}

		/* whole window */
		aw->appwindz[0] = AddAppWindowDropZone(aw->appwin, 0, (ULONG)aw,
										WBDZA_Left, 0,
										WBDZA_Top, 0, 
										WBDZA_Width, aw->windows[WID_MAIN]->Width,
										WBDZA_Height, aw->windows[WID_MAIN]->Height,						
									TAG_END);
	}

	aw->has_dz = TRUE;
}

/* Activate/disable menus related to an open archive
 * busy - indicates if window is busy (eg. extract process running)
 */
static void window_menu_activation(void *awin, BOOL enable, BOOL busy)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;

	if(enable) {
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,8,0)); //draglock
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,2,0)); //open arc
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,0,0)); //new arc
		//OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,9,0)); //quit
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,1,0)); //viewmode1
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,1,1)); //viewmode2
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,3,0)); //extract
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,5,0)); //arc info
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0)); //edit/select all
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0)); //edit/clear
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0)); //edit/invert
		if(module_has_add(aw->tab_node)) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
			if(CONFIG_GET_LOCK(no_dropzones) == FALSE) {
				OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,7,0)); //draglock
			}
			CONFIG_UNLOCK;
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,7,0)); //draglock
		}
		if(module_has_del(aw->tab_node)) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
		}
		if(aw->flat_mode == TRUE) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,0)); //collapse
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,1)); //expand
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,0)); //collapse
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,1)); //expand
		}
	} else {
		if(busy == FALSE) {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,5,0)); //arc info
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,2,0)); //open arc
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,0,0)); //new arc
			//OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,9,0)); //quit
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,1,0)); //viewmode1
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,1,1)); //viewmode2
		}
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,3,0)); //extract
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0)); //edit/select all
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0)); //edit/clear
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0)); //edit/invert
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,7,0)); //draglock
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,0)); //collapse
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,2,1)); //expand
	}
}

static void window_menu_set_enable_state(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(tab_get_format(aw->tab_node) == ARC_NONE) {
		window_menu_activation(aw, FALSE, FALSE);
	} else {
		window_menu_activation(aw, TRUE, FALSE);
	}
}

void window_disable_gadgets(void *awin, BOOL disable, BOOL stoppable)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;

	if(disable) {
		window_remove_dropzones(aw);

		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Disabled, TRUE,
				TAG_DONE);

		if(stoppable) {
			SetGadgetAttrs(aw->gadgets[GID_ABORT], aw->windows[WID_MAIN], NULL,
				GA_Disabled, FALSE,
				TAG_DONE);

			if(tab_get_format(aw->tab_node) != ARC_NONE) {
				SetWindowPointer(aw->windows[WID_MAIN],
								WA_PointerType, POINTERTYPE_PROGRESS,
								TAG_DONE);
			}
		} else {
			if(tab_get_format(aw->tab_node) != ARC_NONE) {
				SetWindowPointer(aw->windows[WID_MAIN],
								WA_BusyPointer, TRUE,
								WA_PointerDelay, TRUE,
								TAG_DONE);
			}	
		}

	} else {
		window_add_dropzones(aw, !aw->drag_lock);

		SetGadgetAttrs(aw->gadgets[GID_ABORT], aw->windows[WID_MAIN], NULL,
				GA_Disabled, TRUE,
				TAG_DONE);

		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Disabled, FALSE,
				TAG_DONE);
				
		SetWindowPointer(aw->windows[WID_MAIN],
							WA_BusyPointer, FALSE,
							TAG_DONE);
	}

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_OPENWB], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);

	if(tab_get_format(aw->tab_node) == ARC_NONE) {
		window_menu_activation(aw, FALSE, FALSE);
		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
			GA_Disabled, TRUE,
		TAG_DONE);
		SetWindowPointer(aw->windows[WID_MAIN],
							WA_BusyPointer, FALSE,
							TAG_DONE);
	} else {
		window_menu_activation(aw, !disable, TRUE);
	}

	if(aw->flat_mode == FALSE) disable = TRUE;

	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
}


static BOOL window_open_dest(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return OpenWorkbenchObjectA(tab_get_dest(aw->tab_node), NULL);
}

static void collapse_tree(struct avalanche_window *aw, BOOL expand)
{
	SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
		LISTBROWSER_Labels, ~0, TAG_DONE);

	if(expand) {
		/* expand instead of collapsing */
		ShowAllListBrowserChildren(window_get_dirtree_list(aw));
	} else {
		/* collapse */
		HideAllListBrowserChildren(window_get_dirtree_list(aw));
	}

	SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
		LISTBROWSER_Labels, window_get_dirtree_list(aw), TAG_DONE);

}

static ULONG array_count_selected_items(struct avalanche_window *aw, struct Node *tab_node)
{
	ULONG total_selectable = 0;
	ULONG selected = 0;

	for(int i = 0; i < tab_get_total_items(tab_node); i++) {
        	if(aw->flat_mode) {
				struct arc_entries *arc_e = tab_get_arc_entry(tab_node, i);
				if(arc_e->dir == FALSE) {
					if(arc_e->selected) selected++;
					total_selectable++;
				}
			}
	}

	tab_set_total_selectable(tab_node, total_selectable);
	return selected;
}

static ULONG list_count_selected_items(struct Node *tab_node)
{
	ULONG total_selectable = 0;
	ULONG selected = 0;
	struct List *list = tab_get_listbrowser_list(tab_node);
	struct Node *fnode;

	for(fnode = list->lh_Head; fnode->ln_Succ; fnode=fnode->ln_Succ) {
		ULONG checked = FALSE;
		ULONG checkbox = FALSE;

		GetListBrowserNodeAttrs(fnode,
				LBNA_CheckBox, &checkbox,
				LBNA_Checked, &checked,
				TAG_DONE);

		if(checkbox != FALSE) { /* has checkbox, ie. not dir */
			total_selectable++;

			if(checked) selected++;
		}
	}

	tab_set_total_selectable(tab_node, total_selectable);
	return selected;
}

static ULONG window_count_selected(void *awin, struct Node *tab_node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	ULONG selected = 0;

	if(aw->flat_mode) {
		selected = array_count_selected_items(aw, tab_node);
	} else {
		selected = list_count_selected_items(tab_node);
	}

	if(window_tab_is_current(aw, tab_node)) {
		progress_set_selected(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], selected, tab_get_total_selectable(tab_node));
	}
	
	return selected;
}

static void toggle_item(struct avalanche_window *aw, struct Node *node, ULONG select, BOOL detach_list)
{
	/* detach_list detached the list and also doesn't update the underlying array */
	if(detach_list) {
		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);
	}

	ULONG current;
	ULONG selected;
	struct arc_entries *userdata;

	if(detach_list && aw->flat_mode) {
		GetListBrowserNodeAttrs(node, LBNA_UserData, (struct arc_entries *)&userdata, TAG_DONE);
		if(userdata == NULL) return;
	}

	if(select == 2) {
		GetListBrowserNodeAttrs(node, LBNA_Checked, &current, TAG_DONE);
		selected = !current;
	} else {
		selected = select;
	}

	SetListBrowserNodeAttrs(node, LBNA_Checked, selected, TAG_DONE);

	if(detach_list && aw->flat_mode) {
		userdata->selected = selected;
	}

	if(detach_list) {
		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, window_get_lblist(aw),
			LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
			TAG_DONE);
	}
}

static int simple_sort(const char *a, const char *b)
{
	return strcmp(a, b);
}

static int sort(const char *a, const char *b)
{
	ULONG i = 0;
	
	while ((a[i]) && (b[i])) {
		if(a[i] != b[i]) {
			if(a[i] == '/') return -1;
			if(b[i] == '/') return 1;
			return StrnCmp(locale_get_locale(), a, b, 1, SC_COLLATE2);
		}
		i++;
	}
	
	if((a[i] == 0) && (b[i] == 0)) return 0;
	if((a[i] == 0) && (b[i] != 0)) return -1;
	if((a[i] != 0) && (b[i] == 0)) return 1;

	return 0; /* shouldn't get here */
}

static int sort_array(const void *a, const void *b)
{
	struct arc_entries *c = *(struct arc_entries **)a;
	struct arc_entries *d = *(struct arc_entries **)b;

	return simple_sort(c->name, d->name);
}

#ifdef __amigaos4__
static int32 lbsortfunc(struct Hook *h, APTR obj, struct LBSortMsg *msg)
#else
static LONG __saveds lbsortfunc(__reg("a0") struct Hook *h, __reg("a2") APTR obj, __reg("a1") struct LBSortMsg *msg)
#endif
{
	/* TODO: "top" is relative, maybe we need to check lbsm_Direction */

	/* Always float ^Parent to the top */
	if(strcmp(msg->lbsm_DataA.Text, "/") == 0) return -1;
	if(strcmp(msg->lbsm_DataB.Text, "/") == 0) return 1;

	/* Float directories above files */
	if((msg->lbsm_UserDataA == NULL) && (msg->lbsm_UserDataB != NULL)) return -1;
	if((msg->lbsm_UserDataB == NULL) && (msg->lbsm_UserDataA != NULL)) return 1;

	/* Otherwise sort alphabetically */
#ifdef __amigaos4__
	return sort(msg->lbsm_DataA.Text, msg->lbsm_DataB.Text);
#else
	return simple_sort(msg->lbsm_DataA.Text, msg->lbsm_DataB.Text);
#endif
}


static long extract_internal(void *awin, char *archive, char *newdest, struct Node *node, struct Node *tab_node)
{
	long ret = 0;
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	struct avalanche_config *config = get_config();

	if(archive && newdest) {
		tab_set_current_item(tab_node, 0);

		if((node == NULL) && (aw->flat_mode) && (tab_get_format(tab_node) != ARC_XFD)) {
			ret = module_extract_array(awin, tab_node, tab_get_arc_array(tab_node, 0), tab_get_total_items(tab_node), newdest);
		} else {
			ret = module_extract(awin, tab_node, node, archive, newdest);
		}

		CONFIG_LOCK;
		if((ret == 0) && (config->openwb == TRUE)) window_open_dest(awin);
		CONFIG_UNLOCK;
		
		tab_set_disabled(tab_node, FALSE, TRUE);
		window_count_selected(awin, tab_node);
	}

	return ret;
}

#ifdef __amigaos4__
static void extract_p(void)
#else
static void __saveds extract_p(void)
#endif
{
	/* Tell the main process we are started */
	avalanche_signal(SIGBREAKF_CTRL_F);
	
	/* Wait for UserData */
	Wait(SIGBREAKF_CTRL_E);
	
	/* Find our task */
	struct Task *extract_task = FindTask(NULL);
	struct avalanche_extract_userdata *aeu = (struct avalanche_extract_userdata *)extract_task->tc_UserData;
	struct avalanche_window *aw = (struct avalanche_window *)aeu->awin;
	
	/* Call Extract on our new process */
	extract_internal(aeu->awin, aeu->archive, aeu->newdest, aeu->node, aeu->tab_node);
	
	/* Free UserData */
	FreeVec(aeu);
	
	/* Signal that we've finished */
	avalanche_signal(aeu->sig);
}


long extract(void *awin, const char *archive, const char *newdest, struct Node *node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	struct avalanche_extract_userdata *aeu = AllocVec(sizeof(struct avalanche_extract_userdata), MEMF_CLEAR);
	
	tab_clear_abort(aw->tab_node);
		
	if(aeu == NULL) return -2;
	
	aeu->awin = awin;
	aeu->archive = archive;
	aeu->newdest = newdest;
	aeu->node = node;
	aeu->tab_node = aw->tab_node;
	aeu->sig = tab_get_signal(aw->tab_node);
	
	tab_set_disabled(aw->tab_node, TRUE, TRUE);
	
	/* Ensure there are no pending signals for this already */
	tab_signal_clear(aw->tab_node);

	struct Process *extract_process = CreateNewProcTags(NP_Entry, extract_p,
						NP_Name, "Avalanche extract process",
						NP_Priority, -1,
						TAG_DONE);
	
	/* Wait for the process to start up */
	Wait(SIGBREAKF_CTRL_F);
	
	/* Poke UserData */
	extract_process->pr_Task.tc_UserData = aeu;
	
	/* Signal the process to continue */
	Signal(extract_process, SIGBREAKF_CTRL_E);
	
	return 0;
}

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, int selected, struct avalanche_window *aw, struct Node *tab_node)
{
	BOOL dir_seen = FALSE;
	ULONG flags = 0;
	ULONG gen = 0;
	LONG *csize = NULL;
	int i = 0;
	char *name_copy = NULL;
	Object *glyph = NULL;
	ULONG tag1 = LBNCA_Integer;
	ULONG val1 = 0;
	ULONG tag2 = TAG_IGNORE;
	ULONG val2 = 0;
	ULONG tag3 = LBNCA_Integer;
	ULONG val3 = 0;
	ULONG tag4 = TAG_IGNORE;
	ULONG val4 = 0;
	BOOL debug = get_config()->debug;
	ULONG protect_bits = 0;
	ULONG *crunchsize = 0;
	BOOL crypted = FALSE;
	BOOL link = FALSE;
	BOOL title_needs_free = FALSE;
	BOOL disk = FALSE;
	BOOL partial = FALSE;
	void *xuserdata = userdata;
	const char *comment = NULL;
	const char *linkname = NULL;
	char *title = name;

	char datestr[20];
	char hsparwed[9];
	struct ClockData cd;

	/* Set Year to 0 */
	cd.year = 0;

	if(userdata && aw->flat_mode) {
		struct arc_entries *ud = (struct arc_entries *)userdata;
		xuserdata = ud->userdata;
	}
	if(tab_get_format(tab_node) == ARC_XAD) {
		xad_get_filedate(xuserdata, &cd, tab_node);
		protect_bits = xad_get_fileprotection(xuserdata, tab_node);
		comment = xad_get_comment(xuserdata, tab_node);
		link = xad_is_link(xuserdata, tab_node);
		linkname = xad_get_link(xuserdata, tab_node);
		disk = xad_is_disk(tab_node);
		partial = xad_is_partial(xuserdata, tab_node);
	}

	crunchsize = module_get_crunched_size(tab_node, xuserdata);
	crypted = module_is_crypted(tab_node, xuserdata);

	if(protect_bits & EXDF_HOLD)       strcpy(hsparwed, "h"); else strcpy(hsparwed, "-");
	if(protect_bits & EXDF_SCRIPT)     strcat(hsparwed, "s"); else strcat(hsparwed, "-");
	if(protect_bits & EXDF_PURE)       strcat(hsparwed, "p"); else strcat(hsparwed, "-");
	if(protect_bits & EXDF_ARCHIVE)    strcat(hsparwed, "a"); else strcat(hsparwed, "-");
	if(protect_bits & EXDF_NO_READ)    strcat(hsparwed, "-"); else strcat(hsparwed, "r");
	if(protect_bits & EXDF_NO_WRITE)   strcat(hsparwed, "-"); else strcat(hsparwed, "w");
	if(protect_bits & EXDF_NO_EXECUTE) strcat(hsparwed, "-"); else strcat(hsparwed, "e");
	if(protect_bits & EXDF_NO_DELETE)  strcat(hsparwed, "-"); else strcat(hsparwed, "d");

	if(CheckDate(&cd) == 0)
		Amiga2Date(0, &cd);

	if(dir) {
		if(aw->flat_mode) {
			switch(selected) {
				case 0:
					glyph = glyph_label_get(AVALANCHE_GLYPH_COMP_DIR_UNSEL);
				break;
				case 1:
					glyph = glyph_label_get(AVALANCHE_GLYPH_COMP_DIR_SEL);
				break;
				case 2:
				default:
					glyph = glyph_label_get(AVALANCHE_GLYPH_COMP_DIR_PARSEL);
				break;
			}
		} else {
			glyph = glyph_get(AVALANCHE_GLYPH_DRAWER);
		}
		tag1 = LBNCA_CopyText;
		val1 = TRUE;
		tag2 = LBNCA_Text;
		val2 = (ULONG)locale_get_string(MSG_DIR);

		snprintf(datestr, 20, "\0");
	} else {
		if(partial == FALSE) {
			if(disk == FALSE) {
				if(crypted == FALSE) {
					if(link == FALSE) {
						glyph = glyph_get(AVALANCHE_GLYPH_POPFILE);
					} else {
						glyph = glyph_get(AVALANCHE_GLYPH_LINK);
					}
				} else {
					glyph = glyph_get(AVALANCHE_GLYPH_CRYPTFILE);
				}
			} else {
				glyph = glyph_get(AVALANCHE_GLYPH_DISK);
			}
		} else {
			glyph = glyph_get(AVALANCHE_GLYPH_CORRUPT);
		}
		
		val1 = (ULONG)size;
		
		if(cd.year > 0) {
			snprintf(datestr, 20, "%04u-%02u-%02u %02u:%02u:%02u", cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);
		} else {
			strcpy(datestr, "");
		}
		
		if(crunchsize == NULL) {
			tag3 = LBNCA_CopyText;
			val3 = TRUE;
			tag4 = LBNCA_Text;
			val4 = (ULONG)locale_get_string(MSG_STORED);
		} else {
			val3 = (ULONG)crunchsize;
		}
	}

	if(linkname != NULL) {
		ULONG title_len = strlen(name) + strlen(linkname) + 5;
		if(title = AllocVec(title_len, MEMF_PRIVATE | MEMF_CLEAR)) {
			snprintf(title, title_len, "%s -> %s", name, linkname);
			title_needs_free = TRUE;
		} else {
			title = name;
		}
	}

#ifdef AVALANCHE_SIMPLELIST
	struct Node *node = AllocListBrowserNode(4,
		LBNA_UserData, userdata,
		LBNA_CheckBox, !dir,
		LBNA_Checked, selected,
		LBNA_Flags, flags,
		LBNA_Generation, gen,
		LBNA_Column, 0,
			LBNCA_Image, glyph,
		LBNA_Column, 1,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, title,
		LBNA_Column, 2,
			tag1, val1,
			tag2, val2,
		LBNA_Column, 3,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, datestr,
		TAG_DONE);
#else
	struct Node *node = AllocListBrowserNode(7,
		LBNA_UserData, userdata,
		LBNA_CheckBox, !dir,
		LBNA_Checked, selected,
		LBNA_Flags, flags,
		LBNA_Generation, gen,
		LBNA_Column, 0,
			LBNCA_Image, glyph,
		LBNA_Column, 1,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, title,
		LBNA_Column, 2,
			tag1, val1,
			tag2, val2,
		LBNA_Column, 3,
			tag3, val3,
			tag4, val4,
		LBNA_Column, 4,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, hsparwed,
		LBNA_Column, 5,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, datestr,
		LBNA_Column, 6,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, comment,
		TAG_DONE);

#endif

	if(title_needs_free) FreeVec(title);

	AddTail(tab_get_listbrowser_list(tab_node), node);
}

static ULONG count_dir_level(const char *filename)
{
	ULONG i = 0;
	ULONG level = 0;
	while(filename[i+1]) { /* ignore any trailing slash */
		if(filename[i] == '/') level++;
		i++;
	}

	return level;
}

static char *extract_path_part(const char *path, int level)
{
	char *npath = NULL;
	char *idx = path;
	if(path == NULL) return NULL;

	for(int i=0; i<level; i++) {
		idx = strchr(idx + 1, '/');
		if(idx == NULL) return NULL;
	}
	
	if(npath = AllocVec(idx - path + 1, MEMF_CLEAR)) {
		strncpy(npath, path, idx - path);
		return npath;
	}
	
	return NULL;
}


static BOOL check_if_subdir(struct Node *tab_node, int dir_entry, const char *dir_name)
{
	ULONG len = strlen(dir_name)+2;
	char *dir_name_slash = AllocVec(len, MEMF_PRIVATE);
	if(dir_name_slash == NULL) return FALSE;
	snprintf(dir_name_slash, len, "%s/", dir_name);

	for(int j = 0; j < dir_entry; j++) {
		struct arc_entries *dir_e = tab_get_dir_entry(tab_node, j, FALSE);
		if((dir_e) && (strlen(dir_e->name) > strlen(dir_name_slash)) && (strncmp(dir_e->name, dir_name_slash, strlen(dir_name_slash)) == 0)) {
			FreeVec(dir_name_slash);
			return TRUE;
		}
	}
	FreeVec(dir_name_slash);
	return FALSE;
}

static BOOL check_duplicates(struct Node *tab_node, int dir_entry, const char *dir_name)
{
	for(int j = 0; j < dir_entry; j++) {
		struct arc_entries *dir_e = tab_get_dir_entry(tab_node, j, FALSE);
		if((dir_e) && (strlen(dir_name) > 0) && (strcmp(dir_e->name, dir_name) == 0)) {
			return TRUE;
		}
	}
	return FALSE;
}

static void highlight_current_dir(struct avalanche_window *aw, struct Node *tab_node)
{
	struct Node *cur_node = tab_dir_find_current_node(tab_node);

	if(cur_node) {
		SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_SelectedNode, cur_node,
			TAG_DONE);
	}
}

static void window_update_tab_title(struct avalanche_window *aw, struct Node *tab_node)
{
	const char *fn = tab_get_archive_name(tab_node);
	
	SetGadgetAttrs(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL,
		CLICKTAB_Labels, ~0,
		CLICKTAB_MinorLabelChange, TRUE,
		TAG_DONE);

	SetClickTabNodeAttrs(tab_node, TNA_Text, FilePart(fn),
		TNAHintInfo, fn,
		TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL,
		CLICKTAB_Labels, &aw->tab_list,
		CLICKTAB_MinorLabelChange, TRUE,
		TAG_DONE);
		
	RefreshGList(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL, 1);
}

static void window_update_title(struct avalanche_window *aw, struct Node *tab_node)
{
	const char *archive = tab_get_archive_name(tab_node);
	const char *c_dir = tab_get_current_dir(tab_node);
	if(archive != NULL) {
		if(c_dir) {
			int title_len = strlen(VERS) + strlen((archive)) + strlen(c_dir);

			if((title_len < TITLE_MAX_SIZE) || ((title_len - TITLE_MAX_SIZE + 3) > strlen(c_dir))) {
				snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s] - %s", VERS, (archive), c_dir);
			} else {
				snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s] - ...%s", VERS, (archive), c_dir + (strlen(c_dir) - (title_len - TITLE_MAX_SIZE + 3)));
			}
		} else {
			snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s]", VERS, (archive));
		}
		SetWindowTitles(window_get_window(aw), (UBYTE *) ~0, aw->title);

		window_update_tab_title(aw, tab_node); /* TODO: fix for multi-tab */
		
	} else {
		SetWindowTitles(window_get_window(aw), (UBYTE *) ~0, VERS);
	}
}

/* Enumerate select state of dir "dir"
 * returns 0 = none selected, 1 = all selected, 2 = some selected
 * Browser mode only!
 */
static int window_enum_dir(struct Node *tab_node, const char *dir)
{
	int sel_state = 2;
	ULONG count_sel = 0;
	ULONG count_unsel = 0;

#ifdef __amigaos4__
DebugPrintF("[Avalanche] Dir %s\n", dir);
#endif

	for(int i = 0; i < tab_get_total_items(tab_node); i++) {
		struct arc_entries *arc_e = tab_get_arc_entry(tab_node, i);
		if(arc_e->dir == TRUE) continue;

#ifdef __amigaos4__
DebugPrintF("[Avalanche] Checking %s against %s\n", arc_e->name, dir);
#endif

		if((dir == NULL) || (strncmp(arc_e->name, dir, strlen(dir)) == 0)) {
			if(arc_e->selected) {
#ifdef __amigaos4__
DebugPrintF("[Avalanche] Matched - selected\n");
#endif
				count_sel++;
			} else {
#ifdef __amigaos4__
DebugPrintF("[Avalanche] Matched - not selected\n");
#endif
				count_unsel++;
			}
		}
	}
	
	if((count_sel == 0) && (count_unsel > 0)) sel_state = 0;
	if((count_unsel == 0) && (count_sel > 0)) sel_state = 1;

#ifdef __amigaos4__
DebugPrintF("[Avalanche] Returning state %d\n", sel_state);
#endif

	return sel_state;
}

static void window_flat_browser_tree_construct(struct avalanche_window *aw, struct Node *tab_node)
{
	ULONG root_glyph = AVALANCHE_GLYPH_ROOT;

	int dir_entry = 0;

	struct arc_entries **dir_a = tab_alloc_dir_array(tab_node);
	if(dir_a == NULL) return;

	for(int it = 0; it < tab_get_total_items(tab_node); it++) {
		struct arc_entries *arc_e = tab_get_arc_entry(tab_node, it);
		char *dir_name = AllocVec(strlen(arc_e->name)+1, MEMF_CLEAR);
		int i = 0;
		int slash = 0;
		int last_slash = 0;

		BOOL dupe = FALSE;

		if(dir_name == NULL) continue;

		while(arc_e->name[i+1]) {
			if(arc_e->name[i] == '/') {
				slash++;
				last_slash = i;
			}
			dir_name[i] = arc_e->name[i];
			i++;
		}
		dir_name[last_slash] = '\0';

		dupe = check_duplicates(tab_node, dir_entry, dir_name);

		if((slash > 0) && (dupe == FALSE)) {
			for(int l = 1; l < slash; l++) {
				char *part_dir = extract_path_part(dir_name, l);
				dupe = check_duplicates(tab_node, dir_entry, part_dir);
				if(dupe) {
					FreeVec(part_dir);
					continue;
				}
				struct arc_entries *dir_e = tab_get_dir_entry(tab_node, dir_entry, TRUE);
				if(dir_e == NULL) continue;
				
				dir_e->name = part_dir;
				dir_e->level = l;

				dir_entry++;
			}

			struct arc_entries *dir_e = tab_get_dir_entry(tab_node, dir_entry, TRUE);
			if(dir_e == NULL) continue;
			
			dir_e->name = dir_name;
			dir_e->level = slash;
			dir_e->dir = FALSE;

			dir_entry++;

			if(dir_entry > tab_get_dir_tree_size(tab_node)) {
				open_error_req(locale_get_string(MSG_ERR_TREE_ALLOC), locale_get_string(MSG_OK), aw);
				return;
			}

		} else {
			FreeVec(dir_name);
		}
	}

	for(int i = 0; i < dir_entry; i++) {
		struct arc_entries *dir_e = tab_get_dir_entry(tab_node, i, FALSE);
		if((check_if_subdir(tab_node, dir_entry, dir_e->name))) {
			dir_e->dir = TRUE;  //LBFLG_HASCHILDREN
		} else {
			dir_e->dir = FALSE;
		}
	}

	tab_set_dir_tree_size(tab_node, dir_entry);

	if(tab_get_format(tab_node) == ARC_XAD) {
		if(xad_arc_is_corrupt(tab_node)) {
			root_glyph = AVALANCHE_GLYPH_CORRUPT;
		} else if(xad_is_diskfile(tab_node)) {
			root_glyph = AVALANCHE_GLYPH_DISK;
		} else if(xad_arc_is_crypted(tab_node)) {
			root_glyph = AVALANCHE_GLYPH_CRYPTFILE;
		}
	}

	tab_dir_add_root_node(tab_node, root_glyph, dir_entry);

	for(int i = 0; i <= dir_entry; i++) {
		ULONG flags = LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN;
		struct arc_entries *dir_e = tab_get_dir_entry(tab_node, i, FALSE);
		if(dir_e == NULL) break;
		if(dir_e->dir == FALSE) flags = 0;

		struct Node *node = AllocListBrowserNode(1,
									LBNA_UserData, dir_e->name,
									LBNA_Flags, flags,
									LBNA_Generation, dir_e->level + 1,
									LBNA_Column, 0,
										LBNCA_Image, LabelObj,
											LABEL_DisposeImage, FALSE,
											LABEL_Image, glyph_get(AVALANCHE_GLYPH_DRAWER),
											LABEL_Underscore, NULL,
											LABEL_Text, " ",
											LABEL_Text, FilePart(dir_e->name),
										LabelEnd,
									TAG_DONE);

		AddTail(tab_get_dirtree_list(tab_node), node);
	}

	if(window_tab_is_current(aw, tab_node)) window_update_title(aw, tab_node);
}

static void window_flat_browser_construct(struct avalanche_window *aw, struct Node *tab_node)
{
	ULONG level = 0;
	ULONG skip_dir_len = 0;

	if(window_tab_is_current(aw, tab_node)) {
		if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);
	}

	FreeListBrowserList(tab_get_listbrowser_list(tab_node));

	const char *c_dir = tab_get_current_dir(tab_node);

	if(c_dir) {
		level = count_dir_level(c_dir) + 1;
		skip_dir_len = strlen(c_dir);
	}

	if(tab_get_format(tab_node) != ARC_XFD) {
		/* Add fake dir entries (first, so by default they're at the top) */
		
		if(level > 0) {
			/* Add "parent" entry */
#if AVALANCHE_SIMPLELIST
			struct Node *node = AllocListBrowserNode(4,
									LBNA_UserData, NULL,
									LBNA_Generation, 1,
									LBNA_CheckBox, FALSE,
									LBNA_Column, 0,
										LBNCA_Image, glyph_get(AVALANCHE_GLYPH_PARENT),
									LBNA_Column, 1,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "/",
									LBNA_Column, 2,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, locale_get_string(MSG_PARENT),
									LBNA_Column, 3,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "",
								TAG_DONE);
#else
			struct Node *node = AllocListBrowserNode(7,
									LBNA_UserData, NULL,
									LBNA_CheckBox, FALSE,
									LBNA_Generation, 1,
									LBNA_Column, 0,
										LBNCA_Image, glyph_get(AVALANCHE_GLYPH_PARENT),
									LBNA_Column, 1,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "/",
									LBNA_Column, 2,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, locale_get_string(MSG_PARENT),
									LBNA_Column, 3,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "",
									LBNA_Column, 4,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "",
									LBNA_Column, 5,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "",
									LBNA_Column, 6,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, "",
								TAG_DONE);
#endif
			AddTail(tab_get_listbrowser_list(tab_node), node);
		}
		
		for(int it = 0; it < tab_get_dir_tree_size(tab_node); it++) {
			/* Only show current level - NB dir levels are different from file levels */
			struct arc_entries *dir_e = tab_get_dir_entry(tab_node, it, FALSE);
			const char *c_dir = tab_get_current_dir(tab_node);
			if((dir_e && ((dir_e->level - 1) == level)) &&
				((c_dir == NULL) || (strncmp(dir_e->name, c_dir, skip_dir_len) == 0))) {
				addlbnode(dir_e->name + skip_dir_len, &zero, TRUE, NULL, window_enum_dir(tab_node, dir_e->name), aw, tab_node);
			}
		}
	}

	/* Add files */

	for(int it = 0; it < tab_get_total_items(tab_node); it++) {
		/* Only show current level */
		struct arc_entries *arc_e = tab_get_arc_entry(tab_node, it);
		const char *c_dir = tab_get_current_dir(tab_node);
		if((arc_e->level == level) && (arc_e->dir == FALSE) &&
			((c_dir == NULL) || (strncmp(arc_e->name, c_dir, strlen(c_dir)) == 0))) {
			addlbnode(arc_e->name + skip_dir_len,
				arc_e->size, arc_e->dir, arc_e, arc_e->selected, aw, tab_node);
		}
	}

	if(window_tab_is_current(aw, tab_node)) {
		window_update_title(aw, tab_node);

		if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);
	}
}

static void addlbnode_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin, struct Node *tab_node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(item == 0) {
		tab_set_current_item(tab_node, 0);
		if(aw->windows[WID_MAIN] && aw->gadgets[GID_PROGRESS]) {
			tab_set_total_items(tab_node, total);
			//progress_set_archive_level(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], 0, total);
		}
	}

	if(aw->flat_mode) {
		struct arc_entries **arc_a = tab_get_arc_array(tab_node, (item==0) ? total : 0);

		if(arc_a) {
			struct arc_entries *arc_e = tab_get_arc_entry(tab_node, item);
			if(arc_e) {
			
				arc_e->name = name;
				arc_e->size = size;
				arc_e->dir = dir;
				arc_e->userdata = userdata;
				arc_e->selected = TRUE;
				
				arc_e->level = 0;

				/* Count the slashes to find directory level */
				arc_e->level = count_dir_level(name);
				
				if(item == (total - 1)) {
					/* Sort the array */
					#ifdef __amigaos4__ /* doesn't like OS3, may not be needed anyway */
					qsort(arc_a, total, sizeof(struct arc_entries *), sort_array);
					#endif
					window_flat_browser_tree_construct(aw, tab_node);
					window_flat_browser_construct(aw, tab_node);
				}
			}
		}
	} else {
		addlbnode(name, size, dir, userdata, TRUE, aw, tab_node);
	}
}

static void update_fuelgauge_text(struct avalanche_window *aw)
{
	ULONG current_item  = tab_get_current_item(aw->tab_node);
	current_item++;
	tab_set_current_item(aw->tab_node, current_item);

	if(aw->windows[WID_MAIN] == NULL) return;

	/* TODO: Need to propagate tab_node to this function so we can check if tab is the active one */
	progress_set_archive_level(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], tab_get_current_item(aw->tab_node), tab_get_total_items(aw->tab_node));
}

void window_update_fuelgauge_text(void *awin, struct Node *tab_node)
{
	char *msg;
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;
	if(aw->gadgets[GID_PROGRESS] == NULL) return;

	ULONG total = tab_get_total_items(tab_node);

	if(total == 0) {
		if(window_tab_is_current(aw, tab_node)) {
			progress_set_scanning(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], 0);
		}
	}

	total++;

	tab_set_total_items(tab_node, total);

	ULONG current = tab_get_current_item(tab_node);

	if(current < 99) {
		current++;
		tab_set_current_item(tab_node, current);
		return;
	}

	tab_set_current_item(tab_node, 0);
	if(window_tab_is_current(aw, tab_node)) {
		progress_set_scanning(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], total);
	}
}

static void *array_get_userdata_quiet(void *awin, void *arc_entry)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->flat_mode) {
		struct arc_entries *arc_e = (struct arc_entries *)arc_entry;
		if(arc_e->selected) return arc_e->userdata;
	}

	return NULL;
}

void *array_get_userdata(void *awin, void *arc_entry)
{
	update_fuelgauge_text(awin);
	return array_get_userdata_quiet(awin, arc_entry);
}

static const char *get_item_filename(void *awin, struct Node *node)
{
	void *userdata = NULL;
	const char *fn = NULL;
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

	if(aw->flat_mode) {
		struct arc_entries *arc_entry = (struct arc_entries *)userdata;
		return module_get_item_filename(aw->tab_node, arc_entry->userdata);
	} else {
		return module_get_item_filename(aw->tab_node, userdata);
	}
}

static void window_tree_add(struct avalanche_window *aw)
{
	struct TagItem attrs[2];

	if(aw->gadgets[GID_TREE] != NULL) return;

	attrs[0].ti_Tag = CHILD_WeightedWidth;
	attrs[0].ti_Data = 20;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

	aw->gadgets[GID_TREE] = ListBrowserObj,
					GA_ID, GID_TREE,
					HINTINFO, locale_get_string(MSG_HI_TREE),
					GA_RelVerify, TRUE,
					GA_Disabled, !aw->flat_mode,
					LISTBROWSER_ColumnInfo, aw->dtci,
					LISTBROWSER_Labels, window_get_dirtree_list(aw),
					LISTBROWSER_ColumnTitles, FALSE,
					LISTBROWSER_FastRender, TRUE,
					LISTBROWSER_Hierarchical, TRUE,
					LISTBROWSER_ShowSelected, TRUE,
					LISTBROWSER_ShowImage, glyph_get(GLYPH_RIGHTARROW),
					LISTBROWSER_HideImage, glyph_get(GLYPH_DOWNARROW),
					LISTBROWSER_LeafImage, NULL,
					LISTBROWSER_AutoFit, TRUE,
					LISTBROWSER_HorizontalProp, TRUE,
				ListBrowserEnd;

#ifdef __amigaos4__
	/* Add tree */
	int ret = IDoMethod(aw->gadgets[GID_TREELAYOUT], LM_ADDCHILD,
			aw->windows[WID_MAIN], aw->gadgets[GID_TREE], NULL);
			
	/* Resize tree to 20, as 0 disables resize */
	IDoMethod(aw->gadgets[GID_BROWSERLAYOUT], LM_MODIFYCHILD,
			aw->windows[WID_MAIN], aw->gadgets[GID_TREELAYOUT], (struct TagItem *)&attrs);
#else
	SetGadgetAttrs(aw->gadgets[GID_TREELAYOUT], aw->windows[WID_MAIN], NULL,
		LAYOUT_AddChild, aw->gadgets[GID_TREE],
		TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_BROWSERLAYOUT], aw->windows[WID_MAIN], NULL,
		LAYOUT_ModifyChild, aw->gadgets[GID_TREELAYOUT],
		TAG_MORE, (struct TagItem *)&attrs,
		TAG_DONE);
#endif

	if(aw->windows[WID_MAIN]) {
		FlushLayoutDomainCache((struct Gadget *)aw->gadgets[GID_MAIN]);
		RethinkLayout((struct Gadget *)aw->gadgets[GID_BROWSERLAYOUT],
			aw->windows[WID_MAIN], NULL, TRUE);
	}
}

static void window_tree_remove(struct avalanche_window *aw)
{
	struct TagItem attrs[2];

	if(aw->gadgets[GID_TREE] == NULL) return;

	attrs[0].ti_Tag = CHILD_WeightedWidth;
	attrs[0].ti_Data = 0;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

#ifdef __amigaos4__
	/* Remove tree */
	IDoMethod(aw->gadgets[GID_TREELAYOUT], LM_REMOVECHILD,
			aw->windows[WID_MAIN], aw->gadgets[GID_TREE]);

	/* Shrink tree to minimum size (remove empty area where it was) */
	IDoMethod(aw->gadgets[GID_BROWSERLAYOUT], LM_MODIFYCHILD,
			aw->windows[WID_MAIN], aw->gadgets[GID_TREELAYOUT], (struct TagItem *)&attrs);
#else
	SetGadgetAttrs(aw->gadgets[GID_TREELAYOUT], aw->windows[WID_MAIN], NULL,
		LAYOUT_RemoveChild, aw->gadgets[GID_TREE],
		TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_BROWSERLAYOUT], aw->windows[WID_MAIN], NULL,
		LAYOUT_ModifyChild, aw->gadgets[GID_TREELAYOUT],
		TAG_MORE, (struct TagItem *)&attrs,
		TAG_DONE);
#endif

	if(aw->windows[WID_MAIN]) {
		FlushLayoutDomainCache((struct Gadget *)aw->gadgets[GID_MAIN]);
		RethinkLayout((struct Gadget *)aw->gadgets[GID_BROWSERLAYOUT],
			aw->windows[WID_MAIN], NULL, TRUE);
	}

	aw->gadgets[GID_TREE] = NULL;

}

//static void idcmp_hook_func(struct Hook *h, Object *obj, struct IntuiMessage *msg)
#ifndef __amigaos4__
static void __saveds idcmp_hook_func(__reg("a0") struct Hook *h, __reg("a2") Object *obj, __reg("a1") struct IntuiMessage *msg)
{
	ULONG gid;
	// = h->h_Data;
	struct Node *node = NULL;

	switch(msg->Class)
	{
		case IDCMP_IDCMPUPDATE:
			gid = GetTagData(GA_ID, 0, msg->IAddress);

			switch(gid)
			{
				case GID_TABS:
					if((node = (struct Node *)GetTagData(CLICKTAB_NodeClosed, 0, msg->IAddress))) {
						struct avalanche_window *aw = (struct avalanche_window *)tab_get_window(node);
						aw->tab_closed = tab_close(node);
					}
				break;
			}
		break;

		default:
		break;
	}
}
#endif

/* Some tab handling functions */
void window_tab_set(void *awin, struct Node *tab_node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->gadgets[GID_TABS] == NULL) return;
	
	SetGadgetAttrs(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL,
		CLICKTAB_Labels, &aw->tab_list,
		CLICKTAB_CurrentNode, tab_node,
		TAG_DONE);

	aw->tab_node = tab_node;

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, tab_get_listbrowser_list(tab_node),
				LISTBROWSER_SortColumn, 1,
				LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
			TAG_DONE);

	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, tab_get_dirtree_list(tab_node), TAG_DONE);

	if(aw->flat_mode) highlight_current_dir(aw, tab_node);

	window_update_title(aw, tab_node);

	if(tab_get_disabled(tab_node)) {
		window_disable_gadgets(awin, TRUE, tab_get_stoppable(tab_node));
	} else {
		window_disable_gadgets(awin, FALSE, tab_get_stoppable(tab_node));
		window_count_selected(awin, tab_node);
	}
}

static void window_tab_switch(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	struct Node *tab_node = NULL;
	
	GetAttr(CLICKTAB_CurrentNode, aw->gadgets[GID_TABS], (ULONG *)&tab_node);
	
	if(aw->tab_node != tab_node) {
		window_tab_set(awin, tab_node);
	}
}

void window_tab_refresh(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->gadgets[GID_TABS] == NULL) return;

	window_tab_switch(awin);

	SetGadgetAttrs(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL,
		CLICKTAB_Labels, &aw->tab_list,
		TAG_DONE);
	
	RefreshGList(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL, 1);
}

const ULONG window_tab_count(void *awin, int change)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	aw->tab_count += change;
	
	return aw->tab_count;
}

/* Window functions */
BOOL window_tab_create(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	aw->tab_node = tab_create(aw, &aw->tab_list);
	
	if(aw->tab_node == NULL) return FALSE;
	
	return TRUE;
}

void *window_create(struct avalanche_config *config, char *archive, struct MsgPort *winport, struct MsgPort *appport)
{
	struct avalanche_window *aw = AllocVec(sizeof(struct avalanche_window), MEMF_CLEAR | MEMF_PRIVATE);
	if(!aw) return NULL;
	
	struct Hook *lbsort_hook = (struct Hook *)&(aw->lbsorthook);

	ULONG tag_default_position = WINDOW_Position;

	/* Listbrowser */
	aw->lbsorthook.h_Entry = lbsortfunc;
	aw->lbsorthook.h_SubEntry = NULL;
	aw->lbsorthook.h_Data = NULL;

#ifdef AVALANCHE_SIMPLELIST

	aw->lbci = AllocLBColumnInfo(4, 
		LBCIA_Column, 0,
			LBCIA_Title, "",
			LBCIA_Weight, 10,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, FALSE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string(MSG_NAME),
			LBCIA_Weight, 55,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
			LBCIA_CompareHook, lbsort_hook,
		LBCIA_Column, 2,
			LBCIA_Title,  locale_get_string(MSG_SIZE),
			LBCIA_Weight, 15,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 3,
			LBCIA_Title,  locale_get_string(MSG_DATE),
			LBCIA_Weight, 20,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		TAG_DONE);

#else
	aw->lbci = AllocLBColumnInfo(7,
		LBCIA_Column, 0,
			LBCIA_Title, "",
			LBCIA_Weight, 5,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, FALSE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string(MSG_NAME),
			LBCIA_Weight, 45,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
			LBCIA_CompareHook, lbsort_hook,
		LBCIA_Column, 2,
			LBCIA_Title,  locale_get_string(MSG_SIZE),
			LBCIA_Weight, 10,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 3,
			LBCIA_Title,  locale_get_string(MSG_PACKEDSIZE),
			LBCIA_Weight, 10,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 4,
			LBCIA_Title,  locale_get_string(MSG_PERMISSIONS),
			LBCIA_Weight, 5,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 5,
			LBCIA_Title,  locale_get_string(MSG_DATE),
			LBCIA_Weight, 15,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 6,
			LBCIA_Title,  locale_get_string(MSG_COMMENT),
			LBCIA_Weight, 10,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
			LBCIA_CompareHook, lbsort_hook,
		TAG_DONE);
#endif

	aw->dtci = AllocLBColumnInfo(1, 
		LBCIA_Column, 0,
			LBCIA_Title, "",
			LBCIA_Weight, 100,
			LBCIA_DraggableSeparator, FALSE,
			LBCIA_Sortable, FALSE,
		TAG_DONE);

	if(aw->menu = AllocVec(sizeof(menu), MEMF_PRIVATE))
		CopyMem(&menu, aw->menu, sizeof(menu));

	CONFIG_LOCK;
	if(config->win_x && config->win_y) tag_default_position = TAG_IGNORE;
	
	/* Copy global to local config */
	if(config->viewmode == 0) aw->flat_mode = TRUE;
	if(config->drag_lock) aw->drag_lock = TRUE;
		else aw->drag_lock = FALSE;

	/* ASL hook */
	aw->aslfilterhook.h_Entry = aslfilterfunc;
	aw->aslfilterhook.h_SubEntry = NULL;
	aw->aslfilterhook.h_Data = NULL;
	
#ifndef __amigaos4__
	/* IDCMP hook - OS3.2 tab close only */
	aw->idcmphook.h_Entry = idcmp_hook_func;
	aw->idcmphook.h_SubEntry = NULL;
	aw->idcmphook.h_Data = aw;
	
	aw->tab_closed = FALSE;
#endif
	
	NewList(&aw->tab_list);

	if(window_tab_create(aw) == FALSE) {
		/* Initial tab creation failed - this is fatal */
		FreeVec(aw);
		return NULL;
	}
	if(archive) tab_set_archive(aw->tab_node, archive);

	/* Create the window object */
	aw->objects[OID_MAIN] = WindowObj,
		WA_ScreenTitle, VERS,
		WA_Title, VERS,
		WA_Activate, TRUE,
		WA_DepthGadget, TRUE,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_SizeGadget, TRUE,
		WA_Top, config->win_y,
		WA_Left, config->win_x,
		WA_Width, config->win_w,
		WA_Height, config->win_h,
		WA_IDCMP, IDCMP_MENUPICK | IDCMP_RAWKEY | IDCMP_GADGETUP | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE,
		WINDOW_NewMenu, aw->menu,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_Icon, (struct DiskObject *)config->iconify_icon,
		WINDOW_IconNoDispose, TRUE,
		WINDOW_IconTitle, "Avalanche",
		WINDOW_SharedPort, winport,
		WINDOW_AppPort, appport,
		/* Enable HintInfo */
		WINDOW_GadgetHelp, TRUE,
#ifndef __amigaos4__
		WINDOW_HintInfo, &aw->hi,
		/* Hook only needed on OS3.2 for tab close gadget */
		WINDOW_IDCMPHook, &aw->idcmphook,
		WINDOW_IDCMPHookBits, IDCMP_IDCMPUPDATE,
#endif
		tag_default_position, WPOS_CENTERSCREEN,
		WINDOW_ParentGroup, aw->gadgets[GID_MAIN] = LayoutVObj,
			//LAYOUT_DeferLayout, TRUE,
			LAYOUT_SpaceOuter, TRUE,
			LAYOUT_AddChild, LayoutHObj,
				LAYOUT_VertAlignment, LALIGN_BOTTOM,
				LAYOUT_AddChild, LayoutHObj,
					LAYOUT_AddChild, LayoutHObj,
						LAYOUT_BevelStyle, BVS_BUTTON,
						LAYOUT_BevelState, IDS_SELECTED,
						LAYOUT_SpaceOuter, TRUE,
						LAYOUT_FillPen, 3,
						LAYOUT_AddChild,  aw->gadgets[GID_EXTRACT] = ButtonObj,
							GA_ID, GID_EXTRACT,
							HINTINFO, locale_get_string(MSG_HI_EXTRACT),
							GA_RelVerify, TRUE,
							GA_Image, glyph_get(AVALANCHE_GLYPH_EXTRACT),
							GA_Disabled, (!archive),
							ButtonEnd,
						CHILD_NominalSize, TRUE,
					LayoutEnd,
					LAYOUT_AddChild, LayoutHObj,
						LAYOUT_BevelStyle, BVS_GROUP,
						LAYOUT_SpaceOuter, TRUE,
						LAYOUT_EvenSize, TRUE,
						LAYOUT_AddChild,  aw->gadgets[GID_ARCHIVE] = ButtonObj,
							GA_ID, GID_ARCHIVE,
							HINTINFO, locale_get_string(MSG_HI_ARCHIVE),
							GA_RelVerify, TRUE,
							GA_Image, glyph_get(AVALANCHE_GLYPH_OPENFILE),
						ButtonEnd,
						CHILD_NominalSize, TRUE,
						LAYOUT_AddChild,  aw->gadgets[GID_OPENWB] = ButtonObj,
							GA_ID, GID_OPENWB,
							GA_RelVerify, TRUE,
							GA_Image, glyph_get(AVALANCHE_GLYPH_OPENDRAWER),
							HINTINFO, locale_get_string(MSG_OPENINWB),
						ButtonEnd,
						CHILD_NominalSize, TRUE,
						LAYOUT_AddChild,  aw->gadgets[GID_ABORT] = ButtonObj,
							GA_ID, GID_ABORT,
							GA_RelVerify, TRUE,
							GA_Image, glyph_get(AVALANCHE_GLYPH_STOP),
							HINTINFO, locale_get_string(MSG_HI_ABORT),
							GA_Disabled, TRUE,
						ButtonEnd,
						CHILD_NominalSize, TRUE,
					LayoutEnd,
				LayoutEnd,
				CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, aw->gadgets[GID_TABS] = ClickTabObj,
					GA_ID, GID_TABS,
					GA_RelVerify, TRUE,
					GA_Underscore, 13, // disable kb shortcuts
					CLICKTAB_Labels, &aw->tab_list,
					CLICKTAB_LabelTruncate, TRUE,
					CLICKTAB_CloseImage, glyph_get(AVALANCHE_GLYPH_TABCLOSE),
					//CLICKTAB_EvenSize, FALSE,
					CLICKTAB_AutoFit, TRUE,
#ifndef __amigaos4__
					ICA_TARGET, ICTARGET_IDCMP,
#else
					CLICKTAB_FlagImage, glyph_get(AVALANCHE_GLYPH_BUSY),
#endif
				ClickTabEnd,
			LayoutEnd,
			CHILD_WeightedHeight, 0,

			LAYOUT_AddChild, LayoutVObj,		
				LAYOUT_AddChild, aw->gadgets[GID_BROWSERLAYOUT] = LayoutHObj,
					LAYOUT_AddChild, aw->gadgets[GID_TREELAYOUT] = LayoutHObj,
					EndGroup,
					CHILD_WeightedWidth, 0,
					LAYOUT_WeightBar, TRUE,
					LAYOUT_AddChild,  aw->gadgets[GID_LIST] = ListBrowserObj,
						GA_ID, GID_LIST,
						HINTINFO, locale_get_string(MSG_HI_LIST),
						GA_RelVerify, TRUE,
						LISTBROWSER_ColumnInfo, aw->lbci,
						LISTBROWSER_Labels, window_get_lblist(aw),
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_TitleClickable, TRUE,
						LISTBROWSER_SortColumn, 1,
						LISTBROWSER_Striping, LBS_ROWS,
						LISTBROWSER_FastRender, TRUE,
						LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
						LISTBROWSER_HorizontalProp, TRUE,
					ListBrowserEnd,
					CHILD_WeightedWidth, 80,
				LayoutEnd,
#ifndef __amigaos4__
				LAYOUT_AddChild,  aw->gadgets[GID_PROGRESS] = FuelGaugeObj,
					GA_ID, GID_PROGRESS,
					HINTINFO, locale_get_string(MSG_HI_PROGRESS),
					FUELGAUGE_Percent, FALSE,
				FuelGaugeEnd,
				CHILD_WeightedHeight, 0,
#endif
			LayoutEnd,
		EndGroup,
	EndWindow;

	CONFIG_UNLOCK;

	/*  Object creation sucessful?
	 */
	if (aw->objects[OID_MAIN]) {
		/* Add to our window list */
		add_to_window_list(aw);
		return aw;
	} else {
		FreeVec(aw);
		return NULL;
	}
}

void window_open(void *awin, struct MsgPort *appwin_mp)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	aw->appwin_mp = appwin_mp; /* store for later use */

	if(aw->windows[WID_MAIN]) {
		WindowToFront(aw->windows[WID_MAIN]);
	} else {
		if(aw->flat_mode) {
			aw->menu[MENU_FLATMODE].nm_Flags |= CHECKED;
			/* Add the tree if needed */
			window_tree_add(aw);
		} else {
			aw->menu[MENU_LISTMODE].nm_Flags |= CHECKED;
			/* Remove the tree if needed */
			window_tree_remove(aw);
		}

		if(aw->drag_lock) {
			aw->menu[MENU_DRAGLOCK].nm_Flags |= CHECKED;
		}

		aw->windows[WID_MAIN] = (struct Window *)RA_OpenWindow(aw->objects[OID_MAIN]);
		
		if(aw->windows[WID_MAIN]) {
#ifdef __amigaos4__
			if(aw->gadgets[GID_PROGRESS] == NULL) {
				struct Screen *scrn = LockPubScreen(NULL);
				struct DrawInfo *dri = GetScreenDrawInfo(scrn);

				progress_get_area(aw->windows[WID_MAIN], &aw->p_width, &aw->p_height, &aw->p_sz_height);

				aw->gadgets[GID_PROGRESSFR] = NewObject(
					NULL,
					"frbuttonclass", /* We need a gadgetclass object as a container to prevent lockup, this one works */
					GA_Left, scrn->WBorLeft + 2,
					GA_RelBottom, scrn->WBorBottom - (aw->p_sz_height/2),
					GA_BottomBorder, TRUE,
					GA_Width, aw->p_width,
					GA_Height, aw->p_height,
					GA_DrawInfo, dri,
					GA_ReadOnly, TRUE,
					GA_Disabled, TRUE,
					GA_Image, aw->gadgets[GID_PROGRESS] = NewObject(
						NULL,
						"gaugeiclass",
						GA_ID, GID_PROGRESS,
						GAUGEIA_Level, 0,
						IA_Top, (int)(- ceil((scrn->WBorBottom + aw->p_sz_height) / 2)),
						IA_Left, -4,
						IA_Height, 1 + aw->p_height,
						IA_Width, aw->p_width,
						IA_Label, NULL,
						IA_InBorder, TRUE,
						GAUGEIA_Ticks, 10,
						GAUGEIA_Min, 0,
						GAUGEIA_Max, 100,
						IA_Screen, scrn,
					TAG_DONE),
				TAG_DONE);
				
				RefreshSetGadgetAttrs(aw->gadgets[GID_PROGRESSFR],
					aw->windows[WID_MAIN], NULL,
					GA_Width, aw->p_width,
				TAG_DONE);
			
				FreeScreenDrawInfo(scrn, dri);
				UnlockPubScreen(NULL, scrn);
			}
			AddGList(aw->windows[WID_MAIN], (struct Gadget *)aw->gadgets[GID_PROGRESSFR], (UWORD)~0, -1, NULL);
#endif
			
			aw->appwin = AddAppWindowA(0, (ULONG)aw, aw->windows[WID_MAIN], appwin_mp, NULL);
			window_add_dropzones(aw, !aw->drag_lock);

			/* Refresh archive on window open */
			if(tab_get_format(aw->tab_node) != ARC_NONE) window_req_open_archive(awin, get_config(), TRUE);
			window_menu_set_enable_state(aw);
		}
		aw->iconified = FALSE;
	}
}

void window_close(void *awin, BOOL iconify)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	/* Close new archive window if it's attached to this one */
	newarc_window_close_if_associated(awin);

	/* TODO: tab_close_all() should be here instead?? */

	if(aw->windows[WID_MAIN]) {
		window_remove_dropzones(aw);
		if(aw->appwin) {
			RemoveAppWindow(aw->appwin);
			aw->appwin = NULL;
		}

		if(iconify) {
			RA_Iconify(aw->objects[OID_MAIN]);
		} else {
			RA_CloseWindow(aw->objects[OID_MAIN]);
		}
		aw->windows[WID_MAIN] = NULL;

		if(iconify) {
			aw->iconified = TRUE;
		}
	}
}

void window_dispose(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	/* Remove from window list */
	del_from_window_list(awin);

	/* Ensure appwindow is removed */
	if(aw->windows[WID_MAIN]) window_close(aw, FALSE);
	
	/* Dispose window object */
	DisposeObject(aw->objects[OID_MAIN]);

#ifdef __amigaos4__
	/* Ensure progress bar is disposed */
	DisposeObject(aw->gadgets[GID_PROGRESSFR]);
	DisposeObject(aw->gadgets[GID_PROGRESS]);
#endif

	/* Free archive browser list and column info */
	if(aw->lbci) FreeLBColumnInfo(aw->lbci);
	if(aw->dtci) FreeLBColumnInfo(aw->dtci);
	if(aw->menu) FreeVec(aw->menu);

	/* Free all tabs (only ever one presently) */
	tab_close_all(&aw->tab_list);

	FreeVec(aw);
}

static void parent_dir(struct avalanche_window *aw, struct Node *tab_node)
{
	if(tab_set_current_dir_to_parent(tab_node)) {

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

		window_flat_browser_construct(aw, tab_node);

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, window_get_lblist(aw),
			LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
		TAG_DONE);

		highlight_current_dir(aw, tab_node);
	}
}

static void window_tree_handle(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	ULONG tmp = 0;
	struct Node *node = NULL;

	GetAttr(LISTBROWSER_RelEvent, aw->gadgets[GID_TREE], (APTR)&tmp);

	switch(tmp) {
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_TREE], (APTR)&node);

			if(aw->flat_mode) {
				void *userdata = NULL;
				char *cdir = NULL;

				GetListBrowserNodeAttrs(node,
					LBNA_UserData, &userdata,
				TAG_DONE);

				if(userdata) {
					cdir = AllocVec(strlen(userdata) + 2, MEMF_CLEAR);
					if(cdir == NULL) return;

					strncpy(cdir, userdata, strlen(userdata));
					strcat(cdir, "/"); // add trailing slash
				}
				
				tab_set_current_dir(aw->tab_node, cdir);
				if(cdir) FreeVec(cdir); /* tab_set_current_dir copies the string */

				/* switch to dir */
				SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, ~0, TAG_DONE);

				window_flat_browser_construct(aw, aw->tab_node);

				SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, window_get_lblist(aw),
					LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
				TAG_DONE);
			}
		break;
	}
}
					

static void window_list_handle(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	ULONG tmp = 0;
	struct Node *node = NULL;
	long ret = 0;
	
	GetAttr(LISTBROWSER_RelEvent, aw->gadgets[GID_LIST], (APTR)&tmp);
	switch(tmp) {
		case LBRE_CHECKED:
			if(aw->flat_mode) {
				GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);

				void *userdata = NULL;

				GetListBrowserNodeAttrs(node,
					LBNA_UserData, &userdata,
				TAG_DONE);

				if(userdata != NULL) {
					struct arc_entries *arc_entry = (struct arc_entries *)userdata;
					arc_entry->selected = TRUE;
				}
			}
			window_count_selected(aw, aw->tab_node);
		break;

		case LBRE_UNCHECKED:
			if(aw->flat_mode) {
				GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);

				void *userdata = NULL;

				GetListBrowserNodeAttrs(node,
					LBNA_UserData, &userdata,
				TAG_DONE);

				if(userdata != NULL) {
					struct arc_entries *arc_entry = (struct arc_entries *)userdata;
					arc_entry->selected = FALSE;
				}
			}
			window_count_selected(aw, aw->tab_node);
		break;

#if 0 /* This selects items when single-clicked off the checkbox -
it's incompatible with double-clicking as it resets the listview */
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw, node, 2, TRUE);
			window_count_selected(aw, aw->tab_node);
		break;
#endif
		case LBRE_DOUBLECLICK:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);

			if(aw->flat_mode) {
				void *userdata = NULL;

				GetListBrowserNodeAttrs(node,
					LBNA_UserData, &userdata,
				TAG_DONE);

				if(userdata == NULL) {
					/* this is currently one of our fake dir nodes
					 * NB: this is likely to change to point to the array! */

					char *dir = NULL;

					/* this will be easier with the array pointer -
					 * we need to get the full path which isn't here! */
					GetListBrowserNodeAttrs(node,
						LBNA_Column, 1,
						LBNCA_Text, &dir, 
					TAG_DONE);

					/* Special case: parent dir */
					if(strcmp(dir, "/") == 0) return parent_dir(aw, aw->tab_node);

					ULONG cdir_len = 0;
					const char *c_dir = tab_get_current_dir(aw->tab_node);
					if(c_dir) cdir_len = strlen(c_dir);
					
					char *cdir = AllocVec(cdir_len + 1 + strlen(dir) + 2, MEMF_CLEAR);
					
					if(c_dir) {
						strncpy(cdir, c_dir, cdir_len);
					}
					
					AddPart(cdir, dir, cdir_len + 1 + strlen(dir) + 2);
					strcat(cdir, "/"); // add trailing slash
					tab_set_current_dir(aw->tab_node, cdir);

					/* switch to dir */
					SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
						LISTBROWSER_Labels, ~0, TAG_DONE);

					window_flat_browser_construct(aw, aw->tab_node);
					highlight_current_dir(aw, aw->tab_node);

					SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
						LISTBROWSER_Labels, window_get_lblist(aw),
						LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
					TAG_DONE);

					break;

				}
			}

			toggle_item(aw, node, 1, TRUE); /* ensure selected */
			window_count_selected(aw, aw->tab_node);

			ULONG dest_path_len = strlen(CONFIG_GET_LOCK(tmpdir)) + strlen(get_item_filename(aw, node)) + 4;
			char *dest_path = AllocVec(dest_path_len, MEMF_CLEAR | MEMF_PRIVATE);

			if(dest_path == NULL) {
				CONFIG_UNLOCK;
				return;
			}
			
			strncpy(dest_path, CONFIG_GET(tmpdir), dest_path_len - 1);
			CONFIG_UNLOCK;

			ret = extract(aw, tab_get_archive(aw->tab_node), dest_path, node);
			if(ret == 0) {
				tab_signal_wait(aw->tab_node);
				AddPart(dest_path, get_item_filename(aw, node), dest_path_len);
				
				tab_add_to_delete_list(aw->tab_node, dest_path);
				OpenWorkbenchObjectA(dest_path, NULL);
			} else {
				show_error(ret, aw);
			}
			
			FreeVec(dest_path);
		break;
	}
}

void window_update_archive(void *awin, char *archive)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	tab_set_archive(aw->tab_node, archive);

	/* TODO: move this to tab */
	if(aw->flat_mode) {
		tab_set_current_dir(aw->tab_node, NULL);
	}
}

void window_update_sourcedir(void *awin, char *sourcedir)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	CONFIG_LOCK_EX;
	struct avalanche_config *config = get_config();
	if(config->sourcedir) FreeVec(config->sourcedir);
	config->sourcedir = strdup_vec(sourcedir);
	CONFIG_UNLOCK;
}

void window_fuelgauge_update(void *awin, struct Node *tab_node, ULONG size, ULONG total_size, const char *filename)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;
	if(window_tab_is_current(aw, tab_node) == FALSE) return;

	progress_set_file_level(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], size, total_size, filename, P_WIDTH, P_HEIGHT);
}

/* select: 0 = deselect all, 1 = select all, 2 = toggle all */
void window_modify_all_list(void *awin, ULONG select)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	struct Node *node;
	BOOL selected = FALSE;

	if(select == 1) selected = TRUE;

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	if(aw->flat_mode) {
		for(int i = 0; i < tab_get_total_items(aw->tab_node); i++) {
			struct arc_entries *arc_e = tab_get_arc_entry(aw->tab_node, i);
			const char *c_dir = tab_get_current_dir(aw->tab_node);
			if((c_dir == NULL) || (strncmp(arc_e->name, c_dir, strlen(c_dir)) == 0)) {
				if(select == 2) {
					arc_e->selected = !arc_e->selected;
				} else {
					arc_e->selected = selected;
				}
			}
		}
	}

	struct List *lblist = window_get_lblist(aw);

	for(node = lblist->lh_Head; node->ln_Succ; node=node->ln_Succ) {
		toggle_item(aw, node, select, FALSE);
	}

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, lblist,
				LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
			TAG_DONE);

	window_count_selected(aw, aw->tab_node);
}

const char *window_req_new_lha(void *awin, char *drawer)
{	
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		if(AslRequestTags(aslreq,
				ASLFR_DoSaveMode, TRUE,
				ASLFR_TitleText, locale_get_string(MSG_SELECTARCHIVE) ,
				ASLFR_InitialDrawer, drawer,
			TAG_DONE)) {

			ULONG len = strlen(aslreq->fr_Drawer) + strlen(aslreq->fr_File) + 5;
			char *archive;
			if(archive = AllocVec(len, MEMF_PRIVATE)) {
				strncpy(archive, aslreq->fr_Drawer, len);
				AddPart(archive, aslreq->fr_File, len);
			} else {
				FreeAslRequest(aslreq);
				return NULL;
			}
			
			char *arc_r = NULL;
			
			if(arc_r = newarc_add_ext(archive, "lha")) {
				tab_set_archive(aw->tab_node, arc_r);
			} else {
				tab_set_archive(aw->tab_node, archive);
			}
			
			FreeVec(archive);

		} else {
			return NULL;
		}

		FreeAslRequest(aslreq);
	}

	return tab_get_archive(aw->tab_node);
}

const char *window_req_dest(void *awin, BOOL force_req)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	char *dest = NULL;

	if(force_req == FALSE) {
		BOOL npe = CONFIG_GET_LOCK(no_prompt_extract);
		if(npe) dest = CONFIG_GET(dest);
		CONFIG_UNLOCK;

		if(npe) return(dest);
	}

	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		if(AslRequestTags(aslreq,
				ASLFR_DoSaveMode, TRUE,
				ASLFR_TitleText,  locale_get_string( MSG_SELECTDESTINATION ) ,
				ASLFR_InitialDrawer, tab_get_dest(aw->tab_node),
				ASLFR_DrawersOnly, TRUE,
				TAG_DONE)) {

			tab_set_dest(aw->tab_node, aslreq->fr_Drawer);
		} else {
			return NULL;
		}

		FreeAslRequest(aslreq);
	}

	return tab_get_dest(aw->tab_node);
}

/* Returns FALSE if req cancelled */
static BOOL window_req_archive(struct avalanche_window *aw, struct avalanche_config *config)
{
	BOOL ret = FALSE;
	char *srcdir = NULL;
	
	struct Hook *asl_hook = (struct Hook *)&(aw->aslfilterhook);

	if(CONFIG_GET_LOCK(disable_asl_hook)) {
		asl_hook = NULL;
	}
	char *sdir = strdup_vec(CONFIG_GET(sourcedir));
	CONFIG_UNLOCK;
	
	char *arc = NULL;
	
	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		const char *archive = NULL;
		if(archive = tab_get_archive_name(aw->tab_node)) {
			srcdir = strdup_vec(archive);
			char *p = PathPart(srcdir);
			*p = '\0';
			arc = FilePart(archive);
		} else {
			srcdir = strdup_vec(sdir);
		}
		
		if(AslRequestTags(aslreq,
				ASLFR_DoMultiSelect, FALSE,
				ASLFR_TitleText,  locale_get_string( MSG_SELECTARCHIVE ) ,
				ASLFR_InitialFile, arc,
				ASLFR_InitialDrawer, srcdir,
				ASLFR_FilterFunc, asl_hook,
			TAG_DONE)) {
		
			ULONG len = strlen(aslreq->fr_Drawer) + strlen(aslreq->fr_File) + 5;
			char *arc = AllocVec(len, MEMF_PRIVATE);
			strncpy(arc, aslreq->fr_Drawer, len);
			AddPart(arc, aslreq->fr_File, len);
			if((tab_get_disabled(aw->tab_node)) || (tab_get_format(aw->tab_node) != ARC_NONE)) {
				window_tab_create(aw);
			}
			tab_set_archive(aw->tab_node, arc);
			FreeVec(arc);
			ret = TRUE;
		}
		
		if(srcdir) FreeVec(srcdir);
		FreeAslRequest(aslreq);
	}
	if(sdir) FreeVec(sdir);
	
	return ret;
}

static BOOL window_req_archive_split(struct avalanche_window *aw, struct avalanche_config *config)
{
	BOOL ret = FALSE;
	char *srcdir = NULL;
	void *xs = NULL;
	char *sdir = strdup_vec(CONFIG_GET_LOCK(sourcedir));
	CONFIG_UNLOCK;
	
	char *arc = NULL;
	
	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		if(AslRequestTags(aslreq,
							ASLFR_DoMultiSelect, TRUE,
							ASLFR_TitleText, locale_get_string(MSG_SELECTSPLITARCHIVE) ,
							ASLFR_InitialFile, arc,
							ASLFR_InitialDrawer, sdir,
					TAG_DONE)) {

			ret = TRUE;

			if(aslreq->fr_NumArgs) {
				struct WBArg *frargs = aslreq->fr_ArgList;
				for(int i = 0; i < aslreq->fr_NumArgs; i++) {
					if(frargs->wa_Lock) {
						char *file = NULL;
						if(file = AllocVec(1024, MEMF_CLEAR)) {
							NameFromLock(frargs->wa_Lock, file, 1024);
							if(*frargs->wa_Name) {
								AddPart(file, frargs->wa_Name, 1024);
								xs = xad_split(file, xs);
								if(xs == NULL) return FALSE;
							}
						}
					}
				}

				if((tab_get_disabled(aw->tab_node)) || (tab_get_format(aw->tab_node) != ARC_NONE)) {
					window_tab_create(aw);
				}
				tab_set_split(aw->tab_node, xs);

			} else {
				ULONG len = strlen(aslreq->fr_Drawer) + strlen(aslreq->fr_File) + 5;
				char *arc = AllocVec(len, MEMF_PRIVATE);
				strncpy(arc, aslreq->fr_Drawer, len);
				AddPart(arc, aslreq->fr_File, len);
				if((tab_get_disabled(aw->tab_node)) || (tab_get_format(aw->tab_node) != ARC_NONE)) {
					window_tab_create(aw);
				}
				tab_set_archive(aw->tab_node, arc);
				FreeVec(arc);
			}
		}

		if(srcdir) FreeVec(srcdir);
		FreeAslRequest(aslreq);
	}
	if(sdir) FreeVec(sdir);
	
	return ret;
}

static void window_req_open_archive_internal(void *awin, struct Node *tab_node, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	long retxfd = 0;
	long retark = 0;

	if(window_tab_is_current(aw, tab_node)) {
		if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										WA_PointerDelay, TRUE,
										TAG_DONE);

		if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
											LISTBROWSER_Labels, ~0, TAG_DONE);

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
							LISTBROWSER_Labels, ~0, TAG_DONE);
	}

	tab_reset(tab_node);

	tab_set_format(tab_node, ARC_XAD); /* Set in advance for flat browser tree use */

	ULONG active_modules = CONFIG_GET_LOCK(activemodules);
	CONFIG_UNLOCK;

	const char *archive = tab_get_archive(tab_node);

	if(archive == AVALANCHE_SPLIT_ARCHIVE) {
		/* Split archive, only works with XAD */

	        if(active_modules & ARC_XAD) {
        	        ret = xad_info(archive, tab_get_split(tab_node), config, aw, tab_node, addlbnode_cb);
        	} else {
                	ret = 10;
        	}

		if(ret != 0) {
			open_error_req(module_get_error(tab_node, ret), locale_get_string(MSG_OK), aw);
			tab_set_format(tab_node, ARC_NONE);
		}
	} else {

		if(active_modules & ARC_XAD) {
			ret = xad_info(archive, NULL, config, aw, tab_node, addlbnode_cb);
		} else {
			ret = 10;
		}
		if(ret != 0) { /* if xad failed try xfd */
			tab_set_format(tab_node, ARC_XFD);

			if(active_modules & ARC_XFD) {
				retxfd = xfd_info(archive, aw, tab_node, addlbnode_cb);
			} else {
				retxfd = 1;
			}

			if(retxfd != 0) {
				tab_set_format(tab_node, ARC_DEARK);
				if(active_modules & ARC_DEARK) {
					retark = deark_info(archive, config, aw, tab_node, addlbnode_cb);
				} else {
					retark = 1;
				}

				if(retark != 0) {
					/* Failed to open with any lib - show generic error rather than XAD's */
					tab_set_format(tab_node, ARC_NONE);
					open_error_req(locale_get_string(MSG_UNABLETOOPENFILE), locale_get_string(MSG_OK), aw);
				}
			}
		}
	}

	module_register(tab_node);

	if(window_tab_is_current(aw, tab_node)) {
		window_menu_set_enable_state(aw);

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, tab_get_listbrowser_list(tab_node),
					LISTBROWSER_SortColumn, 1,
					LISTBROWSER_AutoFit, AVALANCHE_AUTOFIT,
				TAG_DONE);

		if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, tab_get_dirtree_list(tab_node), TAG_DONE);
		if(aw->flat_mode) {
			highlight_current_dir(aw, tab_node);
		}

		window_update_title(aw, tab_node);
		window_count_selected(awin, tab_node);
	} else {
		window_update_tab_title(aw, tab_node);
	}

	tab_set_disabled(tab_node, FALSE, FALSE);
}

#ifdef __amigaos4__
static void window_req_open_archive_p(void)
#else
static void __saveds window_req_open_archive_p(void)
#endif
{
	/* Tell the main process we are started */
	avalanche_signal(SIGBREAKF_CTRL_F);

	/* Wait for UserData */
	Wait(SIGBREAKF_CTRL_E);

	/* Find our task */
	struct Task *list_task = FindTask(NULL);
	struct avalanche_extract_userdata *aeu = (struct avalanche_extract_userdata *)list_task->tc_UserData;
	struct avalanche_window *aw = (struct avalanche_window *)aeu->awin;

	/* Call Open on our new process */
	window_req_open_archive_internal(aeu->awin, aeu->tab_node, get_config());

	/* Free UserData */
	FreeVec(aeu);

	/* Signal that we've finished */
	avalanche_signal(aeu->sig);
}

static void window_req_open_archive_any(struct avalanche_window *aw, struct avalanche_config *config)
{
	struct avalanche_extract_userdata *aeu = AllocVec(sizeof(struct avalanche_extract_userdata), MEMF_CLEAR);

	if(aeu == NULL) return;

	aeu->awin = aw;
	aeu->archive = NULL;
	aeu->newdest = NULL;
	aeu->node = NULL;
	aeu->tab_node = aw->tab_node;
	aeu->sig = tab_get_signal(aw->tab_node);

	tab_clear_abort(aw->tab_node);
	tab_set_disabled(aw->tab_node, TRUE, TRUE);

	/* Ensure there are no pending signals for this already */
	tab_signal_clear(aw->tab_node);

	struct Process *list_process = CreateNewProcTags(NP_Entry, window_req_open_archive_p,
						NP_Name, "Avalanche list process",
						NP_Priority, -1,
						TAG_DONE);

	/* Wait for the process to start up */
	Wait(SIGBREAKF_CTRL_F);

	/* Poke UserData */
	list_process->pr_Task.tc_UserData = aeu;

	/* Signal the process to continue */
	Signal(list_process, SIGBREAKF_CTRL_E);

	return;
}

void window_req_open_archive(void *awin, struct avalanche_config *config, BOOL refresh_only)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(refresh_only == FALSE) {
		BOOL ret = window_req_archive(aw, config);
		if(ret == FALSE) return;

		if(aw->flat_mode) {
			tab_set_current_dir(aw->tab_node, NULL);
		}
	}

	window_req_open_archive_any(aw, config);
}

static void window_req_open_archive_split(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	BOOL ret = window_req_archive_split(aw, config);
	if(ret == FALSE) return;

	if(aw->flat_mode) {
		tab_set_current_dir(aw->tab_node, NULL);
	}

	window_req_open_archive_any(aw, config);
}

Object *window_get_object(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return aw->objects[OID_MAIN];
}

void *window_get_window(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw == NULL) {
		return NULL;
	} else {
		return aw->windows[WID_MAIN];
	}
}

static void *window_get_lbnode_quiet(void *awin, struct Node *node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	void *userdata = NULL;
	ULONG checked = FALSE;

	GetListBrowserNodeAttrs(node,
			LBNA_Checked, &checked,
			LBNA_UserData, &userdata,
		TAG_DONE);

	if(aw->flat_mode) {
		struct arc_entries *arc_entry = (struct arc_entries *)userdata;
		if(arc_entry == NULL) return NULL; /* dummy entry */
		userdata = arc_entry->userdata;
	}

	if(checked == FALSE) return NULL;
	return userdata;
}

void *window_get_lbnode(void *awin, struct Node *node)
{
	update_fuelgauge_text(awin);
	return window_get_lbnode_quiet(awin, node);
}

struct List *window_get_lblist(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return tab_get_listbrowser_list(aw->tab_node);
}

ULONG window_handle_input(void *awin, UWORD *code)
{
	return RA_HandleInput(window_get_object(awin), code);
}

BOOL window_edit_add(void *awin, char *file, char *root)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(module_vscan(awin, file, NULL, 0, FALSE) == 0) {
		progress_set_adding(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESS], aw->gadgets[GID_PROGRESSFR], file, P_WIDTH, P_HEIGHT);
		module_free(aw->tab_node);
		return module_add(aw, aw->tab_node, file, tab_get_current_dir(aw->tab_node), root);
	}

	return FALSE;
}

BOOL window_edit_add_wbarg(void *awin, struct WBArg *wbarg)
{
	BOOL ret = TRUE;
	
	if(wbarg->wa_Lock) {
		char *file = NULL;
		if(file = AllocVec(1024, MEMF_CLEAR)) {
			NameFromLock(wbarg->wa_Lock, file, 1024);

			tab_set_disabled(window_get_current_tab(awin), TRUE, FALSE);

			if(*wbarg->wa_Name) {
				AddPart(file, wbarg->wa_Name, 1024);
			}

#ifdef __amigaos4__
			if(object_is_dir(file)) {
				recursive_scan(awin, file, file);
			} else {
				ret = window_edit_add(awin, file, NULL);
			}
#else
			BPTR lock = Lock(file, ACCESS_READ);
			if(lock) {
				if(object_is_dir(lock)) {
					recursive_scan(awin, lock, file);
				} else {
					ret = window_edit_add(awin, file, NULL);
				}
				UnLock(lock);
			}
#endif
			tab_set_disabled(window_get_current_tab(awin), FALSE, FALSE);

			FreeVec(file);
		}
	}
	/* Archive refresh - maybe happens elsewhere? */

	return ret;
}

static void window_edit_add_req(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	BOOL ok = FALSE;
	char *file;

	if(module_has_add(aw->tab_node) == FALSE) return;
	
	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		if(AslRequestTags(aslreq, ASLFR_DoMultiSelect, TRUE, TAG_DONE)) {
			if(aslreq->fr_NumArgs) {
				struct WBArg *frargs = aslreq->fr_ArgList;
				for(int i = 0; i < aslreq->fr_NumArgs; i++) {
					BOOL ret = window_edit_add_wbarg(awin, frargs);
					if(ret == FALSE) break; /* FALSE = Abort */
					frargs++;
				}
			} else {
				ULONG len = strlen(aslreq->fr_Drawer) + strlen(aslreq->fr_File) + 5;
				file = AllocVec(len, MEMF_PRIVATE);
				strncpy(file, aslreq->fr_Drawer, len);
				AddPart(file, aslreq->fr_File, len);

				tab_set_disabled(window_get_current_tab(awin), TRUE, FALSE);

#ifdef __amigaos4__
				if(object_is_dir(file)) {
					recursive_scan(awin, file, aslreq->fr_Drawer);
				} else {
					ok = window_edit_add(awin, file, NULL);
				}
#else
				BPTR lock = Lock(file, ACCESS_READ);
				if(lock) {
					if(object_is_dir(lock)) {
						recursive_scan(awin, lock, aslreq->fr_Drawer);
					} else {
						ok = window_edit_add(awin, file, NULL);
					}
					UnLock(lock);
				}
#endif

				// why is this here? ok = window_edit_add(aw, file, NULL); /* TRUE = cont, FALSE = abort */
				tab_set_disabled(window_get_current_tab(awin), FALSE, FALSE);
			}
		}
		FreeAslRequest(aslreq);
	}

	/* Refresh the archive */
	window_req_open_archive(aw, config, TRUE);
}

static void window_edit_del(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	char *filename;
	struct Node *node;
	struct List *list = window_get_lblist(awin);
	
	if(module_has_del(aw->tab_node) == FALSE) return;

	tab_set_current_item(aw->tab_node, 0);
	tab_set_disabled(window_get_current_tab(awin), TRUE, FALSE);

	/* module_free(aw);
	 * TODO: copy the files to delete into a list so we can release the archive */

	/* Count selected nodes */
	ULONG entries = 0;
	ULONG total = 0;
	for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
		void *userdata = window_get_lbnode(awin, node);
		if(userdata) entries++;
		total++;
	}

	if(entries < total) {
		if(ask_yesno_req(awin, locale_get_string(MSG_CONFIRMDELETE))) {

			/* Create array of names */
			char **name_array = AllocVec(entries * sizeof(char *), MEMF_CLEAR | MEMF_PRIVATE);
			ULONG i = 0;

			if(name_array) {

				for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
					void *userdata = window_get_lbnode(awin, node);
					if(userdata) {
						name_array[i] = strdup_vec(module_get_item_filename(aw->tab_node, userdata));
						i++;
					}
				}

				module_free(aw->tab_node);
				module_del(aw, aw->tab_node, name_array, entries);

				for(i = 0; i<entries; i++) {
					FreeVec(name_array[i]);
				}
				FreeVec(name_array);
			}
		}
	} else {
		open_error_req(locale_get_string(MSG_ARCHIVEMUSTHAVEENTRIES), locale_get_string(MSG_OK), awin);
	}

	tab_set_disabled(window_get_current_tab(awin), FALSE, FALSE);

	/* Refresh the archive */
	window_req_open_archive(aw, config, TRUE);

	return;
}

static void toggle_drag_lock(struct avalanche_window *aw, struct MenuItem *item)
{
	window_remove_dropzones(aw);

	if(item->Flags & CHECKED) {
		aw->drag_lock = TRUE;
	} else {
		aw->drag_lock = FALSE;
	}

	window_add_dropzones(aw, !aw->drag_lock);
}

static void toggle_flat_mode(struct avalanche_window *aw, struct avalanche_config *config, BOOL on)
{
	aw->flat_mode = on;

	BOOL disable = !aw->flat_mode;

/*
	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
		GA_Disabled, disable,
	TAG_DONE);
*/

	/* Add the tree if needed */
	if(aw->flat_mode) {
		window_tree_add(aw);
	} else {
		window_tree_remove(aw);
	}

	if(tab_get_format(aw->tab_node) != ARC_NONE) window_req_open_archive(aw, config, TRUE);
}

static BOOL window_key_shift(struct avalanche_window *aw)
{
	UWORD quals = 0;

	GetAttr(WINDOW_Qualifier, aw->objects[OID_MAIN], (ULONG *)&quals);

#ifdef __amigaos4__
	DebugPrintF("[Avalanche] Quals: %d\n", quals);
#endif

	if(quals & ANY_SHIFT) return TRUE;
	return FALSE;
}

ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code, struct MsgPort *winport, struct MsgPort *AppPort)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	ULONG done = WIN_DONE_OK;

#ifndef __amigaos4__
	if(check_closetab(awin)) done = WIN_DONE_CLOSED;
#endif

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
			done = WIN_DONE_CLOSED;
		break;

		case WMHI_GADGETUP:
			switch (result & WMHI_GADGETMASK) {
				case GID_ARCHIVE:
					if(window_key_shift(aw)) {
						window_req_open_archive_split(awin, config);
					} else {
						window_req_open_archive(awin, config, FALSE);
					}
				break;
					
				case GID_OPENWB:
					window_open_dest(awin);
				break;

				case GID_EXTRACT:
				{
					const char *dest = window_req_dest(aw, FALSE);
					if(dest != NULL) {
						ret = extract(awin, tab_get_archive(aw->tab_node), dest, NULL);
						if(ret != 0) show_error(ret, awin);
					}
				}
				break;

				case GID_ABORT:
					if(tab_get_disabled(aw->tab_node)) tab_abort(aw->tab_node);
				break;
				
				case GID_LIST:
					window_list_handle(awin);
				break;
				
				case GID_TREE:
					window_tree_handle(awin);
				break;
				
				case GID_TABS:
				{
					struct Node *tabnode = NULL;

#ifdef __amigaos4__ /* OS4 checks for tab close events here */
					GetAttr(CLICKTAB_NodeClosed, aw->gadgets[GID_TABS], &tabnode);

					if(tabnode) { /* Tab closed */
						if(tab_close(tabnode)) {
							done = WIN_DONE_CLOSED;
						}
					} else
#endif
					{
						GetAttr(CLICKTAB_CurrentNode, (Object *)aw->gadgets[GID_TABS], (ULONG *)&tabnode);
						
						if(window_tab_is_current(aw, tabnode) == FALSE) {
							/* Tab switched */
							window_tab_set(aw, tabnode);
						}
					}
				}
				break;
			}
			break;

		case WMHI_RAWKEY:
			switch(result & WMHI_GADGETMASK) {
				case RAWKEY_ESC:
					if(tab_get_disabled(aw->tab_node)) {
						tab_abort(aw->tab_node);
					} else {
						done = WIN_DONE_CLOSED;
					}
				break;

				case RAWKEY_RETURN:
					ret = extract(awin, tab_get_archive(aw->tab_node), tab_get_dest(aw->tab_node), NULL);
					if(ret != 0) show_error(ret, awin);
				break;
			}
		break;

		case WMHI_ICONIFY:
			window_close(awin, TRUE);
		break;

		case WMHI_UNICONIFY:
			window_open(awin, appwin_mp);
		break;
		
		case WMHI_NEWSIZE:
			if(tab_get_disabled(aw->tab_node) == FALSE) {
				window_remove_dropzones(aw);
				window_add_dropzones(aw, !aw->drag_lock);
			}
#ifdef __amigaos4__
			progress_set_new_width(aw->windows[WID_MAIN], aw->gadgets[GID_PROGRESSFR], aw->gadgets[GID_PROGRESS]);
			progress_get_area(aw->windows[WID_MAIN], &aw->p_width, &aw->p_height, &aw->p_sz_height);
#endif
		break;

		case WMHI_MENUPICK:
			while((code != MENUNULL) && (done != WIN_DONE_CLOSED)) {
				if(aw->windows[WID_MAIN] == NULL) continue;
				void *new_awin;
				struct MenuItem *item = ItemAddress(aw->windows[WID_MAIN]->MenuStrip, code);
				switch(MENUNUM(code)) {
					case 0: //project
						switch(ITEMNUM(code)) {
							case 0: // new archive
								newarc_window_open(aw);
							break;

							case 1: // new tab
								window_tab_create(aw);
							break;

							case 2: //open
								switch(SUBNUM(code)) {
									case 0: //archive
										window_req_open_archive(awin, config, FALSE);
									break;
									case 1: //split
										window_req_open_archive_split(awin, config);
									break;
								}
							break;

							case 3: //extract
							{
								const char *dest = window_req_dest(aw, TRUE);
								if(dest != NULL) {
									ret = extract(awin, tab_get_archive(aw->tab_node), dest, NULL);
									if(ret != 0) show_error(ret, awin);
								}
							}
							break;

							case 5: //info
								req_show_arc_info(awin, aw->tab_node);
							break;

							case 6: //about
								show_about(awin);
							break;

							case 7: //check version
								if(window_get_window(awin))
									SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, TRUE,
											TAG_DONE);

#ifdef __amigaos4__
								http_check_version(TRUE);
#else
								http_check_version(FALSE);
#endif
								if(window_get_window(awin))
									SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);
							break;
							
							case 9: //quit
								if(ask_quit(awin)) {
									done = WIN_DONE_QUIT;
								}
							break;
						}
					break;
					
					case 1: //edit
						switch(ITEMNUM(code)) {
							case 0: //select all
								window_modify_all_list(awin, 1);
							break;
							
							case 1: //clear selection
								window_modify_all_list(awin, 0);
							break;

							case 2: //invert selection
								window_modify_all_list(awin, 2);
							break;
							
							case 4: //add files
								window_edit_add_req(awin, config);
							break;
							
							case 5: //del files
								window_edit_del(awin, config);
							break;

							case 7: //drag lock
								toggle_drag_lock(aw, item);
							break;
						}
					break;
					
					case 2: //window
						switch(ITEMNUM(code)) {
							case 0: // new window
								new_awin = window_create(config, NULL, winport, AppPort);
								if(new_awin == NULL) break;
								window_open(new_awin, appwin_mp);
							break;

							case 1: // view mode
								switch(SUBNUM(code)) {
									case 0: //flat browser mode
										toggle_flat_mode(aw, config, TRUE);
									break;
									case 1: // list mode
										toggle_flat_mode(aw, config, FALSE);
									break;
								}
							break;

							case 2: // dir tree
								switch(SUBNUM(code)) {
									case 0: // collapse
										collapse_tree(aw, FALSE);
									break;
									case 1: // expand
										collapse_tree(aw, TRUE);
									break;
								}
							break;

							case 4: // close
								done = WIN_DONE_CLOSED;
							break;
						}
					break;

					case 3: //settings
						switch(ITEMNUM(code)) {
							case 0: //snapshot
								/* fetch current win posn */
								CONFIG_LOCK;
								GetAttr(WA_Top, aw->objects[OID_MAIN], (APTR)&config->win_y);
								GetAttr(WA_Left, aw->objects[OID_MAIN], (APTR)&config->win_x);
								GetAttr(WA_Width, aw->objects[OID_MAIN], (APTR)&config->win_w);
								GetAttr(WA_Height, aw->objects[OID_MAIN], (APTR)&config->win_h);
								CONFIG_UNLOCK;
								
								warning_req(aw, locale_get_string(MSG_SNAPSHOT_WARNING));
							break;
								
							case 1: //prefs
								config_window_open(config);
							break;
						}
					break;
				}
				code = item->NextSelect;
			}
		break; //WMHI_MENUPICK
	}

	return done;
}

#ifndef __amigaos4__
BOOL check_closetab(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	/* This flag is set in the IDCMP hook if tab is closed
	 * In future this might only indicate if the last tab is closed */

	BOOL done = aw->tab_closed;

	/* Clear flag for next time */
	aw->tab_closed = FALSE;

	return done;
}
#endif

void fill_menu_labels(void)
{
	menu[0].nm_Label = locale_get_string( MSG_PROJECT );
	menu[1].nm_Label = locale_get_string( MSG_NEWARCHIVE );
	menu[2].nm_Label = locale_get_string( MSG_NEWTAB );
	menu[3].nm_Label = locale_get_string( MSG_M_OPEN );
	menu[4].nm_Label = locale_get_string( MSG_M_OPEN_ARCHIVE );
	menu[5].nm_Label = locale_get_string( MSG_M_OPEN_SPLIT );
	menu[6].nm_Label = locale_get_string(MSG_M_EXTRACT);
	menu[8].nm_Label = locale_get_string( MSG_ARCHIVEINFO );
	menu[9].nm_Label = locale_get_string( MSG_ABOUT );
	menu[10].nm_Label = locale_get_string( MSG_CHECKVERSION );
	menu[12].nm_Label = locale_get_string( MSG_QUIT );
	menu[13].nm_Label = locale_get_string( MSG_EDIT );
	menu[14].nm_Label = locale_get_string( MSG_SELECTALL );
	menu[15].nm_Label = locale_get_string( MSG_CLEARSELECTION );
	menu[16].nm_Label = locale_get_string( MSG_INVERTSELECTION );
	menu[18].nm_Label = locale_get_string( MSG_ADDFILES );
	menu[19].nm_Label = locale_get_string( MSG_DELFILES );
	menu[21].nm_Label = locale_get_string( MSG_DRAGLOCK );
	menu[22].nm_Label = locale_get_string( MSG_WINDOW );
	menu[23].nm_Label = locale_get_string( MSG_NEWWINDOW );
	menu[24].nm_Label = locale_get_string( MSG_VIEWMODE );
	menu[MENU_FLATMODE].nm_Label = locale_get_string( MSG_VIEWMODEBROWSER );
	menu[MENU_LISTMODE].nm_Label = locale_get_string( MSG_VIEWMODELIST );
	menu[27].nm_Label = locale_get_string( MSG_DIR_TREE );
	menu[28].nm_Label = locale_get_string( MSG_COLLAPSE_ALL );
	menu[29].nm_Label = locale_get_string( MSG_EXPAND_ALL );
	menu[31].nm_Label = locale_get_string( MSG_CLOSE );
	menu[32].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[33].nm_Label = locale_get_string( MSG_SNAPSHOT );
	menu[34].nm_Label = locale_get_string( MSG_PREFERENCES );
}

struct Node *window_get_current_tab(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return aw->tab_node;
}

void window_tab_detach(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->gadgets[GID_TABS] == NULL) return;
	
	SetGadgetAttrs(aw->gadgets[GID_TABS],
		window_get_window(aw), NULL,
		CLICKTAB_Labels, ~0,
		CLICKTAB_MinorLabelChange, TRUE,
		TAG_DONE);
}
