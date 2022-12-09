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

#include "avalanche.h"
#include "config.h"
#include "module.h"
#include "win.h"

#include "xad.h"
#include "xfd.h"
#include "xvs.h"

/*** Virus Scanning ***/
static ULONG module_vscan(void *awin, char *file, UBYTE *buf, ULONG len)
{
	long res = 0;
	struct avalanche_config *config = get_config();

	if(config->virus_scan) {
		if(buf == NULL) {
			res = xvs_scan(file, len, awin);
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
	const char *fn = NULL;

	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			fn = xad_get_filename(userdata, awin);
		break;
		case ARC_XFD:
			fn = xfd_get_filename(userdata);
		break;
	}

	return fn;
}

void module_free(void *awin)
{
		switch(window_get_archiver(awin)) {
		case ARC_XAD:
			xad_free(awin);
		break;

		case ARC_XFD:
			xfd_free(awin);
		break;
	}
}

void module_show_info(void *awin)
{
	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			xad_show_arc_info(awin);
		break;
		case ARC_XFD:
			xfd_show_arc_info(awin);
		break;
	}
}

const char *module_get_format(void *awin)
{
	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			return xad_get_arc_format(awin);
		break;
		case ARC_XFD:
			return xfd_get_arc_format(awin);
		break;
	}

	return NULL;
}

long module_extract(void *awin, void *node, void *archive, void *newdest)
{
	long ret = 0;

	switch(window_get_archiver(awin)) {
		case ARC_XAD:
			if(node == NULL) {
				ret = xad_extract(awin, archive, newdest, window_get_lblist(awin), window_get_lbnode, module_vscan);
			} else {
				ULONG pud = 0;
				ret = xad_extract_file(awin, archive, newdest, node, window_get_lbnode, module_vscan, &pud);
			}
		break;
		case ARC_XFD:
			ret = xfd_extract(awin, archive, newdest, module_vscan);
		break;
	}

	return ret;
}

void module_exit(void)
{
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
