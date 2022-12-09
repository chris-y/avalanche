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

#ifndef MODULE_H
#define MODULE_H

#include <exec/types.h>

const char *module_get_item_filename(void *awin, void *userdata);
void module_free(void *awin);
void module_show_info(void *awin);
const char *module_get_format(void *awin);
long module_extract(void *awin, void *node, void *archive, void *newdest);
void module_exit(void);
BOOL module_recog(void* fullfilename);
#endif
