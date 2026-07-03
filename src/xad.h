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

#ifndef XAD_H
#define XAD_H 1

#include <utility/date.h>

#include "avalanche.h"

void xad_exit(void);
ULONG xad_get_ver(ULONG *ver, ULONG *rev);
ULONG xad_get_filedate(void *xfi, struct ClockData *cd, struct Node *tab_node);
ULONG xad_get_fileprotection(void *xfi, struct Node *tab_node);
const char *xad_get_comment(void *xfi, struct Node *tab_node);
const char *xad_get_link(void *xfi, struct Node *tab_node);
BOOL xad_is_link(void *userdata, struct Node *tab_node);
BOOL xad_is_disk(struct Node *tab_node); /* disk image (no fs) */
BOOL xad_is_diskfile(struct Node *tab_node); /* disk archive (fs) */
BOOL xad_recog(char *file);
long xad_info(const char *file, struct avalanche_config *config, void *awin, struct Node *tab_node, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin, struct Node *tab_node));

BOOL xad_arc_is_crypted(struct Node *tab_node);
BOOL xad_arc_is_corrupt(struct Node *tab_node);

long xad_extract(void *awin, struct Node *tab_node, const char *file, const char *dest, struct List *list, void *(getnode)(void *awin, struct Node *node));
long xad_extract_file(void *awin, struct Node *tab_node, const char *file, const char *dest, struct Node *node, void *(getnode)(void *awin, struct Node *node), ULONG *pud);
long xad_extract_array(void *awin, struct Node *tab_node, ULONG total_items, const char *dest, void **array, void *(getuserdata)(void *awin, void *arc_entry));

void xad_register(struct module_functions *funcs);
#endif
