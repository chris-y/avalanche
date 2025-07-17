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

#include <intuition/pointerclass.h>

#include <libraries/asl.h>
#include <libraries/gadtools.h>

#include <proto/bitmap.h>
#include <proto/button.h>
#include <proto/drawlist.h>
#include <proto/getfile.h>
#include <proto/glyph.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/getfile.h>
#include <gadgets/listbrowser.h>
#include <images/bitmap.h>
#include <images/drawlist.h>
#include <images/glyph.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "http.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
#include "module.h"
#include "new.h"
#include "req.h"
#include "win.h"

#include "deark.h"
#include "xad.h"
#include "xfd.h"

#include "Avalanche_rev.h"

enum {
	GID_MAIN = 0,
	GID_ARCHIVE,
	GID_DEST,
	GID_OPENWB,
	GID_BROWSERLAYOUT,
	GID_TREELAYOUT,
	GID_TREE,
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
	BOOL selected;
	BOOL dir;
	void *userdata;
	ULONG level;
};

#define AVALANCHE_DROPZONES 2
#define TITLE_MAX_SIZE 100

struct avalanche_window {
	struct MinNode node;
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];
	struct NewMenu *menu;
	ULONG archiver;
	struct ColumnInfo *lbci;
	struct List lblist;
	struct ColumnInfo *dtci;
	struct List dir_tree;
	struct Hook aslfilterhook;
	struct Hook appwindzhook;
	struct Hook lbsorthook;
	struct MsgPort *appwin_mp;
	struct AppWindow *appwin;
	struct AppWindowDropZone *appwindz[AVALANCHE_DROPZONES];
	char *archive;
	char *dest;
	struct MinList deletelist;
	struct arc_entries **arc_array;
	struct arc_entries **dir_array;
	ULONG dir_tree_size;
	ULONG current_item;
	ULONG total_items;
	BOOL archive_needs_free;
	void *archive_userdata;
	BOOL flat_mode;
	BOOL drag_lock;
	BOOL iconified;
	BOOL disabled;
	BOOL abort_requested;
	BYTE process_exit_sig;
	struct module_functions mf;
	char *current_dir;
	struct Node *root_node;
	char title[TITLE_MAX_SIZE];
};

static struct List winlist;

#ifndef __amigaos4__
#define fr_NumArgs rf_NumArgs
#define fr_ArgList rf_ArgList
#endif

/** Extract process **/

static struct Task* avalanche_process = NULL;

struct avalanche_extract_userdata {
	void *awin;
	char *archive;
	char *newdest;
	struct Node *node;
};

/** Glyphs **/
#define AVALANCHE_GLYPH_ROOT 800
#define AVALANCHE_GLYPH_OPENDRAWER 801

static struct DrawList dl_opendrawer[] = {
	{DLST_LINE, 90, 50, 90, 80, 1},
	{DLST_LINE, 90, 80, 10, 80, 1},
	{DLST_LINE, 10, 80, 10, 50, 1},
	{DLST_LINE, 10, 50, 90, 50, 1},
	{DLST_LINE, 90, 50, 70, 20, 1},
	{DLST_LINE, 70, 20, 30, 20, 1},
	{DLST_LINE, 30, 20, 10, 50, 1},
	{DLST_LINE, 30, 20, 20, 20, 1},
	{DLST_LINE, 20, 20, 20, 40, 1},

	{DLST_LINE, 70, 20, 80, 20, 1},
	{DLST_LINE, 80, 20, 80, 40, 1},

	{DLST_LINE, 40, 65, 60, 65, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_archiveroot[] = {
	{DLST_LINE, 10, 90, 60, 90, 1},
	{DLST_LINE, 60, 90, 60, 40, 1},
	{DLST_LINE, 60, 40, 10, 40, 1},
	{DLST_LINE, 10, 40, 10, 90, 1},

	{DLST_LINE, 60, 90, 90, 70, 1},
	{DLST_LINE, 60, 40, 90, 20, 1},
	{DLST_LINE, 10, 40, 40, 20, 1},

	{DLST_LINE, 90, 70, 90, 20, 1},
	{DLST_LINE, 90, 20, 40, 20, 1},

	{DLST_LINE, 35, 40, 65, 20, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};


/** Menu **/

static struct NewMenu menu[] = {
	{NM_TITLE,  NULL,           0,  0, 0, 0,}, // 0 Project
	{NM_ITEM,   NULL,         "N", 0, 0, 0,}, // 0 New archive
	{NM_ITEM,   NULL,         "+", 0, 0, 0,}, // 1 New window
	{NM_ITEM,   NULL,         "O", 0, 0, 0,}, // 2 Open
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 3
	{NM_ITEM,   NULL , "!", NM_ITEMDISABLED, 0, 0,}, // 4 Archive Info
	{NM_ITEM,   NULL ,        "?", 0, 0, 0,}, // 5 About
	{NM_ITEM,   NULL ,        0, 0, 0, 0,}, // 6 Check version

	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 7
	{NM_ITEM,   NULL,         "Q", 0, 0, 0,}, // 8 Quit

	{NM_TITLE,  NULL,               0,  0, 0, 0,}, // 1 Edit
	{NM_ITEM,   NULL,       "A", NM_ITEMDISABLED, 0, 0,}, // 0 Select All
	{NM_ITEM,   NULL ,  "Z", NM_ITEMDISABLED, 0, 0,}, // 1 Clear Selection
	{NM_ITEM,   NULL , "I", NM_ITEMDISABLED, 0, 0,}, // 2 Invert
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 3
	{NM_ITEM,   NULL,       "+", NM_ITEMDISABLED, 0, 0,}, // 4 Add files
	{NM_ITEM,   NULL,       0, NM_ITEMDISABLED, 0, 0,}, // 5 Delete files
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 6
	{NM_ITEM,   NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 7 Toggle drag lock

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Settings
	{NM_ITEM,	NULL , 0, 0, 0, 0,}, // 0 View mode
	{NM_SUB,	NULL , 0, CHECKIT, ~1, 0,}, // 0 Browser
	{NM_SUB,	NULL , 0, CHECKIT, ~2, 0,}, // 1 List
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 1
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 2 Snapshot
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 3 Preferences

	{NM_END,   NULL,        0,  0, 0, 0,},
};

#define MENU_DRAGLOCK 18
#define MENU_FLATMODE 21
#define MENU_LISTMODE 22

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

	/* Only change if we are able to add files */
	if(aw->mf.add == NULL) return 0;

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

static Object *get_glyph(ULONG glyph)
{
	Object *glyphobj = NULL;
	char *img = NULL;
	struct Screen *screen = LockPubScreen(NULL);

	if(get_config()->aiss) {
		switch(glyph) {
			case GLYPH_POPDRAWER:
				img = "TBimages:list_drawer";
			break;

			case AVALANCHE_GLYPH_OPENDRAWER:
				img = "TBimages:draweropen";
			break;

			case GLYPH_POPFILE:
				img = "TBimages:list_file";
			break;

			case AVALANCHE_GLYPH_ROOT:
				img = "TBimages:list_archive";
			break;

			case GLYPH_UPARROW:
				img = "TBimages:list_nav_north";
			break;

			case GLYPH_RIGHTARROW:
				img = "TBimages:autobutton_rtarrow";
			break;

			case GLYPH_DOWNARROW:
				img = "TBimages:autobutton_dnarrow";
			break;

			default:
				img = "TBimages:list_blank";
			break;
		}

		glyphobj = BitMapObj,
					BITMAP_SourceFile, img,
					BITMAP_Masking, TRUE,
					BITMAP_Screen, screen,
					/* BITMAP_Height, 16,
					BITMAP_Width, 16, */
				BitMapEnd;
	} else {
		if((glyph == AVALANCHE_GLYPH_OPENDRAWER) ||
			(glyph == AVALANCHE_GLYPH_ROOT)) {
				
			struct DrawList *dl = NULL;
				
			if(glyph == AVALANCHE_GLYPH_OPENDRAWER) {
				dl = &dl_opendrawer;
			} else {
				dl = &dl_archiveroot;
			}
				
			glyphobj = DrawListObj,
					DRAWLIST_Directives, dl,
					DRAWLIST_RefHeight, 100,
					DRAWLIST_RefWidth, 100,
					IA_Width, 16,
					IA_Height, 16,
				End;
		} else {
			glyphobj = GlyphObj,
						GLYPH_Glyph, glyph,
					GlyphEnd;
		}
	}

	UnlockPubScreen(NULL, screen);

	return glyphobj;
}

static void window_free_archive_path(struct avalanche_window *aw)
{
	/* NB: uses free() not FreeVec() */
	if(aw->archive) free(aw->archive);
	aw->archive = NULL;
	aw->archive_needs_free = FALSE;
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
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,8,0)); //quit
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,0,0)); //viewmode1
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,0,1)); //viewmode2

		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,4,0)); //arc info
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0)); //edit/select all
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0)); //edit/clear
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0)); //edit/invert
		if(aw->mf.add) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
		}
		if(aw->mf.del) {
			OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
		}
	} else {
		if(busy == FALSE) {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,4,0)); //arc info
		} else {
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,8,0)); //draglock
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,2,0)); //open arc
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,0,0)); //new arc
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,8,0)); //quit
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,0,0)); //viewmode1
			OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(2,0,1)); //viewmode2
		}
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0)); //edit/select all
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0)); //edit/clear
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0)); //edit/invert
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,4,0)); //edit/add
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,5,0)); //edit/del
	}
}

static void window_menu_set_enable_state(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->archiver == ARC_NONE) {
		window_menu_activation(aw, FALSE, FALSE);
	} else {
		window_menu_activation(aw, TRUE, FALSE);
	}
}

static BOOL window_open_dest(void *awin)
{	
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	return OpenWorkbenchObjectA(aw->dest, NULL);
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
			LISTBROWSER_Labels, &aw->lblist,
			TAG_DONE);
	}
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

static void free_arc_array(struct avalanche_window *aw)
{
	if((aw->arc_array == NULL) || (aw->total_items == 0)) return;
	
	for(int i = 0; i < aw->total_items; i++) {
		FreeVec(aw->arc_array[i]);
	}
	FreeVec(aw->arc_array);
	aw->arc_array = NULL;
	
	if((aw->dir_array == NULL) || (aw->dir_tree_size == 0)) return;

	for(int i = 0; i < aw->dir_tree_size; i++) {
		if(aw->dir_array[i]) {
			if(aw->dir_array[i]->name) FreeVec(aw->dir_array[i]->name);
			FreeVec(aw->dir_array[i]);
		}
	}
	FreeVec(aw->dir_array);
	aw->dir_array = NULL;
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


static long extract_internal(void *awin, char *archive, char *newdest, struct Node *node)
{
	long ret = 0;
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	struct avalanche_config *config = get_config();

	if(archive && newdest) {
		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
										WA_PointerType, POINTERTYPE_PROGRESS,
										TAG_DONE);
		window_reset_count(awin);
		window_disable_gadgets(awin, TRUE);

		if((node == NULL) && (aw->flat_mode) && (aw->archiver != ARC_XFD)) {
			ret = module_extract_array(awin, aw->arc_array, aw->total_items, newdest);
		} else {
			ret = module_extract(awin, node, archive, newdest);
		}

		if((ret == 0) && (config->openwb == TRUE)) window_open_dest(awin);

		window_disable_gadgets(awin, FALSE);

		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);
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
	Signal(avalanche_process, SIGBREAKF_CTRL_F);
	
	/* Wait for UserData */
	Wait(SIGBREAKF_CTRL_E);
	
	/* Find our task */
	struct Task *extract_task = FindTask(NULL);
	struct avalanche_extract_userdata *aeu = (struct avalanche_extract_userdata *)extract_task->tc_UserData;
	struct avalanche_window *aw = (struct avalanche_window *)aeu->awin;
	
	/* Call Extract on our new process */
	extract_internal(aeu->awin, aeu->archive, aeu->newdest, aeu->node);
	
	/* Free UserData */
	FreeVec(aeu);
	
	/* Signal that we've finished */
	Signal(avalanche_process, aw->process_exit_sig);
	
	FreeSignal(aw->process_exit_sig);
	aw->process_exit_sig = 0;
}


long extract(void *awin, char *archive, char *newdest, struct Node *node)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	struct avalanche_extract_userdata *aeu = AllocVec(sizeof(struct avalanche_extract_userdata), MEMF_CLEAR);
	
	if(aeu == NULL) return -2;
	
	aeu->awin = awin;
	aeu->archive = archive;
	aeu->newdest = newdest;
	aeu->node = node;
	
	if((aw->process_exit_sig = AllocSignal(-1)) == -1) {
		FreeVec(aeu);
		return -2;
	}
	
	/* Ensure there are no pending signals for this already */
	SetSignal(0L, aw->process_exit_sig);
	
	avalanche_process = FindTask(NULL);
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

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, BOOL selected, struct avalanche_window *aw)
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
	BOOL debug = get_config()->debug;

	char datestr[20];
	struct ClockData cd;

	if(aw->archiver != ARC_XFD) {
		if(userdata && aw->flat_mode) {
			struct arc_entries *ud = (struct arc_entries *)userdata;
			if(aw->archiver == ARC_XAD) xad_get_filedate(ud->userdata, &cd, aw);
		} else {
			if(aw->archiver == ARC_XAD) xad_get_filedate(userdata, &cd, aw);
		}
	}

	if(CheckDate(&cd) == 0)
		Amiga2Date(0, &cd);

	if(dir) {
		glyph = get_glyph(GLYPH_POPDRAWER);
		tag1 = LBNCA_CopyText;
		val1 = TRUE;
		tag2 = LBNCA_Text;
		val2 = (ULONG)locale_get_string(MSG_DIR);

		snprintf(datestr, 20, "\0");
	} else {
		glyph = get_glyph(GLYPH_POPFILE);
		val1 = (ULONG)size;
		snprintf(datestr, 20, "%04u-%02u-%02u %02u:%02u:%02u", cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);
	}

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
			LBNCA_Text, name,
		LBNA_Column, 2,
			tag1, val1,
			tag2, val2,
		LBNA_Column, 3,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, datestr,
		TAG_DONE);

	AddTail(&aw->lblist, node);
}

static ULONG count_dir_level(char *filename)
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


static BOOL check_if_subdir(struct avalanche_window *aw, int dir_entry, const char *dir_name)
{
	char *dir_name_slash = AllocVec(strlen(dir_name) + 2, MEMF_PRIVATE);
	if(dir_name_slash == NULL) return FALSE;
	sprintf(dir_name_slash, "%s/", dir_name);

	for(int j = 0; j < dir_entry; j++) {
		if((aw->dir_array[j]) && (strlen(aw->dir_array[j]->name) > strlen(dir_name_slash)) && (strncmp(aw->dir_array[j]->name, dir_name_slash, strlen(dir_name_slash)) == 0)) {
			FreeVec(dir_name_slash);
			return TRUE;
		}
	}
	FreeVec(dir_name_slash);
	return FALSE;
}

static BOOL check_duplicates(struct avalanche_window *aw, int dir_entry, const char *dir_name)
{
	for(int j = 0; j < dir_entry; j++) {
		if((aw->dir_array[j]) && (strlen(dir_name) > 0) && (strcmp(aw->dir_array[j]->name, dir_name) == 0)) {
			return TRUE;
		}
	}
	return FALSE;
}

static void highlight_current_dir(struct avalanche_window *aw)
{
	struct Node *cur_node = NULL;

	if(aw->current_dir == NULL) {
		cur_node = aw->root_node;
	} else {
		struct List *list = &aw->dir_tree;
		struct Node *node;
		char *userdata = NULL;
		char *cur_dir = strdup(aw->current_dir);

		if(cur_dir) cur_dir[strlen(cur_dir) - 1] = '\0';

		for(node = list->lh_Head; node->ln_Succ; node=node->ln_Succ) {
			GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

			if((userdata) && (strcmp(cur_dir, userdata) == 0)) {
				cur_node = node;
				break;
			}
		}
		if(cur_dir) free(cur_dir);
	}

	if(cur_node) {
		SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_SelectedNode, cur_node,
			TAG_DONE);
	}
}

static void window_update_title(struct avalanche_window *aw)
{
	if(aw->archive != NULL) {
		if(aw->current_dir) {
			int title_len = strlen(VERS) + strlen(FilePart(aw->archive)) + strlen(aw->current_dir);

			if((title_len < TITLE_MAX_SIZE) || ((title_len - TITLE_MAX_SIZE + 3) > strlen(aw->current_dir))) {
				snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s] - %s", VERS, FilePart(aw->archive), aw->current_dir);
			} else {
				snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s] - ...%s", VERS, FilePart(aw->archive), aw->current_dir + (strlen(aw->current_dir) - (title_len - TITLE_MAX_SIZE + 3)));		
			}
		} else {
			snprintf(aw->title, TITLE_MAX_SIZE, "%s [%s]", VERS, FilePart(aw->archive));
		}
		SetWindowTitles(window_get_window(aw), (UBYTE *) ~0, aw->title);
	} else {
		SetWindowTitles(window_get_window(aw), (UBYTE *) ~0, VERS);
	}
}

static void window_flat_browser_tree_construct(struct avalanche_window *aw)
{
	FreeListBrowserList(&aw->dir_tree);
	aw->root_node = NULL;

	int dir_entry = 0;
	aw->dir_tree_size = aw->total_items * 2;
	aw->dir_array = AllocVec(sizeof(struct arc_entries *) * aw->dir_tree_size, MEMF_CLEAR);
	if(aw->dir_array == NULL) return;

	for(int it = 0; it < aw->total_items; it++) {
		char *dir_name = AllocVec(strlen(aw->arc_array[it]->name)+1, MEMF_CLEAR);
		int i = 0;
		int slash = 0;
		int last_slash = 0;

		BOOL dupe = FALSE;

		if(dir_name == NULL) continue;

		while(aw->arc_array[it]->name[i+1]) {
			if(aw->arc_array[it]->name[i] == '/') {
				slash++;
				last_slash = i;
			}
			dir_name[i] = aw->arc_array[it]->name[i];
			i++;
		}
		dir_name[last_slash] = '\0';

		dupe = check_duplicates(aw, dir_entry, dir_name);

		if((slash > 0) && (dupe == FALSE)) {
			for(int l = 1; l < slash; l++) {
				char *part_dir = extract_path_part(dir_name, l);
				dupe = check_duplicates(aw, dir_entry, part_dir);
				if(dupe) {
					FreeVec(part_dir);
					continue;
				}
				aw->dir_array[dir_entry] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
				if(aw->dir_array[dir_entry] == NULL) continue;
				aw->dir_array[dir_entry]->name = part_dir;
				aw->dir_array[dir_entry]->level = l;

				dir_entry++;
			}

			aw->dir_array[dir_entry] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			if(aw->dir_array[dir_entry] == NULL) continue;
			
			aw->dir_array[dir_entry]->name = dir_name;
			aw->dir_array[dir_entry]->level = slash;
			aw->dir_array[dir_entry]->dir = FALSE;

			dir_entry++;

			if(dir_entry > aw->dir_tree_size) {
				open_error_req(locale_get_string(MSG_ERR_TREE_ALLOC), locale_get_string(MSG_OK), aw);
				return;
			}

		} else {
			FreeVec(dir_name);
		}
	}

	for(int i = 0; i < dir_entry; i++) {
		if((check_if_subdir(aw, dir_entry, aw->dir_array[i]->name))) {
			aw->dir_array[i]->dir = TRUE;  //LBFLG_HASCHILDREN
		} else {
			aw->dir_array[i]->dir = FALSE;
		}
	}

	aw->dir_tree_size = dir_entry;

	ULONG flags = LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN;
	if(dir_entry == 0) flags = 0;

	aw->root_node = AllocListBrowserNode(1,
									LBNA_UserData, NULL,
									LBNA_Flags, flags,
									LBNA_Generation, 1,
									LBNA_Column, 0,
										LBNCA_Image, LabelObj,
											LABEL_DisposeImage, TRUE,
											LABEL_Image, get_glyph(AVALANCHE_GLYPH_ROOT),
											LABEL_Underscore, NULL,
											LABEL_Text, " ",
											LABEL_Text, FilePart(aw->archive),
										LabelEnd,
									TAG_DONE);

	AddTail(&aw->dir_tree, aw->root_node);

	for(int i = 0; i <= dir_entry; i++) {
		flags = LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN;
		if(aw->dir_array[i] == NULL) break;
		if(aw->dir_array[i]->dir == FALSE) flags = 0;
		
		struct Node *node = AllocListBrowserNode(1,
									LBNA_UserData, aw->dir_array[i]->name,
									LBNA_Flags, flags,
									LBNA_Generation, aw->dir_array[i]->level + 1,
									LBNA_Column, 0,
										LBNCA_Image, LabelObj,
											LABEL_DisposeImage, TRUE,
											LABEL_Image, get_glyph(GLYPH_POPDRAWER),
											LABEL_Underscore, NULL,
											LABEL_Text, " ",
											LABEL_Text, FilePart(aw->dir_array[i]->name),
										LabelEnd,
									TAG_DONE);

		AddTail(&aw->dir_tree, node);
	}

	window_update_title(aw);
}

static void window_flat_browser_construct(struct avalanche_window *aw)
{
	ULONG level = 0;
	ULONG skip_dir_len = 0;

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);

	FreeListBrowserList(&aw->lblist);

	if(aw->current_dir) {
		level = count_dir_level(aw->current_dir) + 1;
		skip_dir_len = strlen(aw->current_dir);
	}

	if(aw->archiver != ARC_XFD) {
		/* Add fake dir entries (first, so by default they're at the top) */
		
		if(level > 0) {
			/* Add "parent" entry */
			struct Node *node = AllocListBrowserNode(4,
									LBNA_UserData, NULL,
									LBNA_Generation, 1,
									LBNA_CheckBox, FALSE,
									LBNA_Column, 0,
										LBNCA_Image, get_glyph(GLYPH_UPARROW),
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

			AddTail(&aw->lblist, node);
		}
		
		for(int it = 0; it < aw->dir_tree_size; it++) {
			/* Only show current level - NB dir levels are different from file levels */
			if((aw->dir_array[it] && ((aw->dir_array[it]->level - 1) == level)) &&
				((aw->current_dir == NULL) || (strncmp(aw->dir_array[it]->name, aw->current_dir, skip_dir_len) == 0))) {
				addlbnode(aw->dir_array[it]->name + skip_dir_len, &zero, TRUE, NULL, FALSE, aw);
			}
		}
	}

	/* Add files */

	for(int it = 0; it < aw->total_items; it++) {
		/* Only show current level */
		if((aw->arc_array[it]->level == level) && (aw->arc_array[it]->dir == FALSE) &&
			((aw->current_dir == NULL) || (strncmp(aw->arc_array[it]->name, aw->current_dir, strlen(aw->current_dir)) == 0))) {
			addlbnode(aw->arc_array[it]->name + skip_dir_len,
				aw->arc_array[it]->size, aw->arc_array[it]->dir, aw->arc_array[it], aw->arc_array[it]->selected, aw);
		}
	}

	window_update_title(aw);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);

}

static void addlbnode_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(item == 0) {
		aw->current_item = 0;
		if(aw->windows[WID_MAIN] && aw->gadgets[GID_PROGRESS]) {
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

	if(aw->flat_mode) {
		if(item == 0) {
			if(aw->arc_array) free_arc_array(aw);
			aw->arc_array = AllocVec(total * sizeof(struct arc_entries *), MEMF_CLEAR);
		}

		if(aw->arc_array) {
			int i = 0;
			
			aw->arc_array[item] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			
			aw->arc_array[item]->name = name;
			aw->arc_array[item]->size = size;
			aw->arc_array[item]->dir = dir;
			aw->arc_array[item]->userdata = userdata;
			aw->arc_array[item]->selected = TRUE;
			
			aw->arc_array[item]->level = 0;

			/* Count the slashes to find directory level */
			aw->arc_array[item]->level = count_dir_level(name);
			
			if(item == (total - 1)) {
				/* Sort the array */
				#ifdef __amigaos4__ /* doesn't like OS3, may not be needed anyway */
				qsort(aw->arc_array, total, sizeof(struct arc_entries *), sort_array);
				#endif
				window_flat_browser_tree_construct(aw);
				window_flat_browser_construct(aw);
			}
		}
	} else {
		addlbnode(name, size, dir, userdata, TRUE, aw);
	}
}

static void update_fuelgauge_text(struct avalanche_window *aw)
{
	char msg[20];

	aw->current_item++;

	if(aw->windows[WID_MAIN] == NULL) return;

	snprintf(msg, 20, "%lu/%lu", aw->current_item, aw->total_items);

	SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);
}

void *array_get_userdata(void *awin, void *arc_entry)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	update_fuelgauge_text(aw);

	if(aw->flat_mode) {
		struct arc_entries *arc_e = (struct arc_entries *)arc_entry;
		if(arc_e->selected) return arc_e->userdata;
	}

	return NULL;
}

static const char *get_item_filename(void *awin, struct Node *node)
{
	void *userdata = NULL;
	const char *fn = NULL;
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

	if(aw->flat_mode) {
		struct arc_entries *arc_entry = (struct arc_entries *)userdata;
		return module_get_item_filename(awin, arc_entry->userdata);
	} else {
		return module_get_item_filename(awin, userdata);
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
					GA_RelVerify, TRUE,
					GA_Disabled, !aw->flat_mode,
					LISTBROWSER_ColumnInfo, aw->dtci,
					LISTBROWSER_Labels, &(aw->dir_tree),
					LISTBROWSER_ColumnTitles, FALSE,
					LISTBROWSER_FastRender, TRUE,
					LISTBROWSER_Hierarchical, TRUE,
					LISTBROWSER_ShowSelected, TRUE,
					LISTBROWSER_ShowImage, get_glyph(GLYPH_RIGHTARROW),
					LISTBROWSER_HideImage, get_glyph(GLYPH_DOWNARROW),
					LISTBROWSER_LeafImage, NULL,
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


/* Window functions */

void *window_create(struct avalanche_config *config, char *archive, struct MsgPort *winport, struct MsgPort *appport)
{
	struct avalanche_window *aw = AllocVec(sizeof(struct avalanche_window), MEMF_CLEAR | MEMF_PRIVATE);
	if(!aw) return NULL;
	
	ULONG getfile_drawer = GETFILE_Drawer;
	struct Hook *asl_hook = (struct Hook *)&(aw->aslfilterhook);
	
	struct Hook *lbsort_hook = (struct Hook *)&(aw->lbsorthook);

	if(config->disable_asl_hook) {
		asl_hook = NULL;
	}

	ULONG tag_default_position = WINDOW_Position;

	/* Listbrowser */
	aw->lbsorthook.h_Entry = lbsortfunc;
	aw->lbsorthook.h_SubEntry = NULL;
	aw->lbsorthook.h_Data = NULL;

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

	NewList(&aw->lblist);

	aw->dtci = AllocLBColumnInfo(1, 
		LBCIA_Column, 0,
			LBCIA_Title, "",
			LBCIA_Weight, 100,
			LBCIA_DraggableSeparator, FALSE,
			LBCIA_Sortable, FALSE,
		TAG_DONE);

	NewList(&aw->dir_tree);

	if(aw->menu = AllocVec(sizeof(menu), MEMF_PRIVATE))
		CopyMem(&menu, aw->menu, sizeof(menu));

	if(config->win_x && config->win_y) tag_default_position = TAG_IGNORE;
	
	/* Copy global to local config */
	if(config->viewmode == 0) aw->flat_mode = TRUE;
	if(config->drag_lock) aw->drag_lock = TRUE;
		else aw->drag_lock = FALSE;

	/* ASL hook */
	aw->aslfilterhook.h_Entry = aslfilterfunc;
	aw->aslfilterhook.h_SubEntry = NULL;
	aw->aslfilterhook.h_Data = NULL;
	
	if(archive) {
		aw->archive = strdup(archive);
		aw->archive_needs_free = TRUE;
		getfile_drawer = TAG_IGNORE;
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
		WA_IDCMP, IDCMP_MENUPICK | IDCMP_RAWKEY | IDCMP_GADGETUP | IDCMP_NEWSIZE,
		WINDOW_NewMenu, aw->menu,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_IconTitle, "Avalanche",
		WINDOW_SharedPort, winport,
		WINDOW_AppPort, appport,
#ifdef __amigaos4__ /* Enable HintInfo */
		WINDOW_GadgetHelp, TRUE,
#endif
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
						getfile_drawer, config->sourcedir,
						GETFILE_FilterFunc, asl_hook,
					End,
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text,  locale_get_string( MSG_ARCHIVE ) ,
					LabelEnd,
					LAYOUT_AddChild, LayoutHObj,
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
						LAYOUT_AddChild,  aw->gadgets[GID_OPENWB] = ButtonObj,
							GA_ID, GID_OPENWB,
							GA_RelVerify, TRUE,
							GA_Image, get_glyph(AVALANCHE_GLYPH_OPENDRAWER),
#ifdef __amigaos4__ /* HintInfo hasn't made it to OS3.2 yet */
							GA_HintInfo, locale_get_string(MSG_OPENINWB),
#endif
						ButtonEnd,
						CHILD_NominalSize, TRUE,
						CHILD_WeightedWidth, 0,
					LayoutEnd,
				LayoutEnd,
				LAYOUT_WeightBar, TRUE,
				LAYOUT_AddChild,  aw->gadgets[GID_PROGRESS] = FuelGaugeObj,
					GA_ID, GID_PROGRESS,
				FuelGaugeEnd,
				CHILD_WeightedWidth, config->progress_size,
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
						GA_RelVerify, TRUE,
						LISTBROWSER_ColumnInfo, aw->lbci,
						LISTBROWSER_Labels, &(aw->lblist),
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_TitleClickable, TRUE,
						LISTBROWSER_SortColumn, 1,
						LISTBROWSER_Striping, LBS_ROWS,
						LISTBROWSER_FastRender, TRUE,
					ListBrowserEnd,
					CHILD_WeightedWidth, 80,
				LayoutEnd,
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
		
		/* Add to our window list */
		add_to_window_list(aw);
		return aw;
	} else {
		FreeVec(aw);
		return NULL;
	}
}

static void window_remove_dropzones(struct avalanche_window *aw)
{
	if(aw->appwindz) {
		for(int i=0; i<AVALANCHE_DROPZONES; i++) {
			RemoveAppWindowDropZone(aw->appwin, aw->appwindz[i]);
			aw->appwindz[i] = NULL;
		}
	}
}

static void window_add_dropzones(struct avalanche_window *aw)
{
	if(aw->appwin) {
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
			aw->appwin = AddAppWindowA(0, (ULONG)aw, aw->windows[WID_MAIN], appwin_mp, NULL);
			if(aw->drag_lock == FALSE) window_add_dropzones(aw);
		
			/* Refresh archive on window open */
			if(aw->archiver != ARC_NONE) window_req_open_archive(awin, get_config(), TRUE);
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

	if((aw->disabled == TRUE) && (iconify == FALSE)) {
		aw->abort_requested = TRUE;
		Wait(aw->process_exit_sig);
	}

	if(aw->windows[WID_MAIN]) {
		window_remove_dropzones(aw);
		if(aw->appwin) RemoveAppWindow(aw->appwin);
		aw->appwin = NULL;
		if(iconify) {
			RA_Iconify(aw->objects[OID_MAIN]);
		} else {
			RA_CloseWindow(aw->objects[OID_MAIN]);
		}
		aw->windows[WID_MAIN] = NULL;

		if(iconify) {
			aw->iconified = TRUE;
		}

		/* Release archive when window is closed */
		module_free(aw);
		window_free_archive_userdata(aw);
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
	
	/* Free archive browser list and column info */
	FreeListBrowserList(&aw->lblist);
	if(aw->lbci) FreeLBColumnInfo(aw->lbci);
	FreeListBrowserList(&aw->dir_tree);
	if(aw->dtci) FreeLBColumnInfo(aw->dtci);
	if(aw->archive_needs_free) window_free_archive_path(aw);
	if(aw->menu) FreeVec(aw->menu);
	free_arc_array(aw);

	delete_delete_list(aw);

	FreeVec(aw);
}

static void parent_dir(struct avalanche_window *aw)
{
	if(aw->current_dir) {
		aw->current_dir[strlen(aw->current_dir) - 1] = '\0';

		char *slash = strrchr(aw->current_dir, '/');
	
		if(slash == NULL) {
			FreeVec(aw->current_dir);
			aw->current_dir = NULL;
		} else {
			*(slash+1) = '\0';
		}

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

		window_flat_browser_construct(aw);

		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, &aw->lblist,
		TAG_DONE);

		highlight_current_dir(aw);
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
				}

				if(aw->current_dir) {
					FreeVec(aw->current_dir);
					aw->current_dir = NULL;
				}

				if(userdata) {
					strncpy(cdir, userdata, strlen(userdata));
					strcat(cdir, "/"); // add trailing slash
					aw->current_dir = cdir;
				}

				/* switch to dir */
				SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, ~0, TAG_DONE);

				window_flat_browser_construct(aw);

				SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, &aw->lblist,
				TAG_DONE);
			}
		break;
	}
}
					

static void window_list_handle(void *awin, char *tmpdir)
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
		break;

#if 0 /* This selects items when single-clicked off the checkbox -
it's incompatible with double-clicking as it resets the listview */
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw, node, 2, TRUE);
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
					if(strcmp(dir, "/") == 0) return parent_dir(aw);

					ULONG cdir_len = 0;
					if(aw->current_dir) cdir_len = strlen(aw->current_dir);
					
					char *cdir = AllocVec(cdir_len + 1 + strlen(dir) + 2, MEMF_CLEAR);
					
					if(aw->current_dir) {
						strncpy(cdir, aw->current_dir, cdir_len);
						FreeVec(aw->current_dir);
					}
					
					AddPart(cdir, dir, cdir_len + 1 + strlen(dir) + 2);
					strcat(cdir, "/"); // add trailing slash
					aw->current_dir = cdir;

					/* switch to dir */
					SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
						LISTBROWSER_Labels, ~0, TAG_DONE);

					window_flat_browser_construct(aw);
					highlight_current_dir(aw);

					SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
						LISTBROWSER_Labels, &aw->lblist,
					TAG_DONE);

					break;

				}
			}

			toggle_item(aw, node, 1, TRUE); /* ensure selected */
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

	if(aw->flat_mode && aw->current_dir) {
		FreeVec(aw->current_dir);
		aw->current_dir = NULL;
	}

}

void window_update_sourcedir(void *awin, char *sourcedir)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
					GETFILE_Drawer, sourcedir, TAG_DONE);
}

void window_fuelgauge_update(void *awin, ULONG size, ULONG total_size)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(aw->windows[WID_MAIN] == NULL) return;

	SetGadgetAttrs(aw->gadgets[GID_PROGRESS], aw->windows[WID_MAIN], NULL,
					FUELGAUGE_Max, total_size,
					FUELGAUGE_Level, size,
					TAG_DONE);
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
		for(int i = 0; i < aw->total_items; i++) {
			if((aw->current_dir == NULL) || (strncmp(aw->arc_array[i]->name, aw->current_dir, strlen(aw->current_dir)) == 0)) {
				if(select == 2) {
					aw->arc_array[i]->selected = !aw->arc_array[i]->selected;
				} else {
					aw->arc_array[i]->selected = selected;
				}
			}
		}
	}

	for(node = aw->lblist.lh_Head; node->ln_Succ; node=node->ln_Succ) {
		toggle_item(aw, node, select, FALSE);
	}

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &aw->lblist,
			TAG_DONE);
}

char *window_req_dest(void *awin)
{	
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(DoMethod((Object *)aw->gadgets[GID_DEST], GFILE_REQUEST, aw->windows[WID_MAIN])) {
		GetAttr(GETFILE_Drawer, aw->gadgets[GID_DEST], (APTR)&aw->dest);
	}
	return aw->dest;
}

static void window_req_open_archive_internal(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	long retxfd = 0;
	long retark = 0;

	window_disable_gadgets(awin, TRUE);

	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&aw->dir_tree);
	free_arc_array(aw);

	module_free(awin);

	if(aw->archive_needs_free) window_free_archive_path(aw);
	GetAttr(GETFILE_FullFile, aw->gadgets[GID_ARCHIVE], (APTR)&aw->archive);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&aw->lblist);

	aw->archiver = ARC_XAD; /* Set in advance for flat browser tree use */
	if(config->activemodules & ARC_XAD) {
		ret = xad_info(aw->archive, config, aw, addlbnode_cb);
	} else {
		ret = 1;
	}
	if(ret != 0) { /* if xad failed try xfd */
		aw->archiver = ARC_XFD;

		if(config->activemodules & ARC_XFD) {
			retxfd = xfd_info(aw->archive, aw, addlbnode_cb);
		} else {
			retxfd = 1;
		}

		if(retxfd != 0) {
			aw->archiver = ARC_DEARK;
			if(config->activemodules & ARC_DEARK) {
				retark = deark_info(aw->archive, config, aw, addlbnode_cb);
			} else {
				retark = 1;
			}

			if(retark != 0) {
				/* Failed to open with any lib - show generic error rather than XAD's */
				aw->archiver = ARC_NONE;
				open_error_req(locale_get_string(MSG_UNABLETOOPENFILE), locale_get_string(MSG_OK), aw);
			}
		}
	}

	module_register(aw, &aw->mf);

	window_menu_set_enable_state(aw);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &aw->lblist,
				LISTBROWSER_SortColumn, 1,
			TAG_DONE);

	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, &aw->dir_tree, TAG_DONE);

	if(aw->flat_mode) {
		highlight_current_dir(aw);
	}

	window_update_title(aw);

	window_disable_gadgets(awin, FALSE);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);
}

#ifdef __amigaos4__
static void window_req_open_archive_p(void)
#else
static void __saveds window_req_open_archive_p(void)
#endif
{
	/* Tell the main process we are started */
	Signal(avalanche_process, SIGBREAKF_CTRL_F);
	
	/* Wait for UserData */
	Wait(SIGBREAKF_CTRL_E);
	
	/* Find our task */
	struct Task *list_task = FindTask(NULL);
	struct avalanche_extract_userdata *aeu = (struct avalanche_extract_userdata *)list_task->tc_UserData;
	struct avalanche_window *aw = (struct avalanche_window *)aeu->awin;
	
	/* Call Extract on our new process */
	window_req_open_archive_internal(aeu->awin, get_config());
	
	/* Free UserData */
	FreeVec(aeu);
	
	/* Signal that we've finished */
	Signal(avalanche_process, aw->process_exit_sig);
	
	FreeSignal(aw->process_exit_sig);
	aw->process_exit_sig = 0;
}

void window_req_open_archive(void *awin, struct avalanche_config *config, BOOL refresh_only)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;

	if(refresh_only == FALSE) {
		ret = DoMethod((Object *) aw->gadgets[GID_ARCHIVE], GFILE_REQUEST, aw->windows[WID_MAIN]);
		if(ret == 0) return;
	
		if(aw->flat_mode && aw->current_dir) {
			FreeVec(aw->current_dir);
			aw->current_dir = NULL;
		}
	}

	struct avalanche_extract_userdata *aeu = AllocVec(sizeof(struct avalanche_extract_userdata), MEMF_CLEAR);

	if(aeu == NULL) return;
	
	aeu->awin = awin;
	aeu->archive = NULL;
	aeu->newdest = NULL;
	aeu->node = NULL;

	if((aw->process_exit_sig = AllocSignal(-1)) == -1) {
		FreeVec(aeu);
		return;
	}
	
	/* Ensure there are no pending signals for this already */
	SetSignal(0L, aw->process_exit_sig);
	
	avalanche_process = FindTask(NULL);
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

	GetListBrowserNodeAttrs(node,
			LBNA_Checked, &checked,
			LBNA_UserData, &userdata,
		TAG_DONE);

	if(aw->flat_mode) {
		struct arc_entries *arc_entry = (struct arc_entries *)userdata;
		if(arc_entry == NULL) return NULL; /* dummy entry */
		userdata = arc_entry->userdata;
	}

	update_fuelgauge_text(aw);

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

struct module_functions *window_get_module_funcs(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return &aw->mf;
}

ULONG window_handle_input(void *awin, UWORD *code)
{
	return RA_HandleInput(window_get_object(awin), code);
}

BOOL window_edit_add(void *awin, char *file, char *root)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(module_vscan(awin, file, NULL, 0, FALSE) == 0) {
		module_free(aw);
		if(aw->mf.add) return aw->mf.add(aw, aw->archive, file, aw->current_dir, root);
	}

	return FALSE;
}

static BOOL window_edit_add_wbarg(void *awin, struct WBArg *wbarg)
{
	BOOL ret = TRUE;
	
	if(wbarg->wa_Lock) {
		char *file = NULL;
		if(file = AllocVec(1024, MEMF_CLEAR)) {
			NameFromLock(wbarg->wa_Lock, file, 1024);
			if(*wbarg->wa_Name) {

				window_disable_gadgets(awin, TRUE);

				AddPart(file, wbarg->wa_Name, 1024);
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
			window_disable_gadgets(awin, FALSE);

			}
			window_req_open_archive(awin, get_config(), TRUE);
			FreeVec(file);
		}
	}
	return ret;
}

static void window_edit_add_req(void *awin, struct avalanche_config *config)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	BOOL ok = FALSE;
	char *file;

	if(aw->mf.add == NULL) return;
	
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
				strcpy(file, aslreq->fr_Drawer);
				AddPart(file, aslreq->fr_File, len);

				window_disable_gadgets(awin, TRUE);

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

				ok = window_edit_add(aw, file, NULL); /* TRUE = cont, FALSE = abort */
				window_disable_gadgets(awin, FALSE);
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
		}
	} else {
		open_error_req(locale_get_string(MSG_ARCHIVEMUSTHAVEENTRIES), locale_get_string(MSG_OK), awin);
	}

	window_disable_gadgets(awin, FALSE);
	if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);

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
		window_add_dropzones(aw);
	}
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

	if(aw->archiver != ARC_NONE) window_req_open_archive(aw, config, TRUE);
}

ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code, struct MsgPort *winport, struct MsgPort *AppPort)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	ULONG done = WIN_DONE_OK;

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
			if(aw->disabled == TRUE) {
				aw->abort_requested = TRUE;
			} else {
				done = WIN_DONE_CLOSED;
			}
		break;

		case WMHI_GADGETUP:
			switch (result & WMHI_GADGETMASK) {
				case GID_ARCHIVE:
					window_req_open_archive(awin, config, FALSE);
				break;
					
				case GID_DEST:
					window_req_dest(awin);
				break;
				
				case GID_OPENWB:
					window_open_dest(awin);
				break;

				case GID_EXTRACT:
					if(aw->disabled) {
						aw->abort_requested = TRUE;
					} else {
						ret = extract(awin, aw->archive, aw->dest, NULL);
						if(ret != 0) show_error(ret, awin);
					}
				break;

				case GID_LIST:
					window_list_handle(awin, config->tmpdir);
				break;
				
				case GID_TREE:
					window_tree_handle(awin);
				break;
			}
			break;

		case WMHI_RAWKEY:
			switch(result & WMHI_GADGETMASK) {
				case RAWKEY_ESC:
					if(aw->disabled) {
						aw->abort_requested = TRUE;
					} else {
						done = WIN_DONE_CLOSED;
					}
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
		
		case WMHI_NEWSIZE:
			if(aw->disabled == FALSE) {
				window_remove_dropzones(aw);
				if(aw->drag_lock == FALSE) window_add_dropzones(aw);
			}
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

							case 1: // new window
								new_awin = window_create(config, NULL, winport, AppPort);
								if(new_awin == NULL) break;
								window_open(new_awin, appwin_mp);
							break;
							
							case 2: //open
								window_req_open_archive(awin, config, FALSE);
							break;

							case 4: //info
								req_show_arc_info(awin);
							break;

							case 5: //about
								show_about(awin);
							break;

							case 6: //check version
								if(window_get_window(awin))
									SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, TRUE,
											TAG_DONE);

								http_check_version(awin, winport, AppPort, appwin_mp);

								if(window_get_window(awin))
									SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);
							break;
							
							case 8: //quit
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
					
					case 2: //settings
						switch(ITEMNUM(code)) {
							case 0: // view mode
								switch(SUBNUM(code)) {
									case 0: //flat browser mode
										toggle_flat_mode(aw, config, TRUE);
									break;
									case 1: // list mode
										toggle_flat_mode(aw, config, FALSE);
									break;
								}
							break;
				
							case 2: //snapshot
								/* fetch current win posn */
								GetAttr(WA_Top, aw->objects[OID_MAIN], (APTR)&config->win_y);
								GetAttr(WA_Left, aw->objects[OID_MAIN], (APTR)&config->win_x);
								GetAttr(WA_Width, aw->objects[OID_MAIN], (APTR)&config->win_w);
								GetAttr(WA_Height, aw->objects[OID_MAIN], (APTR)&config->win_h);

								warning_req(aw, locale_get_string(MSG_SNAPSHOT_WARNING));
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

	/* This flag is set in the main loop
	 * if ESC is pressed or Abort is clicked */
	return aw->abort_requested;
}

void window_reset_count(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	aw->current_item = 0;
}

void window_disable_gadgets(void *awin, BOOL disable)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	aw->disabled = disable;

	if(aw->windows[WID_MAIN] == NULL) return;

	if(disable) {
		window_remove_dropzones(aw);
		if(aw->appwin) RemoveAppWindow(aw->appwin);
		aw->appwin = NULL;

		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Text,  locale_get_string( MSG_STOP ) ,
			TAG_DONE);
	} else {
		aw->appwin = AddAppWindowA(0, (ULONG)aw, aw->windows[WID_MAIN], aw->appwin_mp, NULL);
		if(aw->drag_lock == FALSE) window_add_dropzones(aw);

		SetGadgetAttrs(aw->gadgets[GID_EXTRACT], aw->windows[WID_MAIN], NULL,
				GA_Text, GID_EXTRACT_TEXT,
			TAG_DONE);

		/* Clear the state of the Abort flag */
		aw->abort_requested = FALSE;
	}

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_DEST], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_OPENWB], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);

	window_menu_activation(aw, !disable, TRUE);


	if(aw->flat_mode == FALSE) disable = TRUE;

	if(aw->gadgets[GID_TREE]) SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);
}

void fill_menu_labels(void)
{
	menu[0].nm_Label = locale_get_string( MSG_PROJECT );
	menu[1].nm_Label = locale_get_string( MSG_NEWARCHIVE );
	menu[2].nm_Label = locale_get_string( MSG_NEWWINDOW );
	menu[3].nm_Label = locale_get_string( MSG_OPEN );
	menu[5].nm_Label = locale_get_string( MSG_ARCHIVEINFO );
	menu[6].nm_Label = locale_get_string( MSG_ABOUT );
	menu[7].nm_Label = locale_get_string( MSG_CHECKVERSION );
	menu[9].nm_Label = locale_get_string( MSG_QUIT );
	menu[10].nm_Label = locale_get_string( MSG_EDIT );
	menu[11].nm_Label = locale_get_string( MSG_SELECTALL );
	menu[12].nm_Label = locale_get_string( MSG_CLEARSELECTION );
	menu[13].nm_Label = locale_get_string( MSG_INVERTSELECTION );
	menu[15].nm_Label = locale_get_string( MSG_ADDFILES );
	menu[16].nm_Label = locale_get_string( MSG_DELFILES );
	menu[18].nm_Label = locale_get_string( MSG_DRAGLOCK );
	menu[19].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[20].nm_Label = locale_get_string( MSG_VIEWMODE );
	menu[MENU_FLATMODE].nm_Label = locale_get_string( MSG_VIEWMODEBROWSER );
	menu[MENU_LISTMODE].nm_Label = locale_get_string( MSG_VIEWMODELIST );
	menu[24].nm_Label = locale_get_string( MSG_SNAPSHOT );
	menu[25].nm_Label = locale_get_string( MSG_PREFERENCES );
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
