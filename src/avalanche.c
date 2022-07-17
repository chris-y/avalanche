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
#include "xad.h"
#include "xfd.h"
#include "xvs.h"

#include "Avalanche_rev.h"

#ifndef __amigaos4__
#define IsMinListEmpty(L) (L)->mlh_Head->mln_Succ == 0
#endif

const char *version = VERSTAG;

struct arc_entries {
	char *name;
	ULONG *size;
	BOOL dir;
	void *userdata;
};

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

enum {
	ARC_NONE = 0,
	ARC_XAD,
	ARC_XFD
};

#define GID_EXTRACT_TEXT  locale_get_string( MSG_EXTRACT ) 

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

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Settings
	{NM_ITEM,	NULL,     0, CHECKIT | MENUTOGGLE, 0, 0,}, // 0 Scan
	{NM_ITEM,	NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 1 HBrowser
	{NM_ITEM,   NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 2 Win posn
	{NM_ITEM,   NULL ,         0, CHECKIT | MENUTOGGLE, 0, 0,}, // 3 Confirm quit
	{NM_ITEM,   NULL ,         0, CHECKIT | MENUTOGGLE, 0, 0,}, // 4 Ignore FS
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 5
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 6 Save settings

	{NM_END,   NULL,        0,  0, 0, 0,},
};

/** Global config **/
#define PROGRESS_SIZE_DEFAULT 20

static char *progname = NULL;
static char *dest;
static char *archive = NULL;
static char *tmpdir = NULL;

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;
static BOOL dir_seen = FALSE;

static BOOL save_win_posn = FALSE;
static BOOL h_browser = FALSE;
static BOOL virus_scan = FALSE;
static BOOL debug = FALSE;
static BOOL confirmquit = FALSE;
static BOOL ignorefs = FALSE;

static ULONG win_x = 0;
static ULONG win_y = 0;
static ULONG win_w = 0;
static ULONG win_h = 0;
static ULONG progress_size = PROGRESS_SIZE_DEFAULT;

static long cx_pri = 0;
static BOOL cx_popup = TRUE;

/** Shared variables **/
static struct MinList deletelist;
static struct List lblist;
static struct Locale *locale = NULL;
static ULONG current_item = 0;
static ULONG total_items = 0;

static struct Window *windows[WID_LAST];
static struct Gadget *gadgets[GID_LAST];
static Object *objects[OID_LAST];

ULONG archiver = ARC_NONE;

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

void *get_window(void)
{
	return windows[WID_MAIN];
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
	if(dest) free(dest);
	dest = NULL;
	dest_needs_free = FALSE;
}

void savesettings(Object *win)
{
	struct DiskObject *dobj;
	UBYTE **oldtooltypes;
	UBYTE *newtooltypes[13];
	char tt_dest[100];
	char tt_tmp[100];
	char tt_winx[15];
	char tt_winy[15];
	char tt_winh[15];
	char tt_winw[15];
	char tt_progresssize[20];

	if(dobj = GetIconTagList(progname, NULL)) {
		oldtooltypes = (UBYTE **)dobj->do_ToolTypes;

		if(dest && (strcmp("RAM:", dest) != 0)) {
			strcpy(tt_dest, "DEST=");
			newtooltypes[0] = strcat(tt_dest, dest);
		} else {
			newtooltypes[0] = "(DEST=RAM:)";
		}

		if(h_browser) {
			newtooltypes[1] = "HBROWSER";
		} else {
			newtooltypes[1] = "(HBROWSER)";
		}

		if(save_win_posn) {
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

		if(virus_scan) {
			newtooltypes[8] = "VIRUSSCAN";
		} else {
			newtooltypes[8] = "(VIRUSSCAN)";
		}

		if(confirmquit) {
			newtooltypes[9] = "CONFIRMQUIT";
		} else {
			newtooltypes[9] = "(CONFIRMQUIT)";
		}

		if(ignorefs) {
			newtooltypes[10] = "IGNOREFS";
		} else {
			newtooltypes[10] = "(IGNOREFS)";
		}

		if(tmpdir && (strcmp("T:", tmpdir) != 0)) {
			strcpy(tt_tmp, "TMPDIR=");
			newtooltypes[11] = strcat(tt_tmp, dest);
		} else {
			newtooltypes[11] = "(TMPDIR=T:)";
		}

		newtooltypes[12] = NULL;

		dobj->do_ToolTypes = (STRPTR *)&newtooltypes;
		PutIconTags(progname, dobj, NULL);
		dobj->do_ToolTypes = (STRPTR *)oldtooltypes;
		FreeDiskObject(dobj);
	}
}

static ULONG ask_quit(void)
{
	if(confirmquit) {
		return ask_quit_req();
	}

	return 1;
}

static void addlbnode(char *name, LONG *size, BOOL dir, void *userdata, BOOL h)
{
	ULONG flags = 0;
	ULONG gen = 0;
	int i = 0;
	char *name_copy = NULL;

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
	xad_get_filedate(userdata, &cd);

	if(CheckDate(&cd) == 0)
		Amiga2Date(0, &cd);

	sprintf(datestr, "%04u-%02u-%02u %02u:%02u:%02u", cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);

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

	AddTail(&lblist, node);
}

static void addlbnodesinglefile(char *name, LONG *size, void *userdata)
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

	AddTail(&lblist, node);
}

int sort(const char *a, const char *b)
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

int sort_array(const void *a, const void *b)
{
	struct arc_entries *c = *(struct arc_entries **)a;
	struct arc_entries *d = *(struct arc_entries **)b;

	return sort(c->name, d->name);
}

void fuelgauge_update(ULONG size, ULONG total_size)
{
				SetGadgetAttrs(gadgets[GID_PROGRESS], windows[WID_MAIN], NULL,
					FUELGAUGE_Max, total_size,
					FUELGAUGE_Level, size,
					TAG_DONE);
}

static void addlbnodexfd_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata)
{
	static struct arc_entries **arc_array = NULL;

	if(gadgets[GID_PROGRESS]) {
		char msg[20];
		sprintf(msg, "%d/%lu", 0, total);
		total_items = total;

		SetGadgetAttrs(gadgets[GID_PROGRESS], windows[WID_MAIN], NULL,
			GA_Text, msg,
			FUELGAUGE_Percent, FALSE,
			FUELGAUGE_Justification, FGJ_CENTER,
			FUELGAUGE_Level, 0,
			TAG_DONE);
	}

	return addlbnodesinglefile(name, size, userdata);
}


static void addlbnode_cb(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata)
{
	static struct arc_entries **arc_array = NULL;

	if(item == 0) {
		current_item = 0;
		if(gadgets[GID_PROGRESS]) {
			char msg[20];
			sprintf(msg, "%d/%lu", 0, total);
			total_items = total;

			SetGadgetAttrs(gadgets[GID_PROGRESS], windows[WID_MAIN], NULL,
				GA_Text, msg,
				FUELGAUGE_Percent, FALSE,
				FUELGAUGE_Justification, FGJ_CENTER,
				FUELGAUGE_Level, 0,
				TAG_DONE);
		}
	}

	if(archiver == ARC_XFD) return addlbnodesinglefile(name, size, userdata);

	if(h_browser) {
		if(item == 0) {
			arc_array = AllocVec(total * sizeof(struct arc_entries *), MEMF_CLEAR);
		}

		if(arc_array) {
			arc_array[item] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
			
			arc_array[item]->name = name;
			arc_array[item]->size = size;
			arc_array[item]->dir = dir;
			arc_array[item]->userdata = userdata;
			
			if(item == (total - 1)) {
				if(debug) qsort(arc_array, total, sizeof(struct arc_entries *), sort_array);
				for(int i=0; i<total; i++) {
					addlbnode(arc_array[i]->name, arc_array[i]->size, arc_array[i]->dir, arc_array[i]->userdata, h_browser);
					FreeVec(arc_array[i]);
				}
				FreeVec(arc_array);
				arc_array = NULL;
			}
		}
	} else {
		addlbnode(name, size, dir, userdata, h_browser);
	}
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

static void open_archive_req(BOOL refresh_only)
{
	dir_seen = FALSE;
	long ret = 0;
	long retxfd = 0;

	if(refresh_only == FALSE) {
		ret = DoMethod(gadgets[GID_ARCHIVE], GFILE_REQUEST, windows[WID_MAIN]);
		if(ret == 0) return;
	}

	if(archive_needs_free) free_archive_path();
	GetAttr(GETFILE_FullFile, gadgets[GID_ARCHIVE], (APTR)&archive);

	SetWindowPointer(windows[WID_MAIN],
					WA_BusyPointer, TRUE,
					TAG_DONE);

	SetGadgetAttrs(gadgets[GID_LIST], windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&lblist);

	ret = xad_info(archive, !ignorefs, addlbnode_cb);
	if(ret != 0) { /* if xad failed try xfd */
		retxfd = xfd_info(archive, addlbnodexfd_cb);
		if(retxfd != 0) show_error(ret);
	}

	if(ret == 0) {
		archiver = ARC_XAD;
		menu_activation(TRUE);
	} else if(retxfd == 0) {
		archiver = ARC_XFD;
		menu_activation(TRUE);
	} else {
		archiver = ARC_NONE;
		menu_activation(FALSE);
	}

	SetGadgetAttrs(gadgets[GID_LIST], windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &lblist,
				LISTBROWSER_SortColumn, 0,
			TAG_DONE);

	SetWindowPointer(windows[WID_MAIN],
					WA_BusyPointer, FALSE,
					TAG_DONE);
}

static void toggle_item(struct Window *win, struct Gadget *list_gad, struct Node *node, ULONG select)
{
	SetGadgetAttrs(list_gad, win, NULL,
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

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, &lblist,
			TAG_DONE);
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
			OffMenu(windows[WID_MAIN], FULLMENUNUM(2,0,0));
		}
	}

	return res;
}

static const char *get_item_filename(struct Node *node)
{
	void *userdata = NULL;
	const char *fn = NULL;

	GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

	switch(archiver) {
		case ARC_XAD:
			fn = xad_get_filename(userdata);
		break;
		case ARC_XFD:
			fn = xfd_get_filename(userdata);
		break;
	}

	return fn;
}

static long extract(char *newdest, struct Node *node)
{
	long ret = 0;

	if(newdest == NULL) newdest = dest;

	if(archive && newdest) {
		SetWindowPointer(windows[WID_MAIN],
				WA_PointerType, POINTERTYPE_PROGRESS,
			TAG_DONE);
		current_item = 0;

		disable_gadgets(TRUE);

		switch(archiver) {
			case ARC_XAD:
				if(node == NULL) {
					ret = xad_extract(archive, newdest, &lblist, getlbnode, vscan);
				} else {
					ULONG pud = 0;
					ret = xad_extract_file(archive, newdest, node, getlbnode, vscan, &pud);
				}
			break;
			case ARC_XFD:
				ret = xfd_extract(archive, newdest, vscan);
			break;
		}

		disable_gadgets(FALSE);

		SetWindowPointer(windows[WID_MAIN],
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

static void delete_delete_list(void)
{
	struct Node *node;
	struct Node *nnode;

	if(IsMinListEmpty((struct MinList *)&deletelist) == FALSE) {
		node = (struct Node *)GetHead((struct List *)&deletelist);

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


static void add_to_delete_list(char *fn)
{
	struct Node *node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
	if(node) {
		node->ln_Name = strdup(fn);
		AddTail((struct List *)&deletelist, (struct Node *)node);
	}
}

static void UnregisterCx(CxObj *CXBroker, struct MsgPort *CXMP)
{
	CxMsg *msg;

	DeleteCxObj(CXBroker);
	while(msg = (CxMsg *)GetMsg(CXMP))
		ReplyMsg((struct Message *)msg);

	DeletePort(CXMP);
}

static struct MsgPort *RegisterCx(CxObj **CXBroker)
{
	struct MsgPort *CXMP = NULL;
	struct NewBroker newbroker;
	CxObj *broker = NULL;

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
	struct MsgPort *appwin_mp = NULL;
	struct AppWindow *appwin = NULL;
	struct AppMessage *appmsg = NULL;
	ULONG appwin_sig = 0;

	struct Hook aslfilterhook;
	aslfilterhook.h_Entry = aslfilterfunc;
	aslfilterhook.h_SubEntry = NULL;
	aslfilterhook.h_Data = NULL;

	ULONG tag_default_position = WINDOW_Position;

	struct ColumnInfo *lbci = AllocLBColumnInfo(3, 
		LBCIA_Column, 0,
			LBCIA_Title,  locale_get_string( MSG_NAME ) ,
			LBCIA_Weight, 65,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string( MSG_SIZE ) ,
			LBCIA_Weight, 15,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 2,
			LBCIA_Title,  locale_get_string( MSG_DATE ) ,
			LBCIA_Weight, 20,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		TAG_DONE);

	NewList(&lblist);

	if(virus_scan) menu[12].nm_Flags |= CHECKED;
	if(h_browser) menu[13].nm_Flags |= CHECKED;
	if(save_win_posn) menu[14].nm_Flags |= CHECKED;
	if(confirmquit) menu[15].nm_Flags |= CHECKED;
	if(ignorefs) menu[16].nm_Flags |= CHECKED;
	if(progname == NULL) menu[18].nm_Flags |= NM_ITEMDISABLED;

	if(win_x && win_y) tag_default_position = TAG_IGNORE;

	if(AppPort = CreateMsgPort()) {
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
			WA_Top, win_x,
			WA_Left, win_y,
			WA_Width, win_w,
			WA_Height, win_h,
			WINDOW_NewMenu, menu,
			WINDOW_IconifyGadget, TRUE,
			WINDOW_IconTitle, "Avalanche",
			WINDOW_AppPort, AppPort,
			tag_default_position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_MAIN] = LayoutVObj,
				//LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, LayoutHObj,
					LAYOUT_AddChild, LayoutVObj,
						LAYOUT_AddChild, gadgets[GID_ARCHIVE] = GetFileObj,
							GA_ID, GID_ARCHIVE,
							GA_RelVerify, TRUE,
							GETFILE_TitleText,  locale_get_string( MSG_SELECTARCHIVE ) ,
							GETFILE_FullFile, archive,
							GETFILE_ReadOnly, TRUE,
							GETFILE_FilterFunc, &aslfilterhook,
						End,
						CHILD_WeightedHeight, 0,
						CHILD_Label, LabelObj,
							LABEL_Text,  locale_get_string( MSG_ARCHIVE ) ,
						LabelEnd,
						LAYOUT_AddChild, gadgets[GID_DEST] = GetFileObj,
							GA_ID, GID_DEST,
							GA_RelVerify, TRUE,
							GETFILE_TitleText,  locale_get_string( MSG_SELECTDESTINATION ) ,
							GETFILE_Drawer, dest,
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
					LAYOUT_AddChild, gadgets[GID_PROGRESS] = FuelGaugeObj,
						GA_ID, GID_PROGRESS,
					FuelGaugeEnd,
					CHILD_WeightedWidth, progress_size,
				LayoutEnd,
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild, gadgets[GID_LIST] = ListBrowserObj,
						GA_ID, GID_LIST,
						GA_RelVerify, TRUE,
						LISTBROWSER_ColumnInfo, lbci,
						LISTBROWSER_Labels, &lblist,
						LISTBROWSER_ColumnTitles, TRUE,
						LISTBROWSER_TitleClickable, TRUE,
						LISTBROWSER_SortColumn, 0,
						LISTBROWSER_Striping, LBS_ROWS,
						LISTBROWSER_FastRender, TRUE,
						LISTBROWSER_Hierarchical, h_browser,
					ListBrowserEnd,
					LAYOUT_AddChild, gadgets[GID_EXTRACT] = ButtonObj,
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
		if (objects[OID_MAIN]) {

			if(cx_mp = RegisterCx(&cx_broker)) {
				cx_signal = (1 << cx_mp->mp_SigBit);
			}

			/*  Open the window.
			 */
			if (windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN])) {

				/* Open initial archive, if there is one. */
				if(archive) open_archive_req(TRUE);

				ULONG wait, signal, app = (1L << AppPort->mp_SigBit);
				ULONG done = FALSE;
				ULONG result;
				UWORD code;
				long ret = 0;
				ULONG tmp = 0;
				struct Node *node;

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
										case CXCMD_UNIQUE:
											//uniconify
										break;
										case CXCMD_DISAPPEAR:
											//iconify
										break;

										/* Nothing to disable yet, here for later use */
										case CXCMD_ENABLE:
										case CXCMD_DISABLE:
										default:
										break;
									}
								break;
								case CXM_IEVENT:
									/* TODO: popup hotkey */
								default:
								break;
							}
						}
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
									open_archive_req(TRUE);
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

							ReplyMsg((struct Message *)appmsg);
						}
					} else {
						while((done == FALSE) && ((result = RA_HandleInput(objects[OID_MAIN], &code) ) != WMHI_LASTMSG)) {
							switch (result & WMHI_CLASSMASK) {
								case WMHI_CLOSEWINDOW:
									if(ask_quit()) {
										windows[WID_MAIN] = NULL;
										done = TRUE;
									}
								break;

								case WMHI_GADGETUP:
									switch (result & WMHI_GADGETMASK) {
										case GID_ARCHIVE:
											open_archive_req(FALSE);
										break;
										
										case GID_DEST:
											if(dest_needs_free) free_dest_path();
											DoMethod((Object *)gadgets[GID_DEST], GFILE_REQUEST, windows[WID_MAIN]);
											GetAttr(GETFILE_Drawer, gadgets[GID_DEST], (APTR)&dest);
										break;

										case GID_EXTRACT:
											ret = extract(NULL, NULL);
											if(ret != 0) show_error(ret);
										break;

										case GID_LIST:
											GetAttr(LISTBROWSER_RelEvent, gadgets[GID_LIST], (APTR)&tmp);
											switch(tmp) {
#if 0 /* This selects items when single-clicked off the checkbox -
         it's incompatible with doube-clicking as it resets the listview */
												case LBRE_NORMAL:
													GetAttr(LISTBROWSER_SelectedNode, gadgets[GID_LIST], (APTR)&node);
													toggle_item(windows[WID_MAIN], gadgets[GID_LIST], node, 2);
												break;
#endif
												case LBRE_DOUBLECLICK:
													GetAttr(LISTBROWSER_SelectedNode, gadgets[GID_LIST], (APTR)&node);
													toggle_item(windows[WID_MAIN], gadgets[GID_LIST], node, 1); /* ensure selected */
													char fn[1024];
													strcpy(fn, tmpdir);
													ret = extract(fn, node);
													if(ret == 0) {
														AddPart(fn, get_item_filename(node), 1024);
														add_to_delete_list(fn);
														OpenWorkbenchObjectA(fn, NULL);
													} else {
														show_error(ret);
													}
												break;
											}
										break;

									}
									break;

								case WMHI_RAWKEY:
									switch(result & WMHI_GADGETMASK) {
										case RAWKEY_ESC:
											if(ask_quit()) {
												done = TRUE;
											}
										break;

										case RAWKEY_RETURN:
											ret = extract(NULL, NULL);
											if(ret != 0) show_error(ret);
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
														open_archive_req(FALSE);
													break;
												
													case 2: //info
														switch(archiver) {
															case ARC_XAD:
																xad_show_arc_info();
															break;
															case ARC_XFD:
																xfd_show_arc_info();
															break;
														}
													break;

													case 3: //about
														show_about();
													break;
												
													case 5: //quit
														if(ask_quit()) {
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
														
														SetGadgetAttrs(gadgets[GID_LIST], windows[WID_MAIN], NULL,
																LISTBROWSER_Hierarchical, h_browser, TAG_DONE);

														open_archive_req(TRUE);
													break;
													
													case 2: //save window position
														save_win_posn = !save_win_posn;
													break;

													case 3: //save window position
														confirmquit = !confirmquit;
													break;

													case 4: //ignore fs
														ignorefs = !ignorefs;
														open_archive_req(TRUE);
													break;

													case 6: //save settings
														savesettings(objects[OID_MAIN]);
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
			if(cx_broker && cx_mp) UnregisterCx(cx_broker, cx_mp);
		}

		if(lbci) FreeLBColumnInfo(lbci);
		RemoveAppWindow(appwin);
		if(appwin_mp) DeleteMsgPort(appwin_mp);
		DeleteMsgPort(AppPort);
	}
}

static void gettooltypes(UBYTE **tooltypes)
{
	char *s;

	s = ArgString(tooltypes, "CX_POPUP", "YES");
	if(MatchToolValue(s, "NO")) {
		cx_popup = FALSE;
	}

	cx_pri = ArgInt(tooltypes, "CX_PRIORITY", 0);

	dest = strdup(ArgString(tooltypes, "DEST", "RAM:"));
	dest_needs_free = TRUE;

	if(tmpdir) strncpy(tmpdir, ArgString(tooltypes, "TMPDIR", "T:"), 90);

	if(FindToolType(tooltypes, "HBROWSER")) h_browser = TRUE;
	if(FindToolType(tooltypes, "VIRUSSCAN")) virus_scan = TRUE;
	if(FindToolType(tooltypes, "SAVEWINPOSN")) save_win_posn = TRUE;
	if(FindToolType(tooltypes, "CONFIRMQUIT")) confirmquit = TRUE;
	if(FindToolType(tooltypes, "IGNOREFS")) ignorefs = TRUE;
	if(FindToolType(tooltypes, "DEBUG")) debug = TRUE;

	progress_size = ArgInt(tooltypes, "PROGRESSSIZE", PROGRESS_SIZE_DEFAULT);

	win_x = ArgInt(tooltypes, "WINX", 0);
	win_y = ArgInt(tooltypes, "WINY", 0);
	win_w = ArgInt(tooltypes, "WINW", 0);
	win_h = ArgInt(tooltypes, "WINH", 0);
}

static void fill_menu_labels(void)
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
	menu[11].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[12].nm_Label = locale_get_string( MSG_SCANFORVIRUSES );
	menu[13].nm_Label = locale_get_string( MSG_HIERARCHICALBROWSEREXPERIMENTAL );
	menu[14].nm_Label = locale_get_string( MSG_SAVEWINDOWPOSITION );
	menu[15].nm_Label = locale_get_string( MSG_CONFIRMQUIT );
	menu[16].nm_Label = locale_get_string( MSG_IGNOREFILESYSTEMS );
	menu[18].nm_Label = locale_get_string( MSG_SAVESETTINGS );
}

/** Main program **/
int main(int argc, char **argv)
{
	char *tmp = NULL;

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

	tmpdir = AllocVec(100, MEMF_CLEAR);
	NewMinList(&deletelist);
	
	UBYTE **tooltypes = ArgArrayInit(argc, argv);
	gettooltypes(tooltypes);
	ArgArrayDone();

	Locale_Open("avalanche.catalog", 0, 0);

	fill_menu_labels();

	if(tmp = AllocVec(20, MEMF_CLEAR)) {
		sprintf(tmp, "Avalanche.%lx", GetUniqueID());
		AddPart(tmpdir, tmp, 100);
		UnLock(CreateDir(tmpdir));
		FreeVec(tmp);
	}

	gui();

	Locale_Close();

	delete_delete_list();
	DeleteFile(tmpdir);

	if(tmpdir) FreeVec(tmpdir);
	if(progname != NULL) FreeVec(progname);
	if(archive_needs_free) free_archive_path();
	if(dest_needs_free) free_dest_path();
	
	FreeListBrowserList(&lblist);
	xad_exit();
	xfd_exit();
	libs_close();

	return 0;
}
