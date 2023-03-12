/* Avalanche
 * (c) 2022-3 Chris Young
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

#ifndef REQ_H
#define REQ_H 1

#include <exec/types.h>

ULONG ask_question(void *awin, char *q, char *f);
ULONG ask_password(void *awin, char *pw, ULONG pwlen);
ULONG ask_quit_req(void *awin);
ULONG ask_quithide_req(void);
void show_error(long code, void *awin);
void show_about(void *awin);
void open_error_req(char *message, char *button, void *awin);
void open_info_req(char *message, char *buttons, void *awin);
void req_show_arc_info(void *awin);
#endif
