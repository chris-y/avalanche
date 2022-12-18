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
			if(msg = AllocVec(strlen(locale_get_string( MSG_UNABLETOOPENLIBRARY )) + strlen(locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED )) + strlen("xvs.library") + 4, MEMF_CLEAR)) {
				sprintf(msg, locale_get_string( MSG_UNABLETOOPENLIBRARY ), "xvs.library", 33);
				strcat(msg, locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED ));
				open_error_req(msg, locale_get_string( MSG_OK ), awin );
				FreeVec(msg);
			}
			return -1;
		}
	
		if(xvsSelfTest() == FALSE) {
			if(msg = AllocVec(strlen(locale_get_string( MSG_XVSLIBRARYFAILEDSELFTEST )) + strlen(locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED )) + 1, MEMF_CLEAR)) {
				sprintf(msg, "%s\n%s", locale_get_string( MSG_XVSLIBRARYFAILEDSELFTEST ), locale_get_string( MSG_VIRUSSCANNINGWILLBEDISABLED ));
				open_error_req(msg, locale_get_string( MSG_OK ), awin );
				FreeVec(msg);
			}
			return -3;
		}
	}
	return 0;
}

static long xvs_scan_virus(char *file, UBYTE *buf, ULONG len, void *awin)
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
			BOOL deleted = DeleteFile(file);

			if(deleted) {
				sprintf(message,  locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , file, xvsfi->xvsfi_Name);
				strcat(message, locale_get_string(MSG_FILEHASBEENDELETED));
			} else {
				sprintf(message,  locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , file, xvsfi->xvsfi_Name);
				strcat(message, locale_get_string(MSG_FILECOULDNOTBEDELETED));
			}
		} else {
			sprintf(message,  locale_get_string( MSG_VIRUSFOUNDETECTIONNAME ) , "----", xvsfi->xvsfi_Name);
		}

		open_error_req(message,  locale_get_string( MSG_OK ), awin );
	}

	xvsFreeObject(xvsfi);

	return result;
}

long xvs_scan_buffer(UBYTE *buf, ULONG len, void *awin)
{
	return xvs_scan_virus(NULL, buf, len, awin);
}

long xvs_scan(char *file, ULONG len, void *awin)
{
	UBYTE *buffer = NULL;
	BPTR fh = 0;
	long res = 0;

	if(len == 0) return 0;

	buffer = AllocVec(len, MEMF_ANY | MEMF_PRIVATE);
	if(buffer == NULL) {
		char message[200];
		sprintf(message, locale_get_string( MSG_OUTOFMEMORYSCANNINGFILE ), file);
		open_error_req(message, locale_get_string( MSG_OK ), awin);
		return -2;
	}

	if(fh = Open(file, MODE_OLDFILE)) {
		Read(fh, buffer, len);
		Close(fh);
	}

	res = xvs_scan_virus(file, buffer, len, awin);

	FreeVec(buffer);

	return res;
}
