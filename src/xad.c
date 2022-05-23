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

#include <stdio.h>
#include <string.h>

#include "avalanche.h"
#include "libs.h"
#include "locale.h"
#include "req.h"
#include "xad.h"

#ifdef __amigaos4__
#define SetFileDate SetDate
#define xadMasterBase XadMasterBase
#endif

enum {
	PUD_NONE = 0,
	PUD_ABORT = 1,
	PUD_SKIP = 2,
	PUD_OVER = 3
};

enum {
	XNONE = 0,
	XFILE,
	XDISK,
	XDISKFILE
};

static struct xadArchiveInfo *ai = NULL;
static int arctype = XNONE;
static char *pw = NULL;

static void xad_free_ai(struct xadArchiveInfo *a)
{
	xadFreeInfo(a);
	xadFreeObjectA(a, NULL);
}

void xad_free(void)
{
	if(ai) xad_free_ai(ai);
	ai = NULL;
}

void xad_exit(void)
{
	xad_free();
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
	if(arctype == XDISK) return 0;

	struct xadFileInfo *fi = (struct xadFileInfo *)xfi;

	return xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
				XAD_GETDATECLOCKDATA, cd,
				XAD_MAKELOCALDATE, TRUE,
				TAG_DONE);
}

const char *xad_get_filename(void *userdata)
{
	if(arctype == XDISK) return "disk.img";

	struct xadFileInfo *fi = (struct xadFileInfo *)userdata;

	return fi->xfi_FileName;
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
				res = ask_question( locale_get_string( MSG_ALREADYEXISTSOVERWRITE ) , xpi->xpi_FileName);
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

		case XADPMODE_PROGRESS:
			if(xpi->xpi_FileInfo) {
				fuelgauge_update(xpi->xpi_CurrentSize, xpi->xpi_FileInfo->xfi_Size);
			} else if(xpi->xpi_DiskInfo) {
				fuelgauge_update(xpi->xpi_CurrentSize,
					xpi->xpi_DiskInfo->xdi_TotalSectors * xpi->xpi_DiskInfo->xdi_SectorSize);
			}
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

void xad_show_arc_info(void)
{
	char message[100];
	char *type;

	if(!ai) return;

	switch(arctype) {
		case XFILE:
			type =  locale_get_string( MSG_FILEARCHIVE ) ;
		break;
		case XDISK:
			type =  locale_get_string( MSG_DISKARCHIVE ) ;
		break;
		case XDISKFILE:
			type =  locale_get_string( MSG_DISKIMAGE ) ;
		break;
		default:
			type =  locale_get_string( MSG_UNKNOWN ) ;
		break;
	}

	sprintf(message, "%s %s", ai->xai_Client->xc_ArchiverName, type);	

	open_info_req(message,  locale_get_string( MSG_OK ) );
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

		xc = xadRecogFile(len, buf,
				//XAD_IGNOREFLAGS, XADAIF_DISKARCHIVE,
				TAG_DONE);
	}

	FreeVec(buf);

	if(xc == NULL) return FALSE;

	return TRUE;
}

long xad_info(char *file, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata))
{
	long err = 0;
	struct xadFileInfo *fi;
	struct xadDiskInfo *di;
	struct xadArchiveInfo *dai = NULL;
	ULONG total = 0;
	ULONG i = 0;
	ULONG size = 0;

	libs_xad_init();
	if(xadMasterBase == NULL) return -1;

	xad_free();
	ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

	arctype = XNONE;

	if(ai) {
		if((err = xadGetInfo(ai,
				XAD_INFILENAME, file,
				TAG_DONE)) == 0) {
			if(ai->xai_DiskInfo) arctype = XDISK;
			if(ai->xai_FileInfo) arctype = XFILE; /* We only support one of file/disk so file preferred */

		}

		if((arctype == XNONE) || (arctype == XDISK)) {
			dai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

			if(arctype == XNONE) {
				err = xadGetDiskInfo(dai,
					XAD_INFILENAME, file,
					TAG_DONE);
			} else {
				struct TagItem ti[2];
				ti[0].ti_Tag = XAD_INFILENAME;
				ti[0].ti_Data = (ULONG)file;
				ti[1].ti_Tag = TAG_DONE;

				err = xadGetDiskInfo(dai,
					XAD_INDISKARCHIVE, &ti,
					TAG_DONE);
			}

			if(err == 0) {
				arctype = DISKFILE;
				xad_free();
				ai = dai;
			} else {
				xad_free_ai(dai);
			}
		}
		
		if(arctype == XDISK) {
			/* Count entries (disks) */
			/* We only support archives which have disks or files, not mixed */
			di = ai->xai_DiskInfo;
			while(di) {
				total++;
				di = di->xdi_Next;
			}
					
			/* Add to list */
			di = ai->xai_DiskInfo;
			while(di) {
				size = di->xdi_SectorSize * di->xdi_TotalSectors;
				addnode("disk.img", &size, 0, i, total, di);
				i++;
				di = di->xdi_Next;
			}

		} else if (arctype != XNONE) {
			/* Count entries (files) */
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

long xad_extract_file(char *file, char *dest, struct Node *node, void *(getnode)(struct Node *node), ULONG (scan)(char *file, UBYTE *buf, ULONG len), ULONG *pud)
{
	long err = 0;
	char destfile[1024];
	struct xadFileInfo *fi = NULL;
	struct xadDiskInfo *di = NULL;
	struct DateStamp ds;
	char *fn = NULL;

	struct Hook progress_hook;
	progress_hook.h_Entry = xad_progress;
	progress_hook.h_SubEntry = NULL;
	progress_hook.h_Data = pud;

	if(arctype == XDISK) {
		di = (struct xadDiskInfo *)getnode(node);
	} else {
		fi = (struct xadFileInfo *)getnode(node);
	}

	if(fi || di) {
		strcpy(destfile, dest);

		if(fi) {
			fn = fi->xfi_FileName;
		} else {
			fn = "disk.img";
		}

		if(AddPart(destfile, fn, 1024)) {
			if((di) || (!xad_is_dir(fi))) {
				if(((fi && (fi->xfi_Flags & XADFIF_CRYPTED)) || (di && (di->xdi_Flags & XADDIF_CRYPTED))) && (pw == NULL)) {
					pw = AllocVec(100, MEMF_CLEAR);
					err = ask_password(pw, 100);
					if(err == 0) {
						FreeVec(pw);
						pw = NULL;
					}
				}

				switch(arctype) {
					case XFILE:
						err = xadFileUnArc(ai,
									XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
									XAD_MAKEDIRECTORY, TRUE,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;

					case XDISK:
						err = xadDiskUnArc(ai, XAD_ENTRYNUMBER, di->xdi_EntryNumber,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;

					case XDISKFILE:
						err = xadDiskFileUnArc(ai,
									XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
									XAD_MAKEDIRECTORY, TRUE,
									XAD_OUTFILENAME, destfile,
									XAD_PASSWORD, pw,
									XAD_PROGRESSHOOK, &progress_hook,
									TAG_DONE);
						break;
				}

				if(*pud == PUD_ABORT) {
					if(pw) FreeVec(pw);
					pw = NULL;
					return XADERR_BREAK;
				}

				if(err == XADERR_OK) {
					if(fi) scan(destfile, NULL, fi->xfi_Size);
					if(di) scan(destfile, NULL, di->xdi_SectorSize * di->xdi_TotalSectors);
				}

				if(fi) {
					err = xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
										XAD_GETDATEDATESTAMP, &ds,
										XAD_MAKELOCALDATE, TRUE,
										TAG_DONE);

					if(err != XADERR_OK) {
						if(pw) FreeVec(pw);
						pw = NULL;
						return err;
					}

					SetProtection(destfile, xad_get_fileprotection(fi));
					SetFileDate(destfile, &ds);
					SetComment(destfile, fi->xfi_Comment);
				}
			}
		}
		if(fi) fi = fi->xfi_Next;
		if(di) di = di->xdi_Next;
	}

	if(pw) FreeVec(pw);
	pw = NULL;
	return err;
}

/* returns 0 on success */
long xad_extract(char *file, char *dest, struct List *list, void *(getnode)(struct Node *node), ULONG (scan)(char *file, UBYTE *buf, ULONG len))
{
	long err = XADERR_OK;
	struct Node *node;
	ULONG pud = 0;

	if(ai) {
		for(node = list->lh_Head; node->ln_Succ; node=node->ln_Succ) {
			err = xad_extract_file(file, dest, node, getnode, scan, &pud);
			if(err != XADERR_OK) return err;
		}
	}

	return err;
}
