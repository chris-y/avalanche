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

const char *xfd_get_filename(void *userdata);
BOOL xfd_recog(char *file);
void xfd_show_arc_info(void);
long xfd_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata));
long xfd_extract(char *file, char *dest, ULONG (scan)(char *file, UBYTE *buf, ULONG len));
void xfd_exit(void);
#endif
