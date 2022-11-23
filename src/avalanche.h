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

#ifndef AVALANCHE_H
#define AVALANCHE_H 1

#include <intuition/classusr.h>

#ifndef MEMF_PRIVATE
#define MEMF_PRIVATE 0L
#endif

enum {
	ARC_NONE = 0,
	ARC_XAD,
	ARC_XFD
};

enum {
	WIN_DONE_OK = 0,
	WIN_DONE_CLOSED,
	WIN_DONE_QUIT
};

struct avalanche_config {
	char *progname;
	
	char *dest; /* default destination */
	char *tmpdir;
	int tmpdirlen;

	BOOL save_win_posn;
	BOOL h_browser;
	BOOL virus_scan;
	BOOL debug;
	BOOL confirmquit;
	BOOL ignorefs;

	ULONG win_x;
	ULONG win_y;
	ULONG win_w;
	ULONG win_h;
	ULONG progress_size;

	int cx_pri;
	BOOL cx_popup;
	char *cx_popkey;
};

char *strdup(const char *s);
struct avalanche_config *get_config(void);
ULONG ask_quit(void *awin);
void savesettings(Object *win);
long extract(void *awin, char *archive, char *newdest, struct Node *node);
void set_arexx_archive(char *arc);

/* window list */
void add_to_window_list(void *awin);
void del_from_window_list(void *awin);

#ifndef __amigaos4__
#define IsMinListEmpty(L) (L)->mlh_Head->mln_Succ == 0
#endif

#endif
