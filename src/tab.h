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

#ifndef AVALANCHE_TAB_H
#define AVALANCHE_TAB_H 1
#include <exec/lists.h>

struct arc_entries {
	char *name;
	ULONG *size;
	BOOL selected;
	BOOL dir;
	void *userdata;
	ULONG level;
};

struct Node *tab_create(void *awin, struct List *tab_list);

/* Close tab; returns TRUE if last tab */
BOOL tab_close(struct Node *tab_node);

/* Close all tabs */
void tab_close_all(struct List *tab_list);

/* Reset list etc */
void tab_reset(struct Node *tab_node);

/* Alloc the root node in the dir tree */
struct Node *tab_dir_add_root_node(struct Node *tab_node, ULONG glyph, ULONG dir_entries);

/** Setters **/

/* Set archive/dest, or NULL to free */
void tab_set_archive(struct Node *tab_node, const char *archive);
void tab_set_dest(struct Node *tab_node, const char *dest);
void tab_set_format(struct Node *tab_node, ULONG format);
void tab_set_current_item(struct Node *tab_node, ULONG item);
void tab_set_total_items(struct Node *tab_node, ULONG total);
void tab_set_total_selectable(struct Node *tab_node, ULONG total);
void tab_set_dir_tree_size(struct Node *tab_node, ULONG size);
void tab_set_current_dir(struct Node *tab_node, const char *dir);

/* returns FALSE if already at root, TRUE otherwise */
BOOL tab_set_current_dir_to_parent(struct Node *tab_node);

/** Getters **/
const char *tab_get_archive(struct Node *tab_node);
const char *tab_get_dest(struct Node *tab_node);
const char *tab_get_current_dir(struct Node *tab_node);
const ULONG tab_get_format(struct Node *tab_node);
const ULONG tab_get_current_item(struct Node *tab_node);
const ULONG tab_get_total_items(struct Node *tab_node);
const ULONG tab_get_total_selectable(struct Node *tab_node);
const ULONG tab_get_dir_tree_size(struct Node *tab_node);
struct List *tab_get_listbrowser_list(struct Node *tab_node);
struct List *tab_get_dirtree_list(struct Node *tab_node);

struct Node *tab_dir_find_current_node(struct Node *tab_node);

/* Get arc_array, allocate new of size alloc_new, if allow_new>0, otherwise return current */ 
struct arc_entries **tab_get_arc_array(struct Node *tab_node, ULONG alloc_new);

/* Dir array is always alloced afresh */
struct arc_entries **tab_alloc_dir_array(struct Node *tab_node);

/* Get arc entry from array, will alocate if not found */
struct arc_entries *tab_get_arc_entry(struct Node *tab_node, ULONG entry);
struct arc_entries *tab_get_dir_entry(struct Node *tab_node, ULONG entry);

/* Get the window the tab is in */
void *tab_get_window(struct Node *tab_node);

#endif

