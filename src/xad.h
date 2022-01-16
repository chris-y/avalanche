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

#ifndef XAD_H
#define XAD_H 1

long xad_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, void *userdata));
long xad_extract(char *file, char *dest, struct List *list, void *(getnode)(struct Node *node));
char *xad_error(long code);
void xad_exit(void);
#endif
