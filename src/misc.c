/* Avalanche
 * (c) 2023-5 Chris Young
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "win.h"
#include "misc.h"
#include "req.h"

/** Useful functions **/
#ifndef __amigaos4__
struct Node *GetHead(struct List *list)
{
	struct Node *res = NULL;

	if ((NULL != list) && (NULL != list->lh_Head->ln_Succ))
	{
		res = list->lh_Head;
	}
	return res;
}

struct Node *GetPred(struct Node *node)
{
	if (node->ln_Pred->ln_Pred == NULL) return NULL;
	return node->ln_Pred;
}

struct Node *GetSucc(struct Node *node)
{
	if (node->ln_Succ->ln_Succ == NULL) return NULL;
	return node->ln_Succ;
}
#endif

char *strdup_vec(const char *s)
{
	BOOL cmq = FALSE;
	size_t len = strlen (s) + 1;
	
	if(len & 0x3) cmq = TRUE;
	
	char *result = (char*) AllocVec(len, MEMF_PRIVATE);
	if (result == (char*) 0)
		return (char*) 0;
		
	if(cmq) {
		CopyMemQuick(s, result, len);
	} else {
		CopyMem(s, result, len);
	}
	return result;
}

#ifdef __amigaos4__
int32 recursive_scan(void *awin, CONST_STRPTR name, const char *root)
{
	int32 success = FALSE;
	APTR  context = ObtainDirContextTags( EX_StringNameInput,name,
					 EX_DoCurrentDir,TRUE, /* for recursion cd etc */
					 EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE),
					 TAG_END);
	if(context)
	{
		struct ExamineData *dat;

		while((dat = ExamineDir(context)))
		{
			if( EXD_IS_LINK(dat) ) /* all link types - check first ! */
			{
				if( EXD_IS_SOFTLINK(dat) ) 
				{
				}
				else   /* a hardlink */
				{
				}
			}
			else if( EXD_IS_FILE(dat) )
			{
				char *file;
				if(file = AllocVec(1024, MEMF_CLEAR)) {
					NameFromLock(GetCurrentDir(), file, 1024);
					AddPart(file, dat->Name, 1024);
					window_edit_add(awin, file, root);
					FreeVec(file);
				}
			}
			else if( EXD_IS_DIRECTORY(dat) )
			{
				if( ! recursive_scan(awin, dat->Name, root ) )  /* recurse */
				{
					break;
				}
			}
		}

		if( ERROR_NO_MORE_ENTRIES == IoErr() )
		{
			success = TRUE;           /* normal success exit */
		}
		else
		{
			show_dos_error(IoErr(), awin); /* failure */
		}
	        
	}
	else
	{
		show_dos_error(IoErr(), awin);  /* no context */
	}

	ReleaseDirContext(context);          /* NULL safe */
	return(success);
}

BOOL object_is_dir(char *filename)
{
	BOOL ret = FALSE;
	struct ExamineData *exd = ExamineObjectTags(EX_StringNameInput, filename, TAG_END);

	if(exd) {
		if(EXD_IS_DIRECTORY(exd)) ret = TRUE;

		FreeDosObject(DOS_EXAMINEDATA, exd);
	}
	
	return ret;
}
#else
BOOL object_is_dir(BPTR lock)
{
	BOOL ret = FALSE;
	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
	
	if(fib) {
		if(Examine(lock, fib)) {
			if(fib->fib_DirEntryType > 0) ret = TRUE;
		}
		
		FreeDosObject(DOS_FIB, fib);
	}
	
	return ret;
}

void recursive_scan(void *awin, BPTR lock, const char *root)
{
	struct ExAllControl *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
	if (!eac) return;
	BOOL more;
	ULONG exalldata_size = sizeof(struct ExAllData) * 10;
	struct ExAllData *ead = AllocVec(exalldata_size, MEMF_CLEAR);

	eac->eac_LastKey = 0;
	do {
		more = ExAll(lock, ead, exalldata_size, ED_TYPE, eac);
		if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) {
			/* ExAll failed abnormally */
			show_dos_error(IoErr(), awin);
			break;
		}
		if (eac->eac_Entries == 0) {
			/* ExAll failed normally with no entries */
			continue; /* ("more" is *usually* zero) */
		}

		do {
			char *file;
			if(file = AllocVec(1024, MEMF_CLEAR)) {
				NameFromLock(lock, file, 1024);
				AddPart(file, ead->ed_Name, 1024);
				
				if(ead->ed_Type > 0) { /* dir? */
					BPTR lock = Lock(file, ACCESS_READ);
					if(lock) {
						recursive_scan(awin, lock, root);
						UnLock(lock);
					}
				} else {
					window_edit_add(awin, file, root);
				}
				FreeVec(file);
			}

			/* get next ead */
			ead = ead->ed_Next;
		} while (ead);
	} while (more);

	FreeDosObject(DOS_EXALLCONTROL, eac);
	FreeVec(ead);
}
#endif
