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

#include <stdio.h>
#include <stdlib.h>
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

static int mod_zip_error(void *awin, int err, char *file)
{
	char msg[100];

	snprintf(msg, 100, locale_get_string(MSG_LHAERROR), file); /* This is intentionally MSG_LHAERROR */
	return open_error_req(msg, locale_get_string(MSG_SKIPRETRYABORT), awin);
}

static BOOL mod_zip_del(void *awin, const char *archive, char **files, ULONG count)
{
	int user_choice;
	char cmd[1024];
	
	for(int i = 0; i < count; i++) {
		snprintf(cmd, 1024, "zip -qd \"%s\" \"%s\"", archive, files[i]);

		int err = SystemTags(cmd,
					SYS_Input, NULL,
					SYS_Output, NULL,
					SYS_Error, NULL,
					NP_Name, "Avalanche Zip Delete process",
					TAG_DONE);

		if(err != 0) {
			user_choice = mod_zip_error(awin, err, files[i]);
			
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

/* Add files to Zip archive
 * dir - directory user is currently in within the archive
 * root - root of directory where the files are being added from
 */
static BOOL mod_zip_add(void *awin, const char *archive, char *file, char *dir, const char *root)
{
	int err;
	char cmd[1024];

	snprintf(cmd, 1024, "zip -qNj \"%s\" \"%s\"", archive, file);

	//printf("%s\n", cmd);

	err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, NULL,
				SYS_Error, NULL,
				//NP_CurrentDir, root,
				NP_Name, "Avalanche Zip Add process",
				TAG_DONE);

	if(err != 0) {
		int user_choice = mod_zip_error(awin, err, file);

		switch(user_choice) {
			case 0: // abort
				return FALSE;
			break;

			case 1: // skip
			break;
			
			case 2: // retry
				return mod_zip_add(awin, archive, file, dir, root);
			break;
		}
	}

	return TRUE;
}

BOOL mod_zip_new(void *awin, char *archive)
{
	BOOL ret = FALSE;
	ULONG new_arc_size = strlen(CONFIG_GET_LOCK(tmpdir)) + strlen(NEW_ARC_NAME) + 2;
	char *tmpfile = AllocVec(new_arc_size, MEMF_CLEAR);

	if(tmpfile) {
		BPTR fh = 0;
		strncpy(tmpfile, CONFIG_GET(tmpdir), new_arc_size);
		AddPart(tmpfile, NEW_ARC_NAME, new_arc_size);

		if(fh = Open(tmpfile, MODE_NEWFILE)) {
			FPuts(fh, new_arc_text);
			Close(fh);

			ret = mod_zip_add(awin, archive, tmpfile, NULL, CONFIG_GET(tmpdir));

			DeleteFile(tmpfile);
		}

	}
	
	CONFIG_UNLOCK;		
	return ret;
}

#ifdef __amigaos4__
#define AVALANCHE_ZIP_VER_CMD "version zip"
#else
#define AVALANCHE_ZIP_VER_CMD "version c:zip"
#endif

void mod_zip_get_ver(ULONG *ver, ULONG *rev)
{
	BPTR fh = 0;
	BPTR thor = FALSE;
	
	CONFIG_LOCK;
	ULONG tmpfile_len = CONFIG_GET(tmpdirlen) + 30;
	char *tmpfile = AllocVec(tmpfile_len, MEMF_CLEAR);
	if(tmpfile == NULL) {
		CONFIG_UNLOCK;
		return 0;
	}
	strncpy(tmpfile, CONFIG_GET(tmpdir), tmpfile_len);
	AddPart(tmpfile, "zip_tmp", tmpfile_len);
	CONFIG_UNLOCK;

	if(fh = Open(tmpfile, MODE_NEWFILE)) {
		ULONG err = SystemTags(AVALANCHE_ZIP_VER_CMD,
				SYS_Input, NULL,
				SYS_Output, fh,
				SYS_Error, NULL,
				NP_Name, "Avalanche Zip version process",
				TAG_DONE);

		Close(fh);
		
	}
	
	char buf[20];
	char *dot = NULL;
	char *p = buf;
			
	if(fh = Open(tmpfile, MODE_OLDFILE)) {
		char *res = (char *)&buf;
		while(res != NULL) {
			res = FGets(fh, buf, 20);

			if(strncmp(p, "Zip ", 4) == 0) {
				p += 4;
				break;
			}
		}

		*ver = strtol(p, &dot, 10);
		*rev = strtol(dot + 1, NULL, 10);
			
		Close(fh);
		DeleteFile(tmpfile);
	}

	FreeVec(tmpfile);
}

void mod_zip_register(struct module_functions *funcs)
{
	funcs->add = mod_zip_add;
	funcs->del = mod_zip_del;
}
