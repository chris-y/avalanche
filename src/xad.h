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

#ifndef XAD_H
#define XAD_H 1

#include <utility/date.h>

#include "avalanche.h"

void xad_exit(void);
ULONG get_xad_ver(void);
ULONG xad_get_filedate(void *xfi, struct ClockData *cd, void *awin);
BOOL xad_recog(char *file);
long xad_info(char *file, struct avalanche_config *config, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin));
long xad_extract(void *awin, char *file, char *dest, struct List *list, void *(getnode)(void *awin, struct Node *node));
long xad_extract_file(void *awin, char *file, char *dest, struct Node *node, void *(getnode)(void *awin, struct Node *node), ULONG *pud);

void xad_register(struct module_functions *funcs);
#endif
