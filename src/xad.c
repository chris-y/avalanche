/* Avalanche
 * (c) 2022 Chris Young
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

long xad_info(char *file, void(*addnode)(char *name, LONG *size, void *userdata))
{
	long err = 0;
	struct xadFileInfo *fi;

	xad_init();
	if(xadMasterBase == NULL) return -1;

	if(ai) xad_free();

	ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

	if(ai) {
		if(err = xadGetInfo(ai,
				XAD_INFILENAME, file,
				XAD_IGNOREFLAGS, XADAIF_DISKARCHIVE,
				TAG_DONE) == 0) {
			fi = ai->xai_FileInfo;
			while(fi) {
				addnode(fi->xfi_FileName, &fi->xfi_Size, fi);
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
				}
				fi = fi->xfi_Next;
			}
		}
	}
	return err;
}
