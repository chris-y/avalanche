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

#ifndef WIN_H
#define WIN_H 1

#include <exec/types.h>
#include <intuition/intuition.h>
#include <workbench/startup.h>

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
void window_fuelgauge_update(void *awin, struct Node *tab_node, ULONG size, ULONG total_size, const char *filename);
void window_modify_all_list(void *awin, ULONG select);

/* Handle events */
void window_req_open_archive(void *awin, struct avalanche_config *config, BOOL refresh_only);
const char *window_req_dest(void *awin);
ULONG window_handle_input(void *awin, UWORD *code);
ULONG window_handle_input_events(void *awin, struct avalanche_config *config, ULONG result, struct MsgPort *appwin_mp, UWORD code, struct MsgPort *winport, struct MsgPort *AppPort);

/* Get info */
void *window_get_window(void *awin);
Object *window_get_object(void *awin);
void *window_get_lbnode(void *awin, struct Node *node);
struct List *window_get_lblist(void *awin);
void *array_get_userdata(void *awin, void *arc_entry);
BOOL window_get_disabled(void *awin);
BYTE window_get_exit_sig(void *awin);
const char *window_req_new_lha(void *awin, char *drawer);
struct Node *window_get_current_tab(void *awin);
BOOL window_module_has_add(void *awin);

/* Modify archive */
BOOL window_edit_add_wbarg(void *awin, struct WBArg *wbarg);
BOOL window_edit_add(void *awin, char *file, char *root);

/* Misc */
BOOL check_abort(void *awin);
#ifndef __amigaos4__
BOOL check_closetab(void *awin);
#endif
void fill_menu_labels(void);
long extract(void *awin, const char *archive, const char *newdest, struct Node *node);
void add_to_delete_list(void *awin, char *fn);
void window_update_fuelgauge_text(void *awin, struct Node *tab_node);

#endif
