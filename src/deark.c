/* Avalanche
 * (c) 2023 Chris Young
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

#include <stdio.h>
#include <string.h>

#include "avalanche.h"
#include "config.h"
#include "libs.h"
#include "locale.h"
#include "module.h"
#include "req.h"
#include "win.h"
#include "deark.h"

#ifdef __amigaos4__
#define Seek ChangeFilePosition
#endif

struct deark_userdata {
	char *tmpfile;
	char arctype[20];
};

static void deark_free(void *awin)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);

	if(du) {
		if(du->tmpfile) FreeVec(du->tmpfile);
	}

	window_free_archive_userdata(awin);
}

void deark_exit(void)
{
}

static const char *deark_error(long code)
{
	return NULL;
}

static const char *deark_get_arc_format(void *awin)
{
	struct deark_userdata *du = (struct deark_userdata *)window_get_archive_userdata(awin);
	if(!du) return NULL;

	return du->arctype;
}

#if 0
BOOL deark_recog(char *file)
{
	int err;
	char cmd[1024];
	struct avalanche_config *config = get_config();
	char *tmpfile = AllocVec(config->tmpdirlen + 10, MEMF_CLEAR);

	if(tmpfile == NULL) return FALSE;

	strncpy(tmpfile, config->tmpdir, config->tmpdirlen);
	AddPart(tmpfile, "deark_tmp", config->tmpdirlen + 10);

	snprintf(cmd, 1024, "deark -id \"%s\"", file);

	err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, tmpfile,
				SYS_Error, NULL,
				NP_Name, "Avalanche Deark process",
				TAG_DONE);

	if(err == 0) {
		ULONG len;
		char buf[7];
		BPTR fh = 0;
		if(fh = Open(tmpfile, MODE_OLDFILE)) {
			len = Read(fh, buf, 7);
			Close(fh);
		}

		// deletefile()

		if(strncmp(buf, "Module", 7) == 0) return TRUE;

	}

	return FALSE;
}
#endif

long deark_info(char *file, struct avalanche_config *config, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin))
{
	ULONG total = 0;
	int err = 1;
	BPTR fh = 0;
	char cmd[1024];

	struct deark_userdata *du = (struct deark_userdata *)window_alloc_archive_userdata(awin, sizeof(struct deark_userdata));
	if(du == NULL) return -2;

	du->tmpfile = AllocVec(config->tmpdirlen + 10, MEMF_CLEAR);

	if(du->tmpfile == NULL) return FALSE;

	strncpy(du->tmpfile, config->tmpdir, config->tmpdirlen);
	AddPart(du->tmpfile, "deark_tmp", config->tmpdirlen + 10);

	snprintf(cmd, 1024, "deark -l -q \"%s\"", file);

	if(fh = Open(du->tmpfile, MODE_NEWFILE)) {
		err = SystemTags(cmd,
				SYS_Input, NULL,
				SYS_Output, fh,
				SYS_Error, NULL,
				NP_Name, "Avalanche Deark process",
				TAG_DONE);

		Close(fh);
	}

	if(err == 0) {
		char *res;
		char buf[200];
		ULONG i = 0;
		ULONG zero = 0;

		if(fh = Open(du->tmpfile, MODE_OLDFILE)) {
			res = &buf;
			while(res != NULL) {
				res = FGets(fh, buf, 200);
				if(strncmp(buf, "Error: ", 7) == 0) {
					return -1;
				}
				total++;
			}

			Seek(fh, 0, OFFSET_BEGINNING);

			res = &buf;
			while(res != NULL) {
				res = FGets(fh, buf, 200);

				/* Add to list */
				addnode(buf, &zero,
						FALSE, i, total, NULL, config, awin);
				i++;

			}

			Close(fh);
		}

		// deletefile()

	}

	return err;
}

#if 0
static long xad_extract_file_private(void *awin, char *dest, struct xad_userdata *xu, struct xadDiskInfo *di, struct xadFileInfo *fi, ULONG *pud)
{
	long err = 0;
		
	struct DateStamp ds;
	char *fn = NULL;

	struct xadArchiveInfo *ai = xu->ai;

	struct xad_hookdata xhd;
	xhd.pud = pud;
	xhd.awin = awin;

	struct Hook progress_hook;
	progress_hook.h_Entry = xad_progress;
	progress_hook.h_SubEntry = NULL;
	progress_hook.h_Data = &xhd;


	if(fi || di) {
		char destfile[1024];
		strncpy(destfile, dest, 1023);
		destfile[1023] = 0;

		if(fi) {
			fn = fi->xfi_FileName;
		} else {
			fn = "disk.img";
		}

		if(AddPart(destfile, fn, 1024)) {
			if((di) || (!xad_is_dir(fi))) {
				if(((fi && (fi->xfi_Flags & XADFIF_CRYPTED)) || (di && (di->xdi_Flags & XADDIF_CRYPTED))) && (xu->pw == NULL)) {
					xu->pw = AllocVec(100, MEMF_CLEAR);
					err = ask_password(awin, xu->pw, 100);
					if(err == 0) {
						FreeVec(xu->pw);
						xu->pw = NULL;
					}
				}

				switch(xu->arctype) {
					case XFILE:
						err = xadFileUnArc(ai,
									XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
									XAD_MAKEDIRECTORY, TRUE,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, xu->pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;

					case XDISK:
						err = xadDiskUnArc(ai, XAD_ENTRYNUMBER, di->xdi_EntryNumber,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, xu->pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;

					case XDISKFILE:
						err = xadDiskFileUnArc(ai,
									XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
									XAD_MAKEDIRECTORY, TRUE,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, xu->pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;
				}

				if(err != XADERR_OK) {
					if(err == XADERR_PASSWORD) xad_free_pw(awin);
					return err;
				}

				if(*pud == PUD_ABORT) {
					if(xu->pw) FreeVec(xu->pw);
					xu->pw = NULL;
					return XADERR_BREAK;
				}

				if(err == XADERR_OK) {
					if(fi) module_vscan(awin, destfile, NULL, fi->xfi_Size, TRUE);
					if(di) module_vscan(awin, destfile, NULL, di->xdi_SectorSize * di->xdi_TotalSectors, TRUE);
				}

				if(fi) {
					err = xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
										XAD_GETDATEDATESTAMP, &ds,
										XAD_MAKELOCALDATE, TRUE,
										TAG_DONE);

					if(err != XADERR_OK) return err;

					SetProtection(destfile, xad_get_fileprotection(fi));
					SetFileDate(destfile, &ds);
					if(fi && fi->xfi_Comment) SetComment(destfile, fi->xfi_Comment);
				}
			}
		}
	}

	return err;
}


long xad_extract_file(void *awin, char *file, char *dest, struct Node *node, void *(getnode)(void *awin, struct Node *node), ULONG *pud)
{
	long err;
	struct xadFileInfo *fi = NULL;
	struct xadDiskInfo *di = NULL;
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);
	if(xu->arctype == XDISK) {
		di = (struct xadDiskInfo *)getnode(awin, node);
	} else {
		fi = (struct xadFileInfo *)getnode(awin, node);
	}
	
	return xad_extract_file_private(awin, dest, xu, di, fi, pud);
}

/* returns 0 on success */
long xad_extract(void *awin, char *file, char *dest, struct List *list, void *(getnode)(void *awin, struct Node *node))
{
	long err = XADERR_OK;
	struct Node *fnode;
	ULONG pud = 0;

	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);

	if(xu->ai) {
		for(fnode = list->lh_Head; fnode->ln_Succ; fnode=fnode->ln_Succ) {
			err = xad_extract_file(awin, file, dest, fnode, getnode, &pud);
			if(err != XADERR_OK) {
				return err;
			}
		}
	}

	return err;
}

long xad_extract_array(void *awin, ULONG total_items, char *dest, void **array, void *(getuserdata)(void *awin, void *arc_entry))
{
	long err = XADERR_OK;
	ULONG pud = 0;
	struct xadFileInfo *fi = NULL;
	struct xadDiskInfo *di = NULL;

	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);

	if(xu->ai) {
		for(int i = 0; i < total_items; i++) {
			if(xu->arctype == XDISK) {
				di = (struct xadDiskInfo *)getuserdata(awin, array[i]);
			} else {
				fi = (struct xadFileInfo *)getuserdata(awin, array[i]);
			}
	
			if((di == NULL) && (fi == NULL)) continue;

			err = xad_extract_file_private(awin, dest, xu, di, fi, &pud);
			if(err != XADERR_OK) {
				return err;
			}
		}
	}

	return err;
}

#endif

void deark_register(struct module_functions *funcs)
{
	funcs->module[0] = 'A';
	funcs->module[1] = 'R';
	funcs->module[2] = 'K';
	funcs->module[3] = 0;

	funcs->get_filename = NULL; //deark_get_filename;
	funcs->free = deark_free;
	funcs->get_format = deark_get_arc_format;
	funcs->get_subformat = NULL;
	funcs->get_error = deark_error;
	funcs->get_crunchsize = NULL;
}
