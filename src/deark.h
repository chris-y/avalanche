/* Avalanche
 * (c) 2023 Chris Young
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

#ifndef DEARK_H
#define DEARK_H 1

#include "avalanche.h"

BOOL deark_recog(char *file);

long deark_info(char *file, struct avalanche_config *config, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin));
long deark_extract_array(void *awin, ULONG total_items, char *dest, void **array, void *(getuserdata)(void *awin, void *arc_entry));
long deark_extract(void *awin, char *file, char *dest, struct List *list, void *(getnode)(void *awin, struct Node *node));
long deark_extract_file(void *awin, char *file, char *dest, struct Node *node, void *(getnode)(void *awin, struct Node *node));


void deark_register(struct module_functions *funcs);
#endif
