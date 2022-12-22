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

struct module_functions {
	/* TODO: Register extraction modules here too */
	BOOL (*add)(void *awin, char *archive, char *file); /* Returns TRUE on success */
	BOOL (*del)(void *awin, char *archive, char *file); /* Returns TRUE on success */
};

const char *module_get_item_filename(void *awin, void *userdata);
void module_free(void *awin);
void module_show_info(void *awin);
long module_extract(void *awin, void *node, void *archive, void *newdest);
void module_exit(void);
BOOL module_recog(void* fullfilename);
void module_register(void *awin, struct module_functions *mf);

/*** Register extended modules ***/
void mod_zip_register(struct module_functions *funcs);
void mod_lha_register(struct module_functions *funcs);

#endif
