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

#ifndef MISC_H
#define MISC_H 1

#include <exec/lists.h>
#include <exec/nodes.h>

#ifndef __amigaos4__
#define IsMinListEmpty(L) (L)->mlh_Head->mln_Succ == 0

struct Node *GetHead(struct List *list);
struct Node *GetPred(struct Node *node);
struct Node *GetSucc(struct Node *node);

#define IDoMethod DoMethod
#endif

#ifdef __amigaos4__
#define CurrentDir SetCurrentDir

int32 recursive_scan(void *awin, CONST_STRPTR name, const char *root);
BOOL object_is_dir(char *filename);
#else
void recursive_scan(void *awin, BPTR lock, const char *root);
BOOL object_is_dir(BPTR lock);
#endif

char *strdup(const char *s);

#endif
