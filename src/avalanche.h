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

#ifndef AVALANCHE_H
#define AVALANCHE_H 1

#include <intuition/classusr.h>

struct avalanche_config;

#define PROGRESS_SIZE_DEFAULT 20

#ifndef __amigaos4__
	#ifndef MEMF_PRIVATE
		#define MEMF_PRIVATE 0L
	#endif
#endif

#ifdef __amigaos4__
#define BUTTON_TextPadding TAG_Ignore
#endif

extern const ULONG zero;

#define ARC_NONE  0
#define ARC_XAD   1
#define ARC_XFD   2
#define ARC_DEARK 4

enum {
	WIN_DONE_OK = 0,
	WIN_DONE_CLOSED,
	WIN_DONE_QUIT,
};

struct avalanche_config *get_config(void);
ULONG ask_quit(void *awin);
void savesettings(Object *win);
long extract(void *awin, char *archive, char *newdest, struct Node *node);
void free_dest_path(void);

/* window list */
void add_to_window_list(void *awin);
void del_from_window_list(void *awin);

#endif
