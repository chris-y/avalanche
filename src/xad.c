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
#include <proto/exec.h>
#include <proto/xadmaster.h>

#include <string.h>

#include "avalanche.h"
#include "libs.h"
#include "xad.h"

#ifdef __amigaos4__
#define SetFileDate SetDate
#define xadMasterBase XadMasterBase
#endif

struct xadArchiveInfo *ai = NULL;

enum {
	PUD_NONE = 0,
	PUD_ABORT = 1,
	PUD_SKIP = 2,
	PUD_OVER = 3
};

void xad_free(void)
{
	xadFreeInfo(ai);
	xadFreeObjectA(ai, NULL);
	ai = NULL;
}

void xad_exit(void)
{
	if(ai) xad_free();
	libs_xad_exit();
}

char *xad_error(long code)
{
	return xadGetErrorText((ULONG)code);
}

ULONG get_xad_ver(void)
{
	struct Library *lib = (struct Library *)xadMasterBase;
	return lib->lib_Version;
}

static BOOL xad_is_dir(struct xadFileInfo *fi)
{
	if(fi->xfi_Flags & XADFIF_DIRECTORY) return TRUE;
	if((get_xad_ver() == 12) && (fi->xfi_FileName[strlen(fi->xfi_FileName)-1] == '/')) return TRUE;
	return FALSE;
}

static ULONG xad_get_fileprotection(void *xfi)
{
	ULONG protbits;
	struct xadFileInfo *fi = (struct xadFileInfo *)xfi;

	xadConvertProtection(XAD_PROTFILEINFO, fi,
				XAD_GETPROTAMIGA, &protbits,
				TAG_DONE);

	return protbits;
}

ULONG xad_get_filedate(void *xfi, struct ClockData *cd)
{
	struct xadFileInfo *fi = (struct xadFileInfo *)xfi;

	return xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
				XAD_GETDATECLOCKDATA, cd,
				XAD_MAKELOCALDATE, TRUE,
				TAG_DONE);
}

#ifdef __amigaos4__
static ULONG xad_progress(struct Hook *h, APTR obj, struct xadProgressInfo *xpi)
#else
static ULONG __saveds xad_progress(__reg("a0") struct Hook *h, __reg("a2") APTR obj, __reg("a1") struct xadProgressInfo *xpi)
#endif
{
	ULONG *pud = h->h_Data;
	ULONG res;

	switch(xpi->xpi_Mode) {
		case XADPMODE_ASK:
			if(xpi->xpi_Status & XADPIF_OVERWRITE) {
				if(*pud == PUD_SKIP) return (XADPIF_OK | XADPIF_SKIP);
				if(*pud == PUD_OVER) return (XADPIF_OK | XADPIF_OVERWRITE);
				res = ask_question("%s already exists, overwrite?", xpi->xpi_FileName);
				switch(res) {
					case 0: // Abort
						*pud = PUD_ABORT;
						return 0;	
					break;

					case 2: // Yes to all
						*pud = PUD_OVER;
					case 1: // Yes
						return (XADPIF_OK | XADPIF_OVERWRITE);
					break;

					case 4: // No to all
						*pud = PUD_SKIP;
					case 3: // No
						return (XADPIF_OK | XADPIF_SKIP);
					break;
				}
			}
		break;

		case XADPMODE_ERROR:
			if(xpi->xpi_Error != XADERR_SKIP)
				show_error(xpi->xpi_Error);
		break;

		default:
			//printf("%d\n", xpi->xpi_Mode);
		break;
	}

	if(check_abort()) {
		*pud = PUD_ABORT;
		return 0;
	}

	return XADPIF_OK;
}

BOOL xad_recog(char *file)
{
	BPTR fh = 0;
	ULONG len = 0;
	struct xadClient *xc = NULL;

	libs_xad_init();
	if(xadMasterBase == NULL) return FALSE;

#ifndef __amigaos4__
	ULONG xadrs = xadMasterBase->xmb_RecogSize;
#else
	struct xadSystemInfo *xadsi = xadGetSystemInfo();
	ULONG xadrs = xadsi->xsi_RecogSize;
#endif

	UBYTE *buf = AllocVec(xadrs, MEMF_ANY);
	if(buf == NULL) return FALSE;

	if(fh = Open(file, MODE_OLDFILE)) {
		len = Read(fh, buf, xadrs);
		Close(fh);

		xc = xadRecogFile(len, buf, XAD_IGNOREFLAGS, XADAIF_DISKARCHIVE, TAG_DONE);
	}

	FreeVec(buf);

	if(xc == NULL) return FALSE;

	return TRUE;
}

long xad_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata))
{
	long err = 0;
	struct xadFileInfo *fi;
	ULONG total = 0;
	ULONG i = 0;

	libs_xad_init();
	if(xadMasterBase == NULL) return -1;

	if(ai) xad_free();

	ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

	if(ai) {
		if((err = xadGetInfo(ai,
				XAD_INFILENAME, file,
				XAD_IGNOREFLAGS, XADAIF_DISKARCHIVE,
				TAG_DONE)) == 0) {
					
			/* Count entries */
			fi = ai->xai_FileInfo;
			while(fi) {
				total++;
				fi = fi->xfi_Next;
			}
					
			/* Add to list */
			fi = ai->xai_FileInfo;
			while(fi) {
				addnode(fi->xfi_FileName, &fi->xfi_Size,
					(fi->xfi_Flags & XADFIF_DIRECTORY), i, total, fi);
				i++;
				fi = fi->xfi_Next;
			}
		}
	}
	return err;
}

/* returns 0 on success */
long xad_extract(char *file, char *dest, struct List *list, void *(getnode)(struct Node *node), ULONG (scan)(char *file, ULONG len))
{
	char destfile[1024];
	long err = 0;
	struct xadFileInfo *fi;
	struct Node *node;
	struct DateStamp ds;
	ULONG pud = 0;
	char *pw = NULL;

	struct Hook progress_hook;
	progress_hook.h_Entry = xad_progress;
	progress_hook.h_SubEntry = NULL;
	progress_hook.h_Data = &pud;

	if(ai) {
		
		for(node = list->lh_Head; node->ln_Succ; node=node->ln_Succ) {
			fi = (struct xadFileInfo *)getnode(node);

			if(fi) {
				strcpy(destfile, dest);
				if(AddPart(destfile, fi->xfi_FileName, 1024)) {
					if(!xad_is_dir(fi)) {
						if((fi->xfi_Flags & XADFIF_CRYPTED) && (pw == NULL)) {
							pw = AllocVec(100, MEMF_CLEAR);
							err = ask_password(pw);
							if(err == 0) {
								FreeVec(pw);
								pw = NULL;
							}
						}

						err = xadFileUnArc(ai, XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
								XAD_MAKEDIRECTORY, TRUE,
								XAD_OUTFILENAME, destfile,
								XAD_PASSWORD, pw,
								XAD_PROGRESSHOOK, &progress_hook,
								TAG_DONE);

						if(pud == PUD_ABORT) {
							if(pw) FreeVec(pw);
							return 0;
						}

						if(err == XADERR_OK) {
							scan(destfile, fi->xfi_Size);
						}

						err = xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
									XAD_GETDATEDATESTAMP, &ds,
									XAD_MAKELOCALDATE, TRUE,
									TAG_DONE);

						if(err != XADERR_OK) {
							if(pw) FreeVec(pw);
							return err;
						}

						SetProtection(destfile, xad_get_fileprotection(fi));
						SetFileDate(destfile, &ds);
						SetComment(destfile, fi->xfi_Comment);
					}
				}
				fi = fi->xfi_Next;
			}
		}
	}
	if(pw) FreeVec(pw);
	return err;
}
