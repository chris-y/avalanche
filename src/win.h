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

/* Basic window functions */
void *win_create(struct avalanche_config *config, struct MsgPort *winport, struct MsgPort *appport);
void window_open(void *awin, struct MsgPort *appwin_mp);
void window_close(void *awin, BOOL iconify);
void window_dispose(void *awin);

/* Update window */
void window_update_archive(void *awin, char *archive);

/* Handle events */
void window_list_handle(void *awin);
void window_req_open_archive(void *awin, BOOL refresh_only);
char *window_req_dest(void *awin);

/* Get info */
Object *window_get_object(void *awin);

/* Misc */
void fill_menu_labels(void);

#endif
