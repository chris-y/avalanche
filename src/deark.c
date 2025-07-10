/* Avalanche
 * (c) 2023-5 Chris Young
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
#include <proto/exec.h>

#include <dos/dostags.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "misc.h"
#include "module.h"
#include "win.h"
#include "deark.h"

#ifdef __amigaos4__
#define Seek ChangeFilePosition
#endif

enum {
	DEARK_RECOG,
	DEARK_LIST,
	DEARK_EXTRACT,
	DEARK_VERSION
};

struct deark_userdata {
	char *file;
	char *tmpfile;
	char **list;
	long total;
	char *arctype;
	char *last_error;
};

static void free_list(char **list, long items)
{
	for(int i = 0; i < items; i++) {
		if(list[i]) FreeVec(list[i]);
	}	

	if(list) FreeVec(list);
}

static void deark_free(void *awin)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du) {
		if(du->tmpfile) FreeVec(du->tmpfile);
		if(du->file) free(du->file);
		if(du->arctype) free(du->arctype);
		if(du->last_error) free(du->last_error);

		if(du->list) free_list(du->list, du->total);
	}

	window_free_archive_userdata(awin);
}

static const char *deark_error(void *awin, long code)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du == NULL) return NULL;

	return(du->last_error);
}

static long deark_send_command(void *awin, char *file, int command, char ***list, char *dest, long index)
{
	BPTR fh = 0;
	int err = 1;
	char cmd[1024];

	struct avalanche_config *config = get_config();
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du == NULL) return -1;

	if(du->tmpfile == NULL) {
		du->tmpfile = AllocVec(config->tmpdirlen + 25, MEMF_CLEAR);
		if(du->tmpfile == NULL) return 0;

		strcpy(du->tmpfile, config->tmpdir);
		AddPart(du->tmpfile, "deark_tmp", config->tmpdirlen + 25);
	}

	switch(command) {
		case DEARK_LIST:
			snprintf(cmd, 1024, "deark -l -q \"%s\"", file);
		break;

		case DEARK_RECOG:
			snprintf(cmd, 1024, "deark -id \"%s\"", file);
		break;

		case DEARK_EXTRACT:
			snprintf(cmd, 1024, "deark -get %d -od \"%s\" -q \"%s\"", index, dest, file);
		break;

		default:
			return -1;
		break;
	}

	if(fh = Open(du->tmpfile, MODE_NEWFILE)) {
		err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, fh,
				SYS_Error, NULL,
				NP_Name, "Avalanche Deark process",
				TAG_DONE);

		Close(fh);
	}

//	if(err == 0) {
		char *res;
		char buf[200];
		ULONG i = 0;
		ULONG total = 0;

		if(fh = Open(du->tmpfile, MODE_OLDFILE)) {
			res = (char *)&buf;
			while(res != NULL) {
				res = FGets(fh, buf, 200);
				if(strncmp(buf, "Error: ", 7) == 0) {
					if(du->last_error) free(du->last_error);
					du->last_error = strdup(buf);
					Close(fh);
					if(!config->debug) DeleteFile(du->tmpfile);
					return -1;
				}
				if(res) total++;
			}

			*list = AllocVec(total, MEMF_CLEAR);

			Seek(fh, 0, OFFSET_BEGINNING);

			res = (char *)&buf;
			for(i = 0; i < total; i++) {
				res = FGets(fh, buf, 200);

				buf[strlen(buf) - 1] = '\0';

				*list[i] = AllocVec(strlen(buf), MEMF_CLEAR);
				strcpy(*list[i], buf);

			}

			Close(fh);
			if(!config->debug) DeleteFile(du->tmpfile);
			return(total);
		}

//	} else return err;

	return 0;
}

static const char *deark_get_arc_format(void *awin)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);
	if(!du) return NULL;

	return(du->arctype + 7);
}

static const char *deark_get_filename(void *userdata, void *awin)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);
	
	if(du) {
		long i = (long)userdata;
		return du->list[i];
	}
	
	return NULL;

}

long deark_info(char *file, struct avalanche_config *config, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin))
{
	int err = 1;
	BPTR fh = 0;
	struct deark_userdata *du = (struct deark_userdata *)window_alloc_archive_userdata(awin, sizeof(struct deark_userdata));
	if(du == NULL) return -1;

	du->file = strdup(file);

	char **list = NULL;
	long entries = deark_send_command(awin, file, DEARK_RECOG, &list, NULL, 0);
	if(entries > 0) {
		if(strncmp(list[0], "Module:", 7) == 0) {
			du->arctype = strdup(list[0]);
			free_list(list, entries);
		} else {
			return -1;
		}
	}

	du->total = deark_send_command(awin, file, DEARK_LIST, &du->list, NULL, 0);

	if(du->total > 0) {
		char *res;
		char buf[200];
		ULONG i = 0;

		/* Add to list */
		for(i = 0; i < du->total; i++) {
			addnode(du->list[i], &zero,
				FALSE, i, du->total, (void *)i, config, awin);
			i++;
		}

		return 0;
	}

	return err;
}

static long deark_extract_file_private(void *awin, char *dest, struct deark_userdata *du, long idx)
{
	char **list = NULL;
	long err = 1;
	long entries = 0;
	if(idx < 0) return err;
	entries = deark_send_command(awin, du->file, DEARK_EXTRACT, &list, dest, idx);
	if(entries >= 0) {
		if(list) free_list(list, entries);
		return 0;
	}
	return err;
}


long deark_extract_file(void *awin, char *file, char *dest, struct Node *node, void *(getnode)(void *awin, struct Node *node))
{
	long err;
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);
	long idx = (long)getnode(awin, node);
	
	return deark_extract_file_private(awin, dest, du, idx);
}

/* returns 0 on success */
long deark_extract(void *awin, char *file, char *dest, struct List *list, void *(getnode)(void *awin, struct Node *node))
{
	long err = 0;
	struct Node *fnode;

	struct desrk_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du) {
		for(fnode = list->lh_Head; fnode->ln_Succ; fnode=fnode->ln_Succ) {
			err = deark_extract_file(awin, file, dest, fnode, getnode);
			if(err != 0) {
				return err;
			}
		}
	}

	return err;
}

long deark_extract_array(void *awin, ULONG total_items, char *dest, void **array, void *(getuserdata)(void *awin, void *arc_entry))
{
	long err = 0;

	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du) {
		for(int i = 0; i < total_items; i++) {
			long idx = (long)getuserdata(awin, array[i]);
			err = deark_extract_file_private(awin, dest, du, idx);
			if(err != 0) {
				return err;
			}
		}
	}

	return err;
}

ULONG deark_get_ver(ULONG *ver, ULONG *rev)
{
	BPTR fh = 0;
	int err = 1;
	char cmd[1024];

	struct avalanche_config *config = get_config();
	char *tmpfile = AllocVec(config->tmpdirlen + 25, MEMF_CLEAR);
	if(tmpfile == NULL) return 0;

	strcpy(tmpfile, config->tmpdir);	
	AddPart(tmpfile, "deark_tmp", config->tmpdirlen + 25);

	snprintf(cmd, 1024, "deark -version");
	
		if(fh = Open(tmpfile, MODE_NEWFILE)) {
		err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, fh,
				SYS_Error, NULL,
				NP_Name, "Avalanche get Deark version process",
				TAG_DONE);

		Close(fh);
	}

	char *res;
	char buf[200];
	char *dot = NULL;
	char *p = buf;
			
	if(fh = Open(tmpfile, MODE_OLDFILE)) {
		res = (char *)&buf;
		while(res != NULL) {
			res = FGets(fh, buf, 200);
			


			if(strncmp(p, "Deark version: ", 15) == 0) {
				p += 15;
				break;
			}
		}

		*ver = strtol(p, &dot, 10);
		*rev = strtol(dot + 1, NULL, 10);
			
		Close(fh);
		if(!config->debug) DeleteFile(tmpfile);
	}

	return *ver;
}

void deark_register(struct module_functions *funcs)
{
	funcs->module[0] = 'A';
	funcs->module[1] = 'R';
	funcs->module[2] = 'K';
	funcs->module[3] = 0;

	funcs->get_filename = deark_get_filename;
	funcs->free = deark_free;
	funcs->get_format = deark_get_arc_format;
	funcs->get_subformat = NULL;
	funcs->get_error = deark_error;
	funcs->get_crunchsize = NULL;
}
