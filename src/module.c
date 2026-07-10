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

#include <string.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "module.h"
#include "tab.h"
#include "win.h"

#include "deark.h"
#include "xad.h"
#include "xfd.h"
#include "xvs.h"

char *new_arc_text = "Created with " VERS "\0";

/*** Virus Scanning ***/
ULONG module_vscan(void *awin, char *file, UBYTE *buf, ULONG len, BOOL delete)
{
	long res = 0;

	if(CONFIG_GET_LOCK(virus_scan)) {
		CONFIG_UNLOCK;
		if(buf == NULL) {
			res = xvs_scan(file, delete, awin);
		} else {
			res = xvs_scan_buffer(buf, len, awin);
		}

		if((res == -1) || (res == -3)) {
			CONFIG_LOCK_EX;
			struct avalanche_config *config = get_config();
			config->virus_scan = FALSE;
			config->disable_vscan_menu = TRUE;
			CONFIG_UNLOCK;
		}
	} else {
		CONFIG_UNLOCK;
	}

	return res;
}

/*** Extraction ***/
const char *module_get_item_filename(struct Node *tab_node, void *userdata)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->get_filename) return mf->get_filename(userdata, tab_node);

	return NULL;
}

LONG *module_get_crunched_size(struct Node *tab_node, void *userdata)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->get_crunchsize) return mf->get_crunchsize(userdata, tab_node);

	return NULL;
}

BOOL module_is_crypted(struct Node *tab_node, void *userdata)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->is_crypted) return mf->is_crypted(userdata, tab_node);

	return FALSE;
}

void module_free(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->free) mf->free(tab_node);
}

const char *module_get_format(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->get_format) return mf->get_format(tab_node);

	return NULL;
}

const char *module_get_subformat(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->get_subformat) return mf->get_subformat(tab_node);

	return NULL;
}

const char *module_get_error(struct Node *tab_node, long code)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->get_error) return mf->get_error(tab_node, code);

	return NULL;
}

const char *module_get_read_module(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	return mf->module;
}

long module_extract(void *awin, void *tab_node, void *node, void *archive, void *newdest)
{
	long ret = 0;

	switch(tab_get_format(tab_node)) {
		case ARC_XAD:
			if(node == NULL) {
				ret = xad_extract(awin, tab_node, archive, newdest, tab_get_listbrowser_list(tab_node), window_get_lbnode);
			} else {
				ULONG pud = 0;
				ret = xad_extract_file(awin, tab_node, archive, newdest, node, window_get_lbnode, &pud);
			}
		break;
		case ARC_XFD:
			ret = xfd_extract(awin, tab_node, archive, newdest);
		break;
		case ARC_DEARK:
			if(node == NULL) {
				ret = deark_extract(awin, tab_node, archive, newdest, tab_get_listbrowser_list(tab_node), window_get_lbnode);
			} else {
				ret = deark_extract_file(awin, tab_node, archive, newdest, node, window_get_lbnode);
			}
		break;
	}

	return ret;
}

long module_extract_array(void *awin, void *tab_node, void **array, ULONG total_items, void *dest)
{
	long ret = 0;
	
	switch(tab_get_format(tab_node)) {
		case ARC_XAD:
			ret = xad_extract_array(awin, tab_node, total_items, dest, array, array_get_userdata);
		break;
		case ARC_DEARK:
			ret = deark_extract_array(awin, tab_node, total_items, dest, array, array_get_userdata);
		break;
	}
	
	return ret;
}

void module_exit(void)
{
	/* Close libraries associated with all modules */
	/* TODO: This only happens at program exit,
	 * keep track of whether modules are being used
	 * and close the libs early. */
	xad_exit();
	xfd_exit();
}

BOOL module_recog(void* fullfilename)
{
	BOOL found = FALSE;
	found = xad_recog(fullfilename);
	if(found == FALSE) found = xfd_recog(fullfilename);

	return found;
}

static void module_extract_register(struct Node *tab_node, struct module_functions *mf)
{
	/* Remove existing registration */
	mf->module[0] = 'N';
	mf->module[1] = '/';
	mf->module[2] = 'A';
	mf->module[3] = 0;

	mf->get_filename = NULL;
	mf->get_crunchsize = NULL;
	mf->get_format = NULL;
	mf->get_subformat = NULL;
	mf->get_error = NULL;
	mf->free = NULL;

	/* Register correct module */
	switch(tab_get_format(tab_node)) {
		case ARC_XAD:
			xad_register(mf);
		break;
		case ARC_XFD:
			xfd_register(mf);
		break;
		case ARC_DEARK:
			deark_register(mf);
		break;
	}
}

/*** Editing ***/

BOOL module_has_add(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if((mf->add != NULL) && (tab_get_archive(tab_node) != AVALANCHE_SPLIT_ARCHIVE)) return TRUE;
	return FALSE;
}

BOOL module_has_del(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if((mf->del != NULL) && (tab_get_archive(tab_node) != AVALANCHE_SPLIT_ARCHIVE)) return TRUE;
	return FALSE;
}

BOOL module_add(void *awin, struct Node *tab_node, char *file, char *dir, const char *root)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->add == NULL) return FALSE;

	return mf->add(awin, tab_get_archive(tab_node), file, dir, root);
}

BOOL module_del(void *awin, struct Node *tab_node, char **files, ULONG count)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	if(mf->del == NULL) return FALSE;

	return mf->del(awin, tab_get_archive(tab_node), files, count);
}



void module_register(struct Node *tab_node)
{
	struct module_functions *mf = tab_get_module_funcs(tab_node);

	module_extract_register(tab_node, mf);

	const char *format = module_get_format(tab_node);

	/* TODO: close libs if possible, see module_close() */

	/* Remove existing registration */
	mf->add = NULL;
	mf->del = NULL;
	
	/* Register correct module */
	if(format && (strcmp(format, "Zip") == 0)) mod_zip_register(mf);
	if(format && (strcmp(format, "LhA") == 0)) mod_lha_register(mf);
}
