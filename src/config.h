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

#ifndef AVALANCHE_CONFIG_H
#define AVALANCHE_CONFIG_H 1

#include <proto/exec.h>

struct avalanche_config {
	struct SignalSemaphore semaphore;

	char *progname;

	char *sourcedir; /* default source dir for ASL */
	char *dest; /* default destination */
	char *tmpdir;
	int tmpdirlen;

	BOOL disable_asl_hook;
	ULONG viewmode; /* [1 = list, 0 = browser] */
	BOOL virus_scan;
	BOOL debug;
	BOOL ignorefs;
	BOOL disable_vscan_menu;
	BOOL drag_lock;
	BOOL no_dropzones;
	BOOL aiss;
	BOOL openwb; /* open on WB after extract */
	ULONG closeaction; /* [1 = quit, 2 = hide, 0 = cancel] -- same as quit req */
	ULONG activemodules;

	ULONG win_x;
	ULONG win_y;
	ULONG win_w;
	ULONG win_h;
	ULONG progress_size;

	int cx_pri;
	BOOL cx_popup;
	char *cx_popkey;

	void *iconify_icon;
};

/* Call CONFIG_UNLOCK for every CONFIG_GET_LOCK */
#define CONFIG_LOCK ObtainSemaphoreShared((struct SignalSemaphore *)get_config())
#define CONFIG_LOCK_EX ObtainSemaphore((struct SignalSemaphore *)get_config())
#define CONFIG_GET(ATTRIBUTE) get_config()->ATTRIBUTE
#define CONFIG_GET_LOCK(ATTRIBUTE) config_get()->ATTRIBUTE
#define CONFIG_UNLOCK ReleaseSemaphore((struct SignalSemaphore *)get_config())

void config_window_open(struct avalanche_config *config);
void config_window_break(void);

/* Obtains the avalanche_config structure if it is safe to do so.
 * CONFIG_UNLOCK or ReleaseSemaphore() MUST be called when done.
 * If obtaining a lock manually, use get_config() instead */
struct avalanche_config *config_get(void);
#endif
