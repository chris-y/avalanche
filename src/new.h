/* Avalanche
 * (c) 2023 Chris Young
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

#ifndef AVALANCHE_NEW_H
#define AVALANCHE_NEW_H 1

void newarc_window_open(void *awin);
ULONG newarc_window_get_signal(void);
ULONG newarc_window_handle_input(UWORD *code);
BOOL newarc_window_handle_input_events(ULONG result, UWORD code);
void newarc_window_close_if_associated(void *awin);
#endif
