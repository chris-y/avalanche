/* Avalanche
 * (c) 2022 Chris Young
 */

#ifndef LIBS_H
#define LIBS_H 1

#include <intuition/classes.h>

extern Class *ButtonClass;
extern Class *GetFileClass;
extern Class *LabelClass;
extern Class *LayoutClass;
extern Class *ListBrowserClass;
extern Class *RequesterClass;
extern Class *WindowClass;

#define ButtonObj		NewObject(ButtonClass, NULL
#define GetFileObj		NewObject(GetFileClass, NULL
#define LabelObj		NewObject(LabelClass, NULL
#define LayoutHObj		NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ
#define LayoutVObj		NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#define ListBrowserObj	NewObject(ListBrowserClass, NULL
#define RequesterObj	NewObject(RequesterClass, NULL
#define WindowObj		NewObject(WindowClass, NULL

BOOL libs_open(void);
void libs_close(void);
#endif
