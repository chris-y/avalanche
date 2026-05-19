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

struct Node *tab_create(void *awin, struct List *tab_list);

/* Close tab; returns TRUE if last tab */
BOOL tab_close(struct Node *tab_node);

/* Close all tabs */
void tab_close_all(struct List *tab_list);

struct List *tab_get_listbrowser_list(struct Node *tab_node);
struct List *tab_get_dirtree_list(struct Node *tab_node);

/* Get the window the tab is in */
void *tab_get_window(struct Node *tab_node);

#endif

