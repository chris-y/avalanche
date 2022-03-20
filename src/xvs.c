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

#include "avalanche.h"
#include "libs.h"
#include "xvs.h"

long xvs_scan(char *file, ULONG len)
{
	struct xvsFileInfo *xvsfi = NULL;
	ULONG result;
	UBYTE *buffer = NULL;
	BPTR fh = 0;

	if(xvsBase == NULL) {
		libs_xvs_init();
		if(xvsBase == NULL) {
			open_error_req("xvs.library v33 could not be opened, virus scanning will be disabled.\n", "_OK");
			return -1;
		}
	}
	
	if(xvsSelfTest() == FALSE) {
		open_error_req("xvs.library Failed self-test,\nvirus scanning will be disabled.", "_OK");
		return -3;
	}

	buffer = AllocVec(len, MEMF_PRIVATE);
	if(buffer == NULL) {
		open_error_req("Out of memory scanning file", "_OK");
		return -2;
	}

	if(fh = Open(file, MODE_OLDFILE)) {
		Read(fh, buffer, len);
		Close(fh);
	}

	xvsfi = xvsAllocObject(XVSOBJ_FILEINFO);
	if(xvsfi == NULL) {
		FreeVec(buffer);
		open_error_req("Out of memory scanning file", "_OK");
		return -2;
	}

	xvsfi->xvsfi_File = buffer;
	xvsfi->xvsfi_FileLen = len;

	result = xvsCheckFile(xvsfi);

	if((result == XVSFT_DATAVIRUS) || (result == XVSFT_FILEVIRUS) || (result == XVSFT_LINKVIRUS)) {
		char message[200];
		BOOL deleted = DeleteFile(file);

		if(deleted) {
			sprintf(message, "Virus found in %s\nDetection name: %s\n\nFile has been deleted.", file, xvsfi->xvsfi_Name);
		} else {
			sprintf(message, "Virus found in %s\nDetection name: %s\n\nFile could not be deleted.", file, xvsfi->xvsfi_Name);
		}

		open_error_req(message, "_OK");
	}

	xvsFreeObject(xvsfi);
	FreeVec(buffer);

	return result;
}
