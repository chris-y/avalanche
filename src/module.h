/* Avalanche
 * (c) 2022-5 Chris Young
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

/* For new archive creation */
#include "Avalanche_rev.h"
extern char *new_arc_text;
#define NEW_ARC_NAME ".readme"

struct module_functions {
	/* Extraction */
	char module[4];
	const char *(*get_filename)(void *userdata, void *awin);
	const LONG *(*get_crunchsize)(void *userdata, void *awin);
	const char *(*get_format)(void *awin);
	const char *(*get_subformat)(void *awin);
	const char *(*get_error)(void *awin, long code);
	void (*free)(void *awin);

	/* Editing */
	BOOL (*add)(void *awin, char *archive, char *file, char *dir, const char *root); /* Returns TRUE on success */
	BOOL (*del)(void *awin, char *archive, char **files, ULONG count); /* Returns TRUE on success */
};

/* Virus scan */
ULONG module_vscan(void *awin, char *file, UBYTE *buf, ULONG len, BOOL delete);

/* Extraction */
const char *module_get_item_filename(void *awin, void *userdata);
LONG *module_get_crunched_size(void *awin, void *userdata);
void module_free(void *awin);
const char *module_get_format(void *awin);
const char *module_get_subformat(void *awin);
const char *module_get_read_module(void *awin);
const char *module_get_error(void *awin, long code);
long module_extract(void *awin, void *node, void *archive, void *newdest);
long module_extract_array(void *awin, void **array, ULONG total_items, void *dest);
void module_exit(void);
BOOL module_recog(void* fullfilename);

/* Editing */
BOOL module_has_add(void *awin);

/* Create new */
BOOL mod_lha_new(void *awin, char *archive);
BOOL mod_zip_new(void *awin, char *archive);

/*** Register modules ***/
void module_register(void *awin, struct module_functions *mf);

/*** Register extended modules ***/
void mod_zip_register(struct module_functions *funcs);
void mod_lha_register(struct module_functions *funcs);

/*** Get version info ***/
ULONG mod_zip_get_ver(ULONG *ver, ULONG *rev);
ULONG mod_lha_get_ver(ULONG *ver, ULONG *rev);
#endif
