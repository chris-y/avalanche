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
#include <proto/xvs.h>

#include <stdio.h>
#include <string.h>

#include "avalanche.h"
#include "libs.h"
#include "locale.h"
#include "req.h"
#include "xvs.h"

static long xvs_init(void *awin)
{
	if(xvsBase == NULL) {
		char *msg;
		libs_xvs_init();
		if(xvsBase == NULL) {
			int len = strlen(locale_get_string( MSG_UNABLETOOPENLIBRARY )) + strlen(locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED )) + strlen("xvs.library") + 4;
			if(msg = AllocVec(len, MEMF_CLEAR)) {
				snprintf(msg, len, locale_get_string( MSG_UNABLETOOPENLIBRARY ), "xvs.library", 33);
				strncat(msg, locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED ), len);
				open_error_req(msg, locale_get_string( MSG_OK ), awin );
				FreeVec(msg);
			}
			return -1;
		}
	
		if(xvsSelfTest() == FALSE) {
			int len = strlen(locale_get_string( MSG_XVSLIBRARYFAILEDSELFTEST )) + strlen(locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED )) + 2;
			if(msg = AllocVec(len, MEMF_CLEAR)) {
				snprintf(msg, len, "%s\n%s", locale_get_string( MSG_XVSLIBRARYFAILEDSELFTEST ), locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED ));
				open_error_req(msg, locale_get_string( MSG_OK ), awin );
				FreeVec(msg);
			}
			return -3;
		}
	}
	return 0;
}

static long xvs_scan_virus(char *file, UBYTE *buf, ULONG len, BOOL delete, void *awin)
{
	struct xvsFileInfo *xvsfi = NULL;
	ULONG result;

	long err = xvs_init(awin);
	if(err != 0) return err;

	xvsfi = xvsAllocObject(XVSOBJ_FILEINFO);
	if(xvsfi == NULL) {
		FreeVec(buf);
		open_error_req( locale_get_string( MSG_OUTOFMEMORYSCANNINGFILE ) ,  locale_get_string( MSG_OK ), awin );
		return -2;
	}

	xvsfi->xvsfi_File = buf;
	xvsfi->xvsfi_FileLen = len;

	result = xvsCheckFile(xvsfi);

	if((result == XVSFT_DATAVIRUS) || (result == XVSFT_FILEVIRUS) || (result == XVSFT_LINKVIRUS)) {
		char message[200];

		if(file) {
			if(delete == TRUE) {
				BOOL deleted = DeleteFile(file);

				if(deleted) {
					snprintf(message, 200, locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , file, xvsfi->xvsfi_Name);
					strncat(message, locale_get_string(MSG_FILEHASBEENDELETED), 200);
				} else {
					snprintf(message, 200, locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , file, xvsfi->xvsfi_Name);
					strncat(message, locale_get_string(MSG_FILECOULDNOTBEDELETED), 200);
				}
			} else {
				snprintf(message, 200, locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , file, xvsfi->xvsfi_Name);
			}
		} else {
			snprintf(message, 200, locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , "----", xvsfi->xvsfi_Name);
		}

		open_error_req(message,  locale_get_string( MSG_OK ), awin );
	} else {
		result = 0; // clean
	}

	xvsFreeObject(xvsfi);

	return result;
}

long xvs_scan_buffer(UBYTE *buf, ULONG len, void *awin)
{
	return xvs_scan_virus(NULL, buf, len, FALSE, awin);
}

long xvs_scan(char *file, BOOL delete, void *awin)
{
	UBYTE *buffer = NULL;
	BPTR fh = 0;
	long res = 0;

	if(fh = Open(file, MODE_OLDFILE)) {
#ifdef __amigaos4__
		int64 len = GetFileSize(fh);
#else
		Seek(fh, 0, OFFSET_END);
		long len = Seek(fh, 0, OFFSET_BEGINNING);
#endif

		if(len <= 0) return 0;

		buffer = AllocVec(len, MEMF_ANY | MEMF_PRIVATE);
		if(buffer == NULL) {
			char message[200];
			snprintf(message, 199, locale_get_string( MSG_OUTOFMEMORYSCANNINGFILE ), file);
			open_error_req(message, locale_get_string( MSG_OK ), awin);
			return -2;
		}

		Read(fh, buffer, len);
		Close(fh);

		res = xvs_scan_virus(file, buffer, len, delete, awin);
		FreeVec(buffer);
	}

	return res;
}

ULONG xvs_get_ver(ULONG *ver, ULONG *rev)
{
	long err = xvs_init(NULL);
	if(err != 0) return err;
	
	struct Library *lib = (struct Library *)xvsBase;
	if(ver) *ver = lib->lib_Version;
	if(rev) *rev = lib->lib_Revision;
	
	return 0;
}
