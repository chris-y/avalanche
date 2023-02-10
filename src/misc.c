/* Avalanche
 * (c) 2023 Chris Young
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

char *strdup(const char *s)
{
  size_t len = strlen (s) + 1;
  char *result = (char*) malloc (len);
  if (result == (char*) 0)
  return (char*) 0;
  return (char*) memcpy (result, s, len);
}

#ifdef __amigaos4__
int32 recursive_scan(void *awin, CONST_STRPTR name)
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
					DebugPrintF("%s\n", file);
					window_edit_add(awin, file);
				}
            }
            else if( EXD_IS_DIRECTORY(dat) )
            {
                if( ! recursive_scan(awin, dat->Name ) )  /* recurse */
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
            PrintFault(IoErr(),NULL); /* failure - why ? */
        }
	        
    }
    else
    {
        PrintFault(IoErr(),NULL);  /* no context - why ? */
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
	}

	FreeDosObject(DOS_EXAMINEDATA, exd);

	return ret;
}
#endif
