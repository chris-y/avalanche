/* Avalanche
 * (c) 2022 Chris Young
 */

#include <proto/dos.h>
#include <proto/xadmaster.h>
#include "xad.h"

struct Library *xadMasterBase = NULL;

static void xad_init(void)
{
	if(xadMasterBase == NULL) {
		xadMasterBase = (struct Library *)OpenLibrary("xadmaster.library", 12);
	}
}

static void xad_exit(void)
{
	if(xadMasterBase != NULL) {
		CloseLibrary((struct Library *)xadMasterBase);
		xadMasterBase = NULL;
	}
}

/* returns 0 on success */
ULONG xad_extract(char *file, char *dest)
{
	char destfile[1024];
	ULONG err = 0;
	BPTR dir;
	struct xadFileInfo *fi;
	xad_init();
	if(xadMasterBase == NULL) return 1;

	struct xadArchiveInfo *ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);

	if(ai) {
		if(err = xadGetInfo(ai, XAD_INFILENAME, file, TAG_DONE) == 0) {
			fi = ai->xai_FileInfo;
			while(fi) {
				strcpy(destfile, dest);
				if(AddPart(destfile, fi->xfi_FileName, 1024)) {
					//printf("%d: %s\n", fi->xfi_EntryNumber, destfile);
					if(fi->xfi_Flags & XADFIF_DIRECTORY) {
						if(dir = CreateDir(destfile))
							UnLock(dir);
					} else {
						err = xadFileUnArc(ai, XAD_ENTRYNUMBER, fi->xfi_EntryNumber,
							XAD_OUTFILENAME, destfile,
							TAG_DONE);
					}
				}
				fi = fi->xfi_Next;
			}
			xadFreeInfo(ai);
		}
	}

	xadFreeObjectA(ai, NULL);

	xad_exit();
	return err;
}
