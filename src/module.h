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
	const char *(*get_filename)(void *userdata, struct Node *tab_node);
	const LONG *(*get_crunchsize)(void *userdata, struct Node *tab_node);
	BOOL (*is_crypted)(void *userdata, struct Node *tab_node);
	const char *(*get_format)(struct Node *tab_node);
	const char *(*get_subformat)(struct Node *tab_node);
	const char *(*get_error)(struct Node *tab_node, long code);
	void (*free)(struct Node *tab_node);

	/* Editing */
	BOOL (*add)(void *awin, const char *archive, char *file, char *dir, const char *root); /* Returns TRUE on success */
	BOOL (*del)(void *awin, const char *archive, char **files, ULONG count); /* Returns TRUE on success */
};

/* Virus scan */
ULONG module_vscan(void *awin, char *file, UBYTE *buf, ULONG len, BOOL delete);

/* Extraction */
const char *module_get_item_filename(struct Node *tab_node, void *userdata);
LONG *module_get_crunched_size(struct Node *tab_node, void *userdata);
void module_free(struct Node *tab_node);
const char *module_get_format(struct Node *tab_node);
const char *module_get_subformat(struct Node *tab_node);
const char *module_get_read_module(struct Node *tab_node);
const char *module_get_error(struct Node *tab_node, long code);
long module_extract(void *awin, void *tab_node, void *node, void *archive, void *newdest);
long module_extract_array(void *awin, void *tab_node, void **array, ULONG total_items, void *dest);
void module_exit(void);
BOOL module_recog(void* fullfilename);
BOOL module_is_crypted(struct Node *tab_node, void *userdata);

/* Editing */
BOOL module_has_add(struct Node *tab_node);
BOOL module_has_del(struct Node *tab_node);
BOOL module_add(void *awin, struct Node *tab_node, char *file, char *dir, const char *root); /* Returns TRUE on success */
BOOL module_del(void *awin, struct Node *tab_node, char **files, ULONG count); /* Returns TRUE on success */

/* Create new */
BOOL mod_lha_new(void *awin, char *archive);
BOOL mod_zip_new(void *awin, char *archive);

/*** Register modules ***/
void module_register(struct Node *tab_node);

/*** Register extended modules ***/
void mod_zip_register(struct module_functions *funcs);
void mod_lha_register(struct module_functions *funcs);

/*** Get version info ***/
void mod_zip_get_ver(ULONG *ver, ULONG *rev);
BOOL mod_lha_get_ver(ULONG *ver, ULONG *rev); /* Returns FALSE=Original, TRUE=Thor */
#endif
