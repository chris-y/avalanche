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

#ifndef MEMF_PRIVATE
#define MEMF_PRIVATE 0L
#endif

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

BOOL check_abort(void);
void *get_window(void);
char *strdup(const char *s);
void fuelgauge_update(ULONG size, ULONG total_size);
#endif
