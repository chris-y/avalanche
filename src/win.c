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

#include "avalanche.h"

enum {
	GID_MAIN = 0,
	GID_ARCHIVE,
	GID_DEST,
	GID_LIST,
	GID_EXTRACT,
	GID_PROGRESS,
	GID_LAST
};

enum {
	WID_MAIN = 0,
	WID_LAST
};

enum {
	OID_MAIN = 0,
	OID_LAST
};

struct avalanche_window {
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];
	ULONG archiver;
	struct ColumnInfo *lbci;
	struct List lblist;
	struct Hook aslfilterhook;
	struct AppWindow *appwin;
	char *dest;
};

/** Menu **/

struct NewMenu menu[] = {
	{NM_TITLE,  NULL,           0,  0, 0, 0,}, // 0 Project
	{NM_ITEM,   NULL,         "O", 0, 0, 0,}, // 0 Open
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 1
	{NM_ITEM,   NULL , "!", NM_ITEMDISABLED, 0, 0,}, // 2 Archive Info
	{NM_ITEM,   NULL ,        "?", 0, 0, 0,}, // 3 About
	{NM_ITEM,   NM_BARLABEL,        0,  0, 0, 0,}, // 4
	{NM_ITEM,   NULL,         "Q", 0, 0, 0,}, // 5 Quit

	{NM_TITLE,  NULL,               0,  0, 0, 0,}, // 1 Edit
	{NM_ITEM,   NULL,       "A", NM_ITEMDISABLED, 0, 0,}, // 0 Select All
	{NM_ITEM,   NULL ,  "Z", NM_ITEMDISABLED, 0, 0,}, // 1 Clear Selection
	{NM_ITEM,   NULL , "I", NM_ITEMDISABLED, 0, 0,}, // 2 Invert

	{NM_TITLE,  NULL ,              0,  0, 0, 0,}, // 2 Settings
	{NM_ITEM,	NULL,     0, CHECKIT | MENUTOGGLE, 0, 0,}, // 0 Scan
	{NM_ITEM,	NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 1 HBrowser
	{NM_ITEM,   NULL , 0, CHECKIT | MENUTOGGLE, 0, 0,}, // 2 Win posn
	{NM_ITEM,   NULL ,         0, CHECKIT | MENUTOGGLE, 0, 0,}, // 3 Confirm quit
	{NM_ITEM,   NULL ,         0, CHECKIT | MENUTOGGLE, 0, 0,}, // 4 Ignore FS
	{NM_ITEM,   NM_BARLABEL,            0,  0, 0, 0,}, // 5
	{NM_ITEM,   NULL ,        0,  0, 0, 0,}, // 6 Save settings

	{NM_END,   NULL,        0,  0, 0, 0,},
};


/* Private functions */
static void toggle_item(struct Window *win, struct Gadget *list_gad, struct Node *node, ULONG select)
{
	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	ULONG current;
	ULONG selected;

	if(select == 2) {
		GetListBrowserNodeAttrs(node, LBNA_Checked, &current, TAG_DONE);
		selected = !current;
	} else {
		selected = select;
	}

	SetListBrowserNodeAttrs(node, LBNA_Checked, selected, TAG_DONE);

	SetGadgetAttrs(list_gad, win, NULL,
			LISTBROWSER_Labels, &lblist,
			TAG_DONE);
}


/* Window functions */

void *win_create(struct avalanche_config *config, struct MsgPort *winport, struct MsgPort *appport)
{
	struct avalanche_window *aw = AllocVec(sizeof(struct avalanche_window), MEMF_CLEAR);
	if(!aw) return NULL;
	
	ULONG tag_default_position = WINDOW_Position;

	aw->lbci = AllocLBColumnInfo(3, 
		LBCIA_Column, 0,
			LBCIA_Title,  locale_get_string(MSG_NAME),
			LBCIA_Weight, 65,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 1,
			LBCIA_Title,  locale_get_string(MSG_SIZE),
			LBCIA_Weight, 15,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		LBCIA_Column, 2,
			LBCIA_Title,  locale_get_string(MSG_DATE),
			LBCIA_Weight, 20,
			LBCIA_DraggableSeparator, TRUE,
			LBCIA_Sortable, TRUE,
			LBCIA_SortArrow, TRUE,
			LBCIA_AutoSort, TRUE,
		TAG_DONE);

	NewList(&lblist);

	if(config->virus_scan) menu[12].nm_Flags |= CHECKED;
	if(config->h_browser) menu[13].nm_Flags |= CHECKED;
	if(config->save_win_posn) menu[14].nm_Flags |= CHECKED;
	if(config->confirmquit) menu[15].nm_Flags |= CHECKED;
	if(config->ignorefs) menu[16].nm_Flags |= CHECKED;
	if(config->progname == NULL) menu[18].nm_Flags |= NM_ITEMDISABLED;

	if(config->win_x && config->win_y) tag_default_position = TAG_IGNORE;
	
	
	aw->aslfilterhook.h_Entry = aslfilterfunc;
	aw->aslfilterhook.h_SubEntry = NULL;
	aw->aslfilterhook.h_Data = NULL;
	
	
	/* Create the window object */
	aw->objects[OID_MAIN] = WindowObj,
		WA_ScreenTitle, VERS,
		WA_Title, VERS,
		WA_Activate, TRUE,
		WA_DepthGadget, TRUE,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_SizeGadget, TRUE,
		WA_Top, config->win_x,
		WA_Left, config->win_y,
		WA_Width, config->win_w,
		WA_Height, config->win_h,
		WINDOW_NewMenu, menu,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_IconTitle, "Avalanche",
		WINDOW_SharedPort, winport,
		WINDOW_AppPort, appport,
		tag_default_position, WPOS_CENTERSCREEN,
		WINDOW_ParentGroup, aw->gadgets[GID_MAIN] = LayoutVObj,
			//LAYOUT_DeferLayout, TRUE,
			LAYOUT_SpaceOuter, TRUE,
			LAYOUT_AddChild, LayoutHObj,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild,  aw->gadgets[GID_ARCHIVE] = GetFileObj,
						GA_ID, GID_ARCHIVE,
						GA_RelVerify, TRUE,
						GETFILE_TitleText,  locale_get_string( MSG_SELECTARCHIVE ) ,
						GETFILE_FullFile, archive,
						GETFILE_ReadOnly, TRUE,
						GETFILE_FilterFunc, &(aw->aslfilterhook),
					End,
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text,  locale_get_string( MSG_ARCHIVE ) ,
					LabelEnd,
					LAYOUT_AddChild,  aw->gadgets[GID_DEST] = GetFileObj,
						GA_ID, GID_DEST,
						GA_RelVerify, TRUE,
						GETFILE_TitleText,  locale_get_string( MSG_SELECTDESTINATION ) ,
						GETFILE_Drawer, dest,
						GETFILE_DoSaveMode, TRUE,
						GETFILE_DrawersOnly, TRUE,
						GETFILE_ReadOnly, TRUE,
					End,
					CHILD_WeightedHeight, 0,
					CHILD_Label, LabelObj,
						LABEL_Text,  locale_get_string( MSG_DESTINATION ) ,
					LabelEnd,
				LayoutEnd,
				LAYOUT_WeightBar, TRUE,
				LAYOUT_AddChild,  aw->gadgets[GID_PROGRESS] = FuelGaugeObj,
					GA_ID, GID_PROGRESS,
				FuelGaugeEnd,
				CHILD_WeightedWidth, progress_size,
			LayoutEnd,
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, LayoutVObj,
				LAYOUT_AddChild,  aw->gadgets[GID_LIST] = ListBrowserObj,
					GA_ID, GID_LIST,
					GA_RelVerify, TRUE,
					LISTBROWSER_ColumnInfo, aw->lbci,
					LISTBROWSER_Labels, &(aw->lblist),
					LISTBROWSER_ColumnTitles, TRUE,
					LISTBROWSER_TitleClickable, TRUE,
					LISTBROWSER_SortColumn, 0,
					LISTBROWSER_Striping, LBS_ROWS,
					LISTBROWSER_FastRender, TRUE,
					LISTBROWSER_Hierarchical, h_browser,
				ListBrowserEnd,
				LAYOUT_AddChild,  aw->gadgets[GID_EXTRACT] = ButtonObj,
					GA_ID, GID_EXTRACT,
					GA_RelVerify, TRUE,
					GA_Text, GID_EXTRACT_TEXT,
				ButtonEnd,
				CHILD_WeightedHeight, 0,
			LayoutEnd,
		EndGroup,
	EndWindow;


	/*  Object creation sucessful?
	 */
	if (objects[OID_MAIN]) {
		

	
	return aw;
}

void window_open(void *awin, struct MsgPort *appwin_mp)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->windows[WID_MAIN]) {
		aw->WindowToFront(windows[WID_MAIN]);
	} else {
		aw->windows[WID_MAIN] = (struct Window *)RA_OpenWindow(aw->objects[OID_MAIN]);
		
		if(aw->windows[WID_MAIN]) {
			aw->appwin = AddAppWindowA(0, aw, aw->windows[WID_MAIN], appwin_mp, NULL);
		}
	}
}			

void window_close(void *awin, BOOL iconify)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	if(aw->windows[WID_MAIN]) {
		RemoveAppWindow(aw->appwin);
		RA_CloseWindow(aw->objects[OID_MAIN]);
		aw->windows[WID_MAIN] = NULL;
	}
}

void window_dispose(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	/* Ensure appwindow is removed */
	if(aw->windows[WID_MAIN]) window_close(aw, FALSE);
	
	/* Dispose window object */
	DisposeObject(aw->objects[OID_MAIN]);
	
	/* Free archive browser list and column info */
	FreeListBrowserList(&aw->lblist);
	if(aw->lbci) FreeLBColumnInfo(lbci);
}

void window_list_handle(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	ULONG tmp = 0;
	struct Node *node = NULL;
	
	GetAttr(LISTBROWSER_RelEvent, aw->gadgets[GID_LIST], (APTR)&tmp);
	switch(tmp) {
#if 0 /* This selects items when single-clicked off the checkbox -
it's incompatible with doube-clicking as it resets the listview */
		case LBRE_NORMAL:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw->windows[WID_MAIN], aw->gadgets[GID_LIST], node, 2);
		break;
#endif
		case LBRE_DOUBLECLICK:
			GetAttr(LISTBROWSER_SelectedNode, aw->gadgets[GID_LIST], (APTR)&node);
			toggle_item(aw->windows[WID_MAIN], aw->gadgets[GID_LIST], node, 1); /* ensure selected */
			char fn[1024];
			strcpy(fn, config.tmpdir);
			ret = extract(fn, node);
			if(ret == 0) {
				AddPart(fn, get_item_filename(node), 1024);
				add_to_delete_list(fn);
				OpenWorkbenchObjectA(fn, NULL);
			} else {
				show_error(ret);
			}
		break;
	}
}


void window_update_archive(void *awin, char *archive)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	SetGadgetAttrs(aw->gadgets[GID_ARCHIVE], aw->windows[WID_MAIN], NULL,
					GETFILE_FullFile, archive, TAG_DONE);
}

char *window_req_dest(void *awin)
{	
	struct avalanche_window *aw = (struct avalanche_window *)awin;
	
	DoMethod((Object *)aw->gadgets[GID_DEST], GFILE_REQUEST, aw->windows[WID_MAIN]);
	GetAttr(GETFILE_Drawer, aw->gadgets[GID_DEST], (APTR)&aw->dest);
	
	return aw->dest;
}

void window_req_open_archive(void *awin, BOOL refresh_only)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	dir_seen = FALSE;
	long ret = 0;
	long retxfd = 0;

	if(refresh_only == FALSE) {
		ret = DoMethod((Object *) aw->gadgets[GID_ARCHIVE], GFILE_REQUEST, aw->windows[WID_MAIN]);
		if(ret == 0) return;
	}

	// do we need this?  if(archive_needs_free) free_archive_path();
	GetAttr(GETFILE_FullFile, aw->gadgets[GID_ARCHIVE], (APTR)&aw->archive);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
										WA_BusyPointer, TRUE,
										TAG_DONE);

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
			LISTBROWSER_Labels, ~0, TAG_DONE);

	FreeListBrowserList(&aw->lblist);

	ret = xad_info(aw->archive, !config.ignorefs, addlbnode_cb);
	if(ret != 0) { /* if xad failed try xfd */
		retxfd = xfd_info(aw->archive, addlbnodexfd_cb);
		if(retxfd != 0) show_error(ret);
	}

	if(ret == 0) {
		archiver = ARC_XAD;
		menu_activation(TRUE);
	} else if(retxfd == 0) {
		archiver = ARC_XFD;
		menu_activation(TRUE);
	} else {
		archiver = ARC_NONE;
		menu_activation(FALSE);
	}

	SetGadgetAttrs(aw->gadgets[GID_LIST], aw->windows[WID_MAIN], NULL,
				LISTBROWSER_Labels, &lblist,
				LISTBROWSER_SortColumn, 0,
			TAG_DONE);

	if(aw->windows[WID_MAIN]) SetWindowPointer(aw->windows[WID_MAIN],
											WA_BusyPointer, FALSE,
											TAG_DONE);
}


Object *window_get_object(void *awin)
{
	struct avalanche_window *aw = (struct avalanche_window *)awin;

	return aw->objects[OID_MAIN];
}

void fill_menu_labels(void)
{
	menu[0].nm_Label = locale_get_string( MSG_PROJECT );
	menu[1].nm_Label = locale_get_string( MSG_OPEN );
	menu[3].nm_Label = locale_get_string( MSG_ARCHIVEINFO );
	menu[4].nm_Label = locale_get_string( MSG_ABOUT );
	menu[6].nm_Label = locale_get_string( MSG_QUIT );
	menu[7].nm_Label = locale_get_string( MSG_EDIT );
	menu[8].nm_Label = locale_get_string( MSG_SELECTALL );
	menu[9].nm_Label = locale_get_string( MSG_CLEARSELECTION );
	menu[10].nm_Label = locale_get_string( MSG_INVERTSELECTION );
	menu[11].nm_Label = locale_get_string( MSG_SETTINGS );
	menu[12].nm_Label = locale_get_string( MSG_SCANFORVIRUSES );
	menu[13].nm_Label = locale_get_string( MSG_HIERARCHICALBROWSEREXPERIMENTAL );
	menu[14].nm_Label = locale_get_string( MSG_SAVEWINDOWPOSITION );
	menu[15].nm_Label = locale_get_string( MSG_CONFIRMQUIT );
	menu[16].nm_Label = locale_get_string( MSG_IGNOREFILESYSTEMS );
	menu[18].nm_Label = locale_get_string( MSG_SAVESETTINGS );
}


