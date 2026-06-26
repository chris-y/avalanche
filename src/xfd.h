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

#ifndef XFD_H
#define XFD_H 1

ULONG xfd_get_ver(ULONG *ver, ULONG *rev);
void xfd_exit(void);
BOOL xfd_recog(char *file);
long xfd_info(const char *file, void *awin, struct Node *tab_node, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin, struct Node *tab_node));
long xfd_extract(void *awin, struct Node *tab_node, const char *file, const char *dest);

void xfd_register(struct module_functions *funcs);

#endif
