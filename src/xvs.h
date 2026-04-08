/* Avalanche
 * (c) 2022-5 Chris Young
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

#ifndef XVS_H
#define XVS_H 1

ULONG xvs_get_ver(ULONG *ver, ULONG *rev);
long xvs_scan(char *file, BOOL delete, void *awin);
long xvs_scan_buffer(UBYTE *buf, ULONG len, void *awin);
#endif
