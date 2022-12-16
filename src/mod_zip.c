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

#include <proto/dos.h>

#include <exec/types.h>

#include "lib/zipc/zipc.h"

#include "locale.h"
#include "module.h"
#include "req.h"

static void mod_zip_show_error(void *awin, zipc_file_t *zip)
{
	open_error_req(zipcError(zip), locale_get_string(MSG_OK), awin);
}

static BOOL mod_zip_add(void *awin, char *archive, char *file)
{
	int err = 0;
	zipc_file_t *zip = zipcOpen(archive, "w");

	if(zip) {
		err = zipcCopyFile(zip, FilePart(file), file, 0, 1);
		if(err != 0) {
			mod_zip_show_error(awin, zip);
			zipcClose(zip);
			return FALSE;
		}

		err = zipcClose(zip);
		if(err != 0) {
			mod_zip_show_error(awin, zip);
			zipcClose(zip);
			return FALSE;
		}
		return TRUE;
	} else {
		open_error_req(locale_get_string(MSG_UNABLETOOPENZIP), locale_get_string(MSG_OK), awin);
	}

	return FALSE;
}

void mod_zip_register(struct module_functions *funcs)
{
	funcs->add = mod_zip_add;
//	funcs->del = mod_zip_del;
}
