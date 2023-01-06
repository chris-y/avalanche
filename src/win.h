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

#ifndef WIN_H
#define WIN_H 1

#include <exec/types.h>

#include "avalanche.h"

struct MsgPort;

/* Basic window functions */
void *window_create(struct avalanche_config *config, char *archive, struct MsgPort *winport, struct MsgPort *appport);
void window_open(void *awin, struct MsgPort *appwin_mp);
void window_close(void *awin, BOOL iconify);
void window_dispose(void *awin);

/* Update window */
void window_update_archive(void *awin, char *archive);
void window_update_sourcedir(void *awin, char *sourcedir);
void window_toggle_hbrowser(void *awin, BOOL h_browser);
void window_fuelgauge_update(void *awin, ULONG size, ULONG total_size);
void window_modify_all_list(void *awin, ULONG select);

/* Handle events */
void window_list_handle(void *awin, char *tmpdir);
void window_req_open_archive(void *awin, struct avalanche_config *config, BOOL refresh_only);
char *window_req_dest(void *awin);
ULONG window_handle_input(void *awin, UWORD *code);
ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code);

/* Get info */
void *window_get_window(void *awin);
Object *window_get_object(void *awin);
void *window_get_lbnode(void *awin, struct Node *node);
struct List *window_get_lblist(void *awin);
ULONG window_get_archiver(void *awin);

/* Misc */
void window_disable_gadgets(void *awin, BOOL disable);
BOOL check_abort(void *awin);
void window_reset_count(void *awin);
void fill_menu_labels(void);

/* Archiver userdata */
void *window_get_archive_userdata(void *awin);
void window_set_archive_userdata(void *awin, void *userdata);
void *window_alloc_archive_userdata(void *awin, ULONG size);
void window_free_archive_userdata(void *awin);
#endif
