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

#include <string.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "module.h"
#include "win.h"

#include "xad.h"
#include "xfd.h"
#include "xvs.h"

/*** Virus Scanning ***/
ULONG module_vscan(void *awin, char *file, UBYTE *buf, ULONG len, BOOL delete)
{
	long res = 0;
	struct avalanche_config *config = get_config();

	if(config->virus_scan) {
		if(buf == NULL) {
			res = xvs_scan(file, delete, awin);
		} else {
			res = xvs_scan_buffer(buf, len, awin);
		}

		if((res == -1) || (res == -3)) {
			config->virus_scan = FALSE;
			config->disable_vscan_menu = TRUE;
			
		}
	}

	return res;
}

/*** Extraction ***/
const char *module_get_item_filename(void *awin, void *userdata)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	if(mf->get_filename) return mf->get_filename(userdata, awin);

	return NULL;
}

void module_free(void *awin)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	if(mf->free) mf->free(awin);
}

const char *module_get_format(void *awin)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	if(mf->get_format) return mf->get_format(awin);

	return NULL;
}

const char *module_get_subformat(void *awin)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	if(mf->get_subformat) return mf->get_subformat(awin);

	return NULL;
}

const char *module_get_error(void *awin, long code)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	if(mf->get_error) return mf->get_error(code);

	return NULL;
}

const char *module_get_read_module(void *awin)
{
	struct module_functions *mf = window_get_module_funcs(awin);

	return mf->module;
}

long module_extract(void *awin, void *node, void *archive, void *newdest)
{
	long ret = 0;

	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			if(node == NULL) {
				ret = xad_extract(awin, archive, newdest, window_get_lblist(awin), window_get_lbnode);
			} else {
				ULONG pud = 0;
				ret = xad_extract_file(awin, archive, newdest, node, window_get_lbnode, &pud);
			}
		break;
		case ARC_XFD:
			ret = xfd_extract(awin, archive, newdest);
		break;
	}

	return ret;
}

long module_extract_array(void *awin, void **array, ULONG total_items, void *dest)
{
	long ret = 0;
	
	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			ret = xad_extract_array(awin, total_items, dest, array, array_get_userdata);
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

static void module_extract_register(void *awin, struct module_functions *mf)
{
	/* Remove existing registration */
	mf->module[0] = 'N';
	mf->module[1] = '/';
	mf->module[2] = 'A';
	mf->module[3] = 0;

	mf->get_filename = NULL;
	mf->get_format = NULL;
	mf->get_subformat = NULL;
	mf->get_error = NULL;
	mf->free = NULL;

	/* Register correct module */
	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			xad_register(mf);
		break;
		case ARC_XFD:
			xfd_register(mf);
		break;
	}
}

/*** Editing ***/

BOOL module_has_add(void *awin)
{
	struct module_functions *mf = window_get_module_funcs(awin);
	
	if(mf->add != NULL) return TRUE;
	return FALSE;
}

void module_register(void *awin, struct module_functions *mf)
{
	module_extract_register(awin, mf);

	const char *format = module_get_format(awin);

	/* TODO: close libs if possible, see module_close() */

	/* Remove existing registration */
	mf->add = NULL;
	mf->del = NULL;
	
	/* Register correct module */
	if(format && (strcmp(format, "Zip") == 0)) mod_zip_register(mf);
	if(format && (strcmp(format, "LhA") == 0)) mod_lha_register(mf);
}
