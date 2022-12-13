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

#include "modules/zip.h"

#include "locale.h"
#include "module.h"
#include "req.h"

static void mod_zip_show_error(void *awin, long err)
{
	open_error_req(zip_strerror(err), locale_get_string(MSG_OK), awin);
}

static BOOL mod_zip_add(void *awin, char *archive, char *file)
{
	long err = 0;
	struct zip_t *zip = zip_open(archive, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');

	if(zip) {
		err = zip_entry_open(zip, FilePart(file));
		if(err < 0) {
			mod_zip_show_error(awin, err);
			zip_close(zip);
			return FALSE;
		}
		err = zip_entry_fwrite(zip, file);
		if(err < 0) {
			mod_zip_show_error(awin, err);
			zip_close(zip);
			return FALSE;
		}
		err = zip_entry_close(zip);
		if(err < 0) {
			mod_zip_show_error(awin, err);
			zip_close(zip);
			return FALSE;
		}

		zip_close(zip);
		return TRUE;
	}
	
	return FALSE;
}

static BOOL mod_zip_del(void *awin, char *archive, char *file)
{
	long err = 0;
	struct zip_t *zip = zip_open(archive, 0, 'd');

	if(zip) {
		err = zip_entries_delete(zip, &file, 1);
		if(err < 0) {
			mod_zip_show_error(awin, err);
			zip_close(zip);
			return FALSE;
		}

		zip_close(zip);
		return TRUE;
	}

	return FALSE;
}

void mod_zip_register(struct module_functions *funcs)
{
	funcs->add = mod_zip_add;
	funcs->del = mod_zip_del;
}
