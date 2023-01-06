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

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/types.h>

#include "module.h"

static BOOL mod_lha_del(void *awin, char *archive, char **files, ULONG count)
{
	int err;
	char cmd[1024];
	
	for(int i = 0; i < count; i++) {
		snprintf(cmd, 1024, "lha -I d \"%s\" \"%s\"", archive, files[i]);

		err = SystemTags(cmd,
					SYS_Input, NULL,
					SYS_Output, NULL,
//					SYS_Error, NULL,
					NP_Name, "Avalanche LhA Delete process",
					TAG_DONE);

		if(err == -1) return FALSE;
	}

	return TRUE;
}

static BOOL mod_lha_add(void *awin, char *archive, char *file)
{
	int err;
	char cmd[1024];
	snprintf(cmd, 1024, "lha -I a \"%s\" \"%s\"", archive, file);

	err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, NULL,
//				SYS_Error, NULL,
				NP_Name, "Avalanche LhA Add process",
				TAG_DONE);
	
	if(err == -1) return FALSE;
	
	return TRUE;
}

void mod_lha_register(struct module_functions *funcs)
{
	funcs->add = mod_lha_add;
	funcs->del = mod_lha_del;
}
