/* 
 *
 *   File: interface.h
 *
 *   Copyright (C) 2009-2013 Darran Kartaschew
 *
 *   This file is part of the gMTP package.
 *
 *   gMTP is free software; you can redistribute it and/or modify
 *   it under the terms of the BSD License as included within the
 *   file 'COPYING' located in the root directory
 *
 */

#ifndef _INTERFACE_H
#define _INTERFACE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    // Main Window List

    enum fileListID {
        COL_FILENAME = 0,
        COL_FILENAME_HIDDEN,
        COL_FILENAME_ACTUAL,
        COL_FILESIZE,
        COL_FILEID,
        COL_ISFOLDER,
        COL_FILESIZE_HID,
        COL_TYPE,
        COL_TRACK_NUMBER,
        COL_TRACK_NUMBER_HIDDEN,
        COL_TITLE,
        COL_FL_ARTIST,
        COL_FL_ALBUM,
        COL_YEAR,
        COL_GENRE,
        COL_DURATION,
        COL_DURATION_HIDDEN,
        COL_ICON,
        COL_LOCATION,
        NUM_COLUMNS
    };

    enum folderListID {
        COL_FOL_NAME = 0,
        COL_FOL_NAME_HIDDEN,
        COL_FOL_ID,
        COL_FOL_ICON,
        NUM_FOL_COLUMNS
    };

    // Playlist windows lists.

    enum fileTrackID {
        COL_ARTIST = 0,
        COL_ALBUM,
        COL_TRACKID,
        COL_TRACKNAME,
        COL_TRACKDURATION,
        NUM_TCOLUMNS
    };

    enum filePlaylistID {
        COL_PL_ORDER_NUM = 0,
        COL_PL_ARTIST,
        COL_PL_ALBUM,
        COL_PL_TRACKID,
        COL_PL_TRACKNAME,
        COL_PL_TRACKDURATION,
        NUM_PL_COLUMNS
    };

    typedef struct {
        uint32_t itemid;
        gboolean isFolder;
        gchar *filename;
        uint64_t filesize;
        LIBMTP_filetype_t filetype;
        gchar *location;
    } FileListStruc;

    typedef struct {
        uint32_t album_id;
        gchar* filename;
    } Album_Struct;

    // File operation enums

    enum MTP_OVERWRITEOP {
        MTP_ASK,
        MTP_SKIP,
        MTP_SKIP_ALL,
        MTP_OVERWRITE,
        MTP_OVERWRITE_ALL
    };

    // Main Window widgets
    GtkListStore *fileList;
    GtkTreeStore *folderList;
    GtkTreeSelection *fileSelection;
    GtkTreeSelection *folderSelection;
    gulong folderSelectHandler;
    gulong fileSelectHandler;

    GtkWidget* create_windowMain(void);
    void setWindowTitle(gchar *foldername);
    GtkWidget* create_windowMainContextMenu(void);
    GtkWidget* create_windowMainColumnContextMenu(void);
    GtkWidget* create_windowFolderContextMenu(void);

    void SetToolbarButtonState(gboolean);
    void statusBarSet(gchar *text);
    void statusBarClear();

    gboolean fileListClear();
    GSList* getFileGetList2Add();
    gboolean fileListAdd();
    gchar* displayRenameFileDialog(gchar* currentfilename);
    gboolean fileListRemove(GList *List);
    gboolean fileListDownload(GList *List);
    GList* fileListGetSelection();
    gboolean fileListClearSelection();
    gboolean fileListSelectAll(void);

    gboolean folderListRemove(GList *List);
    gboolean folderListClear();
    gboolean folderListAdd(LIBMTP_folder_t *folders, GtkTreeIter *parent);
    int64_t folderListGetSelection(void);
    gchar *folderListGetSelectionName(void);
    gboolean folderListDownload(gchar *foldername, uint32_t folderid);

    int64_t getTargetFolderLocation(void);
    gboolean folderListAddDialog(LIBMTP_folder_t *folders, GtkTreeIter *parent, GtkTreeStore *fl);
    void __fileMove(GtkTreeRowReference *Row);

    // Flags for overwriting files of host PC and device.
    gint fileoverwriteop;
    // Flag to allow overwrite of files on device.
    gint deviceoverwriteop;

    // Find options and variables.
    gboolean inFindMode;
    GSList *searchList;
    void g_free_search(FileListStruc *file);
    GtkWidget *FindToolbar_entry_FindText;
    GtkWidget *FindToolbar_checkbutton_FindFiles;
    GtkWidget *FindToolbar_checkbutton_TrackInformation;

    // Aggreegate function for adding a file to the device.
    void __filesAdd(gchar* filename);

    gchar *calculateFriendlySize(const uint64_t value);
    
    // Error dialog.
    void displayError(gchar* msg);
    void displayInformation(gchar* msg);

    // New Folder Dialog;
    gchar* displayFolderNewDialog(void);

    // Overwrite this file dialog?
    gint displayFileOverwriteDialog(gchar *filename);

    // Multidevice/Multistorage dialog;
    gint displayMultiDeviceDialog(void);
    gint displayDeviceStorageDialog(void);
    gchar* displayChangeDeviceNameDialog(gchar* devicename);
   
    // Add track to playlist dialog;
    int32_t displayAddTrackPlaylistDialog(gboolean showNew /* = TRUE */);

    // Widget for find toolbar
    GtkWidget *findToolbar;
    
    // Parent container for the main toolbar.
    GtkWidget *handlebox1;
    GtkWidget *toolbarMain;

    // Widgets for menu items;
    GtkWidget *fileConnect;
    GtkWidget *fileAdd;
    GtkWidget *fileDownload;
    GtkWidget *fileRemove;
    GtkWidget *fileRename;
    GtkWidget *fileMove;
    GtkWidget *fileNewFolder;
    GtkWidget *fileRemoveFolder;
    GtkWidget *fileRescan;
    GtkWidget *editDeviceName;
    GtkWidget *editFormatDevice;
    GtkWidget *editAddAlbumArt;
    GtkWidget *editFind;
    GtkWidget *editSelectAll;
    GtkWidget *contextMenu;
    GtkWidget *contextMenuColumn;
    GtkWidget *contestMenuFolder;
    GtkWidget* cfileAdd;
    GtkWidget* cfileNewFolder;
    GtkWidget *toolbuttonAddFile;
#if HAVE_GTK3 == 0
    GtkTooltips *tooltipsToolbar;
#endif

    // Columns in main file view;
    GtkTreeViewColumn *column_Size;
    GtkTreeViewColumn *column_Type;
    GtkTreeViewColumn *column_Track_Number;
    GtkTreeViewColumn *column_Title;
    GtkTreeViewColumn *column_Artist;
    GtkTreeViewColumn *column_Album;
    GtkTreeViewColumn *column_Year;
    GtkTreeViewColumn *column_Genre;
    GtkTreeViewColumn *column_Duration;
    GtkTreeViewColumn *column_Location;

    // Main menu widgets
    GtkWidget *menu_view_filesize;
    GtkWidget *menu_view_filetype;
    GtkWidget *menu_view_track_number;
    GtkWidget *menu_view_title;
    GtkWidget *menu_view_artist;
    GtkWidget *menu_view_album;
    GtkWidget *menu_view_year;
    GtkWidget *menu_view_genre;
    GtkWidget *menu_view_duration;
    GtkWidget *menu_view_folders;
    GtkWidget *menu_view_toolbar;

    // Column view context menu;
    GtkWidget* cViewSize;
    GtkWidget* cViewType;
    GtkWidget* cViewTrackName;
    GtkWidget* cViewTrackNumber;
    GtkWidget* cViewArtist;
    GtkWidget* cViewAlbum;
    GtkWidget* cViewYear;
    GtkWidget* cViewGenre;
    GtkWidget* cViewDuration;

    // Combobox used in AddTrackPlaylist feature.
    GtkWidget *combobox_AddTrackPlaylist;

    int64_t fileMoveTargetFolder;

#ifdef  __cplusplus
}
#endif

#endif  /* _INTERFACE_H */


