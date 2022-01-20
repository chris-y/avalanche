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

#include "libs.h"
#include "xad.h"

struct xadMasterBase *xadMasterBase = NULL;

struct xadArchiveInfo *ai = NULL;

static void xad_init(void)
{
	if(xadMasterBase == NULL) {
		xadMasterBase = (struct xadMasterBase *)OpenLibrary("xadmaster.library", 12);
	}
}

void xad_free(void)
{
	xadFreeInfo(ai);
	xadFreeObjectA(ai, NULL);
	ai = NULL;
}

void xad_exit(void)
{
	if(ai) xad_free();

	if(xadMasterBase != NULL) {
		CloseLibrary((struct Library *)xadMasterBase);
		xadMasterBase = NULL;
	}
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

long xad_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata))
{
	long err = 0;
	struct xadFileInfo *fi;
	ULONG total = 0;
	ULONG i = 0;

	xad_init();
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
long xad_extract(char *file, char *dest, struct List *list, void *(getnode)(struct Node *node))
{
	char destfile[1024];
	long err = 0;
	struct xadFileInfo *fi;
	struct Node *node;
	struct DateStamp ds;

	if(ai) {
		
		for(node = list->lh_Head; node->ln_Succ; node=node->ln_Succ) {
			fi = (struct xadFileInfo *)getnode(node);

			if(fi) {
				strcpy(destfile, dest);
				if(AddPart(destfile, fi->xfi_FileName, 1024)) {
					err = xadFileUnArc(ai, XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
							XAD_MAKEDIRECTORY, TRUE,
							XAD_OUTFILENAME, destfile,
							TAG_DONE);

					if(err != XADERR_OK) return err;

					err = xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
								XAD_GETDATEDATESTAMP, &ds,
								XAD_MAKELOCALDATE, TRUE,
								TAG_DONE);

					if(err != XADERR_OK) return err;

					SetProtection(destfile, xad_get_fileprotection(fi));
					SetFileDate(destfile, &ds);
					SetComment(destfile, fi->xfi_Comment);
				}
				fi = fi->xfi_Next;
			}
		}
	}
	return err;
}
