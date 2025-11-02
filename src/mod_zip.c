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

#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <exec/types.h>

#ifdef __amigaos4__
#include <proto/zip.h>
#include <libraries/zip.h>
#endif

#include "libs.h"
#include "locale.h"
#include "module.h"
#include "req.h"

#include "Avalanche_rev.h"

#ifdef __amigaos4__
static void mod_zip_show_error(void *awin, zip_t *zip)
{
	open_error_req(zip_error_strerror(zip_get_error(zip)), locale_get_string(MSG_OK), awin);
}

static BOOL mod_zip_del(void *awin, char *archive, char **files, ULONG count)
{
	int err = 0;
	zip_t *zip = zip_open(archive, 0, &err);

	if(zip) {
		for(int i = 0; i < count; i++) {
			zip_int64_t index = zip_name_locate(zip, files[i], 0);
			if(index == -1) {
				mod_zip_show_error(awin, zip);
				zip_discard(zip);
				return FALSE;
			}

			err = zip_delete(zip, index);
			if(err == -1) {
				mod_zip_show_error(awin, zip);
				zip_discard(zip);
				return FALSE;
			}
		}

		err = zip_close(zip);
		if(err == -1) {
			mod_zip_show_error(awin, zip);
			zip_discard(zip);
			return FALSE;
		}
		return TRUE;
	} else {
		open_error_req(zip_error_strerror(&err), locale_get_string(MSG_OK), awin);
	}

	return FALSE;
}

static BOOL mod_zip_add_file(void *awin, zip_t *zip, char *file, char *dir, BOOL new, const char *root)
{
	int err = 0;
	char *fullfile = NULL;
	zip_source_t *src = NULL;

	if(new == FALSE) {
		src = zip_source_file(zip, file, 0, -1);
		if(src == NULL) {
			mod_zip_show_error(awin, zip);
			zip_discard(zip);
			return FALSE;
		}
	} else {
		src = zip_source_buffer(zip, new_arc_text, strlen(new_arc_text), 0);
		if(src == NULL) {
			mod_zip_show_error(awin, zip);
			zip_discard(zip);
			return FALSE;
		}
	}

	if(dir != NULL) {
		if(root != NULL) {
			ULONG fullfile_len = strlen(file) - strlen(root) + strlen(dir) + 2;
			fullfile = AllocVec(fullfile_len, MEMF_CLEAR);

			if(fullfile) {
				strncpy(fullfile, dir, fullfile_len);
				if((root[strlen(root)-1] != '/') && (root[strlen(root)-1] != ':')) {
					/* Skip past the slash or colon so AddPart doesn't think we want a different dir */
					AddPart(fullfile, file + strlen(root) + 1, fullfile_len);
				} else {
					AddPart(fullfile, file + strlen(root), fullfile_len);
				}
			}
		} else {
			ULONG fullfile_len = strlen(FilePart(file)) + strlen(dir) + 2;
			fullfile = AllocVec(fullfile_len, MEMF_CLEAR);

			if(fullfile) {
				strncpy(fullfile, dir, fullfile_len);
				AddPart(fullfile, FilePart(file), fullfile_len);
			}
		}
	} else {
		if(root != NULL) {
			if((root[strlen(root)-1] != '/') && (root[strlen(root)-1] != ':')) {
				/* Skip past the slash or colon so AddPart doesn't think we want a different dir */
					fullfile = file + strlen(root) + 1;
				} else {
					fullfile = file + strlen(root);
				}
		} else {
			fullfile = FilePart(file);
		}
	}

	err = zip_file_add(zip, fullfile, src, 0);

	if(dir && fullfile) FreeVec(fullfile);

	if(err == -1) {
		mod_zip_show_error(awin, zip);
		zip_discard(zip);
		return FALSE;
	}

	err = zip_close(zip);
	if(err == -1) {
		mod_zip_show_error(awin, zip);
		zip_discard(zip);
		return FALSE;
	}
	return TRUE;
}

static BOOL mod_zip_add(void *awin, char *archive, char *file, char *dir, const char *root)
{
	int err = 0;
	zip_t *zip = zip_open(archive, 0, &err);

	if(zip) {
		return mod_zip_add_file(awin, zip, file, dir, FALSE, root);
	} else {
		open_error_req(zip_error_strerror(&err), locale_get_string(MSG_OK), awin);
	}

	return FALSE;
}
#endif

ULONG mod_zip_get_ver(ULONG *ver, ULONG *rev)
{
#ifdef __amigaos4__
	if(!libs_zip_init()) return 0;
	
	struct Library *lib = (struct Library *)ZipBase;
	if(ver) *ver = lib->lib_Version;
	if(rev) *rev = lib->lib_Revision;
	
	return *ver;
#else
	*ver = 0;
	*rev = 0;
	return 0;
#endif
}

BOOL mod_zip_new(void *awin, char *archive)
{
#ifdef __amigaos4__
	int err = 0;

	if(!libs_zip_init()) return FALSE;

	zip_t *zip = zip_open(archive, ZIP_CREATE, &err);

	if(zip) {
		return mod_zip_add_file(awin, zip, NEW_ARC_NAME, NULL, TRUE, NULL);
	} else {
		open_error_req(zip_error_strerror(&err), locale_get_string(MSG_OK), awin);
	}
#endif
	return FALSE;
}

void mod_zip_register(struct module_functions *funcs)
{
#ifdef __amigaos4__
	if(libs_zip_init()) {
		funcs->add = mod_zip_add;
		funcs->del = mod_zip_del;
	}
#endif
}
