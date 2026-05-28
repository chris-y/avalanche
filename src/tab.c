/* Avalanche
 * (c) 2026 Chris Young
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

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>

#include <proto/clicktab.h>
#include <proto/listbrowser.h>

#include <gadgets/clicktab.h>
#include <gadgets/listbrowser.h>

#include <images/label.h>

#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "config.h"
#include "glyph.h"
#include "libs.h"
#include "misc.h"
#include "module.h"
#include "tab.h"

#include <string.h>

struct arc_entries;

struct avalanche_tab {
	void *awin;
	struct List lblist;
	struct List dir_tree;
	char *archive;
	char *dest;
	ULONG format;
	struct arc_entries **arc_array;
	struct arc_entries **dir_array;
	ULONG dir_tree_size;
	ULONG total_items;
	ULONG current_item;
	ULONG total_selectable;
	char *current_dir;
	struct Node *root_node;
	void *archive_userdata;
	struct module_functions mf;
	struct MinList deletelist;
	BOOL disabled;
	BOOL abort_requested;
	BYTE process_exit_sig;
};

static struct avalanche_tab *tab_get_tab(struct Node *tab_node)
{
	struct avalanche_tab *at;

	GetClickTabNodeAttrs(tab_node,
		TNA_UserData, &at,
		TAG_DONE);

	return at;
}

static void tab_free_dir_array(struct avalanche_tab *at)
{
	if((at->dir_array == NULL) || (at->dir_tree_size == 0)) return;

	for(int i = 0; i < at->dir_tree_size; i++) {
		if(at->dir_array[i]) {
			if(at->dir_array[i]->name) FreeVec(at->dir_array[i]->name);
			FreeVec(at->dir_array[i]);
			at->dir_array[i] = NULL;
		}
	}
	FreeVec(at->dir_array);
	at->dir_array = NULL;
	at->dir_tree_size = 0;
}

static void tab_free_arc_array(struct avalanche_tab *at)
{
	if((at->arc_array == NULL) || (at->total_items == 0)) return;
	
	for(int i = 0; i < at->total_items; i++) {
		FreeVec(at->arc_array[i]);
	}
	FreeVec(at->arc_array);
	at->arc_array = NULL;
	
	tab_free_dir_array(at);

	at->total_selectable = 0;
	at->total_items = 0;
	at->current_item = 0;
}

static void tab_delete_delete_list(struct avalanche_tab *at)
{
	struct Node *node;
	struct Node *nnode;

	if(IsMinListEmpty((struct MinList *)&at->deletelist) == FALSE) {
		node = (struct Node *)GetHead((struct List *)&at->deletelist);

		do {
			nnode = (struct Node *)GetSucc((struct Node *)node);
			Remove((struct Node *)node);
			if(node->ln_Name) {
				DeleteFile(node->ln_Name);
				FreeVec(node->ln_Name);
			}
			FreeVec(node);
		} while((node = nnode));
	}
}

void tab_add_to_delete_list(struct Node *tab_node, char *fn)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	char *tmpdir = CONFIG_GET_LOCK(tmpdir);

	/* Ensure we're only deleting things in our temp dir */
	if(strncmp(tmpdir, fn, strlen(tmpdir)) != 0) {
		CONFIG_UNLOCK;
		return;
	}

	CONFIG_UNLOCK;

	struct Node *node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
	if(node) {
		node->ln_Name = strdup_vec(fn);
		AddTail((struct List *)&at->deletelist, (struct Node *)node);
	}
}

const char *tab_get_archive(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	return (const char *)at->archive;
}

const char *tab_get_dest(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const char *)at->dest;
}

const char *tab_get_current_dir(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const char *)at->current_dir;
}

const ULONG tab_get_format(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const ULONG)at->format;
}

const ULONG tab_get_current_item(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const ULONG)at->current_item;
}

const ULONG tab_get_total_items(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const ULONG)at->total_items;
}

const ULONG tab_get_total_selectable(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const ULONG)at->total_selectable;
}

const ULONG tab_get_dir_tree_size(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return (const ULONG)at->dir_tree_size;
}

struct List *tab_get_listbrowser_list(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return &at->lblist;
}

struct List *tab_get_dirtree_list(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return &at->dir_tree;
}

void *tab_get_window(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return at->awin;
}

const BOOL tab_get_disabled(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return at->disabled;
}

struct module_functions *tab_get_module_funcs(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

        return &at->mf;
}

struct arc_entries **tab_alloc_dir_array(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	tab_free_dir_array(at);

	at->dir_tree_size = at->total_items * 2; /* temp set to large value */
		
	at->dir_array = AllocVec(sizeof(struct arc_entries *) * at->dir_tree_size, MEMF_CLEAR);

	return (const struct arc_entries **)at->dir_array;
}

struct Node *tab_dir_add_root_node(struct Node *tab_node, ULONG glyph, ULONG dir_entries)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	ULONG flags = LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN;
	if(dir_entries == 0) flags = 0;
	
	FreeListBrowserList(&at->dir_tree);

	at->root_node = AllocListBrowserNode(1,
									LBNA_UserData, NULL,
									LBNA_Flags, flags,
									LBNA_Generation, 1,
									LBNA_Column, 0,
										LBNCA_Image, LabelObj,
											LABEL_DisposeImage, FALSE,
											LABEL_Image, glyph_get(glyph),
											LABEL_Underscore, NULL,
											LABEL_Text, " ",
											LABEL_Text, FilePart(at->archive),
										LabelEnd,
									TAG_DONE);

	AddTail(&at->dir_tree, at->root_node);

	return at->root_node;
}

struct Node *tab_dir_find_current_node(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	struct Node *cur_node = NULL;

	if(at->current_dir == NULL) {
		cur_node = at->root_node;
	} else {
		struct List *list = &at->dir_tree;
		struct Node *node;
		char *userdata = NULL;
		char *cur_dir = strdup_vec(at->current_dir);

		if(cur_dir) cur_dir[strlen(cur_dir) - 1] = '\0';

		for(node = list->lh_Head; node->ln_Succ; node=node->ln_Succ) {
			GetListBrowserNodeAttrs(node, LBNA_UserData, &userdata, TAG_DONE);

			if((userdata) && (strcmp(cur_dir, userdata) == 0)) {
				cur_node = node;
				break;
			}
		}
		if(cur_dir) FreeVec(cur_dir);
	}
	
	return cur_node;
}

struct arc_entries **tab_get_arc_array(struct Node *tab_node, ULONG alloc_new)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	if(alloc_new > 0) {
		if(at->arc_array) tab_free_arc_array(at);
		at->arc_array = AllocVec(alloc_new * sizeof(struct arc_entries *), MEMF_CLEAR);
	}
	
	return (const struct arc_entries **)at->arc_array;
}

struct arc_entries *tab_get_arc_entry(struct Node *tab_node, ULONG entry)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	if(at->arc_array == NULL) return NULL;

	if(at->arc_array[entry] == NULL) {
		at->arc_array[entry] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
	}

	return (struct arc_entries *)at->arc_array[entry];
}

struct arc_entries *tab_get_dir_entry(struct Node *tab_node, ULONG entry, BOOL alloc_new)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	if(at->dir_array == NULL) return NULL;

	if((at->dir_array[entry] == NULL) && (alloc_new)) {
		at->dir_array[entry] = AllocVec(sizeof(struct arc_entries), MEMF_CLEAR);
	}

	return (struct arc_entries *)at->dir_array[entry];
}

void tab_set_archive(struct Node *tab_node, const char *archive)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	/* Free old archive */
	if(at->archive) FreeVec(at->archive);
	at->archive = NULL;

	/* Alloc new archive */
	if(archive) at->archive = strdup_vec(archive);
}

void tab_set_dest(struct Node *tab_node, const char *dest)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	/* Free old dest */
	if(at->dest) FreeVec(at->dest);
	at->dest = NULL;

	/* Alloc new dest */
	if(dest) at->dest = strdup_vec(dest);
}

void tab_set_format(struct Node *tab_node, ULONG format)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->format = format;
}

void tab_set_current_item(struct Node *tab_node, ULONG item)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->current_item = item;
}

void tab_set_total_items(struct Node *tab_node, ULONG total)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->total_items = total;
}

void tab_set_total_selectable(struct Node *tab_node, ULONG total)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->total_selectable = total;
}

void tab_set_dir_tree_size(struct Node *tab_node, ULONG size)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->dir_tree_size = size;
}

BOOL tab_set_current_dir_to_parent(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	if(at->current_dir == NULL) return FALSE;
	
	at->current_dir[strlen(at->current_dir) - 1] = '\0';

	char *slash = strrchr(at->current_dir, '/');

	if(slash == NULL) {
		FreeVec(at->current_dir);
		at->current_dir = NULL;
	} else {
		*(slash+1) = '\0';
	}
	
	return TRUE;
}

void tab_set_current_dir(struct Node *tab_node, const char *dir)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	/* Free old dir */
	if(at->current_dir) FreeVec(at->current_dir);
	at->current_dir = NULL;

	/* Alloc new archive */
	if(dir) at->current_dir = strdup_vec(dir);
}

void tab_set_disabled(struct Node *tab_node, BOOL disable)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	at->disabled = disable;
	
	if(disable) {
		/* TODO: set tab flag */
	} else {
		/* Clear the state of the Abort flag */
		at->abort_requested = FALSE;
	}
}

void *tab_get_archive_userdata(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	return at->archive_userdata;
}

void tab_set_archive_userdata(struct Node *tab_node, void *userdata)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->archive_userdata = userdata;
}

void *tab_alloc_archive_userdata(struct Node *tab_node, ULONG size)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	at->archive_userdata = AllocVec(size, MEMF_CLEAR | MEMF_PRIVATE);
        return at->archive_userdata;
}

void tab_free_archive_userdata(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	if(at->archive_userdata) {
		FreeVec(at->archive_userdata);
		at->archive_userdata = NULL;
	}
}

const ULONG tab_get_signal(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	return 1 << at->process_exit_sig;
}

void tab_signal_clear(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	SetSignal(0L, 1 << at->process_exit_sig);
}

void tab_signal_wait(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	Wait(1 << at->process_exit_sig);
}

void tab_signal_signal(const ULONG sig, struct Task *process)
{
	Signal(process, sig);
}

struct Node *tab_create(void *awin, struct List *tab_list)
{
	struct avalanche_tab *at = AllocVec(sizeof(struct avalanche_tab), MEMF_CLEAR | MEMF_PRIVATE);

	/* create process signal
	 * TODO: use less signals!
	 */
	if((at->process_exit_sig = AllocSignal(-1)) == -1) {
		FreeVec(at);
		return NULL;
	}

	at->awin = awin;
	at->archive = NULL;

	/* Create tab node.
	 * Currently we put the tab structure into userdata but
	 * maybe it's more efficient to extend the ClickTabNode? */
	struct Node *tab_node = AllocClickTabNode(TNA_Text, "Avalanche",
						TNA_Number, 0,
						TNA_UserData, at,
						TNA_CloseGadget, TRUE,
						TAG_DONE);
	AddTail(tab_list, tab_node);

	/* Initialise the archive browser lists and the delete list */
	NewList(&at->lblist);
	NewList(&at->dir_tree);
	NewMinList(&at->deletelist);

	/* Set local dest */
	tab_set_dest(tab_node, CONFIG_GET_LOCK(dest));
        CONFIG_UNLOCK;

	return tab_node;
}

void tab_abort(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);
	
	if(at->disabled) at->abort_requested = TRUE;
}

/* Check if abort button is pressed - only called from xad hook */
const BOOL tab_check_abort(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	/* This flag is set in the main loop
	 * if ESC is pressed or Abort is clicked */
	return at->abort_requested;
}

void tab_reset(struct Node *tab_node)
{
	struct avalanche_tab *at = tab_get_tab(tab_node);

	FreeListBrowserList(&at->dir_tree);
	FreeListBrowserList(&at->lblist);
	tab_free_arc_array(at);
	
	module_free(tab_node);
}

BOOL tab_close(struct Node *tab_node)
{
	if(tab_node == NULL) return FALSE;
	
	struct avalanche_tab *at = tab_get_tab(tab_node);

	if((at->disabled == TRUE)) {
		//SetSignal(0L, aw->process_exit_sig);
		at->abort_requested = TRUE;
		Wait(at->process_exit_sig);
	}

	Remove(tab_node);
	FreeClickTabNode(tab_node);

	FreeListBrowserList(&at->lblist);
	FreeListBrowserList(&at->dir_tree);

	tab_free_arc_array(at);
	tab_free_archive_userdata(tab_node);

	tab_set_archive(tab_node, NULL);
	tab_set_dest(tab_node, NULL);

	/* Delete items in the delete list */
	tab_delete_delete_list(at);
	
	/* Release archive when tab is closed */
	module_free(tab_node);

	FreeSignal(at->process_exit_sig);
	
	FreeVec(at);

	return TRUE; /* return TRUE if last tab closed */
}

void tab_close_all(struct List *tab_list)
{
	struct Node *fnode = NULL;

	for(fnode = tab_list->lh_Head; fnode->ln_Succ; fnode=fnode->ln_Succ) {
		tab_close(fnode);
	}
}

