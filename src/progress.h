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

/* Set progress bar values for full archive progress */
void progress_set_archive_level(struct Window *win, void *gauge, void *frame, ULONG current, ULONG total);

/* Set progress bar scanning progress */
void progress_set_scanning(struct Window *win, void *gauge, void *frame, ULONG total);

/* Set progress bar values for single file progress */
void progress_set_file_level(struct Window *win, void *gauge, void *frame, ULONG level, ULONG max);
#endif
