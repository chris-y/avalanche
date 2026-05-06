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

#ifndef AVALANCHE_PROGRESS_H
#define AVALANCHE_PROGRESS_H 1

void progress_set_text(struct Window *win, void *gauge, ULONG current, ULONG total);
void progress_set_scanning(struct Window *win, void *gauge, ULONG total);
void progress_set_level(struct Window *win, void *gauge, ULONG level, ULONG max);
#endif
