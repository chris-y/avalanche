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
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/xfdmaster.h>

#include "avalanche.h"
#include "libs.h"
#include "locale.h"
#include "module.h"
#include "req.h"
#include "win.h"
#include "xfd.h"

#define XFD_ERR_LIBOPEN -1
#define XFD_ERR_MEM -2
#define XFD_ERR_FILEOPEN -3

struct xfd_userdata {
	struct xfdBufferInfo *bi;
	char *fn;
	UBYTE *buffer;
};

static void xfd_free(void *awin)
{
	struct xfd_userdata *xu = (struct xfd_userdata *)window_get_archive_userdata(awin);
	if(xu) {
		if(xu->bi != NULL) xfdFreeObject(xu->bi);
		if(xu->buffer != NULL) FreeVec(xu->buffer);
		if(xu->fn != NULL) {
			free(xu->fn);
			xu->fn = NULL;
		}
	}

	window_free_archive_userdata(awin);
}

static const char *xfd_get_filename(void *userdata, void *awin)
{
	return userdata;
}

static const char *xfd_get_arc_format(void *awin)
{
	struct xfd_userdata *xu = (struct xfd_userdata *)window_get_archive_userdata(awin);
	if(!xu->bi) return NULL;
	
	return xu->bi->xfdbi_PackerName;
}

static const char *xfd_error(long code)
{
	if(code < 0) {
		switch(code) {
			case XFD_ERR_MEM:
				return locale_get_string(MSG_OUTOFMEMORY);
			break;
			case XFD_ERR_FILEOPEN:
				return locale_get_string(MSG_UNABLETOOPENFILE);
			break;	
		}
	}

	return NULL;
}

BOOL xfd_recog(char *file)
{
	BPTR fh = 0;
	ULONG len;
	BOOL res = FALSE;

	libs_xfd_init();
	if(xfdMasterBase == NULL) return FALSE;

	struct xfdMasterBase *xfdmb = (struct xfdMasterBase *)xfdMasterBase;

	struct xfdBufferInfo *xfdbi = xfdAllocObject(XFDOBJ_BUFFERINFO);
	if(xfdbi == NULL) return FALSE;

	UBYTE *buf = AllocVec(xfdmb->xfdm_MinBufferSize, MEMF_ANY);
	if(buf == NULL) {
		xfdFreeObject(xfdbi);
		return FALSE;
	}

	if(fh = Open(file, MODE_OLDFILE)) {
		len = Read(fh, buf, xfdmb->xfdm_MinBufferSize);
		Close(fh);

		xfdbi->xfdbi_SourceBuffer = buf;
		xfdbi->xfdbi_SourceBufLen = len;
		xfdbi->xfdbi_Flags = XFDFB_RECOGEXTERN;

		res = xfdRecogBuffer(xfdbi);
	}

	FreeVec(buf);
	xfdFreeObject(xfdbi);

	return res;
}

long xfd_info(char *file, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, void *awin))
{
	BPTR fh = 0;
	ULONG len;
	BOOL res = FALSE;

	libs_xfd_init();
	if(xfdMasterBase == NULL) return XFD_ERR_LIBOPEN;

	struct xfdMasterBase *xfdmb = (struct xfdMasterBase *)xfdMasterBase;
	
	xfd_free(awin);
	struct xfd_userdata *xu = (struct xfd_userdata *)window_alloc_archive_userdata(awin, sizeof(struct xfd_userdata));

	xu->bi = xfdAllocObject(XFDOBJ_BUFFERINFO);
	if(xu->bi == NULL) return XFD_ERR_MEM;
	struct xfdBufferInfo *bi = xu->bi;

	if(fh = Open(file, MODE_OLDFILE)) {
#ifdef __amigaos4__
		len = (ULONG)GetFileSize(fh);
#else
		Seek(fh, 0, OFFSET_END);
		len = Seek(fh, 0, OFFSET_BEGINNING);
#endif

		xu->buffer = AllocVec(len, MEMF_ANY);
		if(xu->buffer == NULL) {
			Close(fh);
			xfdFreeObject(xu->bi);
			xu->bi = NULL;
			return XFD_ERR_MEM;
		}

		len = Read(fh, xu->buffer, len);
		Close(fh);

		bi->xfdbi_SourceBuffer = xu->buffer;
		bi->xfdbi_SourceBufLen = len;
		bi->xfdbi_Flags = XFDFB_RECOGEXTERN;

		res = xfdRecogBuffer(bi);
	}

	if(res == TRUE) {
		xu->fn = strdup(FilePart(file));
		/* Add to list */
		addnode(xu->fn, &bi->xfdbi_FinalTargetLen, 0, 0, 1, xu->fn, awin);

		return 0;
	}

	xfd_free(awin);
	return XFD_ERR_FILEOPEN;
}

long xfd_extract(void *awin, char *file, char *dest)
{
	BPTR fh;
	char *pw = NULL;
	ULONG pwlen = 100;
	ULONG err;
	ULONG res = 1;

	struct xfd_userdata *xu = (struct xfd_userdata *)window_get_archive_userdata(awin);
	struct xfdBufferInfo *bi = xu->bi;

	if(bi->xfdbi_PackerFlags & XFDPFF_PASSWORD) {
		if(bi->xfdbi_MaxSpecialLen > 0)
			pwlen = bi->xfdbi_MaxSpecialLen;
		pw = AllocVec(pwlen, MEMF_CLEAR);
		err = ask_password(awin, pw, pwlen);
		if(err == 0) {
			FreeVec(pw);
			pw = NULL;
		} else {
			bi->xfdbi_Special = pw;
		}
	}

	if(xfdDecrunchBuffer(bi) == TRUE) {
		if(module_vscan(awin, NULL, bi->xfdbi_TargetBuffer, bi->xfdbi_TargetBufSaveLen, TRUE) < 4) {
			char destfile[1024];
			strncpy(destfile, dest, 1023);
			destfile[1023] = 0;
			if(AddPart(destfile, xu->fn, 1024)) {
				if(fh = Open(destfile, MODE_OLDFILE)) {
					res = ask_question(awin, locale_get_string( MSG_ALREADYEXISTSOVERWRITE ) , xu->fn);
					Close(fh);
				}

				if((res == 1) || (res == 2)) {
					if(fh = Open(destfile, MODE_NEWFILE)) {
						Write(fh, bi->xfdbi_TargetBuffer, bi->xfdbi_TargetBufSaveLen);
						Close(fh);
					}
				}
			}
		}
		FreeMem(bi->xfdbi_TargetBuffer, bi->xfdbi_TargetBufLen);
	} else {
		open_error_req( locale_get_string( MSG_ERRORDECRUNCHING ) ,  locale_get_string( MSG_OK ) , awin);
	}
	return 0;
}

void xfd_exit(void)
{
	libs_xfd_exit();
}

void xfd_register(struct module_functions *funcs)
{
	funcs->module[0] = 'X';
	funcs->module[1] = 'F';
	funcs->module[2] = 'D';
	funcs->module[3] = 0;

	funcs->get_filename = xfd_get_filename;
	funcs->free = xfd_free;
	funcs->get_format = xfd_get_arc_format;
	funcs->get_subformat = NULL;
	funcs->get_error = xfd_error;
}
