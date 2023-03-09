/* Avalanche
 * (c) 2022-3 Chris Young
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
#include <proto/glyph.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/getfile.h>
#include <gadgets/listbrowser.h>
#include <images/glyph.h>
#include <images/label.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "locale.h"
#include "misc.h"
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
	GID_PARENT,
	GID_DIR,
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
	struct AppWindow *appwin;
	char *archive;
	char *dest;
	struct MinList deletelist;
	struct arc_entries **arc_array;
	struct arc_entries **dir_array;
	ULONG current_item;
	ULONG total_items;
	BOOL archive_needs_free;
	void *archive_userdata;
	BOOL h_mode;
	BOOL flat_mode;
	BOOL iconified;
	char *current_dir;
};

static struct List winlist;
static ULONG zero = 0;

/** Menu **/

struct NewMenu menu[] = {
	{NM_TITLE,  NULL,           0,  0, 0, 0,}, // 0 Project
	{NM_ITEM,   NULL,         "O", 0, 0, 0,}, // 0 Open
	{NM_ITEM,   NULL,         "+", 0, 0, 0,}, // 1 New window
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 2
	{NM_ITEM,   NULL , "!", NM_ITEMDISABLED, 0, 0,}, // 3 Archive Info
	{NM_ITEM,   NULL ,        "?", 0, 0, 0,}, // 4 About
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 5
	{NM_ITEM,   NULL,         "Q", 0, 0, 0,}, // 6 Quit

	{NM_TITLE,  NULL,               0,  0, 0, 0,}, // 1 Edit
	{NM_ITEM,   NULL,       "A", NM_ITEMDISABLED, 0, 0,}, // 0 Select All
	{NM_ITEM,   NULL ,  "Z", NM_ITEMDISABLED, 0, 0,}, // 1 Clear Selection
	{NM_ITEM,   NULL , "I", NM_ITEMDISABLED, 0, 0,}, // 2 Invert

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Settings
	{NM_ITEM,	NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 0 HBrowser
	{NM_ITEM,	NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 1 Flat Browser
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 2
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 3 Snapshot
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 4 Preferences

	{NM_END,   NULL,        0,  0, 0, 0,},
};

#define MENU_HMODE 13
#define MENU_FLATMODE 14

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
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,3,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OnMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0));
	} else {
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(0,3,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,0,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,1,0));
		OffMenu(aw->windows[WID_MAIN], FULLMENUNUM(1,2,0));
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

static void toggle_item(struct avalanche_window *aw, struct Node *node, ULONG select, BOOL detach_list)
{
	if(detach_list) {
		SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);
	}

	ULONG current;
	ULONG selected;
	struct arc_entries *userdata;

	if(aw->flat_mode) {
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

	if(aw->flat_mode) {
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
	
	if(aw->dir_array == NULL) return;

	for(int i = 0; i < aw->total_items; i++) {
		if(aw->dir_array[i]) {
			if(aw->dir_array[i]->name) FreeVec(aw->dir_array[i]->name);
			FreeVec(aw->dir_array[i]);
		}
	}
	FreeVec(aw->dir_array);
	aw->dir_array = NULL;
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

long extract(void *awin, char *archive, char *newdest, struct Node *node)
{
	long ret = 0;
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	if(archive && newdest) {
		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
										WA_PointerType, POINTERTYPE_PROGRESS,
										TAG_DONE);
		window_reset_count(awin);
		window_disable_gadgets(awin, TRUE);

		if((node == NULL) && (aw->flat_mode)) {
			ret = module_extract_array(awin, aw->arc_array, aw->total_items, newdest);
		} else {
			ret = module_extract(awin, node, archive, newdest);
		}

		window_disable_gadgets(awin, FALSE);

		if(window_get_window(awin)) SetWindowPointer(window_get_window(awin),
											WA_BusyPointer, FALSE,
											TAG_DONE);
	}

	return ret;
}

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, BOOL h, BOOL selected, struct avalanche_window *aw)
{
	BOOL dir_seen = FALSE;
	ULONG flags = 0;
	ULONG gen = 0;
	int i = 0;
	char *name_copy = NULL;
	ULONG glyph = GLYPH_POPFILE;
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

	if(dir) glyph = GLYPH_POPDRAWER;

	struct Node *node = AllocListBrowserNode(4,
		LBNA_UserData, userdata,
		LBNA_CheckBox, !dir,
		LBNA_Checked, selected,
		LBNA_Flags, flags,
		LBNA_Generation, gen,
		LBNA_Column, 0,
			LBNCA_Image, GlyphObj,
						GLYPH_Glyph, glyph,
						GlyphEnd,
		LBNA_Column, 1,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, name,
		LBNA_Column, 2,
			LBNCA_Integer, size,
		LBNA_Column, 3,
			LBNCA_CopyText, TRUE,
			LBNCA_Text, datestr,
		TAG_DONE);

	AddTail(&aw->lblist, node);
}

static void addlbnodesinglefile(char *name, LONG *size, void *userdata, struct avalanche_window *aw)
{
	struct Node *node = AllocListBrowserNode(4,
		LBNA_UserData, userdata,
		LBNA_CheckBox, TRUE,
		LBNA_Checked, TRUE,
		LBNA_Flags, 0,
		LBNA_Generation, 0,
		LBNA_Column, 0,
			LBNCA_Image, GlyphObj,
						GLYPH_Glyph, GLYPH_POPFILE,
						GlyphEnd,
		LBNA_Column, 1,
			LBNCA_Text, name,
		LBNA_Column, 2,
			LBNCA_Integer, size,
		LBNA_Column, 3,
			//LBNCA_CopyText, TRUE,
			LBNCA_Text, NULL,
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

static void window_flat_browser_tree_construct(struct avalanche_window *aw)
{
	char **prev_dirs = AllocVec(sizeof(char *) * (aw->total_items + 1), MEMF_CLEAR);
	int dir_entry = 0;
	aw->dir_array = AllocVec(sizeof(struct arc_entries *) * (aw->total_items + 1), MEMF_CLEAR);
	if(aw->dir_array == NULL) return;

	SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, ~0, TAG_DONE);

	for(int it = 0; it < aw->total_items; it++) {
		char *dir_name = AllocVec(strlen(aw->arc_array[it]->name)+1, MEMF_CLEAR);
		int i = 0;
		int slash = 0;
		int last_slash = 0;

		BOOL dupe = FALSE;

		if(dir_name == NULL) continue;

		prev_dirs[it] = NULL;

		while(aw->arc_array[it]->name[i+1]) {
			if(aw->arc_array[it]->name[i] == '/') {
				slash++;
				last_slash = i;
			}
			dir_name[i] = aw->arc_array[it]->name[i];
			i++;
		}
		dir_name[last_slash] = '\0';

		for(int j = 0; j < it; j++) {
			if((prev_dirs[j]) && (strlen(dir_name) > 0) && (strcmp(prev_dirs[j], dir_name) == 0)) {
				dupe = TRUE;
			}
		}

		if((slash > 0) && (dupe == FALSE)) {
			#ifdef __amigaos4__
			DebugPrintF("%s [%d]\n", dir_name, slash); //FilePart()
			#endif
			
			aw->dir_array[dir_entry] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			if(aw->dir_array[dir_entry] == NULL) continue;
			
			aw->dir_array[dir_entry]->name = dir_name;
			aw->dir_array[dir_entry]->level = slash;
			aw->dir_array[dir_entry]->dir = FALSE;
							
			if((dir_entry > 0) && (aw->dir_array[dir_entry-1]->level < slash)) {
				aw->dir_array[dir_entry-1]->dir = TRUE;  //LBFLG_HASCHILDREN
			}

			dir_entry++;

			prev_dirs[it] = strdup(dir_name);
		} else {
			FreeVec(dir_name);
		}
	}

	ULONG flags = LBFLG_HASCHILDREN;
	if(dir_entry == 0) flags = 0;

	struct Node *node = AllocListBrowserNode(1,
									LBNA_UserData, NULL,
									LBNA_Flags, flags,
									LBNA_Generation, 1,
									LBNA_Column, 0,
										LBNCA_Image, GlyphObj,
											GLYPH_Glyph, GLYPH_POPDRAWER,
										GlyphEnd,
									TAG_DONE);

	AddTail(&aw->dir_tree, node);

	for(int i = 0; i <= dir_entry; i++) {
		flags = LBFLG_HASCHILDREN;
		if(aw->dir_array[i] == NULL) break;
		if(aw->dir_array[i]->dir == FALSE) flags = 0;
		
		struct Node *node = AllocListBrowserNode(1,
									LBNA_UserData, aw->dir_array[i]->name,
									LBNA_Flags, flags,
									LBNA_Generation, aw->dir_array[i]->level + 1,
									LBNA_Column, 0,
										LBNCA_CopyText, TRUE,
										LBNCA_Text, FilePart(aw->dir_array[i]->name),
									TAG_DONE);

		AddTail(&aw->dir_tree, node);
	}

	for(int it = 0; it < aw->total_items; it++) {
		if(prev_dirs[it] != NULL) free(prev_dirs[it]);
	}
	if(prev_dirs) FreeVec(prev_dirs);
	
	SetGadgetAttrs(aw->gadgets[GID_TREE], aw->windows[WID_MAIN], NULL,
					LISTBROWSER_Labels, &aw->dir_tree,
					TAG_DONE);
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

	for(int it = 0; it < aw->total_items; it++) {
		/* Only show current level */
		if((aw->arc_array[it]->level == level) && (aw->arc_array[it]->dir == FALSE) &&
			((aw->current_dir == NULL) || (strncmp(aw->arc_array[it]->name, aw->current_dir, strlen(aw->current_dir)) == 0))) {
			addlbnode(aw->arc_array[it]->name + skip_dir_len,
				aw->arc_array[it]->size, aw->arc_array[it]->dir, aw->arc_array[it], FALSE, aw->arc_array[it]->selected, aw);
		}
	}
	
	/* Add fake dir entries */
	char *prev_dir_name = NULL;

	for(int it = 0; it < aw->total_items; it++) {
		if(aw->arc_array[it]->name == NULL) continue;
		char *dir_name = AllocVec(strlen(aw->arc_array[it]->name) +1, MEMF_CLEAR);
		int i = 0;
		int slash = 0;

		if(dir_name == NULL) continue;

		if(aw->arc_array[it]->level != (level + 1)) continue;
							
		while(aw->arc_array[it]->name[i+1]) {
			if(aw->arc_array[it]->name[i] == '/') {
				slash++;
				if(slash == (level + 1)) {
					dir_name[i] = '\0';
					break;
				}
			}
			dir_name[i] = aw->arc_array[it]->name[i];
			i++;
		}
		if((prev_dir_name == NULL) || (prev_dir_name && (strcmp(prev_dir_name, dir_name) != 0))) {
			addlbnode(dir_name + skip_dir_len, &zero, TRUE, NULL, FALSE, FALSE, aw);
			if(prev_dir_name) free(prev_dir_name);
			prev_dir_name = strdup(dir_name);
		}

		FreeVec(dir_name);
	}
	if(prev_dir_name != NULL) free(prev_dir_name);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);

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
			aw->arc_array[item]->selected = TRUE;
			
			if(item == (total - 1)) {
				if(config->debug) qsort(aw->arc_array, total, sizeof(struct arc_entries *), sort_array);
				for(int i=0; i<total; i++) {
					addlbnode(aw->arc_array[i]->name, aw->arc_array[i]->size, aw->arc_array[i]->dir, aw->arc_array[i], aw->h_mode, TRUE, aw);
					FreeVec(aw->arc_array[i]);
				}
				FreeVec(aw->arc_array);
				aw->arc_array = NULL;
			}
		}
	} else if(aw->flat_mode) {
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
				qsort(aw->arc_array, total, sizeof(struct arc_entries *), sort_array);
				window_flat_browser_tree_construct(aw);
				window_flat_browser_construct(aw);
			}
		}
	} else {
		addlbnode(name, size, dir, userdata, aw->h_mode, TRUE, aw);
	}
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

	if(aw->flat_mode) {
		if(aw->arc_array) free_arc_array(aw);
		aw->arc_array = AllocVec(sizeof(struct arc_entries *), MEMF_CLEAR);

		if(aw->arc_array) {
			aw->arc_array[0] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			
			aw->arc_array[0]->name = name;
			aw->arc_array[0]->size = size;
			aw->arc_array[0]->dir = dir;
			aw->arc_array[0]->userdata = userdata;
			
			aw->arc_array[0]->level = 0;
		
			window_flat_browser_construct(aw);
		}
	} else {
		addlbnodesinglefile(name, size, userdata, aw);
	}

	return;
}

void *array_get_userdata(void *awin, void *arc_entry)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

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


/* Window functions */

void *window_create(struct avalanche_config *config, char *archive, struct MsgPort *winport, struct MsgPort *appport)
{
	struct avalanche_window *aw = AllocVec(sizeof(struct avalanche_window), MEMF_CLEAR | MEMF_PRIVATE);
	if(!aw) return NULL;
	
	ULONG getfile_drawer = GETFILE_Drawer;
	struct Hook *asl_hook = (struct Hook *)&(aw->aslfilterhook);
	
	if(config->disable_asl_hook) {
		asl_hook = NULL;
	}

	ULONG tag_default_position = WINDOW_Position;

	aw->lbci = AllocLBColumnInfo(4, 
		LBCIA_Column, 0,
			LBCIA_Title, "",
			LBCIA_Weight, 10,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, FALSE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string(MSG_NAME),
			LBCIA_Weight, 65,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
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
	aw->h_mode = config->h_browser;
	aw->flat_mode = FALSE; /* TODO: Add to global config */

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
						getfile_drawer, config->sourcedir,
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
			LAYOUT_AddChild, LayoutHObj,
				LAYOUT_AddChild,  aw->gadgets[GID_PARENT] = ButtonObj,
					GA_ID, GID_PARENT,
					GA_RelVerify, TRUE,
					GA_Image, GlyphObj,
						GLYPH_Glyph, GLYPH_UPARROW,
					GlyphEnd,
					GA_Disabled, !aw->flat_mode,
				ButtonEnd,
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild,  aw->gadgets[GID_DIR] = StringObj,
					GA_ID, GID_DIR,
					GA_ReadOnly, TRUE,
					GA_Disabled, !aw->flat_mode,
				StringEnd,
				CHILD_WeightedHeight, 0,
			LayoutEnd,
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, LayoutVObj,
				LAYOUT_AddChild, LayoutHObj,
					LAYOUT_AddChild,  aw->gadgets[GID_TREE] = ListBrowserObj,
						GA_ID, GID_TREE,
						GA_RelVerify, TRUE,
						LISTBROWSER_ColumnInfo, aw->dtci,
						LISTBROWSER_Labels, &(aw->dir_tree),
						LISTBROWSER_ColumnTitles, FALSE,
						LISTBROWSER_FastRender, TRUE,
						LISTBROWSER_Hierarchical, TRUE,
					ListBrowserEnd,
					CHILD_WeightedWidth, 20,
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

void window_open(void *awin, struct MsgPort *appwin_mp)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->windows[WID_MAIN]) {
		WindowToFront(aw->windows[WID_MAIN]);
	} else {
		if(aw->h_mode) aw->menu[MENU_HMODE].nm_Flags |= CHECKED;
		if(aw->flat_mode) aw->menu[MENU_FLATMODE].nm_Flags |= CHECKED;

		aw->windows[WID_MAIN] = (struct Window *)RA_OpenWindow(aw->objects[OID_MAIN]);
		
		if(aw->windows[WID_MAIN]) {
			aw->appwin = AddAppWindowA(0, (ULONG)aw, aw->windows[WID_MAIN], appwin_mp, NULL);
			
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
	
	if(aw->windows[WID_MAIN]) {
		RemoveAppWindow(aw->appwin);
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

void window_tree_handle(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	ULONG tmp = 0;
	struct Node *node = NULL;

	GetAttr(LISTBROWSER_RelEvent, aw->gadgets[GID_LIST], (APTR)&tmp);

	switch(tmp) {
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);

			if(aw->flat_mode) {
				void *userdata = NULL;

				GetListBrowserNodeAttrs(node,
					LBNA_UserData, &userdata,
				TAG_DONE);

				if(userdata == NULL) return;
				aw->current_dir = userdata;

				/* switch to dir */
				SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
					STRINGA_TextVal, aw->current_dir,
				TAG_DONE);

				char *slash = strrchr(aw->current_dir, '/');
				
				if(slash) {
					SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
						GA_Disabled, FALSE,
					TAG_DONE);
				} else {
					SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
						GA_Disabled, TRUE,
					TAG_DONE);
				}
				
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
					

void window_list_handle(void *awin, char *tmpdir)
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
					SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
						STRINGA_TextVal, aw->current_dir,
					TAG_DONE);

					SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
						GA_Disabled, FALSE,
					TAG_DONE);

					SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
						LISTBROWSER_Labels, ~0, TAG_DONE);

					window_flat_browser_construct(aw);

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

/* select: 0 = deselect all, 1 = select all, 2 = toggle all */
void window_modify_all_list(void *awin, ULONG select)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	struct Node *node;
	BOOL selected;

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

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

	free_arc_array(aw);
	if(aw->current_dir) {
		FreeVec(aw->current_dir);
		aw->current_dir = NULL;
	}

	if(aw->archive_needs_free) window_free_archive_path(aw);
	GetAttr(GETFILE_FullFile, aw->gadgets[GID_ARCHIVE], (APTR)&aw->archive);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
		STRINGA_TextVal, aw->current_dir,
	TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&aw->lblist);

	ret = xad_info(aw->archive, config, aw, addlbnode_cb);
	if(ret != 0) { /* if xad failed try xfd */
		retxfd = xfd_info(aw->archive, aw, addlbnodexfd_cb);
		if(retxfd != 0) {
			/* Failed to open with any lib - show generic error rather than XAD's */
			open_error_req(locale_get_string(MSG_UNABLETOOPENFILE), locale_get_string(MSG_OK), aw);
		}
	}

	if(ret == 0) {
		aw->archiver = ARC_XAD;
	} else if(retxfd == 0) {
		aw->archiver = ARC_XFD;
	} else {
		aw->archiver = ARC_NONE;
	}

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

	if(aw->flat_mode) {
		struct arc_entries *arc_entry = (struct arc_entries *)userdata;
		if(arc_entry == NULL) return NULL; /* dummy entry */
		userdata = arc_entry->userdata;
	}

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

ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code, struct MsgPort *winport, struct MsgPort *AppPort)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	long ret = 0;
	ULONG done = WIN_DONE_OK;

	switch (result & WMHI_CLASSMASK) {
		case WMHI_CLOSEWINDOW:
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

				case GID_PARENT:
					if(aw->current_dir) {
						aw->current_dir[strlen(aw->current_dir) - 1] = '\0';

						char *slash = strrchr(aw->current_dir, '/');
					
						if(slash == NULL) {
							FreeVec(aw->current_dir);
							aw->current_dir = NULL;

							SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
								GA_Disabled, TRUE,
							TAG_DONE);

						} else {
							*(slash+1) = '\0';
						}

						SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
							STRINGA_TextVal, aw->current_dir,
						TAG_DONE);

						SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
							LISTBROWSER_Labels, ~0, TAG_DONE);

						window_flat_browser_construct(aw);

						SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
							LISTBROWSER_Labels, &aw->lblist,
						TAG_DONE);
					}
				break;

				case GID_EXTRACT:
					ret = extract(awin, aw->archive, aw->dest, NULL);
					if(ret != 0) show_error(ret, awin);
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
				void *new_awin;
				struct MenuItem *item = ItemAddress(aw->windows[WID_MAIN]->MenuStrip, code);
				switch(MENUNUM(code)) {
					case 0: //project
						switch(ITEMNUM(code)) {
							case 0: //open
								window_req_open_archive(awin, config, FALSE);
							break;
							
							case 1: // new window
								new_awin = window_create(config, NULL, winport, AppPort);
								if(new_awin == NULL) break;
								window_open(new_awin, appwin_mp);
							break;
							
							case 3: //info
								req_show_arc_info(awin);
							break;

							case 4: //about
								show_about(awin);
							break;
							
							case 6: //quit
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
						}
					break;
					
					case 2: //settings
						switch(ITEMNUM(code)) {
							case 0: //browser mode
								aw->h_mode = !aw->h_mode;
									
								window_toggle_hbrowser(awin, aw->h_mode);
								window_req_open_archive(awin, config, TRUE);
							break;
					
							case 1: //flat browser mode TODO: this should be MX with above
								aw->flat_mode = !aw->flat_mode;
								BOOL disable = !aw->flat_mode;

								SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
									GA_Disabled, disable,
								TAG_DONE);

								if((disable == FALSE) && (aw->current_dir == NULL)) disable = TRUE;

								SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
									GA_Disabled, disable,
								TAG_DONE);


								window_req_open_archive(awin, config, TRUE);
							break;
										
							case 3: //snapshot
								/* fetch current win posn */
								GetAttr(WA_Top, aw->objects[OID_MAIN], (APTR)&config->win_x);
								GetAttr(WA_Left, aw->objects[OID_MAIN], (APTR)&config->win_y);
								GetAttr(WA_Width, aw->objects[OID_MAIN], (APTR)&config->win_w);
								GetAttr(WA_Height, aw->objects[OID_MAIN], (APTR)&config->win_h);
							break;
								
							case 4: //prefs
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

	if((disable == FALSE) && (aw->flat_mode == FALSE)) disable = TRUE;

	SetGadgetAttrs(aw->gadgets[GID_DIR], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable,
		TAG_DONE);

	if((disable == FALSE) && (aw->current_dir == NULL)) disable = TRUE;

	SetGadgetAttrs(aw->gadgets[GID_PARENT], aw->windows[WID_MAIN], NULL,
			GA_Disabled, disable ,
		TAG_DONE);

}

void fill_menu_labels(void)
{
	menu[0].nm_Label = locale_get_string( MSG_PROJECT );
	menu[1].nm_Label = locale_get_string( MSG_OPEN );
	menu[2].nm_Label = locale_get_string( MSG_NEWWINDOW );
	menu[4].nm_Label = locale_get_string( MSG_ARCHIVEINFO );
	menu[5].nm_Label = locale_get_string( MSG_ABOUT );
	menu[7].nm_Label = locale_get_string( MSG_QUIT );
	menu[8].nm_Label = locale_get_string( MSG_EDIT );
	menu[9].nm_Label = locale_get_string( MSG_SELECTALL );
	menu[10].nm_Label = locale_get_string( MSG_CLEARSELECTION );
	menu[11].nm_Label = locale_get_string( MSG_INVERTSELECTION );
	menu[12].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[MENU_HMODE].nm_Label = locale_get_string( MSG_HIERARCHICALBROWSEREXPERIMENTAL );
	menu[MENU_FLATMODE].nm_Label = locale_get_string( MSG_FLATBROWSER );
	menu[16].nm_Label = locale_get_string( MSG_SNAPSHOT );
	menu[17].nm_Label = locale_get_string( MSG_PREFERENCES );
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
