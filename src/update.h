/* Avalanche
 * (c) 2025 Chris Young
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

#ifndef AVALANCHE_UPDATE_H
struct avalanche_version_numbers {
	const char *name;
        const char *check_url;
        const char *download_url;
        ULONG current_version;
        ULONG current_revision;
        ULONG latest_version;
        ULONG latest_revision;
        BOOL update_available;
};

enum {
        ACHECKVER_AVALANCHE = 0,
        ACHECKVER_XAD,
        ACHECKVER_XFD,
        ACHECKVER_XVS,
        ACHECKVER_DEARK,
#ifdef __amigaos4__
        ACHECKVER_ZIP,
#endif
        ACHECKVER_MAX
};


void update_gui(struct avalanche_version_numbers avn[], void *ssl_ctx);

#endif
