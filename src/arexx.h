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

#ifndef AREXX_H
#define AREXX_H

#include <exec/types.h>

#include <stdbool.h>

enum {
	RXEVT_NONE = 0,
	RXEVT_OPEN,
	RXEVT_SHOW
};

bool ami_arexx_init(ULONG *rxsig);
void ami_arexx_handle(void);
void ami_arexx_send(const char *);
void ami_arexx_cleanup(void);

char *arexx_get_event(void);
void arexx_free_event(void);
#endif
