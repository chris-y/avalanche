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

#ifndef REQ_H
#define REQ_H 1

ULONG ask_question(char *q, char *f);
ULONG ask_password(char *pw, ULONG pwlen);
ULONG ask_quit(void);
void show_error(long code);
void show_about(void);
void open_error_req(char *message, char *button);
void open_info_req(char *message, char *buttons);
#endif
