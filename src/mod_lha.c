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

#include <stdio.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dostags.h>
#include <exec/types.h>

#include "avalanche.h"
#include "config.h"
#include "locale.h"
#include "module.h"
#include "req.h"

#ifdef __amigaos4__
#define DeleteFile Delete
#endif

static int mod_lha_error(void *awin, int err, char *file)
{
	char msg[100];

	snprintf(msg, 100, locale_get_string(MSG_LHAERROR), file);
	return open_error_req(msg, locale_get_string(MSG_SKIPRETRYABORT), awin);
}

static BOOL mod_lha_del(void *awin, char *archive, char **files, ULONG count)
{
	int err;
	int user_choice;
	char cmd[1024];
	
	for(int i = 0; i < count; i++) {
		snprintf(cmd, 1024, "lha -I d \"%s\" \"%s\"", archive, files[i]);

		err = SystemTags(cmd,
					SYS_Input, NULL,
					SYS_Output, NULL,
					SYS_Error, NULL,
					NP_Name, "Avalanche LhA Delete process",
					TAG_DONE);

		if(err != 0) {
			user_choice = mod_lha_error(awin, err, files[i]);
			
			switch(user_choice) {
				case 0: // abort
					return FALSE;
				break;
				
				case 1: // skip
				break;
				
				case 2: // retry
					i--;
				break;
			}
		}
	}

	return TRUE;
}

/* Add files to LhA archive
 * dir - directory user is currently in within the archive
 * root - root of directory where the files are being added from
 */
static BOOL mod_lha_add(void *awin, char *archive, char *file, char *dir, const char *root)
{
	int err;
	char cmd[1024];

	if(root == NULL) {
		// single file
		snprintf(cmd, 1024, "lha -I a \"%s\" \"%s\"", archive, file);

	} else {
		/* Ensure HOMEDIR ends with a colon or a slash */
		if((root[strlen(root)-1] != '/') && (root[strlen(root)-1] != ':')) {
			snprintf(cmd, 1024, "lha -I -x a \"%s\" \"%s/\" \"%s\"", archive, root, file+strlen(root)+1);
		} else {
			snprintf(cmd, 1024, "lha -I -x a \"%s\" \"%s\" \"%s\"", archive, root, file+strlen(root));
		}
	}

	//printf("%s\n", cmd);

	err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, NULL,
				SYS_Error, NULL,
				//NP_CurrentDir, /* if homedir doesn't work */
				NP_Name, "Avalanche LhA Add process",
				TAG_DONE);

	if(err != 0) {
		int user_choice = mod_lha_error(awin, err, file);

		switch(user_choice) {
			case 0: // abort
				return FALSE;
			break;

			case 1: // skip
			break;
			
			case 2: // retry
				return mod_lha_add(awin, archive, file, dir, root);
			break;
		}
	}

	return TRUE;
}

BOOL mod_lha_new(void *awin, char *archive)
{
	BOOL ret = FALSE;
	ULONG new_arc_size = strlen(CONFIG_GET_LOCK(tmpdir)) + strlen(NEW_ARC_NAME) + 2;
	char *tmpfile = AllocVec(new_arc_size, MEMF_CLEAR);

	if(tmpfile) {
		BPTR fh = 0;
		strcpy(tmpfile, CONFIG_GET(tmpdir));
		CONFIG_UNLOCK;
		AddPart(tmpfile, NEW_ARC_NAME, new_arc_size);

		if(fh = Open(tmpfile, MODE_NEWFILE)) {
			FPuts(fh, new_arc_text);
			Close(fh);

			ret = mod_lha_add(awin, archive, tmpfile, NULL, NULL);

			DeleteFile(tmpfile);
		}
	} else {
		CONFIG_UNLOCK;
	}
	return ret;
}

void mod_lha_register(struct module_functions *funcs)
{
	funcs->add = mod_lha_add;
	funcs->del = mod_lha_del;
}
