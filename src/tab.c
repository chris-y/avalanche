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

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <proto/clicktab.h>
#include <proto/listbrowser.h>

#include <gadgets/clicktab.h>
#include <gadgets/listbrowser.h>

#include "avalanche.h"
#include "tab.h"

struct avalanche_tab {
	void *awin;
	struct List lblist;
	struct List dir_tree;
#if 0 /* items targetted for moving */
	char *archive;
	char *dest;
	struct MinList deletelist;
	struct arc_entries **arc_array;
	struct arc_entries **dir_array;
	ULONG dir_tree_size;
	ULONG current_item;
	ULONG total_items;
	ULONG total_selectable;
	BOOL archive_needs_free;
	void *archive_userdata;
	BOOL disabled;
	BOOL abort_requested;
	struct Node *root_node;
	struct module_functions mf;
	char *current_dir;
#endif
};


static struct avalanche_tab *tab_get_tab(struct Node *tab_node)
{
	struct avalanche_tab *at;

	GetClickTabNodeAttrs(tab_node,
		TNA_UserData, &at,
		TAG_DONE);

	return at;
}

struct Node *tab_create(void *awin, struct List *tab_list)
{
	struct avalanche_tab *at = AllocVec(sizeof(struct avalanche_tab), MEMF_CLEAR | MEMF_PRIVATE);

	at->awin = awin;

	struct Node *tab_node = AllocClickTabNode(TNA_Text, "Avalanche",
						TNA_Number, 0,
						TNA_UserData, at,
						TNA_CloseGadget, TRUE,
						TAG_DONE);
	AddTail(tab_list, tab_node);

	NewList(&at->lblist);
	NewList(&at->dir_tree);

	return tab_node;
}

BOOL tab_close(struct Node *tab_node)
{
	if(tab_node == NULL) return FALSE;
	
	struct avalanche_tab *at = tab_get_tab(tab_node);

	Remove(tab_node);
	FreeClickTabNode(tab_node);

	FreeListBrowserList(&at->lblist);
	FreeListBrowserList(&at->dir_tree);

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

