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

#ifndef XFD_H
#define XFD_H 1

void xfd_free(void *awin);
const char *xfd_get_filename(void *userdata);
BOOL xfd_recog(char *file);
const char *xfd_get_arc_format(void *awin);
long xfd_info(char *file, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, void *awin));
long xfd_extract(void *awin, char *file, char *dest, ULONG (scan)(void *awin, char *file, UBYTE *buf, ULONG len));
void xfd_exit(void);
const char *xfd_error(long code);
#endif
