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

#ifndef GLYPH_H
#define GLYPH_H 1

#include <images/glyph.h>

enum {
	AVALANCHE_GLYPH_ROOT = 70, /* System Glyph numbers end at ~36 */
	AVALANCHE_GLYPH_POPFILE, /* Distinguish between list and full size */
	AVALANCHE_GLYPH_OPENDRAWER,
	AVALANCHE_GLYPH_NONE,
	AVALANCHE_GLYPH_EXTRACT,
	AVALANCHE_GLYPH_MAX
};


Object *glyph_get(ULONG glyph);
void glyph_init(void);
void glyph_free(void);


#endif
