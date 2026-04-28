/* Avalanche
 * (c) 2022-6 Chris Young
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

#include <proto/intuition.h>

#include <proto/bitmap.h>
#include <proto/drawlist.h>
#include <proto/glyph.h>

#include <images/bitmap.h>
#include <images/drawlist.h>
#include <images/glyph.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "config.h"
#include "glyph.h"
#include "libs.h"


/** Glyphs **/
Object *glyph_cache[AVALANCHE_GLYPH_MAX];

static struct DrawList dl_opendrawer[] = {
	{DLST_LINE, 90, 50, 90, 80, 1},
	{DLST_LINE, 90, 80, 10, 80, 1},
	{DLST_LINE, 10, 80, 10, 50, 1},
	{DLST_LINE, 10, 50, 90, 50, 1},
	{DLST_LINE, 90, 50, 70, 20, 1},
	{DLST_LINE, 70, 20, 30, 20, 1},
	{DLST_LINE, 30, 20, 10, 50, 1},
	{DLST_LINE, 30, 20, 20, 20, 1},
	{DLST_LINE, 20, 20, 20, 40, 1},

	{DLST_LINE, 70, 20, 80, 20, 1},
	{DLST_LINE, 80, 20, 80, 40, 1},

	{DLST_LINE, 40, 65, 60, 65, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_drawer[] = {
	{DLST_LINE, 10, 50, 90, 50, 1},
	{DLST_LINE, 90, 50, 90, 80, 1},
	{DLST_LINE, 90, 80, 10, 80, 1},
	{DLST_LINE, 10, 80, 10, 50, 1},

	/* HANDLE */
	{DLST_LINE, 40, 65, 60, 65, 1},

	/* TOP */
	{DLST_LINE, 10, 50, 30, 30, 1},
	{DLST_LINE, 30, 30, 70, 30, 1},
	{DLST_LINE, 70, 30, 90, 50, 1},

	{DLST_END, 0, 0, 0, 0, 0},
}; 

static struct DrawList dl_file[] = {
	{DLST_LINE, 20, 10, 60, 10, 1},
	{DLST_LINE, 60, 10, 80, 30, 1},
	{DLST_LINE, 80, 30, 80, 90, 1},
	{DLST_LINE, 80, 90, 20, 90, 1},
	{DLST_LINE, 20, 90, 20, 10, 1},

	{DLST_LINE, 60, 10, 60, 30, 1},
	{DLST_LINE, 60, 30, 80, 30, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_cryptfile[] = {
	{DLST_LINE, 20, 10, 60, 10, 1},
	{DLST_LINE, 60, 10, 80, 30, 1},
	{DLST_LINE, 80, 30, 80, 90, 1},
	{DLST_LINE, 80, 90, 20, 90, 1},
	{DLST_LINE, 20, 90, 20, 10, 1},

	{DLST_LINE, 60, 10, 60, 30, 1},
	{DLST_LINE, 60, 30, 80, 30, 1},

	/* Padlock body */
	{DLST_LINE, 40, 70, 60, 70, 1},
	{DLST_LINE, 60, 70, 60, 50, 1},
	{DLST_LINE, 60, 50, 40, 50, 1},
	{DLST_LINE, 40, 50, 40, 70, 1},

	/* Padlock lock */
	{DLST_LINE, 45, 50, 45, 40, 1},
	{DLST_LINE, 45, 40, 46, 39, 1},
	{DLST_LINE, 46, 39, 54, 39, 1},
	{DLST_LINE, 54, 39, 55, 40, 1},
	{DLST_LINE, 55, 40, 55, 50, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_link[] = {
	{DLST_LINE, 20, 10, 60, 10, 1},
	{DLST_LINE, 60, 10, 80, 30, 1},
	{DLST_LINE, 80, 30, 80, 90, 1},
	{DLST_LINE, 80, 90, 20, 90, 1},
	{DLST_LINE, 20, 90, 20, 10, 1},

	{DLST_LINE, 60, 10, 60, 30, 1},
	{DLST_LINE, 60, 30, 80, 30, 1},

	/* Arrow */
	{DLST_LINE, 40, 70, 60, 50, 1},
	{DLST_LINE, 50, 50, 60, 50, 1},
	{DLST_LINE, 60, 60, 60, 50, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_disk[] = {
	{DLST_LINE, 10, 10, 80, 10, 1},
	{DLST_LINE, 80, 10, 90, 20, 1},
	{DLST_LINE, 90, 20, 90, 90, 1},
	{DLST_LINE, 90, 90, 10, 90, 1},
	{DLST_LINE, 10, 90, 10, 10, 1},

	/* Label */
	{DLST_LINE, 20, 90, 20, 50, 1},
	{DLST_LINE, 20, 50, 80, 50, 1},
	{DLST_LINE, 80, 50, 80, 90, 1},

	/* Shutter */
	{DLST_LINE, 30, 10, 30, 40, 1},
	{DLST_LINE, 30, 40, 70, 40, 1},
	{DLST_LINE, 70, 40, 70, 10, 1},

	{DLST_LINE, 55, 15, 65, 15, 1},
	{DLST_LINE, 65, 15, 65, 35, 1},
	{DLST_LINE, 65, 35, 55, 35, 1},
	{DLST_LINE, 55, 35, 55, 15, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_archiveroot[] = {
	{DLST_LINE, 10, 30, 50, 10, 1},
	{DLST_LINE, 50, 10, 90, 30, 1},
	{DLST_LINE, 90, 30, 50, 50, 1},
	{DLST_LINE, 10, 30, 50, 50, 1},

	{DLST_LINE, 10, 30, 10, 70, 1},
	{DLST_LINE, 50, 50, 50, 90, 1},
	{DLST_LINE, 90, 30, 90, 70, 1},

	{DLST_LINE, 10, 70, 50, 90, 1},
	{DLST_LINE, 50, 90, 90, 70, 1},

	{DLST_LINE, 30, 40, 70, 20, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_extract[] = {
	{DLST_LINE, 30, 30, 50, 10, 1},
	{DLST_LINE, 50, 10, 70, 30, 1},
	{DLST_LINE, 50, 10, 50, 80, 1},

	{DLST_LINE, 60, 40, 90, 40, 1},
	{DLST_LINE, 90, 40, 90, 90, 1},
	{DLST_LINE, 90, 90, 10, 90, 1},
	{DLST_LINE, 10, 90, 10, 40, 1},
	{DLST_LINE, 10, 40, 40, 40, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_abort[] = {
#ifndef __amigaos4__ /* DLST_CIRCLE is "incorrectly implemented" in OS4.1 but fixed in OS3.2 */
	{DLST_CIRCLE, 50, 50, 40, 40, 1},
#endif
	{DLST_LINE, 30, 30, 70, 70, 1},
	{DLST_LINE, 30, 70, 70, 30, 1},

	{DLST_END, 0, 0, 0, 0, 0},
};

static struct DrawList dl_none[] = {
	{DLST_END, 0, 0, 0, 0, 0},
};


Object *glyph_get(ULONG glyph)
{
	Object *glyphobj = NULL;
	char *img = NULL;
	char *img_s = NULL;
	char *img_g = NULL;

	if(glyph_cache[glyph] != NULL) return glyph_cache[glyph];

	if(get_config()->aiss) {
		struct Screen *screen = LockPubScreen(NULL);

		switch(glyph) {
			case GLYPH_POPDRAWER:
			case AVALANCHE_GLYPH_DRAWER:
				img = "TBimages:list_drawer";
				img_s = "TBimages:list_drawer_s";
				img_g = "TBimages:list_drawer_g";
			break;

			case AVALANCHE_GLYPH_OPENDRAWER:
#ifdef __amigaos4__
				img = "TBimages:draweropen";
				img_s = "TBimages:draweropen_s";
				img_g = "TBimages:draweropen_g";
#else
				/* This is intentionally the wrong way round */
				img = "TBimages:list_drawer_s";
				img_s = "TBimages:list_drawer";
#endif
			break;

			case AVALANCHE_GLYPH_POPFILE:
				img = "TBimages:list_file";
				img_s = "TBimages:list_file_s";
				img_g = "TBimages:list_file_g";
			break;

			case GLYPH_POPFILE:
				img = "TBimages:open";
				img_s = "TBimages:open_s";
				img_g = "TBimages:open_g";
			break;

			case AVALANCHE_GLYPH_CRYPTFILE:
#ifdef __amigaos4__
				img = "TBimages:list_crypt";
				img_s = "TBimages:list_crypt_s";
				img_g = "TBimages:list_crypt_g";
#else
				img = "TBimages:list_protectfile";
				img_s = "TBimages:list_protectfile_s";
				img_g = "TBimages:list_protectfile_g";
#endif
			break;

			case AVALANCHE_GLYPH_LINK:
#ifdef __amigaos4__
				img = "TBimages:list_filelink";
				img_s = "TBimages:list_filelink_s";
				img_g = "TBimages:list_filelink_g";
#else		
				img = "TBimages:list_linkamiga";
				img_s = "TBimages:list_linkamiga_s";
				img_g = "TBimages:list_linkamiga_g";
#endif
			break;

			case AVALANCHE_GLYPH_DISK:
				img = "TBimages:list_disk";
				img_s = "TBimages:list_disk_s";
				img_g = "TBimages:list_disk_g";
			break;

			case AVALANCHE_GLYPH_EXTRACT:
				img = "TBimages:archiveextract";
				img_s = "TBimages:archiveextract_s";
				img_g = "TBimages:archiveextract_g";
			break;

			case AVALANCHE_GLYPH_STOP:
				img = "TBimages:stop";
				img_s = "TBimages:stop_s";
				img_g = "TBimages:stop_g";
			break;

			case AVALANCHE_GLYPH_ROOT:
				img = "TBimages:list_archive";
				img_s = "TBimages:list_archive_s";
				img_g = "TBimages:list_archive_g";
			break;

			case GLYPH_UPARROW:
#ifdef __amigaos4__
				img = "TBimages:list_nav_north";
#else
				img = "TBimages:list_parent";
#endif
			break;

			case GLYPH_RIGHTARROW:
				img = "TBimages:autobutton_rtarrow";
			break;

			case GLYPH_DOWNARROW:
				img = "TBimages:autobutton_dnarrow";
			break;

			case GLYPH_CHECKMARK:
#ifdef __amigaos4__
				img = "TBimages:list_checkmark";
#else
				img = "TBimages:autobutton_checkbox";
#endif
			break;

			default: // also AVALANCHE_GLYPH_NONE
				img = "TBimages:list_blank";
			break;
		}

		glyphobj = BitMapObj,
					BITMAP_SourceFile, img,
					BITMAP_SelectSourceFile, img_s,
#ifdef __amigaos4__
					BITMAP_DisabledSourceFile, img_g,
#endif
					BITMAP_Masking, TRUE,
					BITMAP_Screen, screen,
					/* BITMAP_Height, 16,
					BITMAP_Width, 16, */
				BitMapEnd;

		UnlockPubScreen(NULL, screen);

	} else {
		if((glyph >= AVALANCHE_GLYPH_ROOT) &&
			(glyph < AVALANCHE_GLYPH_MAX)) {

			struct DrawList *dl = NULL;

			switch(glyph) {
				case AVALANCHE_GLYPH_OPENDRAWER:
					dl = &dl_opendrawer;
				break;
				case AVALANCHE_GLYPH_DRAWER:
					dl = &dl_drawer;
				break;
				case AVALANCHE_GLYPH_POPFILE:
					dl = &dl_file;
				break;
				case AVALANCHE_GLYPH_CRYPTFILE:
					dl = &dl_cryptfile;
				break;
				case AVALANCHE_GLYPH_LINK:
					dl = &dl_link;
				break;
				case AVALANCHE_GLYPH_ROOT:
					dl = &dl_archiveroot;
				break;
				case AVALANCHE_GLYPH_EXTRACT:
					dl = &dl_extract;
				break;
				case AVALANCHE_GLYPH_STOP:
					dl = &dl_abort;
				break;
				case AVALANCHE_GLYPH_DISK:
					dl = &dl_disk;
				break;
				case AVALANCHE_GLYPH_NONE:
					dl = &dl_none;
				break;
			}

			glyphobj = DrawListObj,
					DRAWLIST_Directives, dl,
					DRAWLIST_RefHeight, 100,
					DRAWLIST_RefWidth, 100,
					IA_Width, 16,
					IA_Height, 16,
				End;
		} else {
			glyphobj = GlyphObj,
					GLYPH_Glyph, glyph,
				GlyphEnd;
		}
	}

	glyph_cache[glyph] = glyphobj;
	return glyphobj;
}

void glyph_init(void)
{
	for(int i = 0; i<AVALANCHE_GLYPH_MAX; i++) {
		glyph_cache[i] = NULL;
	}
}

void glyph_free(void)
{
	for(int i = 0; i<AVALANCHE_GLYPH_MAX; i++) {
		if(glyph_cache[i] != NULL) {
			DisposeObject(glyph_cache[i]);
		}
	}
}
