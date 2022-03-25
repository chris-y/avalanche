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
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/xfdmaster.h>

#include "avalanche.h"
#include "libs.h"
#include "req.h"

struct xfdBufferInfo *bi = NULL;
char *fn = NULL;
UBYTE *buffer = NULL;

void xfd_show_arc_info(void)
{
	char message[100];

	if(!bi) return;

	sprintf(message, "%s crunched", bi->xfdbi_PackerName);	

	open_info_req(message, "_OK");
}

BOOL xfd_recog(char *file)
{
	BPTR fh = 0;
	ULONG len = 0;
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

long xfd_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata))
{
	BPTR fh = 0;
	ULONG len = 0;
	BOOL res = FALSE;

	libs_xfd_init();
	if(xfdMasterBase == NULL) return -1;

	struct xfdMasterBase *xfdmb = (struct xfdMasterBase *)xfdMasterBase;

	if(bi != NULL) xfdFreeObject(bi);
	if(buffer != NULL) FreeVec(buffer);
	if(fn != NULL) {
		free(fn);
		fn = NULL;
	}


	bi = xfdAllocObject(XFDOBJ_BUFFERINFO);
	if(bi == NULL) return -2;

	if(fh = Open(file, MODE_OLDFILE)) {
#ifdef __amigaos4__
		len = (ULONG)GetFileSize(fh);
#else
		len = Seek(fh, 0, OFFSET_END);
		len = Seek(fh, 0, OFFSET_BEGINNING);
#endif

		buffer = AllocVec(len, MEMF_ANY);
		if(buffer == NULL) {
			Close(fh);
			xfdFreeObject(bi);
			return -2;
		}

		len = Read(fh, buffer, len);
		Close(fh);

		bi->xfdbi_SourceBuffer = buffer;
		bi->xfdbi_SourceBufLen = len;
		bi->xfdbi_Flags = XFDFB_RECOGEXTERN;

		res = xfdRecogBuffer(bi);
	}

	if(res == TRUE) {
		fn = strdup(FilePart(file));
		/* Add to list */
		addnode(fn, &bi->xfdbi_FinalTargetLen, 0, 0, 1, fn);

		return 0;
	}

	return -3;
}

long xfd_extract(char *file, char *dest, ULONG (scan)(char *file, UBYTE *buf, ULONG len))
{
	char destfile[1024];
	BPTR fh = 0;
	char *pw = NULL;
	ULONG pwlen = 100;
	ULONG err = 0;
	ULONG res = 1;

	if(bi->xfdbi_PackerFlags & XFDPFF_PASSWORD) {
		if(bi->xfdbi_MaxSpecialLen > 0)
			pwlen = bi->xfdbi_MaxSpecialLen;
		pw = AllocVec(pwlen, MEMF_CLEAR);
		err = ask_password(pw, pwlen);
		if(err == 0) {
			FreeVec(pw);
			pw = NULL;
		} else {
			bi->xfdbi_Special = pw;
		}
	}

	if(xfdDecrunchBuffer(bi) == TRUE) {
		if(scan(NULL, bi->xfdbi_TargetBuffer, bi->xfdbi_TargetBufSaveLen) < 4) {
			strcpy(destfile, dest);
			if(AddPart(destfile, fn, 1024)) {
				if(fh = Open(destfile, MODE_OLDFILE)) {
					res = ask_question("%s already exists, overwrite?", fn);
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
		open_error_req("Error decrunching", "_OK");
	}
	return 0;
}

void xfd_exit(void)
{
	if(bi != NULL) xfdFreeObject(bi);
	if(fn != NULL) free(fn);
	if(buffer != NULL) FreeVec(buffer);
	libs_xfd_exit();
}
