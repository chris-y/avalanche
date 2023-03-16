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

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dostags.h>
#include <exec/types.h>

#include "locale.h"
#include "module.h"
#include "req.h"

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

static BOOL mod_lha_add(void *awin, char *archive, char *file, char *dir)
{
	int err;
	char cmd[1024];
	snprintf(cmd, 1024, "lha -I a \"%s\" \"%s\"", archive, file);
#ifdef __amigaos4__
	//DebugPrintF("%s\n",cmd);
#endif
	err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, NULL,
				SYS_Error, NULL,
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
				return mod_lha_add(awin, archive, file);
			break;
		}
	}

	return TRUE;
}

void mod_lha_register(struct module_functions *funcs)
{
	funcs->add = mod_lha_add;
	funcs->del = mod_lha_del;
}
