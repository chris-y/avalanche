/* Avalanche
 * (c) 2022-5 Chris Young
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
#include "config.h"
#include "libs.h"
#include "locale.h"
#include "module.h"
#include "req.h"
#include "win.h"
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

struct xad_hookdata {
	ULONG *pud;
	void *awin;
};

struct xad_userdata {
	struct xadArchiveInfo *ai;
	int arctype;
	char *pw;
};

static void xad_free_ai(struct xadArchiveInfo *a)
{
	xadFreeInfo(a);
	xadFreeObjectA(a, NULL);
}

static void xad_free_pw(void *awin)
{
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);

	if(xu->pw) {
		for(int i = 0; i < strlen(xu->pw); i++)
			xu->pw[i] = '\0';

		FreeVec(xu->pw);
		xu->pw = NULL;
	}

}

static void xad_free(void *awin)
{
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);
	if(xu) {
		if (xu->ai) {
			xad_free_ai(xu->ai);
			xu->ai = NULL;
		}

		xad_free_pw(awin);

	}

	window_free_archive_userdata(awin);
}

void xad_exit(void)
{
	libs_xad_exit();
}

static const char *xad_error(void *awin, long code)
{
	/* suppress user break messages */
	if(code != XADERR_BREAK)
		return xadGetErrorText((ULONG)code);

	return NULL;
}

ULONG xad_get_ver(ULONG *ver, ULONG *rev)
{
	libs_xad_init();
	if(xadMasterBase == NULL) return 0;
	
	struct Library *lib = (struct Library *)xadMasterBase;
	if(ver) *ver = lib->lib_Version;
	if(rev) *rev = lib->lib_Revision;

	return lib->lib_Version;
}

static BOOL xad_is_dir(struct xadFileInfo *fi)
{
	if(fi->xfi_Flags & XADFIF_DIRECTORY) return TRUE;
	if((xad_get_ver() == 12) && (fi->xfi_FileName[strlen(fi->xfi_FileName)-1] == '/')) return TRUE;
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

ULONG xad_get_filedate(void *xfi, struct ClockData *cd, void *awin)
{
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);

	if(xu && xu->arctype == XDISK) return 0;
	if(xfi == NULL) return 0;

	struct xadFileInfo *fi = (struct xadFileInfo *)xfi;

	return xadConvertDates(XAD_DATEXADDATE, &fi->xfi_Date,
				XAD_GETDATECLOCKDATA, cd,
				XAD_MAKELOCALDATE, TRUE,
				TAG_DONE);
}

static LONG *xad_get_crunchsize(void *userdata, void *awin)
{
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);

	if(xu && xu->arctype == XDISK) return NULL;
	if(userdata == NULL) return NULL;

	struct xadFileInfo *fi = (struct xadFileInfo *)userdata;

	return (LONG *)&fi->xfi_CrunchSize;
}

static const char *xad_get_filename(void *userdata, void *awin)
{
	if(userdata == NULL) return NULL;
	
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);
	
	if(xu && (xu->arctype == XDISK)) return "disk.img";

	struct xadFileInfo *fi = (struct xadFileInfo *)userdata; /* userdata is userdata from the node! */

	return fi->xfi_FileName;
}

#ifdef __amigaos4__
static ULONG xad_progress(struct Hook *h, APTR obj, struct xadProgressInfo *xpi)
#else
static ULONG __saveds xad_progress(__reg("a0") struct Hook *h, __reg("a2") APTR obj, __reg("a1") struct xadProgressInfo *xpi)
#endif
{
	struct xad_hookdata *xhd = (struct xad_hookdata *)h->h_Data;
	ULONG *pud = xhd->pud;
	ULONG res;

	switch(xpi->xpi_Mode) {
		case XADPMODE_ASK:
			if(xpi->xpi_Status & XADPIF_OVERWRITE) {
				if(*pud == PUD_SKIP) return (XADPIF_OK | XADPIF_SKIP);
				if(*pud == PUD_OVER) return (XADPIF_OK | XADPIF_OVERWRITE);
				res = ask_question(xhd->awin, locale_get_string( MSG_ALREADYEXISTSOVERWRITE ) , xpi->xpi_FileName);
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
				/* We did show error here, but we also show afterwards
				 * show_error(xpi->xpi_Error, xhd->awin); */
		break;

		case XADPMODE_PROGRESS:
			if(xpi->xpi_FileInfo) {
				window_fuelgauge_update(xhd->awin, xpi->xpi_CurrentSize, xpi->xpi_FileInfo->xfi_Size);
			} else if(xpi->xpi_DiskInfo) {
				window_fuelgauge_update(xhd->awin, xpi->xpi_CurrentSize,
					xpi->xpi_DiskInfo->xdi_TotalSectors * xpi->xpi_DiskInfo->xdi_SectorSize);
			}
		break;

		default:
			//printf("%d\n", xpi->xpi_Mode);
		break;
	}

	if(check_abort(xhd->awin)) {
		*pud = PUD_ABORT;
		return 0;
	}

	return XADPIF_OK;
}

static const char *xad_get_arc_format(void *awin)
{
	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);
	if(!xu->ai) return NULL;

	return xu->ai->xai_Client->xc_ArchiverName;
}

static const char *xad_get_arc_subformat(void *awin)
{
	char *type;

	struct xad_userdata *xu = (struct xad_userdata *)window_get_archive_userdata(awin);
	if(!xu->ai) return NULL;

	switch(xu->arctype) {
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
	
	return type;
}

BOOL xad_recog(char *file)
{
	BPTR fh = 0;
	ULONG len;
	struct xadClient *xc = NULL;

	libs_xad_init();
	if(xadMasterBase == NULL) return FALSE;

#ifndef __amigaos4__
	ULONG xadrs = xadMasterBase->xmb_RecogSize;
#else
	const struct xadSystemInfo *xadsi = xadGetSystemInfo();
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

long xad_info(char *file, struct avalanche_config *config, void *awin, void(*addnode)(char *name, LONG *size, BOOL dir, ULONG item, ULONG total, void *userdata, struct avalanche_config *config, void *awin))
{
	long err = 0;
	struct xadFileInfo *fi;
	struct xadDiskInfo *di;
	struct xadArchiveInfo *dai = NULL;
	struct xadArchiveInfo *ai = NULL;
	ULONG total = 0;
	ULONG i = 0;
	ULONG size;
	BOOL fs = !config->ignorefs;

	libs_xad_init();
	if(xadMasterBase == NULL) return -1;

	xad_free(awin);

	struct xad_userdata *xu = (struct xad_userdata *)window_alloc_archive_userdata(awin, sizeof(struct xad_userdata));
	if(xu == NULL) return -2;

	xu->ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);
	ai = xu->ai;
	xu->arctype = XNONE;

	if(ai) {
		if((err = xadGetInfo(ai,
				XAD_INFILENAME, file,
				TAG_DONE)) == 0) {
			if(ai->xai_DiskInfo) xu->arctype = XDISK;
			if(ai->xai_FileInfo) xu->arctype = XFILE; /* We only support one of file/disk so file preferred */

		}

		if(fs && ((xu->arctype == XNONE) || (xu->arctype == XDISK))) {
			dai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

			if(xu->arctype == XNONE) {
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
				xu->arctype = XDISKFILE;
				xad_free_ai(ai);
				ai = dai;
				xu->ai = dai;
			} else {
				xad_free_ai(dai);
			}
		}
		
		if(xu->arctype == XDISK) {
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
				addnode("disk.img", &size, 0, i, total, di, config, awin);
				i++;
				di = di->xdi_Next;
			}

		} else if (xu->arctype != XNONE) {
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
					(fi->xfi_Flags & XADFIF_DIRECTORY), i, total, fi, config, awin);
				i++;
				fi = fi->xfi_Next;
			}
		}
	}

	if(err != 0) xad_free(awin);

	return err;
}

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


void xad_register(struct module_functions *funcs)
{
	funcs->module[0] = 'X';
	funcs->module[1] = 'A';
	funcs->module[2] = 'D';
	funcs->module[3] = 0;

	funcs->get_filename = xad_get_filename;
	funcs->free = xad_free;
	funcs->get_format = xad_get_arc_format;
	funcs->get_subformat = xad_get_arc_subformat;
	funcs->get_error = xad_error;
	funcs->get_crunchsize = xad_get_crunchsize;
}
