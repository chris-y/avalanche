/* Avalanche
 * (c) 2022-3 Chris Young
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

#ifndef LIBS_H
#define LIBS_H 1

#include <intuition/classes.h>

#ifdef __amigaos4__
#define DeleteFile Delete
#endif

extern Class *ARexxClass;
extern Class *BitMapClass;
extern Class *ButtonClass;
extern Class *CheckBoxClass;
extern Class *ChooserClass;
extern Class *FuelGaugeClass;
extern Class *GetFileClass;
extern Class *GlyphClass;
extern Class *LabelClass;
extern Class *LayoutClass;
extern Class *ListBrowserClass;
extern Class *RequesterClass;
extern Class *StringClass;
extern Class *WindowClass;

#define ARexxObj		NewObject(ARexxClass, NULL
#define BitMapObj		NewObject(BitMapClass, NULL
#define ButtonObj		NewObject(ButtonClass, NULL
#define CheckBoxObj		NewObject(CheckBoxClass, NULL
#define ChooserObj		NewObject(ChooserClass, NULL
#define FuelGaugeObj	NewObject(FuelGaugeClass, NULL
#define GetFileObj		NewObject(GetFileClass, NULL
#define GlyphObj		NewObject(GlyphClass, NULL
#define LabelObj		NewObject(LabelClass, NULL
#define LayoutHObj		NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ
#define LayoutVObj		NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#define ListBrowserObj	NewObject(ListBrowserClass, NULL
#define RequesterObj	NewObject(RequesterClass, NULL
#define StringObj		NewObject(StringClass, NULL
#define WindowObj		NewObject(WindowClass, NULL

BOOL libs_open(void);
void libs_close(void);

BOOL libs_xad_init(void);
void libs_xad_exit(void);

BOOL libs_xfd_init(void);
void libs_xfd_exit(void);

BOOL libs_xvs_init(void);

BOOL libs_zip_init(void);
void libs_zip_exit(void);

#endif
