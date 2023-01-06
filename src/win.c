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

#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#include <intuition/pointerclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>

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

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "locale.h"
#include "module.h"
#include "req.h"
#include "win.h"
#include "xad.h"
#include "xfd.h"

#include "Avalanche_rev.h"

enum {
	GID_MAIN = 0,
	GID_ARCHIVE,
	GID_DEST,
	GID_LIST,
	GID_EXTRACT,
	GID_PROGRESS,
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

struct arc_entries {
	char *name;
	ULONG *size;
	BOOL dir;
	void *userdata;
};

#define AVALANCHE_DROPZONES 2

struct avalanche_window {
	struct MinNode node;
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];
	struct NewMenu *menu;
	ULONG archiver;
	struct ColumnInfo *lbci;
	struct List lblist;
	struct Hook aslfilterhook;
	struct Hook appwindzhook;
	struct AppWindow *appwin;
	struct AppWindowDropZone *appwindz[AVALANCHE_DROPZONES];
	char *archive;
	char *dest;
	struct MinList deletelist;
	struct arc_entries **arc_array;
	ULONG current_item;
	ULONG total_items;
	BOOL archive_needs_free;
	void *archive_userdata;
	BOOL h_mode;
	BOOL iconified;
	struct module_functions mf;
};

struct List winlist;

/** Menu **/

struct NewMenu menu[] = {
	{NM_TITLE,  NULL,           0,  0, 0, 0,}, // 0 Project
	{NM_ITEM,   NULL,         "O", 0, 0, 0,}, // 0 Open
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 1
	{NM_ITEM,   NULL , "!", NM_ITEMDISABLED, 0, 0,}, // 2 Archive Info
	{NM_ITEM,   NULL ,        "?", 0, 0, 0,}, // 3 About
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 4
	{NM_ITEM,   NULL,         "Q", 0, 0, 0,}, // 5 Quit

	{NM_TITLE,  NULL,               0,  0, 0, 0,}, // 1 Edit
	{NM_ITEM,   NULL,       "A", NM_ITEMDISABLED, 0, 0,}, // 0 Select All
	{NM_ITEM,   NULL ,  "Z", NM_ITEMDISABLED, 0, 0,}, // 1 Clear Selection
	{NM_ITEM,   NULL , "I", NM_ITEMDISABLED, 0, 0,}, // 2 Invert
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 3
	{NM_ITEM,   NULL,       "+", NM_ITEMDISABLED, 0, 0,}, // 4 Add files
	{NM_ITEM,   NULL,       0, NM_ITEMDISABLED, 0, 0,}, // 5 Delete files

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Settings
	{NM_ITEM,	NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 0 HBrowser
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 1 Snapshot
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 2
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 3 Preferences

	{NM_END,   NULL,        0,  0, 0, 0,},
};

#define MENU_SETTINGS_HBROWSER 15

#define GID_EXTRACT_TEXT  locale_get_string(MSG_EXTRACT)

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

	/* Only change pointer if we are able to add files */
	if(aw->mf.add == NULL) return 0;

	switch(awdzm->adzm_Action) {
		case ADZMACTION_Enter:
			SetWindowPointer(aw->windows[WID_MAIN], WA_PointerType, POINTERTYPE_COPY, TAG_DONE);
		break;
		
		case ADZMACTION_Leave:
			SetWindowPointer(aw->windows[WID_MAIN], TAG_DONE);
		break;
	}

	return 0;
}

static void window_free_archive_path(struct avalanche_window *aw)
{
	/* NB: uses free() not FreeVec() */
	if(aw->archive) free(aw->archive);
	aw->archive = NULL;
	aw->archive_needs_free = FALSE;
}

/* Activate/disable menus related to an open archive */
static void window_menu_activation(void *awin, BOOL enable)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;

	if(enable) {
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,2,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0));
		if(aw->mf.add) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0));
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0));
		}
		if(aw->mf.del) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0));
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0));
		}
	} else {
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,2,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0));
	}
}

static void window_menu_set_enable_state(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->archiver == ARC_NONE) {
		window_menu_activation(aw, FALSE);
	} else {
		window_menu_activation(aw, TRUE);
	}
}

static void toggle_item(struct avalanche_window *aw, struct Node *node, ULONG select)
{
	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	ULONG current;
	ULONG selected;

	if(select == 2) {
		GetListBrowserNodeAttrs(node, LBNA_Checked, &current, TAG_DONE);
		selected = !current;
	} else {
		selected = select;
	}

	SetListBrowserNodeAttrs(node, LBNA_Checked, selected, TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, &aw->lblist,
			TAG_DONE);
}

static void delete_delete_list(struct avalanche_window *aw)
{
	struct Node *node;
	struct Node *nnode;

	if(IsMinListEmpty((struct MinList *)&aw->deletelist) == FALSE) {
		node = (struct Node *)GetHead((struct List *)&aw->deletelist);

		do {
			nnode = (struct Node *)GetSucc((struct Node *)node);
			Remove((struct Node *)node);
			if(node->ln_Name) {
				DeleteFile(node->ln_Name);
				free(node->ln_Name);
			}
			FreeVec(node);
		} while((node = nnode));
	}
}

static void add_to_delete_list(struct avalanche_window *aw, char *fn)
{
	struct Node *node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
	if(node) {
		node->ln_Name = strdup(fn);
		AddTail((struct List *)&aw->deletelist, (struct Node *)node);
	}
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

	return sort(c->name, d->name);
}

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, BOOL h, struct avalanche_window *aw)
{
	BOOL dir_seen = FALSE;
	ULONG flags = 0;
	ULONG gen = 0;
	int i = 0;
	char *name_copy = NULL;
	BOOL debug = get_config()->debug;

	if(h) {
		gen = 1;
		
		/* Count the slashes to find directory level */
		while(name[i+1]) { /* ignore any trailing slash */
			if(name[i] == '/') gen++;
			i++;
		}

		if(get_xad_ver() == 12) {
			/* In xadmaster.library 12, sometimes directories aren't marked as such */
			if(name[i] == '/') {
				dir = TRUE;
				name_copy = strdup(name);
				name_copy[i] = '\0';
			}
		}

		if (dir) {
			dir_seen = TRUE;
			flags = LBFLG_HASCHILDREN;
			if(debug) flags |= LBFLG_SHOWCHILDREN;
		}

		if((gen > 1) && (dir_seen == FALSE)) {
			/* Probably we have an archive which doesn't have directory nodes */
			gen = 1;
		} else {
			if(debug == FALSE) {
				if(gen > 1) flags |= LBFLG_HIDDEN;
				if(name_copy == NULL) {
					name = FilePart(name);
				} else {
					name = name + (FilePart(name_copy) - name_copy);
					free(name_copy);
				}
			}
		}
	}

	char datestr[20];
	struct ClockData cd;
	xad_get_filedate(userdata, &cd, aw);

	if(CheckDate(&cd) == 0)
		Amiga2Date(0, &cd);

	snprintf(datestr, 20, "%04u-%02u-%02u %02u:%02u:%02u", cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);

	struct Node *node = AllocListBrowserNode(3,
		LBNA_UserData, userdata,
		LBNA_CheckBox, TRUE,
		LBNA_Checked, TRUE,
		LBNA_Flags, flags,
		LBNA_Generation, gen,
		LBNA_Column, 0,
			LBNCA_Text, name,
		LBNA_Column, 1,
			LBNCA_Integer, size,
		LBNA_Column, 2,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, datestr,
		TAG_DONE);

	AddTail(&aw->lblist, node);
}

static void addlbnodesinglefile(char *name, LONG *size, void *userdata, struct avalanche_window *aw)
{
	struct Node *node = AllocListBrowserNode(3,
		LBNA_UserData, userdata,
		LBNA_CheckBox, TRUE,
		LBNA_Checked, TRUE,
		LBNA_Flags, 0,
		LBNA_Generation, 0,
		LBNA_Column, 0,
			LBNCA_Text, name,
		LBNA_Column, 1,
			LBNCA_Integer, size,
		LBNA_Column, 2,
			//LBNCA_CopyText, TRUE,
			LBNCA_Text, NULL,
		TAG_DONE);

	AddTail(&aw->lblist, node);
}

static void addlbnodexfd_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->gadgets[GID_PROGRESS]) {
		char msg[20];
		sprintf(msg, "%d/%lu", 0, total);
		aw->total_items = total;

		SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
						GA_Text, msg,
						FUELGAUGE_Percent, FALSE,
						FUELGAUGE_Justification, FGJ_CENTER,
						FUELGAUGE_Level, 0,
						TAG_DONE);
	}

	addlbnodesinglefile(name, size, userdata, aw);
	return;
}

static void addlbnode_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(item == 0) {
		aw->current_item = 0;
		if(aw->gadgets[GID_PROGRESS]) {
			char msg[20];
			sprintf(msg, "%d/%lu", 0, total);
			aw->total_items = total;

			SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
				GA_Text, msg,
				FUELGAUGE_Percent, FALSE,
				FUELGAUGE_Justification, FGJ_CENTER,
				FUELGAUGE_Level, 0,
				TAG_DONE);
		}
	}

	if(aw->archiver == ARC_XFD) { addlbnodesinglefile(name, size, userdata, aw); return; }

	if(aw->h_mode) {
		if(item == 0) {
			aw->arc_array = AllocVec(total * sizeof(struct arc_entries *), MEMF_CLEAR);
		}

		if(aw->arc_array) {
			aw->arc_array[item] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			
			aw->arc_array[item]->name = name;
			aw->arc_array[item]->size = size;
			aw->arc_array[item]->dir = dir;
			aw->arc_array[item]->userdata = userdata;
			
			if(item == (total - 1)) {
				if(config->debug) qsort(aw->arc_array, total, sizeof(struct arc_entries *), sort_array);
				for(int i=0; i<total; i++) {
					addlbnode(aw->arc_array[i]->name, aw->arc_array[i]->size, aw->arc_array[i]->dir, aw->arc_array[i]->userdata, aw->h_mode, aw);
					FreeVec(aw->arc_array[i]);
				}
				FreeVec(aw->arc_array);
				aw->arc_array = NULL;
			}
		}
	} else {
		addlbnode(name, size, dir, userdata, aw->h_mode, aw);
	}
}

static const char *get_item_filename(void *awin, struct Node *node)
{
	void *userdata = NULL;
	const char *fn = NULL;

	GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

	return module_get_item_filename(awin, userdata);
}


/* Window functions */

void *window_create(struct avalanche_config *config, char *archive, struct MsgPort *winport, struct MsgPort *appport)
{
	struct avalanche_window *aw = AllocVec(sizeof(struct avalanche_window), MEMF_CLEAR | MEMF_PRIVATE);
	if(!aw) return NULL;
	
	struct Hook *asl_hook = (struct Hook *)&(aw->aslfilterhook);
	
	if(config->disable_asl_hook) {
		asl_hook = NULL;
	}

	ULONG tag_default_position = WINDOW_Position;

	aw->lbci = AllocLBColumnInfo(3, 
		LBCIA_Column, 0,
			LBCIA_Title,  locale_get_string(MSG_NAME),
			LBCIA_Weight, 65,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string(MSG_SIZE),
			LBCIA_Weight, 15,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 2,
			LBCIA_Title,  locale_get_string(MSG_DATE),
			LBCIA_Weight, 20,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		TAG_DONE);

	NewList(&aw->lblist);

	if(aw->menu = AllocVec(sizeof(menu), MEMF_PRIVATE))
		CopyMem(&menu, aw->menu, sizeof(menu));

	if(config->win_x && config->win_y) tag_default_position = TAG_IGNORE;
	
	/* Copy global to local config */
	aw->h_mode = config->h_browser;

	/* ASL hook */
	aw->aslfilterhook.h_Entry = aslfilterfunc;
	aw->aslfilterhook.h_SubEntry = NULL;
	aw->aslfilterhook.h_Data = NULL;
	
	if(archive) {
		aw->archive = strdup(archive);
		aw->archive_needs_free = TRUE;
	}
	
	NewMinList(&aw->deletelist);
	
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
		WINDOW_NewMenu, aw->menu,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_IconTitle, "Avalanche",
		WINDOW_SharedPort, winport,
		WINDOW_AppPort, appport,
		tag_default_position, WPOS_CENTERSCREEN,
		WINDOW_ParentGroup, aw->gadgets[GID_MAIN] = LayoutVObj,
			//LAYOUT_DeferLayout, TRUE,
			LAYOUT_SpaceOuter, TRUE,
			LAYOUT_AddChild, LayoutHObj,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild,  aw->gadgets[GID_ARCHIVE] = GetFileObj,
						GA_ID, GID_ARCHIVE,
						GA_RelVerify, TRUE,
						GETFILE_TitleText,  locale_get_string( MSG_SELECTARCHIVE ) ,
						GETFILE_FullFile, aw->archive,
						GETFILE_ReadOnly, TRUE,
						GETFILE_Drawer, config->sourcedir,
						GETFILE_FilterFunc, asl_hook,
					End,
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text,  locale_get_string( MSG_ARCHIVE ) ,
					LabelEnd,
					LAYOUT_AddChild,  aw->gadgets[GID_DEST] = GetFileObj,
						GA_ID, GID_DEST,
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
				LayoutEnd,
				LAYOUT_WeightBar, TRUE,
				LAYOUT_AddChild,  aw->gadgets[GID_PROGRESS] = FuelGaugeObj,
					GA_ID, GID_PROGRESS,
				FuelGaugeEnd,
				CHILD_WeightedWidth, config->progress_size,
			LayoutEnd,
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, LayoutVObj,
				LAYOUT_AddChild,  aw->gadgets[GID_LIST] = ListBrowserObj,
					GA_ID, GID_LIST,
					GA_RelVerify, TRUE,
					LISTBROWSER_ColumnInfo, aw->lbci,
					LISTBROWSER_Labels, &(aw->lblist),
					LISTBROWSER_ColumnTitles, TRUE,
					LISTBROWSER_TitleClickable, TRUE,
					LISTBROWSER_SortColumn, 0,
					LISTBROWSER_Striping, LBS_ROWS,
					LISTBROWSER_FastRender, TRUE,
					LISTBROWSER_Hierarchical, aw->h_mode,
				ListBrowserEnd,
				LAYOUT_AddChild,  aw->gadgets[GID_EXTRACT] = ButtonObj,
					GA_ID, GID_EXTRACT,
					GA_RelVerify, TRUE,
					GA_Text, GID_EXTRACT_TEXT,
				ButtonEnd,
				CHILD_WeightedHeight, 0,
			LayoutEnd,
		EndGroup,
	EndWindow;


	/*  Object creation sucessful?
	 */
	if (aw->objects[OID_MAIN]) {
		GetAttr(GETFILE_Drawer, aw->gadgets[GID_DEST], (APTR)&aw->dest); /* Ensure we have a local dest */
		return aw;
	} else {
		FreeVec(aw);
		return NULL;
	}
}

void window_open(void *awin, struct MsgPort *appwin_mp)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->windows[WID_MAIN]) {
		WindowToFront(aw->windows[WID_MAIN]);
	} else {
		if(aw->h_mode) aw->menu[MENU_SETTINGS_HBROWSER].nm_Flags |= CHECKED;
	
		aw->windows[WID_MAIN] = (struct Window *)RA_OpenWindow(aw->objects[OID_MAIN]);
		
		if(aw->windows[WID_MAIN]) {
			aw->appwin = AddAppWindowA(0, (ULONG)aw, aw->windows[WID_MAIN], appwin_mp, NULL);
		
			aw->appwindzhook.h_Entry = appwindzhookfunc;
			aw->appwindzhook.h_SubEntry = NULL;
			aw->appwindzhook.h_Data = NULL;
			
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

			/* whole window */
			aw->appwindz[0] = AddAppWindowDropZone(aw->appwin, 0, (ULONG)aw,
											WBDZA_Left, 0,
											WBDZA_Top, 0, 
											WBDZA_Width, aw->windows[WID_MAIN]->Width,
											WBDZA_Height, aw->windows[WID_MAIN]->Height,						
										TAG_END);
		
			/* Refresh archive on window open */
			if(aw->archiver != ARC_NONE) window_req_open_archive(awin, get_config(), TRUE);
			window_menu_set_enable_state(aw);

			if(aw->iconified == FALSE) add_to_window_list(awin);
		}
		aw->iconified = FALSE;
	}
}

void window_close(void *awin, BOOL iconify)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->windows[WID_MAIN]) {
		if(aw->appwindz) {
			for(int i=0; i<AVALANCHE_DROPZONES; i++) {
				RemoveAppWindowDropZone(aw->appwin, aw->appwindz[i]);
				aw->appwindz[i] = NULL;
			}
		}
		RemoveAppWindow(aw->appwin);
		if(iconify) {
			RA_Iconify(aw->objects[OID_MAIN]);
		} else {
			RA_CloseWindow(aw->objects[OID_MAIN]);
		}
		aw->windows[WID_MAIN] = NULL;

		if(iconify) {
			aw->iconified = TRUE;
		} else {
			del_from_window_list(awin);
		}

		/* Release archive when window is closed */
		module_free(aw);
		window_free_archive_userdata(aw);
	}
}

void window_dispose(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	/* Ensure appwindow is removed */
	if(aw->windows[WID_MAIN]) window_close(aw, FALSE);
	
	/* Dispose window object */
	DisposeObject(aw->objects[OID_MAIN]);
	
	/* Free archive browser list and column info */
	FreeListBrowserList(&aw->lblist);
	if(aw->lbci) FreeLBColumnInfo(aw->lbci);
	if(aw->archive_needs_free) window_free_archive_path(aw);
	if(aw->menu) FreeVec(aw->menu);

	delete_delete_list(aw);

	FreeVec(aw);
}

void window_list_handle(void *awin, char *tmpdir)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	ULONG tmp = 0;
	struct Node *node = NULL;
	long ret = 0;
	
	GetAttr(LISTBROWSER_RelEvent, aw->gadgets[GID_LIST], (APTR)&tmp);
	switch(tmp) {
#if 0 /* This selects items when single-clicked off the checkbox -
it's incompatible with doube-clicking as it resets the listview */
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw, node, 2);
		break;
#endif
		case LBRE_DOUBLECLICK:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw, node, 1); /* ensure selected */
			char fn[1024];
			strncpy(fn, tmpdir, 1023);
			fn[1023] = 0;
			ret = extract(aw, aw->archive, fn, node);
			if(ret == 0) {
				AddPart(fn, get_item_filename(aw, node), 1024);
				add_to_delete_list(aw, fn);
				OpenWorkbenchObjectA(fn, NULL);
			} else {
				show_error(ret, aw);
			}
		break;
	}
}

void window_update_archive(void *awin, char *archive)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->archive_needs_free) window_free_archive_path(aw);
	aw->archive = strdup(archive);
	aw->archive_needs_free = TRUE;

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
					GETFILE_FullFile, aw->archive, TAG_DONE);
}

void window_update_sourcedir(void *awin, char *sourcedir)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
					GETFILE_Drawer, sourcedir, TAG_DONE);
}

void window_toggle_hbrowser(void *awin, BOOL h_browser)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Hierarchical, h_browser, TAG_DONE);
}

void window_fuelgauge_update(void *awin, ULONG size, ULONG total_size)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
					FUELGAUGE_Max, total_size,
					FUELGAUGE_Level, size,
					TAG_DONE);
}

void window_modify_all_list(void *awin, ULONG select)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	struct Node *node;
	BOOL selected;

	/* select: 0 = deselect all, 1 = select all, 2 = toggle all */
	if(select == 0) selected = FALSE;
	if(select == 1) selected = TRUE;

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	for(node = aw->lblist.lh_Head; node->ln_Succ; node=node->ln_Succ) {

		if(select == 2) {
			ULONG current;
			GetListBrowserNodeAttrs(node, LBNA_Checked, &current, TAG_DONE);
			selected = !current;
		}

		SetListBrowserNodeAttrs(node, LBNA_Checked, selected, TAG_DONE);
	}

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &aw->lblist,
			TAG_DONE);
}

char *window_req_dest(void *awin)
{	
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	DoMethod((Object *)aw->gadgets[GID_DEST], GFILE_REQUEST, aw->windows[WID_MAIN]);
	GetAttr(GETFILE_Drawer, aw->gadgets[GID_DEST], (APTR)&aw->dest);
	
	return aw->dest;
}

void window_req_open_archive(void *awin, struct avalanche_config *config, BOOL refresh_only)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

//	dir_seen = FALSE;
	long ret = 0;
	long retxfd = 0;

	if(refresh_only == FALSE) {
		ret = DoMethod((Object *) aw->gadgets[GID_ARCHIVE], GFILE_REQUEST, aw->windows[WID_MAIN]);
		if(ret == 0) return;
	}

	if(aw->archive_needs_free) window_free_archive_path(aw);
	GetAttr(GETFILE_FullFile, aw->gadgets[GID_ARCHIVE], (APTR)&aw->archive);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&aw->lblist);

	ret = xad_info(aw->archive, config, aw, addlbnode_cb);
	if(ret != 0) { /* if xad failed try xfd */
		retxfd = xfd_info(aw->archive, aw, addlbnodexfd_cb);
		if(retxfd != 0) show_error(ret, aw);
	}

	if(ret == 0) {
		aw->archiver = ARC_XAD;
	} else if(retxfd == 0) {
		aw->archiver = ARC_XFD;
	} else {
		aw->archiver = ARC_NONE;
	}

	module_register(aw, &aw->mf);
	window_menu_set_enable_state(aw);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &aw->lblist,
				LISTBROWSER_SortColumn, 0,
			TAG_DONE);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);
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

void *window_get_lbnode(void *awin, struct Node *node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	void *userdata = NULL;
	ULONG checked = FALSE;
	char msg[20];

	GetListBrowserNodeAttrs(node,
			LBNA_Checked, &checked,
			LBNA_UserData, &userdata,
		TAG_DONE);

	aw->current_item++;
	snprintf(msg, 20, "%lu/%lu", aw->current_item, aw->total_items);

	SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);

	if(checked == FALSE) return NULL;
	return userdata;
}

struct List *window_get_lblist(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return &aw->lblist;
}

ULONG window_get_archiver(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return aw->archiver;
}

ULONG window_handle_input(void *awin, UWORD *code)
{
	return RA_HandleInput(window_get_object(awin), code);
}

BOOL window_edit_add(void *awin, char *file)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	/* virus scan - disabled currently as don't want random files to be deleted */
	//module_vscan(awin, file, NULL, NULL);

	module_free(aw);
	if(aw->mf.add) return aw->mf.add(aw, aw->archive, file);
	return FALSE;
}

static void window_edit_add_req(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	BOOL ok = FALSE;
	char *file;

	if(aw->mf.add == NULL) return;
	
	struct FileRequester *aslreq = AllocAslRequest(ASL_FileRequest, NULL);
	if(aslreq) {
		if(AslRequest(aslreq, NULL)) {
			ULONG len = strlen(aslreq->fr_Drawer) + strlen(aslreq->fr_File) + 5;
			file = AllocVec(len, MEMF_PRIVATE);
			strcpy(file, aslreq->fr_Drawer);
			AddPart(file, aslreq->fr_File, len);
			ok = window_edit_add(aw, file);
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
	
	if(aw->mf.del == NULL) return;

	if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
										WA_BusyPointer, TRUE,
										WA_PointerDelay, TRUE,
										TAG_DONE);

	window_reset_count(awin);
	window_disable_gadgets(awin, TRUE);

	/* module_free(aw);
	 * TODO: copy the files to delete into a list so we can release the archive */

	/* Count selected nodes */
	ULONG entries = 0;
	for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
		void *userdata = window_get_lbnode(awin, node);
		if(userdata) {
			entries++;
		}
	}

	/* Create array of names */
	char **name_array = AllocVec(entries * sizeof(char *), MEMF_CLEAR | MEMF_PRIVATE);
	ULONG i = 0;

	if(name_array) {

		for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
			void *userdata = window_get_lbnode(awin, node);
			if(userdata) {
				name_array[i] = strdup(module_get_item_filename(awin, userdata));
				i++;
			}
		}

		module_free(aw);
		aw->mf.del(aw, aw->archive, name_array, entries);

		for(i = 0; i<entries; i++) {
			free(name_array[i]);
		}
		FreeVec(name_array);
	}

	window_disable_gadgets(awin, FALSE);
	if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);

	/* Refresh the archive */
	window_req_open_archive(aw, config, TRUE);

	return;
}

ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	ULONG done = WIN_DONE_OK;

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
			window_close(awin, FALSE);
			done = WIN_DONE_CLOSED;
		break;

		case WMHI_GADGETUP:
			switch (result & WMHI_GADGETMASK) {
				case GID_ARCHIVE:
					window_req_open_archive(awin, config, FALSE);
				break;
					
				case GID_DEST:
					window_req_dest(awin);
				break;

				case GID_EXTRACT:
					ret = extract(awin, aw->archive, aw->dest, NULL);
					if(ret != 0) show_error(ret, awin);
				break;

				case GID_LIST:
					window_list_handle(awin, config->tmpdir);
				break;
			}
			break;

		case WMHI_RAWKEY:
			switch(result & WMHI_GADGETMASK) {
				case RAWKEY_ESC:
					done = WIN_DONE_CLOSED;
				break;

				case RAWKEY_RETURN:
					ret = extract(awin, aw->archive, aw->dest, NULL);
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
				
		 case WMHI_MENUPICK:
			while((code != MENUNULL) && (done != WIN_DONE_CLOSED)) {
				if(aw->windows[WID_MAIN] == NULL) continue;
				struct MenuItem *item = ItemAddress(aw->windows[WID_MAIN]->MenuStrip, code);
				switch(MENUNUM(code)) {
					case 0: //project
						switch(ITEMNUM(code)) {
							case 0: //open
								window_req_open_archive(awin, config, FALSE);
							break;
							
							case 2: //info
								req_show_arc_info(awin);
							break;

							case 3: //about
								show_about(awin);
							break;
							
							case 5: //quit
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
						}
					break;
					
					case 2: //settings
						switch(ITEMNUM(code)) {
							case 0: //browser mode
								aw->h_mode = !aw->h_mode;
									
								window_toggle_hbrowser(awin, aw->h_mode);
								window_req_open_archive(awin, config, TRUE);
							break;
								
							case 1: //snapshot
								/* fetch current win posn */
								GetAttr(WA_Top, aw->objects[OID_MAIN], (APTR)&config->win_x);
								GetAttr(WA_Left, aw->objects[OID_MAIN], (APTR)&config->win_y);
								GetAttr(WA_Width, aw->objects[OID_MAIN], (APTR)&config->win_w);
								GetAttr(WA_Height, aw->objects[OID_MAIN], (APTR)&config->win_h);
							break;
								
							case 3: //prefs
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

/* Check if abort button is pressed - only called from xad hook */
BOOL check_abort(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	ULONG result;
	UWORD code;

	while((result = RA_HandleInput(aw->objects[OID_MAIN], &code)) != WMHI_LASTMSG ) {
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

void window_reset_count(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	aw->current_item = 0;
}

void window_disable_gadgets(void *awin, BOOL disable)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(disable) {
		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Text,  locale_get_string( MSG_STOP ) ,
			TAG_DONE);
	} else {
		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Text, GID_EXTRACT_TEXT,
			TAG_DONE);
	}

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_DEST], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
}

void fill_menu_labels(void)
{
	menu[0].nm_Label = locale_get_string( MSG_PROJECT );
	menu[1].nm_Label = locale_get_string( MSG_OPEN );
	menu[3].nm_Label = locale_get_string( MSG_ARCHIVEINFO );
	menu[4].nm_Label = locale_get_string( MSG_ABOUT );
	menu[6].nm_Label = locale_get_string( MSG_QUIT );
	menu[7].nm_Label = locale_get_string( MSG_EDIT );
	menu[8].nm_Label = locale_get_string( MSG_SELECTALL );
	menu[9].nm_Label = locale_get_string( MSG_CLEARSELECTION );
	menu[10].nm_Label = locale_get_string( MSG_INVERTSELECTION );
	menu[12].nm_Label = locale_get_string( MSG_ADDFILES );
	menu[13].nm_Label = locale_get_string( MSG_DELFILES );
	menu[14].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[MENU_SETTINGS_HBROWSER].nm_Label = locale_get_string( MSG_HIERARCHICALBROWSEREXPERIMENTAL );
	menu[16].nm_Label = locale_get_string( MSG_SNAPSHOT );
	menu[18].nm_Label = locale_get_string( MSG_PREFERENCES );
}

void *window_get_archive_userdata(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	return aw->archive_userdata;
}

void window_set_archive_userdata(void *awin, void *userdata)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	aw->archive_userdata = userdata;
}

void *window_alloc_archive_userdata(void *awin, ULONG size)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	aw->archive_userdata = AllocVec(size, MEMF_CLEAR | MEMF_PRIVATE);
	return aw->archive_userdata;
}

void window_free_archive_userdata(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->archive_userdata) {
		FreeVec(aw->archive_userdata);
		aw->archive_userdata = NULL;
	}
}
