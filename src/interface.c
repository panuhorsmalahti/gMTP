/* 
 *
 *   File: interface.c
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

#include "config.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#include <gdk/gdkkeysyms-compat.h>
#endif
#include <libgen.h>
#include <libmtp.h>
#include <id3tag.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "progress.h"
#include "properties.h"
#include "preferences.h"
#include "playlist.h"
#include "albumart.h"
#include "formatdevice.h"

void setupFileList();
GtkTreeViewColumn *setupFolderList();
void __fileRemove(GtkTreeRowReference *Row);
void __fileDownload(GtkTreeRowReference *Row);
void __folderRemove(GtkTreeRowReference *Row);

GtkWidget *windowMain;
GtkWidget *scrolledwindowMain;
GtkWidget *toolbuttonAddFile;
GtkWidget *toolbuttonRetrieve;
GtkWidget *toolbuttonRemoveFile;
GtkWidget *toolbuttonRescan;
GtkWidget *toolbuttonAlbumArt;
GtkWidget *toolbuttonPlaylist;
GtkWidget *toolbuttonProperties;
GtkWidget *properties1;
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
GtkWidget *editPlaylist;
GtkWidget *editFind;
GtkWidget *editSelectAll;

GtkWidget *contextMenu;
GtkWidget *contextMenuColumn;
GtkWidget *contextMenuFolder;
GtkWidget* cfileAdd;
GtkWidget* cfileNewFolder;

// Main toolbar parent container.
GtkWidget *handlebox1;
GtkWidget *toolbarMain; // The actual toolbar.

GtkWidget *findToolbar;

GtkWidget *scrolledwindowFolders;

#if HAVE_GTK3 == 0
GtkTooltips *tooltipsToolbar;
#endif

GtkListStore *fileList;
GtkTreeModel *fileListModel;
GtkTreeSelection *fileSelection;
GtkTreeStore *folderList;
GtkTreeModel *folderListModel;
GtkTreeViewColumn *folderColumn;
GtkTreeSelection *folderSelection;
GList *fileSelection_RowReferences = NULL;

gulong folderSelectHandler = 0;
gulong fileSelectHandler = 0;

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

GtkWidget* cViewSize;
GtkWidget* cViewType;
GtkWidget* cViewTrackName;
GtkWidget* cViewTrackNumber;
GtkWidget* cViewArtist;
GtkWidget* cViewAlbum;
GtkWidget* cViewYear;
GtkWidget* cViewGenre;
GtkWidget* cViewDuration;

GtkWidget* cfFolderAdd;
GtkWidget* cfFolderDelete;
GtkWidget* cfFolderRename;
GtkWidget* cfFolderMove;
GtkWidget* cfFolderRefresh;

// Flags for overwriting files of host PC and device.
gint fileoverwriteop = MTP_ASK;
// Flag to allow overwrite of files on device.
gint deviceoverwriteop = MTP_ASK;

// Find options and variables.
gboolean inFindMode = FALSE;
GSList *searchList = NULL;
GtkWidget *FindToolbar_entry_FindText;
GtkWidget *FindToolbar_checkbutton_FindFiles;
GtkWidget *FindToolbar_checkbutton_TrackInformation;

// Combobox used in AddTrackPlaylist feature.
GtkWidget *combobox_AddTrackPlaylist;

// File/Folder Move operations.
int64_t fileMoveTargetFolder = 0;

// ************************************************************************************************

/**
 * Create the main window for the application
 * @return Ptr to the main window widget
 */
GtkWidget* create_windowMain(void) {
    GtkWidget *vbox1;
    GtkWidget *menubarMain;
    GtkWidget *menuitem1;
    GtkWidget *menuitem1_menu;
    GtkWidget *menuseparator1;
    GtkWidget *menuseparator2;
    GtkWidget *menuseparator3;
    GtkWidget *menuseparator4;
    GtkWidget *menuseparator5;
    GtkWidget *menuseparator6;
    GtkWidget *menuseparator7;
    GtkWidget *menuseparator8;
    GtkWidget *menuseparator9;
    GtkWidget *menuseparator10;
    GtkWidget *quit1;
    GtkWidget *menuitem2;
    GtkWidget *menuitem2_menu;
    GtkWidget *preferences1;
    GtkWidget *menuView;
    GtkWidget *menuView_menu;

    GtkWidget *menuitem4;
    GtkWidget *menuitem4_menu;
    GtkWidget *about1;

    gint tmp_toolbar_icon_size;
    GtkWidget *tmp_image;
    GtkWidget *toolbuttonPreferences;
    GtkWidget *hpanel;

    GtkWidget *toolbarSeparator;
    GtkWidget *toolbarSeparator2;
    //GtkWidget *toolbuttonQuit;

    GtkWidget *FindToolbar_hbox_FindToolbar;
    GtkWidget *FindToolbar_label_FindLabel;
    GtkWidget *FindToolbar_FindButton;
    GtkWidget *FindToolbar_CloseButton;

    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new();

    windowMain = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setWindowTitle(NULL);
    gtk_window_set_default_size(GTK_WINDOW(windowMain), 880, 400);
    gtk_window_set_icon_from_file(GTK_WINDOW(windowMain), file_icon48_png, NULL);
#if HAVE_GTK3 == 0
    vbox1 = gtk_vbox_new(FALSE, 0);
#else 
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(windowMain), vbox1);

    menubarMain = gtk_menu_bar_new();
    gtk_widget_show(menubarMain);
    gtk_box_pack_start(GTK_BOX(vbox1), menubarMain, FALSE, FALSE, 0);

    menuitem1 = gtk_menu_item_new_with_mnemonic(_("_File"));
    gtk_widget_show(menuitem1);
    gtk_container_add(GTK_CONTAINER(menubarMain), menuitem1);

    menuitem1_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem1), menuitem1_menu);

    fileConnect = gtk_menu_item_new_with_mnemonic(_("_Connect Device"));
    gtk_widget_show(fileConnect);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileConnect);

    menuseparator4 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator4);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), menuseparator4);

    fileAdd = gtk_menu_item_new_with_label(_("Add Files"));
    gtk_widget_show(fileAdd);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileAdd);

    fileRemove = gtk_menu_item_new_with_label(_("Delete Files"));
    gtk_widget_show(fileRemove);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileRemove);

    fileRename = gtk_menu_item_new_with_label(_("Rename Files"));
    gtk_widget_show(fileRename);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileRename);

    fileMove = gtk_menu_item_new_with_label(_("Move To..."));
    gtk_widget_show(fileMove);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileMove);

    fileDownload = gtk_menu_item_new_with_label(_("Download Files"));
    gtk_widget_show(fileDownload);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileDownload);

    menuseparator1 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator1);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), menuseparator1);

    fileNewFolder = gtk_menu_item_new_with_label(_("Create Folder"));
    gtk_widget_show(fileNewFolder);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileNewFolder);

    fileRemoveFolder = gtk_menu_item_new_with_label(_("Delete Folder"));
    gtk_widget_show(fileRemoveFolder);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileRemoveFolder);

    menuseparator2 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator2);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), menuseparator2);

    fileRescan = gtk_menu_item_new_with_label(_("Refresh Folder"));
    //menuText = gtk_bin_get_child(GTK_BIN(fileRescan));
    //gtk_label_set_text(GTK_LABEL(menuText), _("Refresh Device"));
    gtk_widget_show(fileRescan);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), fileRescan);

    properties1 = gtk_menu_item_new_with_label(_("Device Properties"));
    //menuText = gtk_bin_get_child(GTK_BIN(properties1));
    //gtk_label_set_text(GTK_LABEL(menuText), _("Device Properties"));
    gtk_widget_show(properties1);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), properties1);

    menuseparator3 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator3);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), menuseparator3);

    quit1 = gtk_menu_item_new_with_mnemonic(_("_Quit"));
    gtk_widget_show(quit1);
    gtk_container_add(GTK_CONTAINER(menuitem1_menu), quit1);

    menuitem2 = gtk_menu_item_new_with_mnemonic(_("_Edit"));
    gtk_widget_show(menuitem2);
    gtk_container_add(GTK_CONTAINER(menubarMain), menuitem2);

    menuitem2_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem2), menuitem2_menu);

    editSelectAll = gtk_menu_item_new_with_label(_("Select All"));
    gtk_widget_show(editSelectAll);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editSelectAll);

    menuseparator6 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator6);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), menuseparator6);

    editFind = gtk_menu_item_new_with_label(_("Find"));
    gtk_widget_show(editFind);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editFind);

    menuseparator9 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator9);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), menuseparator9);

    editDeviceName = gtk_menu_item_new_with_label(_("Change Device Name"));
    gtk_widget_show(editDeviceName);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editDeviceName);

    editFormatDevice = gtk_menu_item_new_with_label(_("Format Device"));
    gtk_widget_show(editFormatDevice);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editFormatDevice);

    menuseparator7 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator7);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), menuseparator7);

    editAddAlbumArt = gtk_menu_item_new_with_label(_("Album Art"));
    gtk_widget_show(editAddAlbumArt);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editAddAlbumArt);

    editPlaylist = gtk_menu_item_new_with_label(_("Edit Playlist(s)"));
    gtk_widget_show(editPlaylist);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), editPlaylist);

    menuseparator5 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator5);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), menuseparator5);

    preferences1 = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
    gtk_widget_show(preferences1);
    gtk_container_add(GTK_CONTAINER(menuitem2_menu), preferences1);

    menuView = gtk_menu_item_new_with_mnemonic(_("_View"));
    gtk_widget_show(menuView);
    gtk_container_add(GTK_CONTAINER(menubarMain), menuView);


    menuView_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuView), menuView_menu);

    menu_view_toolbar = gtk_check_menu_item_new_with_label(_("Toolbar"));
    gtk_widget_show(menu_view_toolbar);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_toolbar);

    menuseparator10 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator10);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menuseparator10);

    menu_view_folders = gtk_check_menu_item_new_with_label(_("Folders"));
    gtk_widget_show(menu_view_folders);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_folders);

    menuseparator8 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator8);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menuseparator8);

    menu_view_filesize = gtk_check_menu_item_new_with_label(_("File Size"));
    gtk_widget_show(menu_view_filesize);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_filesize);

    menu_view_filetype = gtk_check_menu_item_new_with_label(_("File Type"));
    gtk_widget_show(menu_view_filetype);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_filetype);

    menu_view_track_number = gtk_check_menu_item_new_with_label(_("Track Number"));
    gtk_widget_show(menu_view_track_number);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_track_number);

    menu_view_title = gtk_check_menu_item_new_with_label(_("Track Name"));
    gtk_widget_show(menu_view_title);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_title);

    menu_view_artist = gtk_check_menu_item_new_with_label(_("Artist"));
    gtk_widget_show(menu_view_artist);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_artist);

    menu_view_album = gtk_check_menu_item_new_with_label(_("Album"));
    gtk_widget_show(menu_view_album);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_album);

    menu_view_year = gtk_check_menu_item_new_with_label(_("Year"));
    gtk_widget_show(menu_view_year);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_year);

    menu_view_genre = gtk_check_menu_item_new_with_label(_("Genre"));
    gtk_widget_show(menu_view_genre);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_genre);

    menu_view_duration = gtk_check_menu_item_new_with_label(_("Duration"));
    gtk_widget_show(menu_view_duration);
    gtk_container_add(GTK_CONTAINER(menuView_menu), menu_view_duration);

    menuitem4 = gtk_menu_item_new_with_mnemonic(_("_Help"));
    gtk_widget_show(menuitem4);
    gtk_container_add(GTK_CONTAINER(menubarMain), menuitem4);

    menuitem4_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem4), menuitem4_menu);

    about1 = gtk_menu_item_new_with_mnemonic(_("_About"));
    gtk_widget_show(about1);
    gtk_container_add(GTK_CONTAINER(menuitem4_menu), about1);

#if HAVE_GTK3 == 0
    handlebox1 = gtk_handle_box_new();
    gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(handlebox1), GTK_SHADOW_ETCHED_OUT);
#else 
    handlebox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_show(handlebox1);
    gtk_box_pack_start(GTK_BOX(vbox1), handlebox1, FALSE, FALSE, 0);

#if HAVE_GTK3 == 0
    tooltipsToolbar = gtk_tooltips_new();
#endif

    toolbarMain = gtk_toolbar_new();
    gtk_widget_show(toolbarMain);
#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(handlebox1), toolbarMain);
#else
    gtk_box_pack_start(GTK_BOX(handlebox1), toolbarMain, TRUE, TRUE, 0);
#endif
    if(Preferences.toolbarStyle != NULL){
        if(g_ascii_strcasecmp(Preferences.toolbarStyle->str, "icon") == 0){
            gtk_toolbar_set_style(GTK_TOOLBAR(toolbarMain), GTK_TOOLBAR_ICONS);
        } else if(g_ascii_strcasecmp(Preferences.toolbarStyle->str, "text") == 0){
            gtk_toolbar_set_style(GTK_TOOLBAR(toolbarMain), GTK_TOOLBAR_TEXT);
        } else if(g_ascii_strcasecmp(Preferences.toolbarStyle->str, "both") == 0){
            gtk_toolbar_set_style(GTK_TOOLBAR(toolbarMain), GTK_TOOLBAR_BOTH);
        }
    } else {
        gtk_toolbar_set_style(GTK_TOOLBAR(toolbarMain), GTK_TOOLBAR_BOTH);
    }
    tmp_toolbar_icon_size = gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbarMain));

#if HAVE_GTK3 == 0
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbarMain), TRUE);
#else
    g_object_set(gtk_settings_get_default(), "gtk-enable-tooltips", TRUE, NULL);
#endif

#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_NETWORK, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("network-workgroup", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonConnect = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Connect"));
    gtk_widget_show(toolbuttonConnect);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonConnect);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonConnect), GTK_TOOLTIPS(tooltipsToolbar), _("Connect/Disconnect to your device."), _("Connect/Disconnect to your device."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonConnect), _("Connect/Disconnect to your device."));
#endif

    toolbarSeparator = (GtkWidget*) gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolbarSeparator), TRUE);
    gtk_widget_show(toolbarSeparator);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbarSeparator);

#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_ADD, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("list-add", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonAddFile = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Add"));
    gtk_widget_show(toolbuttonAddFile);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonAddFile);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonAddFile), GTK_TOOLTIPS(tooltipsToolbar), _("Add Files to your device."), _("Add a varity of Files to your device in the current folder."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonAddFile), _("Add Files to your device."));
#endif
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_REMOVE, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("list-remove", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonRemoveFile = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Delete"));
    gtk_widget_show(toolbuttonRemoveFile);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonRemoveFile);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonRemoveFile), GTK_TOOLTIPS(tooltipsToolbar), _("Delete Files/Folders from your device."), _("Permanently remove files/folders from your device. Note: Albums are stored as *.alb files."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonRemoveFile), _("Delete Files/Folders from your device."));
#endif
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_GOTO_BOTTOM, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("go-bottom", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonRetrieve = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Download"));
    gtk_widget_show(toolbuttonRetrieve);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonRetrieve);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonRetrieve), GTK_TOOLTIPS(tooltipsToolbar), _("Download Files from your device to your Host PC."), _("Download files from your device to your PC. Default Download path is set in the preferences dialog."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonRetrieve), _("Download Files from your device to your Host PC."));
#endif

    toolbarSeparator2 = (GtkWidget*) gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolbarSeparator2), TRUE);
    gtk_widget_show(toolbarSeparator2);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbarSeparator2);

#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_CDROM, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("media-optical", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonAlbumArt = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Album Art"));
    gtk_widget_show(toolbuttonAlbumArt);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonAlbumArt);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonAlbumArt), GTK_TOOLTIPS(tooltipsToolbar), _("Upload an image file as Album Art."), _("Upload a JPG file and assign it as Album Art."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonAlbumArt), _("Upload an image file as Album Art."));
#endif
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_DND_MULTIPLE, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("emblem-documents", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonPlaylist = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Playlists"));
    gtk_widget_show(toolbuttonPlaylist);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonPlaylist);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonPlaylist), GTK_TOOLTIPS(tooltipsToolbar), _("Add and Modify Playlists."), _("Add and Modify Playlists."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonPlaylist), _("Add and Modify Playlists."));
#endif
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("view-refresh", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonRescan = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Refresh"));
    gtk_widget_show(toolbuttonRescan);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonRescan);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonRescan), GTK_TOOLTIPS(tooltipsToolbar), _("Refresh File/Folder listing."), _("Refresh File/Folder listing."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonRescan), _("Refresh File/Folder listing."));
#endif

    toolbarSeparator = (GtkWidget*) gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolbarSeparator), TRUE);
    gtk_widget_show(toolbarSeparator);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbarSeparator);

#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("document-properties", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonProperties = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Properties"));
    gtk_widget_show(toolbuttonProperties);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonProperties);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonProperties), GTK_TOOLTIPS(tooltipsToolbar), _("View Device Properties."), _("View Device Properties."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonProperties), _("View Device Properties."));
#endif
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("preferences-system", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonPreferences = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Preferences"));
    gtk_widget_show(toolbuttonPreferences);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonPreferences);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonPreferences), GTK_TOOLTIPS(tooltipsToolbar), _("View/Change gMTP Preferences."), _("View/Change gMTP Preferences."));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonPreferences), _("View/Change gMTP Preferences."));
#endif

/*    toolbarSeparator = (GtkWidget*) gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolbarSeparator), TRUE);
    gtk_widget_show(toolbarSeparator);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbarSeparator);
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_QUIT, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("application-exit", tmp_toolbar_icon_size);
#endif
    gtk_widget_show(tmp_image);
    toolbuttonQuit = (GtkWidget*) gtk_tool_button_new(tmp_image, _("Quit"));
    gtk_widget_show(toolbuttonQuit);
    gtk_container_add(GTK_CONTAINER(toolbarMain), toolbuttonQuit);
#if HAVE_GTK3 == 0
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolbuttonQuit), GTK_TOOLTIPS(tooltipsToolbar), _("Quit gMTP."), _("Quit"));
#else
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(toolbuttonQuit), _("Quit gMTP."));
#endif
*/
#if HAVE_GTK3 == 0
    gtk_tooltips_enable(tooltipsToolbar);
#endif

    // Find toolbar;
#if HAVE_GTK3 == 0
    findToolbar = gtk_handle_box_new();
    //gtk_widget_show(findToolbar); // Only show when the user selects the menu option.
    gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(findToolbar), GTK_SHADOW_ETCHED_OUT);
#else 
    findToolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_box_pack_start(GTK_BOX(vbox1), findToolbar, FALSE, FALSE, 0);

#if HAVE_GTK3 == 0
    FindToolbar_hbox_FindToolbar = gtk_hbox_new(FALSE, 5);
#else 
    FindToolbar_hbox_FindToolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(FindToolbar_hbox_FindToolbar);
    gtk_container_add(GTK_CONTAINER(findToolbar), FindToolbar_hbox_FindToolbar);
    gtk_container_set_border_width(GTK_CONTAINER(FindToolbar_hbox_FindToolbar), 2);

    FindToolbar_label_FindLabel = gtk_label_new(_("Find:"));
    gtk_widget_show(FindToolbar_label_FindLabel);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_label_FindLabel, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(FindToolbar_label_FindLabel), GTK_JUSTIFY_RIGHT);

    FindToolbar_entry_FindText = gtk_entry_new();
    gtk_widget_show(FindToolbar_entry_FindText);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_entry_FindText, TRUE, TRUE, 0);
    gtk_entry_set_max_length(GTK_ENTRY(FindToolbar_entry_FindText), 256);
#if HAVE_GTK3 == 0
    FindToolbar_FindButton = gtk_button_new_from_stock(GTK_STOCK_FIND);
#else
#if GTK_CHECK_VERSION(3,10,0)
    FindToolbar_FindButton = gtk_button_new_from_icon_name("edit-find", tmp_toolbar_icon_size);
#else
    FindToolbar_FindButton = gtk_button_new_from_stock(GTK_STOCK_FIND);
#endif
#endif
    gtk_widget_show(FindToolbar_FindButton);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_FindButton, FALSE, FALSE, 5);
    //gtk_container_set_border_width(GTK_CONTAINER(FindToolbar_FindButton), 5);

    FindToolbar_checkbutton_FindFiles = gtk_check_button_new_with_mnemonic(_("Filenames"));
    gtk_widget_show(FindToolbar_checkbutton_FindFiles);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_checkbutton_FindFiles, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FindToolbar_checkbutton_FindFiles), TRUE);

    FindToolbar_checkbutton_TrackInformation = gtk_check_button_new_with_mnemonic(_("Track Information"));
    gtk_widget_show(FindToolbar_checkbutton_TrackInformation);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_checkbutton_TrackInformation, FALSE, FALSE, 0);
#if HAVE_GTK3 == 0
    tmp_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, tmp_toolbar_icon_size);
#else
    tmp_image = gtk_image_new_from_icon_name("window-close", tmp_toolbar_icon_size);
#endif

    gtk_widget_show(tmp_image);
    FindToolbar_CloseButton = (GtkWidget*) gtk_button_new();
    gtk_container_add(GTK_CONTAINER(FindToolbar_CloseButton), tmp_image);
    gtk_widget_show(FindToolbar_CloseButton);
    gtk_box_pack_start(GTK_BOX(FindToolbar_hbox_FindToolbar), FindToolbar_CloseButton, FALSE, FALSE, 5);
    gtk_button_set_relief(GTK_BUTTON(FindToolbar_CloseButton), GTK_RELIEF_NONE);

    // Main Window.

    // Hpane for showing both folders and main window.

#if HAVE_GTK3 == 0
    hpanel = gtk_hpaned_new();
#else
    hpanel = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#endif
    gtk_widget_show(hpanel);
    gtk_box_pack_start(GTK_BOX(vbox1), hpanel, TRUE, TRUE, 0);
    gtk_paned_set_position(GTK_PANED(hpanel), 150);


    // Folder Window

    scrolledwindowFolders = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_hide(scrolledwindowFolders);
    gtk_paned_pack1(GTK_PANED(hpanel), scrolledwindowFolders, TRUE, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindowFolders), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    treeviewFolders = gtk_tree_view_new();
    gtk_widget_show(treeviewFolders);
    gtk_container_add(GTK_CONTAINER(scrolledwindowFolders), treeviewFolders);
    gtk_container_set_border_width(GTK_CONTAINER(treeviewFolders), 5);
    folderSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviewFolders));
    gtk_tree_selection_set_mode(folderSelection, GTK_SELECTION_SINGLE);

    folderList = gtk_tree_store_new(NUM_FOL_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, GDK_TYPE_PIXBUF);
    folderColumn = setupFolderList(treeviewFolders);

    folderListModel = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(folderList));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(folderListModel),
            COL_FOL_NAME_HIDDEN, GTK_SORT_ASCENDING);

    gtk_tree_view_set_model(GTK_TREE_VIEW(treeviewFolders), GTK_TREE_MODEL(folderListModel));
    gtk_tree_view_expand_all(GTK_TREE_VIEW(treeviewFolders));
    g_object_unref(folderList);

    // Main file window

    scrolledwindowMain = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindowMain);
    gtk_paned_pack2(GTK_PANED(hpanel), scrolledwindowMain, TRUE, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindowMain), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    treeviewFiles = gtk_tree_view_new();
    gtk_widget_show(treeviewFiles);
    gtk_container_add(GTK_CONTAINER(scrolledwindowMain), treeviewFiles);
    gtk_container_set_border_width(GTK_CONTAINER(treeviewFiles), 5);
    fileSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviewFiles));
    gtk_tree_selection_set_mode(fileSelection, GTK_SELECTION_MULTIPLE);

    fileList = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
            G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, GDK_TYPE_PIXBUF,
            G_TYPE_STRING);
    setupFileList(treeviewFiles);

    fileListModel = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fileList));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fileListModel),
            COL_FILENAME_HIDDEN, GTK_SORT_ASCENDING);

    gtk_tree_view_set_model(GTK_TREE_VIEW(treeviewFiles), GTK_TREE_MODEL(fileListModel));
    g_object_unref(fileList);

    windowStatusBar = gtk_statusbar_new();
    gtk_widget_show(windowStatusBar);
    gtk_box_pack_start(GTK_BOX(vbox1), windowStatusBar, FALSE, FALSE, 0);

    // Build our right-click context menu;
    contextMenu = create_windowMainContextMenu();
    contextMenuColumn = create_windowMainColumnContextMenu();
    contextMenuFolder = create_windowFolderContextMenu();

    // DnD functions

    g_signal_connect((gpointer) scrolledwindowMain, "drag-data-received",
            G_CALLBACK(gmtp_drag_data_received), NULL);

    g_signal_connect((gpointer) treeviewFolders, "drag-data-received",
            G_CALLBACK(gmtpfolders_drag_data_received), NULL);

    g_signal_connect((gpointer) treeviewFolders, "drag-motion",
            G_CALLBACK(gmtpfolders_drag_motion_received), NULL);

    // End Dnd functions

    g_signal_connect((gpointer) windowMain, "destroy",
            G_CALLBACK(on_quit1_activate),
            NULL);

    g_signal_connect((gpointer) properties1, "activate",
            G_CALLBACK(on_deviceProperties_activate),
            NULL);
    g_signal_connect((gpointer) toolbuttonProperties, "clicked",
            G_CALLBACK(on_deviceProperties_activate),
            NULL);

    g_signal_connect((gpointer) quit1, "activate",
            G_CALLBACK(on_quit1_activate),
            NULL);

    g_signal_connect((gpointer) preferences1, "activate",
            G_CALLBACK(on_preferences1_activate),
            NULL);

    g_signal_connect((gpointer) editFind, "activate",
            G_CALLBACK(on_editFind_activate),
            NULL);

    g_signal_connect((gpointer) editSelectAll, "activate",
            G_CALLBACK(on_editSelectAll_activate),
            NULL);

    g_signal_connect((gpointer) editDeviceName, "activate",
            G_CALLBACK(on_editDeviceName_activate),
            NULL);

    g_signal_connect((gpointer) editFormatDevice, "activate",
            G_CALLBACK(on_editFormatDevice_activate),
            NULL);

    g_signal_connect((gpointer) editAddAlbumArt, "activate",
            G_CALLBACK(on_editAddAlbumArt_activate),
            NULL);

    g_signal_connect((gpointer) editPlaylist, "activate",
            G_CALLBACK(on_editPlaylist_activate),
            NULL);

    g_signal_connect((gpointer) fileAdd, "activate",
            G_CALLBACK(on_filesAdd_activate),
            NULL);

    g_signal_connect((gpointer) fileDownload, "activate",
            G_CALLBACK(on_filesDownload_activate),
            NULL);

    g_signal_connect((gpointer) fileRemove, "activate",
            G_CALLBACK(on_filesDelete_activate),
            NULL);

    g_signal_connect((gpointer) fileRename, "activate",
            G_CALLBACK(on_fileRenameFile_activate),
            NULL);

    g_signal_connect((gpointer) fileMove, "activate",
            G_CALLBACK(on_fileMoveFile_activate),
            NULL);

    g_signal_connect((gpointer) fileConnect, "activate",
            G_CALLBACK(on_deviceConnect_activate),
            NULL);

    g_signal_connect((gpointer) fileNewFolder, "activate",
            G_CALLBACK(on_fileNewFolder_activate),
            NULL);

    g_signal_connect((gpointer) fileRemoveFolder, "activate",
            G_CALLBACK(on_fileRemoveFolder_activate),
            NULL);

    g_signal_connect((gpointer) fileRescan, "activate",
            G_CALLBACK(on_deviceRescan_activate),
            NULL);

    g_signal_connect((gpointer) about1, "activate",
            G_CALLBACK(on_about1_activate),
            NULL);

/*    g_signal_connect((gpointer) toolbuttonQuit, "clicked",
            G_CALLBACK(on_quit1_activate),
            NULL);*/

    g_signal_connect((gpointer) toolbuttonRescan, "clicked",
            G_CALLBACK(on_deviceRescan_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonAddFile, "clicked",
            G_CALLBACK(on_filesAdd_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonRemoveFile, "clicked",
            G_CALLBACK(on_filesDelete_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonRetrieve, "clicked",
            G_CALLBACK(on_filesDownload_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonAlbumArt, "clicked",
            G_CALLBACK(on_editAddAlbumArt_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonPlaylist, "clicked",
            G_CALLBACK(on_editPlaylist_activate),
            NULL);

    g_signal_connect((gpointer) toolbuttonConnect, "clicked",
            G_CALLBACK(on_deviceConnect_activate),
            NULL);
    g_signal_connect((gpointer) toolbuttonPreferences, "clicked",
            G_CALLBACK(on_preferences1_activate),
            NULL);

    g_signal_connect((gpointer) treeviewFiles, "row-activated",
            G_CALLBACK(fileListRowActivated),
            NULL);

    g_signal_connect((gpointer) treeviewFolders, "row-activated",
            G_CALLBACK(folderListRowActivated),
            NULL);

    g_signal_connect_swapped(treeviewFiles, "button_press_event",
            G_CALLBACK(on_windowMainContextMenu_activate), contextMenu);

    g_signal_connect_swapped(treeviewFolders, "button_press_event",
            G_CALLBACK(on_windowMainContextMenu_activate), contextMenuFolder);

    g_signal_connect((gpointer) menu_view_filesize, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_filetype, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_track_number, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_title, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_artist, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_album, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_year, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_genre, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_duration, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) menu_view_folders, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);
            
    g_signal_connect((gpointer) menu_view_toolbar, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) FindToolbar_CloseButton, "clicked",
            G_CALLBACK(on_editFindClose_activate),
            NULL);

    g_signal_connect((gpointer) FindToolbar_FindButton, "clicked",
            G_CALLBACK(on_editFindSearch_activate),
            NULL);

    g_signal_connect((gpointer) FindToolbar_entry_FindText, "activate",
            G_CALLBACK(on_editFindSearch_activate),
            NULL);

    folderSelectHandler = g_signal_connect_after((gpointer) folderSelection, "changed",
            G_CALLBACK(on_treeviewFolders_rowactivated),
            NULL);

    fileSelectHandler = g_signal_connect_after((gpointer) fileSelection, "changed",
            G_CALLBACK(on_treeviewFolders_rowactivated),
            NULL);

    gtk_window_add_accel_group(GTK_WINDOW(windowMain), accel_group);
    gtk_menu_set_accel_group(GTK_MENU(menuitem1_menu), accel_group);
    gtk_widget_add_accelerator(fileRemove, "activate", accel_group, GDK_Delete, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileRename, "activate", accel_group, GDK_F2, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileConnect, "activate", accel_group, GDK_F3, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileRescan, "activate", accel_group, GDK_F5, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileNewFolder, "activate", accel_group, GDK_N, GDK_CONTROL_MASK + GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileAdd, "activate", accel_group, GDK_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(fileDownload, "activate", accel_group, GDK_D, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(editPlaylist, "activate", accel_group, GDK_P, GDK_CONTROL_MASK + GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(editAddAlbumArt, "activate", accel_group, GDK_A, GDK_CONTROL_MASK + GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(editSelectAll, "activate", accel_group, GDK_A, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(editFind, "activate", accel_group, GDK_F, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    return windowMain;
}

// ************************************************************************************************

/**
 * Set the text on the status bar within the main window
 * @param text
 */
void statusBarSet(gchar *text) {
    statusBarClear();
    guint c_id1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(windowStatusBar), "");
    gtk_statusbar_push(GTK_STATUSBAR(windowStatusBar), c_id1, text);
}

// ************************************************************************************************

/**
 * Clear the text within the status bar window.
 */
void statusBarClear() {
    guint c_id1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(windowStatusBar), "");
    gtk_statusbar_pop(GTK_STATUSBAR(windowStatusBar), c_id1);
}

// ************************************************************************************************

/**
 * Toggle the active state of the buttons on the toolbar and various menus.
 * @param state
 */
void SetToolbarButtonState(gboolean state) {
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonAddFile), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonRemoveFile), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonRetrieve), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonRescan), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonAlbumArt), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonPlaylist), state);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonProperties), state);
    gtk_widget_set_sensitive(GTK_WIDGET(properties1), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileAdd), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileDownload), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileRemove), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileRename), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileMove), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileNewFolder), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileRemoveFolder), state);
    gtk_widget_set_sensitive(GTK_WIDGET(fileRescan), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editDeviceName), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editFormatDevice), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editAddAlbumArt), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editFind), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editSelectAll), state);
    gtk_widget_set_sensitive(GTK_WIDGET(editPlaylist), state);
    gtk_widget_set_sensitive(GTK_WIDGET(treeviewFiles), state);
    // Only set this if we are using the normal connection method.
    if (!Preferences.use_alt_access_method) {
        gtk_widget_set_sensitive(GTK_WIDGET(treeviewFolders), state);
    }
}

// ************************************************************************************************

/**
 * Construct the main file view within the main application window.
 * @param treeviewFiles
 */
void setupFileList(GtkTreeView *treeviewFiles) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *header;
    GtkWidget *parent;

    // Filename column
    //renderer = gtk_cell_renderer_text_new();
    //column = gtk_tree_view_column_new_with_attributes(_("Filename"), renderer,
    //    "text", COL_FILENAME,
    //    NULL);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Filename"));
    renderer = gtk_cell_renderer_pixbuf_new();

    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", COL_ICON, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", COL_FILENAME_ACTUAL, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_sort_column_id(column, COL_FILENAME_HIDDEN);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    header = gtk_label_new(_("Filename"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Filename column for sorting.
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Filename Hidden", renderer,
            "text", COL_FILENAME_HIDDEN,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // File name actual - used for renaming operations.
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Filename Actual", renderer,
            "text", COL_FILENAME_ACTUAL,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // File Size column
    renderer = gtk_cell_renderer_text_new();
    column_Size = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
            "text", COL_FILESIZE,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Size);
    gtk_tree_view_column_set_sort_column_id(column_Size, COL_FILESIZE_HID);
    gtk_tree_view_column_set_resizable(column_Size, TRUE);
    gtk_tree_view_column_set_spacing(column_Size, 5);
    gtk_tree_view_column_set_visible(column_Size, Preferences.view_size);

    header = gtk_label_new(_("Size"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Size, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Size), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);


    // Folder/FileID column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Object ID", renderer,
            "text", COL_FILEID,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // isFolder column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("isFolder", renderer,
            "text", COL_ISFOLDER,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // File size column - hidden used for sorting the visible file size column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("FileSize Hidden", renderer,
            "text", COL_FILESIZE_HID,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // File Type column
    renderer = gtk_cell_renderer_text_new();
    column_Type = gtk_tree_view_column_new_with_attributes(_("File Type"), renderer,
            "text", COL_TYPE,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Type);
    gtk_tree_view_column_set_sort_column_id(column_Type, COL_TYPE);
    gtk_tree_view_column_set_resizable(column_Type, TRUE);
    gtk_tree_view_column_set_spacing(column_Type, 5);
    gtk_tree_view_column_set_visible(column_Type, Preferences.view_type);

    header = gtk_label_new(_("File Type"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Type, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Type), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Track Number column
    renderer = gtk_cell_renderer_text_new();
    column_Track_Number = gtk_tree_view_column_new_with_attributes(_("Track"), renderer,
            "text", COL_TRACK_NUMBER,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Track_Number);
    gtk_tree_view_column_set_sort_column_id(column_Track_Number, COL_TRACK_NUMBER_HIDDEN);
    gtk_tree_view_column_set_resizable(column_Track_Number, TRUE);
    gtk_tree_view_column_set_spacing(column_Track_Number, 5);
    gtk_tree_view_column_set_visible(column_Track_Number, Preferences.view_track_number);

    header = gtk_label_new(_("Track"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Track_Number, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Track_Number), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Track Num Hidden", renderer,
            "text", COL_TRACK_NUMBER_HIDDEN,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // Track Title column
    renderer = gtk_cell_renderer_text_new();
    column_Title = gtk_tree_view_column_new_with_attributes(_("Track Name"), renderer,
            "text", COL_TITLE,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Title);
    gtk_tree_view_column_set_sort_column_id(column_Title, COL_TITLE);
    gtk_tree_view_column_set_resizable(column_Title, TRUE);
    gtk_tree_view_column_set_spacing(column_Title, 5);
    gtk_tree_view_column_set_visible(column_Title, Preferences.view_track_number);

    header = gtk_label_new(_("Track Name"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Title, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Title), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Artist column
    renderer = gtk_cell_renderer_text_new();
    column_Artist = gtk_tree_view_column_new_with_attributes(_("Artist"), renderer,
            "text", COL_FL_ARTIST,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Artist);
    gtk_tree_view_column_set_sort_column_id(column_Artist, COL_FL_ARTIST);
    gtk_tree_view_column_set_resizable(column_Artist, TRUE);
    gtk_tree_view_column_set_spacing(column_Artist, 5);
    gtk_tree_view_column_set_visible(column_Artist, Preferences.view_artist);

    header = gtk_label_new(_("Artist"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Artist, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Artist), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Album column
    renderer = gtk_cell_renderer_text_new();
    column_Album = gtk_tree_view_column_new_with_attributes(_("Album"), renderer,
            "text", COL_FL_ALBUM,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Album);
    gtk_tree_view_column_set_sort_column_id(column_Album, COL_FL_ALBUM);
    gtk_tree_view_column_set_resizable(column_Album, TRUE);
    gtk_tree_view_column_set_spacing(column_Album, 5);
    gtk_tree_view_column_set_visible(column_Album, Preferences.view_album);

    header = gtk_label_new(_("Album"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Album, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Album), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Year column
    renderer = gtk_cell_renderer_text_new();
    column_Year = gtk_tree_view_column_new_with_attributes(_("Year"), renderer,
            "text", COL_YEAR,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Year);
    gtk_tree_view_column_set_sort_column_id(column_Year, COL_YEAR);
    gtk_tree_view_column_set_resizable(column_Year, TRUE);
    gtk_tree_view_column_set_spacing(column_Year, 5);
    gtk_tree_view_column_set_visible(column_Year, Preferences.view_year);

    header = gtk_label_new(_("Year"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Year, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Year), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Genre column
    renderer = gtk_cell_renderer_text_new();
    column_Genre = gtk_tree_view_column_new_with_attributes(_("Genre"), renderer,
            "text", COL_GENRE,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Genre);
    gtk_tree_view_column_set_sort_column_id(column_Genre, COL_GENRE);
    gtk_tree_view_column_set_resizable(column_Genre, TRUE);
    gtk_tree_view_column_set_spacing(column_Genre, 5);
    gtk_tree_view_column_set_visible(column_Genre, Preferences.view_genre);

    header = gtk_label_new(_("Genre"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Genre, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Genre), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Duration Visible column
    renderer = gtk_cell_renderer_text_new();
    column_Duration = gtk_tree_view_column_new_with_attributes(_("Duration"), renderer,
            "text", COL_DURATION,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Duration);
    gtk_tree_view_column_set_sort_column_id(column_Duration, COL_DURATION_HIDDEN);
    gtk_tree_view_column_set_resizable(column_Duration, TRUE);
    gtk_tree_view_column_set_spacing(column_Duration, 5);
    gtk_tree_view_column_set_visible(column_Duration, Preferences.view_duration);

    header = gtk_label_new(_("Duration"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Duration, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Duration), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);

    // Duration Hidden column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Duration Hidden", renderer,
            "text", COL_DURATION_HIDDEN,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // Location column - only used in search mode.
    renderer = gtk_cell_renderer_text_new();
    column_Location = gtk_tree_view_column_new_with_attributes(_("Location"), renderer,
            "text", COL_LOCATION,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column_Location);
    gtk_tree_view_column_set_sort_column_id(column_Location, COL_LOCATION);
    gtk_tree_view_column_set_resizable(column_Location, TRUE);
    gtk_tree_view_column_set_spacing(column_Location, 5);
    gtk_tree_view_column_set_visible(column_Location, FALSE);

    header = gtk_label_new(_("Location"));
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(column_Location, header);
    parent = gtk_widget_get_ancestor(gtk_tree_view_column_get_widget(column_Location), GTK_TYPE_BUTTON);

    g_signal_connect_swapped(parent, "button_press_event",
            G_CALLBACK(on_windowViewContextMenu_activate), contextMenuColumn);
}

// ************************************************************************************************

GtkTreeViewColumn *setupFolderList(GtkTreeView *treeviewFolders) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeViewColumn *folderColumnInt;

    folderColumnInt = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(folderColumnInt, _("Folder"));

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(folderColumnInt, renderer, FALSE);
    gtk_tree_view_column_set_attributes(folderColumnInt, renderer, "pixbuf", COL_FOL_ICON, NULL);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_tree_view_column_pack_start(folderColumnInt, renderer, TRUE);
    gtk_tree_view_column_set_attributes(folderColumnInt, renderer, "text", COL_FOL_NAME_HIDDEN, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFolders), folderColumnInt);
    gtk_tree_view_column_set_sort_column_id(folderColumnInt, COL_FOL_NAME_HIDDEN);
    gtk_tree_view_column_set_resizable(folderColumnInt, TRUE);
    gtk_tree_view_column_set_spacing(folderColumnInt, 5);

    // Folder column for sorting.
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Folder name Hidden", renderer,
            "text", COL_FOL_NAME_HIDDEN,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFolders), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    return folderColumnInt;
}


// ************************************************************************************************

/**
 * Clear all entries within the main file window.
 * @return
 */
gboolean fileListClear() {
    gtk_list_store_clear(GTK_LIST_STORE(fileList));
    return TRUE;
}


// ************************************************************************************************

/**
 * Clear all entries within the main folder window.
 * @return
 */
gboolean folderListClear() {
    gtk_tree_store_clear(GTK_TREE_STORE(folderList));
    return TRUE;
}

// ************************************************************************************************

/**
 * Display the Add Files dialog box and add the files as selected.
 * @return List of files to add to the device.
 */
GSList* getFileGetList2Add() {
    GSList* files = NULL;
    GtkWidget *FileDialog;
    gchar *savepath = NULL;

    savepath = g_malloc0(8192);
    FileDialog = gtk_file_chooser_dialog_new(_("Select Files to Add"),
            GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_OPEN,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Open"), GTK_RESPONSE_ACCEPT,
            NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(FileDialog), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileDialog), Preferences.fileSystemUploadPath->str);
    if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
        savepath = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(FileDialog));
        // Save our upload path.
        Preferences.fileSystemUploadPath = g_string_assign(Preferences.fileSystemUploadPath, savepath);
        files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(FileDialog));
    }

    gtk_widget_hide(FileDialog);
    gtk_widget_destroy(FileDialog);
    g_free(savepath);
    return files;
}

// ************************************************************************************************

/**
 * Add the applicable files to the file view in the main window.
 * @return
 */
gboolean fileListAdd() {
    GtkTreeIter rowIter;
    //gchar *filename = NULL;
    gchar *filename_hid = NULL;
    gchar *filesize = NULL;
    gchar *filetype = NULL;
    gchar *trackduration = NULL;
    gchar *tracknumber = NULL;
    gchar *fileext = NULL;
    LIBMTP_folder_t *tmpfolder;
    LIBMTP_file_t *tmpfile;
    guint parentID;
    GdkPixbuf *image = NULL;

    // Confirm our currentFolder exists otherwise goto the root folder.
    if (!Preferences.use_alt_access_method) {
        tmpfolder = getCurrentFolderPtr(deviceFolders, currentFolderID);
        if (tmpfolder == NULL)
            currentFolderID = 0;
    } else {
        // deviceFolders is NULL or has garbage. So assume that we actually do exist!
        filesUpateFileList();
    }
    // This ensure that if the current folder or a parent of is deleted in the find mode,
    // the current folder is reset to something sane.

    // Start our file listing.
    if (inFindMode == FALSE) {
        // Since we are in normal mode, hide the location Column.
        gtk_tree_view_column_set_visible(column_Location, FALSE);
        // We start with the folder list...
        if (currentFolderID != 0) {
            // If we are not folderID = 0; then...
            image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
            // Scan the folder list for the current folderID, and set the parent ID,
            if (!Preferences.use_alt_access_method) {
                tmpfolder = deviceFolders;
                parentID = getParentFolderID(tmpfolder, currentFolderID);
            } else {
                parentID = *((guint*) g_queue_peek_tail(stackFolderIDs));
            }
            // Now add in the row information.
            gtk_list_store_append(GTK_LIST_STORE(fileList), &rowIter);
            gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                    //COL_FILENAME, "< .. >",
                    COL_FILENAME_HIDDEN, "     < .. >",
                    COL_FILENAME_ACTUAL, "..",
                    COL_FILESIZE, "",
                    COL_FILEID, parentID,
                    COL_ISFOLDER, TRUE,
                    COL_FILESIZE_HID, (guint64) 0,
                    COL_ICON, image,
                    -1);

            // Indicate we are done with this image.
            g_object_unref(image);
        }

        // Only use device folders if using normal display mode.
        if (!Preferences.use_alt_access_method) {
            // What we scan for is the folder's details where 'parent_id' == currentFolderID and display those.
            tmpfolder = getParentFolderPtr(deviceFolders, currentFolderID);
            while (tmpfolder != NULL) {
                if ((tmpfolder->parent_id == currentFolderID) && (tmpfolder->storage_id == DeviceMgr.devicestorage->id)) {
                    image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
                    gtk_list_store_append(GTK_LIST_STORE(fileList), &rowIter);
                    //filename = g_strdup_printf("< %s >", tmpfolder->name);
                    filename_hid = g_strdup_printf("     < %s >", tmpfolder->name);
                    gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                            //COL_FILENAME, filename,
                            COL_FILENAME_HIDDEN, filename_hid,
                            COL_FILENAME_ACTUAL, tmpfolder->name,
                            COL_FILESIZE, "",
                            COL_FILEID, tmpfolder->folder_id,
                            COL_ISFOLDER, TRUE,
                            COL_FILESIZE_HID, (guint64) 0,
                            COL_ICON, image,
                            -1);
                    //g_free(filename);
                    g_free(filename_hid);
                    // Indicate we are done with this image.
                    g_object_unref(image);
                }
                tmpfolder = tmpfolder->sibling;
            }
        }
        // We don't destroy the structure, only on a rescan operation.

        // We scan for files in the file details we 'parent_id' == currentFolderID and display those.
        tmpfile = deviceFiles;
        while (tmpfile != NULL) {

            if ((tmpfile->parent_id == currentFolderID) && (tmpfile->storage_id == DeviceMgr.devicestorage->id)) {
                gtk_list_store_append(GTK_LIST_STORE(fileList), &rowIter);
                filesize = calculateFriendlySize(tmpfile->filesize);
                fileext = rindex(tmpfile->filename, '.');
                // This accounts for the case with a filename without any "." (period).
                if (!fileext) {
                    filetype = g_strconcat(g_ascii_strup(tmpfile->filename, -1), " File", NULL);
                } else {
                    filetype = g_strconcat(g_ascii_strup(++fileext, -1), " File", NULL);
                }
                
                // Get the type, and if it's one of the supported types, get the metadata for it.
                gboolean useTrack = (tmpfile->filetype == LIBMTP_FILETYPE_MP3) ||
                        (tmpfile->filetype == LIBMTP_FILETYPE_OGG) ||
                        (tmpfile->filetype == LIBMTP_FILETYPE_FLAC) ||
                        (tmpfile->filetype == LIBMTP_FILETYPE_WMA);
                // Ignore track metadata if we are treating all media as regular files.
                if(Preferences.allmediaasfiles){
                    useTrack = FALSE;
                }
                
                // Now if it's a track type file, eg OGG, WMA, MP3 or FLAC, get it's metadata.
                if (useTrack) {
                    LIBMTP_track_t *trackinfo;
                    trackinfo = LIBMTP_Get_Trackmetadata(DeviceMgr.device, tmpfile->item_id);
                    if (trackinfo != NULL) {
                        trackduration = g_strdup_printf("%d:%.2d", (int) ((trackinfo->duration / 1000) / 60),
                                (int) ((trackinfo->duration / 1000) % 60));
                        if (trackinfo->tracknumber != 0) {
                            tracknumber = g_strdup_printf("%d", trackinfo->tracknumber);
                        } else {
                            tracknumber = g_strdup(" ");
                        }
                        // Some basic sanitisation.
                        if (trackinfo->title == NULL) trackinfo->title = g_strdup("");
                        if (trackinfo->artist == NULL) trackinfo->artist = g_strdup("");
                        if (trackinfo->album == NULL) trackinfo->album = g_strdup("");
                        if (trackinfo->date == NULL) {
                            trackinfo->date = g_strdup("");
                        } else {
                            if (strlen(trackinfo->date) > 4)
                                trackinfo->date[4] = '\0'; // Shorten the string to year only, yes this is nasty...
                        }
                        if (trackinfo->genre == NULL) trackinfo->genre = g_strdup("");

                        // Icon
                        image = gdk_pixbuf_new_from_file(file_audio_png, NULL);

                        gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                                //COL_FILENAME, tmpfile->filename,
                                COL_FILENAME_HIDDEN, tmpfile->filename,
                                COL_FILENAME_ACTUAL, tmpfile->filename,
                                COL_FILESIZE, filesize,
                                COL_FILEID, tmpfile->item_id,
                                COL_ISFOLDER, FALSE,
                                COL_FILESIZE_HID, tmpfile->filesize,
                                COL_TYPE, filetype,
                                COL_TRACK_NUMBER, tracknumber,
                                COL_TRACK_NUMBER_HIDDEN, trackinfo->tracknumber,
                                COL_TITLE, trackinfo->title,
                                COL_FL_ARTIST, trackinfo->artist,
                                COL_FL_ALBUM, trackinfo->album,
                                COL_YEAR, trackinfo->date,
                                COL_GENRE, trackinfo->genre,
                                COL_DURATION, trackduration,
                                COL_DURATION_HIDDEN, trackinfo->duration,
                                COL_ICON, image,
                                -1);
                        g_free(trackduration);
                        g_free(tracknumber);
                        trackduration = NULL;
                        tracknumber = NULL;
                        // Indicate we are done with this image.
                        g_object_unref(image);
                        LIBMTP_destroy_track_t(trackinfo);
                    } else {
                        LIBMTP_Dump_Errorstack(DeviceMgr.device);
                        LIBMTP_Clear_Errorstack(DeviceMgr.device);
                    }
                } else {
                    // Determine the file type.
                    if (LIBMTP_FILETYPE_IS_AUDIO(tmpfile->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_audio_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_AUDIOVIDEO(tmpfile->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_video_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_VIDEO(tmpfile->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_video_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_IMAGE(tmpfile->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_image_png, NULL);
                    } else if (tmpfile->filetype == LIBMTP_FILETYPE_ALBUM) {
                        image = gdk_pixbuf_new_from_file(file_album_png, NULL);
                    } else if (tmpfile->filetype == LIBMTP_FILETYPE_PLAYLIST) {
                        image = gdk_pixbuf_new_from_file(file_playlist_png, NULL);
                    } else if (tmpfile->filetype == LIBMTP_FILETYPE_TEXT) {
                        image = gdk_pixbuf_new_from_file(file_textfile_png, NULL);
                    } else if (tmpfile->filetype == LIBMTP_FILETYPE_FOLDER) {
                        image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
                    } else {
                        image = gdk_pixbuf_new_from_file(file_generic_png, NULL);
                    }

                    // let folders through IFF we are in alt connection mode.
                    if (!Preferences.use_alt_access_method && tmpfile->filetype == LIBMTP_FILETYPE_FOLDER) {
                        goto skipAdd;
                    }

                    // Set folder type.
                    int isFolder = FALSE;
                    if (tmpfile->filetype == LIBMTP_FILETYPE_FOLDER) {
                        isFolder = TRUE;
                        filesize = g_strdup("");
                        filetype = g_strdup("");
                        filename_hid = g_strdup_printf("     < %s >", tmpfile->filename);
                    } else {
                        filename_hid = g_strdup(tmpfile->filename);
                    }

                    // Otherwise just show the file information
                    gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                            //COL_FILENAME, tmpfile->filename,
                            COL_FILENAME_HIDDEN, filename_hid,
                            COL_FILENAME_ACTUAL, tmpfile->filename,
                            COL_FILESIZE, filesize,
                            COL_FILEID, tmpfile->item_id,
                            COL_ISFOLDER, isFolder,
                            COL_FILESIZE_HID, tmpfile->filesize,
                            COL_TYPE, filetype,
                            COL_ICON, image,
                            -1);
                    g_free(filename_hid);
skipAdd:

                    // Indicate we are done with this image.
                    g_object_unref(image);
                }

                if (filetype != NULL)
                    g_free(filetype);
                filetype = NULL;

                if (filesize != NULL)
                    g_free(filesize);
                filesize = NULL;
            }
            tmpfile = tmpfile->next;
        }

        // Now update the title bar with our folder name.
        if (!Preferences.use_alt_access_method) {
            setWindowTitle(getFullFolderPath(currentFolderID));
        } else {
            // Construct the place string based on the contents of the stackFolderNameQueue.
            gchar* fullfilename = g_strdup("");
            gchar* tmpfilename = NULL;
            guint stringlength = 0;
            int items = g_queue_get_length(stackFolderNames);
            
            // Add in our names;
            while(items-- > 0){
                tmpfilename = g_strdup_printf("%s/%s", (gchar *)g_queue_peek_nth(stackFolderNames, items), fullfilename);
                g_free(fullfilename);
                fullfilename = tmpfilename;
            }
            // Add in leading slash if needed
            if (*fullfilename != '/') {
                tmpfilename = g_strdup_printf("/%s", fullfilename);
                g_free(fullfilename);
                fullfilename = tmpfilename;
            }
            // Remove trailing slash if needed.
            stringlength = strlen(fullfilename);
            if (stringlength > 1) {
                fullfilename[stringlength - 1] = '\0';
            }
            setWindowTitle(fullfilename);
        }
    } else {
        // We are in search mode, so use the searchList instead as our source!
        gint item_count = 0;
        GSList *tmpsearchList = searchList;
        FileListStruc *itemdata = NULL;
        while (tmpsearchList != NULL) {
            // Add our files/folders.
            itemdata = tmpsearchList->data;
            item_count++;
            // If a folder...
            if (itemdata->isFolder == TRUE) {
                image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
                gtk_list_store_append(GTK_LIST_STORE(fileList), &rowIter);
                //filename = g_strdup_printf("< %s >", tmpfolder->name);
                filename_hid = g_strdup_printf("     < %s >", itemdata->filename);
                gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                        //COL_FILENAME, filename,
                        COL_FILENAME_HIDDEN, filename_hid,
                        COL_FILENAME_ACTUAL, itemdata->filename,
                        COL_FILESIZE, "",
                        COL_FILEID, itemdata->itemid,
                        COL_ISFOLDER, TRUE,
                        COL_FILESIZE_HID, (guint64) 0,
                        COL_ICON, image,
                        COL_LOCATION, itemdata->location,
                        -1);
                //g_free(filename);
                g_free(filename_hid);
                // Indicate we are done with this image.
                g_object_unref(image);

            } else {
                // else if a file...
                gtk_list_store_append(GTK_LIST_STORE(fileList), &rowIter);
                filesize = calculateFriendlySize(itemdata->filesize);
                fileext = rindex(itemdata->filename, '.');
                // This accounts for the case with a filename without any "." (period).
                if (!fileext) {
                    filetype = g_strconcat(g_ascii_strup(itemdata->filename, -1), " File", NULL);
                } else {
                    filetype = g_strconcat(g_ascii_strup(++fileext, -1), " File", NULL);
                }
                // Get the type, and if it's one of the supported types, get the metadata for it.
                gboolean useTrack = (itemdata->filetype == LIBMTP_FILETYPE_MP3) ||
                        (itemdata->filetype == LIBMTP_FILETYPE_OGG) ||
                        (itemdata->filetype == LIBMTP_FILETYPE_FLAC) ||
                        (itemdata->filetype == LIBMTP_FILETYPE_WMA);
                // Ignore track metadata if we are treating all media as regular files.
                if(Preferences.allmediaasfiles){
                    useTrack = FALSE;
                }
                
                // Now if it's a track type file, eg OGG, WMA, MP3 or FLAC, get it's metadata.
                if (useTrack) {
                    LIBMTP_track_t *trackinfo;
                    trackinfo = LIBMTP_Get_Trackmetadata(DeviceMgr.device, itemdata->itemid);
                    if (trackinfo != NULL) {
                        trackduration = g_strdup_printf("%d:%.2d", (int) ((trackinfo->duration / 1000) / 60),
                                (int) ((trackinfo->duration / 1000) % 60));
                        if (trackinfo->tracknumber != 0) {
                            tracknumber = g_strdup_printf("%d", trackinfo->tracknumber);
                        } else {
                            tracknumber = g_strdup(" ");
                        }
                        // Some basic sanitisation.
                        if (trackinfo->title == NULL) trackinfo->title = g_strdup("");
                        if (trackinfo->artist == NULL) trackinfo->artist = g_strdup("");
                        if (trackinfo->album == NULL) trackinfo->album = g_strdup("");
                        if (trackinfo->date == NULL) trackinfo->date = g_strdup("");
                        if (trackinfo->genre == NULL) trackinfo->genre = g_strdup("");

                        // Icon
                        image = gdk_pixbuf_new_from_file(file_audio_png, NULL);

                        gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                                //COL_FILENAME, tmpfile->filename,
                                COL_FILENAME_HIDDEN, itemdata->filename,
                                COL_FILENAME_ACTUAL, itemdata->filename,
                                COL_FILESIZE, filesize,
                                COL_FILEID, itemdata->itemid,
                                COL_ISFOLDER, FALSE,
                                COL_FILESIZE_HID, itemdata->filesize,
                                COL_TYPE, filetype,
                                COL_TRACK_NUMBER, tracknumber,
                                COL_TRACK_NUMBER_HIDDEN, trackinfo->tracknumber,
                                COL_TITLE, trackinfo->title,
                                COL_FL_ARTIST, trackinfo->artist,
                                COL_FL_ALBUM, trackinfo->album,
                                COL_YEAR, trackinfo->date,
                                COL_GENRE, trackinfo->genre,
                                COL_DURATION, trackduration,
                                COL_DURATION_HIDDEN, trackinfo->duration,
                                COL_ICON, image,
                                COL_LOCATION, itemdata->location,
                                -1);
                        g_free(trackduration);
                        g_free(tracknumber);
                        trackduration = NULL;
                        tracknumber = NULL;
                        // Indicate we are done with this image.
                        g_object_unref(image);
                        LIBMTP_destroy_track_t(trackinfo);
                    } else {
                        LIBMTP_Dump_Errorstack(DeviceMgr.device);
                        LIBMTP_Clear_Errorstack(DeviceMgr.device);
                    }
                } else {
                    // Determine the file type.
                    if (LIBMTP_FILETYPE_IS_AUDIO(itemdata->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_audio_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_AUDIOVIDEO(itemdata->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_video_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_VIDEO(itemdata->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_video_png, NULL);
                    } else if (LIBMTP_FILETYPE_IS_IMAGE(itemdata->filetype)) {
                        image = gdk_pixbuf_new_from_file(file_image_png, NULL);
                    } else if (itemdata->filetype == LIBMTP_FILETYPE_ALBUM) {
                        image = gdk_pixbuf_new_from_file(file_album_png, NULL);
                    } else if (itemdata->filetype == LIBMTP_FILETYPE_PLAYLIST) {
                        image = gdk_pixbuf_new_from_file(file_playlist_png, NULL);
                    } else if (itemdata->filetype == LIBMTP_FILETYPE_TEXT) {
                        image = gdk_pixbuf_new_from_file(file_textfile_png, NULL);
                    } else if (itemdata->filetype == LIBMTP_FILETYPE_FOLDER) {
                        image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
                    } else {
                        image = gdk_pixbuf_new_from_file(file_generic_png, NULL);
                    }

                    // let folders through IFF we are in alt connection mode.
                    if (!Preferences.use_alt_access_method && itemdata->filetype == LIBMTP_FILETYPE_FOLDER) {
                        goto skipAdd2;
                    }

                    // Set folder type.
                    //int isFolder = FALSE;
                    if (itemdata->filetype == LIBMTP_FILETYPE_FOLDER) {
                        //isFolder = TRUE;
                        filesize = g_strdup("");
                        filetype = g_strdup("");
                        filename_hid = g_strdup_printf("     < %s >", itemdata->filename);
                    } else {
                        filename_hid = g_strdup(itemdata->filename);
                    }

                    // Otherwise just show the file information
                    gtk_list_store_set(GTK_LIST_STORE(fileList), &rowIter,
                            //COL_FILENAME, tmpfile->filename,
                            COL_FILENAME_HIDDEN, itemdata->filename,
                            COL_FILENAME_ACTUAL, itemdata->filename,
                            COL_FILESIZE, filesize,
                            COL_FILEID, itemdata->itemid,
                            COL_ISFOLDER, FALSE,
                            COL_FILESIZE_HID, itemdata->filesize,
                            COL_TYPE, filetype,
                            COL_ICON, image,
                            COL_LOCATION, itemdata->location,
                            -1);
                    // Indicate we are done with this image.
                    g_free(filename_hid);
skipAdd2:
                    g_object_unref(image);
                }

                if (filetype != NULL)
                    g_free(filetype);
                filetype = NULL;

                if (filesize != NULL)
                    g_free(filesize);
                filesize = NULL;
            }
            tmpsearchList = tmpsearchList->next;
        }
        gchar *tmp_string;
        if (item_count != 1) {
            tmp_string = g_strdup_printf(_("Found %d items"), item_count);
        } else {
            tmp_string = g_strdup_printf(_("Found %d item"), item_count);
        }
        statusBarSet(tmp_string);
        g_free(tmp_string);
    }
    return TRUE;
}


// ************************************************************************************************

/**
 * Add folders to the folder list in main window.
 */
gboolean folderListAdd(LIBMTP_folder_t *folders, GtkTreeIter *parent) {
    GtkTreeIter rowIter;
    GdkPixbuf *image = NULL;

    if (parent == NULL) {
        // Add in the root node.
        image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
        // Now add in the row information.
        gtk_tree_store_append(GTK_TREE_STORE(folderList), &rowIter, parent);
        gtk_tree_store_set(GTK_TREE_STORE(folderList), &rowIter,
                //COL_FOL_NAME, folders->name,
                COL_FOL_NAME_HIDDEN, "/",
                COL_FOL_ID, 0,
                COL_FOL_ICON, image,
                -1);

        // Indicate we are done with this image.
        g_object_unref(image);
        folderListAdd(folders, &rowIter);
        return TRUE;
    }

    while (folders != NULL) {
        // Only add in folder if it's in the current storage device.
        if (folders->storage_id == DeviceMgr.devicestorage->id) {

            image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
            // Now add in the row information.
            gtk_tree_store_append(GTK_TREE_STORE(folderList), &rowIter, parent);
            gtk_tree_store_set(GTK_TREE_STORE(folderList), &rowIter,
                    //COL_FOL_NAME, folders->name,
                    COL_FOL_NAME_HIDDEN, folders->name,
                    COL_FOL_ID, folders->folder_id,
                    COL_FOL_ICON, image,
                    -1);

            // Indicate we are done with this image.
            g_object_unref(image);
            if (folders->child != NULL) {
                // Call our child.
                folderListAdd(folders->child, &rowIter);
            }
        }
        folders = folders->sibling;
    }
    gtk_tree_view_expand_all(GTK_TREE_VIEW(treeviewFolders));
    gtk_tree_view_column_set_sort_order(folderColumn, GTK_SORT_ASCENDING);
    return TRUE;
}


// ************************************************************************************************

/**
 * Download the selected files.
 * @param List The files to download.
 * @return
 */
gboolean fileListDownload(GList *List) {
    GtkWidget *FileDialog;
    gchar *savepath = NULL;
    //savepath = g_malloc0(8192);

    // Let's confirm our download path.
    if (Preferences.ask_download_path == TRUE) {
        FileDialog = gtk_file_chooser_dialog_new(_("Select Path to Download"),
                GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                _("_Cancel"), GTK_RESPONSE_CANCEL,
                _("_Open"), GTK_RESPONSE_ACCEPT,
                NULL);

        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(FileDialog), Preferences.fileSystemDownloadPath->str);
        if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
            gtk_widget_hide(FileDialog);
            savepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
            // Save our download path.
            Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, savepath);
            // We do the deed.
            g_list_foreach(List, (GFunc) __fileDownload, NULL);
            fileoverwriteop = MTP_ASK;
        }
        gtk_widget_destroy(FileDialog);
    } else {
        // We do the deed.
        g_list_foreach(List, (GFunc) __fileDownload, NULL);
        fileoverwriteop = MTP_ASK;
    }
    if (savepath != NULL)
        g_free(savepath);
    return TRUE;
}


// ************************************************************************************************

/**
 * Download the selected folder.
 * @param List The files to download.
 * @return
 */
gboolean folderListDownload(gchar *foldername, uint32_t folderid) {
    GtkWidget *FileDialog;
    gchar *savepath = NULL;
    //savepath = g_malloc0(8192);

    // Let's confirm our download path.
    if (Preferences.ask_download_path == TRUE) {
        FileDialog = gtk_file_chooser_dialog_new(_("Select Path to Download"),
                GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                _("_Cancel"), GTK_RESPONSE_CANCEL,
                _("_Open"), GTK_RESPONSE_ACCEPT,
                NULL);

        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(FileDialog), Preferences.fileSystemDownloadPath->str);
        if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
            gtk_widget_hide(FileDialog);
            savepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
            // Save our download path.
            Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, savepath);
            // We do the deed.
            displayProgressBar(_("File download"));
            folderDownload(foldername, folderid, TRUE);
            destroyProgressBar();
            fileoverwriteop = MTP_ASK;
        }
        gtk_widget_destroy(FileDialog);
    } else {
        // We do the deed.
        displayProgressBar(_("File download"));
        folderDownload(foldername, folderid, TRUE);
        destroyProgressBar();
        fileoverwriteop = MTP_ASK;
    }
    if (savepath != NULL)
        g_free(savepath);
    return TRUE;
}


// ************************************************************************************************

/**
 * Perform each file individually.
 * @param Row
 */
void __fileDownload(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *filename = NULL;
    gchar* fullfilename = NULL;

    gboolean isFolder;
    uint32_t objectID;


    fullfilename = g_malloc0(8192);
    // First of all, lets set the download path.

    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    // Before we download, is it a folder ?
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILENAME_ACTUAL, &filename, COL_FILEID, &objectID, -1);
    if (isFolder == FALSE) {
        // Our strings are not equal, so we get to download the file.
        g_sprintf(fullfilename, "%s/%s", Preferences.fileSystemDownloadPath->str, filename);
        // Now download the actual file from the MTP device.
        // Check if file exists?
        if (access(fullfilename, F_OK) != -1) {
            // We have that file already?
            if (Preferences.prompt_overwrite_file_op == TRUE) {
                if (fileoverwriteop == MTP_ASK) {
                    fileoverwriteop = displayFileOverwriteDialog(filename);
                }
                switch (fileoverwriteop) {
                    case MTP_ASK:
                        break;
                    case MTP_SKIP:
                        fileoverwriteop = MTP_ASK;
                        break;
                    case MTP_SKIP_ALL:
                        break;
                    case MTP_OVERWRITE:
                        filesDownload(filename, objectID);
                        fileoverwriteop = MTP_ASK;
                        break;
                    case MTP_OVERWRITE_ALL:
                        filesDownload(filename, objectID);
                        break;
                }
            } else {
                filesDownload(filename, objectID);
            }
        } else {
            filesDownload(filename, objectID);
        }
    } else {
        // Overwrite critera performed within this call...
        if (g_ascii_strcasecmp(filename, "..") != 0) {
            folderDownload(filename, objectID, TRUE);
        } else {
            g_fprintf(stderr, _("I don't know how to download a parent folder reference?\n"));
            displayError(_("I don't know how to download a parent folder reference?\n"));
        }
    }
    g_free(filename);
    g_free(fullfilename);
}


// ************************************************************************************************

/**
 * Perform each file individually.
 * @param Row
 */
void __fileMove(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;

    gboolean isFolder;
    LIBMTP_folder_t *currentFolder = NULL;
    LIBMTP_folder_t *newFolder = NULL;
    uint32_t objectID;
    int error;

    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    // Before we move, is it a folder ?
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILEID, &objectID, -1);
    if (isFolder == FALSE) {
        if ((error = setNewParentFolderID(objectID, fileMoveTargetFolder)) != 0) {
            displayError(_("Unable to move the selected file?\n"));
            g_fprintf(stderr, "File Move Error: %d\n", error);
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
    } else {
        // Make sure we don't want to move the folder into itself?
        if (objectID == fileMoveTargetFolder) {
            displayError(_("Unable to move the selected folder into itself?\n"));
            g_fprintf(stderr, _("Unable to move the selected folder into itself?\n"));
            return;
        }
        // We have the target folder, so let's check to ensure that we will not create a circular
        // reference by moving a folder underneath it self.
        currentFolder = getCurrentFolderPtr(deviceFolders, objectID);
        if (currentFolder == NULL) {
            // WTF?
            g_fprintf(stderr, "File Move Error: Can't get current folder pointer\n");
            return;
        }
        // Use currentFolder as the starting point, and simply attempt to get the ptr to the new
        // folder based on this point.
        newFolder = getCurrentFolderPtr(currentFolder->child, fileMoveTargetFolder);
        if (newFolder == NULL) {
            // We are alright to proceed.
            if ((error = setNewParentFolderID(objectID, fileMoveTargetFolder)) != 0) {
                displayError(_("Unable to move the selected folder?\n"));
                g_fprintf(stderr, "File Move Error: %d\n", error);
                LIBMTP_Dump_Errorstack(DeviceMgr.device);
                LIBMTP_Clear_Errorstack(DeviceMgr.device);
            }
        } else {
            displayError(_("Unable to move the selected folder underneath itself?\n"));
            g_fprintf(stderr, _("Unable to move the selected folder underneath itself?\n"));
        }
    }
}


// ************************************************************************************************

/**
 * Remove selected files from the device.
 * @param List
 * @return
 */
gboolean fileListRemove(GList *List) {
    // Clear any selection that is present.
    fileListClearSelection();
    // List is a list of Iter's to be removed
    g_list_foreach(List, (GFunc) __fileRemove, NULL);
    // We have 2 options, manually scan the file structure for that file and manually fix up...
    // or do a rescan...
    // I'll be cheap, and do a full rescan of the device.
    deviceRescan();
    return TRUE;
}

// ************************************************************************************************

/**
 * Remove each selected file from the device.
 * @param Row
 */
void __fileRemove(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar* filename = NULL;
    uint32_t objectID;
    gboolean isFolder;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILENAME_ACTUAL, &filename,
            COL_FILEID, &objectID, -1);
    if (isFolder == FALSE) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
        gtk_list_store_remove(GTK_LIST_STORE(fileList), &iter);
        // Now get rid of the actual file from the MTP device.
        filesDelete(filename, objectID);
    } else {
        // Our file is really a folder, so perform a folder remove operation.
        __folderRemove(Row);
    }
    g_free(filename);
}

// ************************************************************************************************

/**
 * Remove the selected folders from the device.
 * @param List
 * @return
 */
gboolean folderListRemove(GList *List) {
    // Clear any selection that is present.
    fileListClearSelection();
    // List is a list of Iter's to be removed
    g_list_foreach(List, (GFunc) __folderRemove, NULL);
    // We have 2 options, manually scan the file structure for that file and manually fix up...
    // or do a rescan...
    // I'll be cheap, and do a full rescan of the device.
    deviceRescan();
    return TRUE;
}

// ************************************************************************************************

/**
 * Remove the indivual folder from the device.
 * @param Row
 */
void __folderRemove(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar* filename = NULL;
    uint32_t objectID;
    gboolean isFolder;

    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILENAME_ACTUAL, &filename,
            COL_FILEID, &objectID, -1);
    if (isFolder == TRUE) {
        if (g_ascii_strcasecmp(filename, "..") != 0) {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
            gtk_list_store_remove(GTK_LIST_STORE(fileList), &iter);
            // Now get rid of the actual file from the MTP device.
            folderDelete(getCurrentFolderPtr(deviceFolders, objectID), 0);
        } else {
            g_fprintf(stderr, _("I don't know how to delete a parent folder reference?\n"));
            displayError(_("I don't know how to delete a parent folder reference?\n"));
        }
    } else {
        // Our folder is really a file, so delete the file instead.
        __fileRemove(Row);
    }
    g_free(filename);
}

// ************************************************************************************************

/**
 * Add an individual file to the device.
 * @param filename
 */
void __filesAdd(gchar* filename) {
    gchar* filename_stripped = NULL;

    filename_stripped = basename(filename);
    if (Preferences.prompt_overwrite_file_op == FALSE) {
        if(fileExists(filename_stripped) == TRUE){
            filesDelete(filename_stripped, getFile(filename_stripped, currentFolderID));
        }
        filesAdd(filename);
        return;
    }
    // I guess we want to know if we should replace the file, but first
    if (deviceoverwriteop == MTP_ASK) {
        if (fileExists(filename_stripped) == TRUE) {
            deviceoverwriteop = displayFileOverwriteDialog(filename_stripped);
            switch (deviceoverwriteop) {
                case MTP_ASK:
                    break;
                case MTP_SKIP:
                    deviceoverwriteop = MTP_ASK;
                    break;
                case MTP_SKIP_ALL:
                    break;
                case MTP_OVERWRITE:
                    if(fileExists(filename_stripped) == TRUE){
                        filesDelete(filename_stripped, getFile(filename_stripped, currentFolderID));
                    }
                    filesAdd(filename);
                    deviceoverwriteop = MTP_ASK;
                    break;
                case MTP_OVERWRITE_ALL:
                    if(fileExists(filename_stripped) == TRUE){
                        filesDelete(filename_stripped, getFile(filename_stripped, currentFolderID));
                    }
                    filesAdd(filename);
                    break;
            }
        } else {
            if(fileExists(filename_stripped) == TRUE){
                filesDelete(filename_stripped, getFile(filename_stripped, currentFolderID));
            }
            filesAdd(filename);
        }
    } else {
        if (deviceoverwriteop == MTP_OVERWRITE_ALL){
            if(fileExists(filename_stripped) == TRUE){
                filesDelete(filename_stripped, getFile(filename_stripped, currentFolderID));
            }
            filesAdd(filename);
        } else if (deviceoverwriteop == MTP_SKIP_ALL) {
            // Ensure we add files that don't exist.
            if (fileExists(filename_stripped) != TRUE) {
                filesAdd(filename);
            }
        }
    }
}

// ************************************************************************************************

/**
 * Get a GList of the TREE ROW REFERENCES that are selected.
 * @return
 */
GList* fileListGetSelection() {
    GList *selectedFiles, *ptr;
    GtkTreeRowReference *ref;
    GtkTreeModel *sortmodel;
    // Lets clear up the old list.
    g_list_free(fileSelection_RowReferences);
    fileSelection_RowReferences = NULL;

    if (gtk_tree_selection_count_selected_rows(fileSelection) == 0) {
        // We have no rows.
        return NULL;
    }
    // So now we must convert each selection to a row reference and store it in a new GList variable
    // which we will return below.
    sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFiles));
    selectedFiles = gtk_tree_selection_get_selected_rows(fileSelection, &sortmodel);
    ptr = selectedFiles;
    while (ptr != NULL) {
        // Add our row into the GSList so it can be parsed by the respective operations, including
        // handling changing from the sort model to the real underlying model.
        ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(fileList),
                gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(sortmodel), (GtkTreePath*) ptr->data));
        fileSelection_RowReferences = g_list_prepend(fileSelection_RowReferences, gtk_tree_row_reference_copy(ref));
        gtk_tree_row_reference_free(ref);
        ptr = ptr->next;
    }
    g_list_foreach(selectedFiles, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedFiles);
    return fileSelection_RowReferences;
}


// ************************************************************************************************

/**
 * Finds the Object ID of the selected folder in the folder view.
 * @return
 */
int64_t folderListGetSelection(void) {
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;
    uint32_t objectID;

    if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
        // We have no rows.
        return -1;
    }
    sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFolders));
    gtk_tree_selection_get_selected(folderSelection, &sortmodel, &iter);
    gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
    gtk_tree_model_get(GTK_TREE_MODEL(folderList), &childiter, COL_FOL_ID, &objectID, -1);
    return objectID;
}


// ************************************************************************************************

/**
 * Finds the Object Name of the selected folder in the folder view.
 * @return
 */
gchar *folderListGetSelectionName(void) {
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;
    gchar *objectName;

    if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
        // We have no rows.
        return NULL;
    }
    sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFolders));
    gtk_tree_selection_get_selected(folderSelection, &sortmodel, &iter);
    gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
    gtk_tree_model_get(GTK_TREE_MODEL(folderList), &childiter, COL_FOL_NAME_HIDDEN, &objectName, -1);
    return objectName;

}

// ************************************************************************************************

/**
 * Clear all selected rows from the main file list.
 * @return
 */
gboolean fileListClearSelection() {
    if (fileSelection != NULL)
        gtk_tree_selection_unselect_all(fileSelection);
    return TRUE;
}


// ************************************************************************************************

/**
 * Select all rows from the main file list.
 * @return
 */
gboolean fileListSelectAll(void) {
    if (fileSelection != NULL)
        gtk_tree_selection_select_all(fileSelection);
    return TRUE;
}


// ************************************************************************************************

/**
 * Display an Error Dialog Message Box.
 * @param msg
 */
void displayError(gchar* msg) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(windowMain),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            msg);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ************************************************************************************************

/**
 * Display an Information Dialog Message Box.
 * @param msg
 */
void displayInformation(gchar* msg) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(windowMain),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "%s",
            msg);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Information"));
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ************************************************************************************************

/**
 * Display the Add New Folder Dialog Box.
 * @return The name of the folder to be created.
 */
gchar* displayFolderNewDialog(void) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    gchar* textfield;

    dialog = gtk_dialog_new_with_buttons(_("New Folder"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    label = gtk_label_new(_("Folder Name:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    textbox = gtk_entry_new();
    gtk_widget_show(textbox);
    gtk_entry_set_max_length(GTK_ENTRY(textbox), 64);
    gtk_entry_set_has_frame(GTK_ENTRY(textbox), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(textbox), TRUE);
    gtk_container_add(GTK_CONTAINER(hbox), textbox);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        textfield = g_strdup(gtk_entry_get_text(GTK_ENTRY(textbox)));
        if (strlen(textfield) == 0) {
            // We have an emtpy string.
            gtk_widget_destroy(dialog);
            return NULL;
        } else {
            gtk_widget_destroy(dialog);
            return textfield;
        }
    } else {
        gtk_widget_destroy(dialog);
        return NULL;
    }
}

// ************************************************************************************************

/**
 * Display the Change Device Name dialog box.
 * @param devicename The new name of the device.
 * @return
 */
gchar* displayChangeDeviceNameDialog(gchar* devicename) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    gchar* textfield;

    dialog = gtk_dialog_new_with_buttons(_("Change Device Name"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    label = gtk_label_new(_("Device Name:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    textbox = gtk_entry_new();
    gtk_widget_show(textbox);
    if (devicename != NULL) {
        gtk_entry_set_text(GTK_ENTRY(textbox), devicename);
    }
    gtk_entry_set_max_length(GTK_ENTRY(textbox), 64);
    gtk_entry_set_has_frame(GTK_ENTRY(textbox), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(textbox), TRUE);
    gtk_container_add(GTK_CONTAINER(hbox), textbox);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        textfield = g_strdup(gtk_entry_get_text(GTK_ENTRY(textbox)));
        if (strlen(textfield) == 0) {
            // We have an emtpy string.
            gtk_widget_destroy(dialog);
            return NULL;
        } else {
            gtk_widget_destroy(dialog);
            return textfield;
        }
    } else {
        gtk_widget_destroy(dialog);
        return NULL;
    }
}

// ************************************************************************************************

/**
 * Display the rename Filename dialog box.
 * @param currentfilename The new name of the file.
 * @return
 */
gchar* displayRenameFileDialog(gchar* currentfilename) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    gchar* textfield;

    dialog = gtk_dialog_new_with_buttons(_("Rename File/Folder"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    label = gtk_label_new(_("File Name:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    textbox = gtk_entry_new();
    gtk_widget_show(textbox);
    if (currentfilename != NULL) {
        gtk_entry_set_text(GTK_ENTRY(textbox), currentfilename);
    }
    gtk_entry_set_max_length(GTK_ENTRY(textbox), 64);
    gtk_entry_set_has_frame(GTK_ENTRY(textbox), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(textbox), TRUE);
    gtk_container_add(GTK_CONTAINER(hbox), textbox);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        textfield = g_strdup(gtk_entry_get_text(GTK_ENTRY(textbox)));
        if (strlen(textfield) == 0) {
            // We have an emtpy string.
            gtk_widget_destroy(dialog);
            return NULL;
        } else {
            gtk_widget_destroy(dialog);
            return textfield;
        }
    } else {
        gtk_widget_destroy(dialog);
        return NULL;
    }
}

// ************************************************************************************************

/**
 * Display the Which Device dialog box.
 * @return
 */
gint displayMultiDeviceDialog(void) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    gchar *tmp_string = NULL;
    gint dialog_selection = 0;

    dialog = gtk_dialog_new_with_buttons(_("Connect to which device?"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else 
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);
#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    label = gtk_label_new(_("Device:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    // Now create the combo box.
#if HAVE_GTK3 == 0
    textbox = gtk_combo_box_new_text();
#else
    textbox = gtk_combo_box_text_new();
#endif
    gtk_widget_show(textbox);
    gtk_container_add(GTK_CONTAINER(hbox), textbox);
    // Now add in our selection strings.
    // We should just take straight strings here, but this is quicker/easier.
    gint i = 0;
    for (i = 0; i < DeviceMgr.numrawdevices; i++) {
        if (DeviceMgr.rawdevices[i].device_entry.vendor != NULL ||
                DeviceMgr.rawdevices[i].device_entry.product != NULL) {
            tmp_string = g_strdup_printf("   %s %s : (%04x:%04x) @ bus %d, dev %d",
                    DeviceMgr.rawdevices[i].device_entry.vendor,
                    DeviceMgr.rawdevices[i].device_entry.product,
                    DeviceMgr.rawdevices[i].device_entry.vendor_id,
                    DeviceMgr.rawdevices[i].device_entry.product_id,
                    DeviceMgr.rawdevices[i].bus_location,
                    DeviceMgr.rawdevices[i].devnum);
        } else {
            tmp_string = g_strdup_printf(_("Unknown : %04x:%04x @ bus %d, dev %d"),
                    DeviceMgr.rawdevices[i].device_entry.vendor_id,
                    DeviceMgr.rawdevices[i].device_entry.product_id,
                    DeviceMgr.rawdevices[i].bus_location,
                    DeviceMgr.rawdevices[i].devnum);
        }
#if HAVE_GTK3 == 0
        gtk_combo_box_append_text(GTK_COMBO_BOX(textbox), tmp_string);
#else
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(textbox), tmp_string);
#endif
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(textbox), 0);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        dialog_selection = gtk_combo_box_get_active(GTK_COMBO_BOX(textbox));
    }
    gtk_widget_destroy(dialog);
    g_free(tmp_string);
    return dialog_selection;
}

// ************************************************************************************************

/**
 * Display the Which Storage Device dialog box.
 * @return
 */
gint displayDeviceStorageDialog(void) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    LIBMTP_devicestorage_t *devicestorage;
    gchar *tmp_string = NULL;
    gint dialog_selection = 0;

    devicestorage = DeviceMgr.device->storage;

    dialog = gtk_dialog_new_with_buttons(_("Connect to which storage device?"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);
    //gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    label = gtk_label_new(_("Storage Device:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    // Now create the combo box.
#if HAVE_GTK3 == 0
    textbox = gtk_combo_box_new_text();
#else
    textbox = gtk_combo_box_text_new();
#endif
    gtk_widget_show(textbox);
    gtk_container_add(GTK_CONTAINER(hbox), textbox);
    // Now add in our selection strings.
    while (devicestorage != NULL) {
        if (devicestorage->StorageDescription != NULL) {
#if HAVE_GTK3 == 0
            gtk_combo_box_append_text(GTK_COMBO_BOX(textbox), devicestorage->StorageDescription);
#else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(textbox), devicestorage->StorageDescription);
#endif
        } else {
            tmp_string = g_strdup_printf(_("Unknown id: %d, %s"), devicestorage->id, calculateFriendlySize((devicestorage->MaxCapacity)));
#if HAVE_GTK3 == 0
            gtk_combo_box_append_text(GTK_COMBO_BOX(textbox), tmp_string);
#else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(textbox), tmp_string);
#endif
        }
        devicestorage = devicestorage->next;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(textbox), 0);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        dialog_selection = gtk_combo_box_get_active(GTK_COMBO_BOX(textbox));
    }
    gtk_widget_destroy(dialog);
    g_free(tmp_string);
    return dialog_selection;

}

// ************************************************************************************************

/**
 * Display the Overwrite File dialog box.
 * @param filename
 * @return
 */
gint displayFileOverwriteDialog(gchar *filename) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_NONE,
            _("File <b>%s</b> already exists in target folder.\nDo you want to:"), filename);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
            _("Skip"), MTP_SKIP,
            _("Skip All"), MTP_SKIP_ALL,
            _("Overwrite"), MTP_OVERWRITE,
            _("Overwrite All"), MTP_OVERWRITE_ALL,
            NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Question: Confirm Overwrite of Existing File?"));
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), MTP_OVERWRITE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return result;
}

// ************************************************************************************************

/**
 * Create the Context Menu widget.
 * @return
 */
GtkWidget* create_windowMainContextMenu(void) {
    GtkWidget* menu;
    //GtkWidget* cfileAdd;
    GtkWidget* cfileRename;
    GtkWidget* cfileRemove;
    GtkWidget* cfileDownload;
    GtkWidget* cfileMove;
    //GtkWidget* cfileNewFolder;
    GtkWidget* cfileRemoveFolder;
    GtkWidget* cfileRescan;
    GtkWidget* cfileAddToPlaylist;
    GtkWidget* cfileRemoveFromPlaylist;
    GtkWidget* menuseparator1;
    GtkWidget* menuseparator2;
    GtkWidget* menuseparator3;

    menu = gtk_menu_new();

    cfileAdd = gtk_menu_item_new_with_label(_("Add Files"));
    gtk_widget_show(cfileAdd);
    gtk_container_add(GTK_CONTAINER(menu), cfileAdd);

    cfileRemove = gtk_menu_item_new_with_label(_("Delete Files"));
    gtk_widget_show(cfileRemove);
    gtk_container_add(GTK_CONTAINER(menu), cfileRemove);

    cfileDownload = gtk_menu_item_new_with_label(_("Download Files"));
    gtk_widget_show(cfileDownload);
    gtk_container_add(GTK_CONTAINER(menu), cfileDownload);

    cfileRename = gtk_menu_item_new_with_label(_("Rename File"));
    gtk_widget_show(cfileRename);
    gtk_container_add(GTK_CONTAINER(menu), cfileRename);

    cfileMove = gtk_menu_item_new_with_label(_("Move To..."));
    gtk_widget_show(cfileMove);
    gtk_container_add(GTK_CONTAINER(menu), cfileMove);

    menuseparator3 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator3);
    gtk_container_add(GTK_CONTAINER(menu), menuseparator3);

    cfileAddToPlaylist = gtk_menu_item_new_with_label(_("Add To Playlist"));
    gtk_widget_show(cfileAddToPlaylist);
    gtk_container_add(GTK_CONTAINER(menu), cfileAddToPlaylist);

    cfileRemoveFromPlaylist = gtk_menu_item_new_with_label(_("Remove From Playlist"));
    gtk_widget_show(cfileRemoveFromPlaylist);
    gtk_container_add(GTK_CONTAINER(menu), cfileRemoveFromPlaylist);

    menuseparator1 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator1);
    gtk_container_add(GTK_CONTAINER(menu), menuseparator1);

    cfileNewFolder = gtk_menu_item_new_with_label(_("Create Folder"));
    gtk_widget_show(cfileNewFolder);
    gtk_container_add(GTK_CONTAINER(menu), cfileNewFolder);

    cfileRemoveFolder = gtk_menu_item_new_with_label(_("Delete Folder"));
    gtk_widget_show(cfileRemoveFolder);
    gtk_container_add(GTK_CONTAINER(menu), cfileRemoveFolder);

    menuseparator2 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator2);
    gtk_container_add(GTK_CONTAINER(menu), menuseparator2);

    cfileRescan = gtk_menu_item_new_with_label(_("Refresh Device"));
    gtk_widget_show(cfileRescan);
    gtk_container_add(GTK_CONTAINER(menu), cfileRescan);

    // Now our call backs.
    g_signal_connect((gpointer) cfileAdd, "activate",
            G_CALLBACK(on_filesAdd_activate),
            NULL);

    g_signal_connect((gpointer) cfileDownload, "activate",
            G_CALLBACK(on_filesDownload_activate),
            NULL);

    g_signal_connect((gpointer) cfileRename, "activate",
            G_CALLBACK(on_fileRenameFile_activate),
            NULL);

    g_signal_connect((gpointer) cfileMove, "activate",
            G_CALLBACK(on_fileMoveFile_activate),
            NULL);

    g_signal_connect((gpointer) cfileRemove, "activate",
            G_CALLBACK(on_filesDelete_activate),
            NULL);

    g_signal_connect((gpointer) cfileNewFolder, "activate",
            G_CALLBACK(on_fileNewFolder_activate),
            NULL);

    g_signal_connect((gpointer) cfileRemoveFolder, "activate",
            G_CALLBACK(on_fileRemoveFolder_activate),
            NULL);

    g_signal_connect((gpointer) cfileRescan, "activate",
            G_CALLBACK(on_deviceRescan_activate),
            NULL);

    g_signal_connect((gpointer) cfileAddToPlaylist, "activate",
            G_CALLBACK(on_fileAddToPlaylist_activate),
            NULL);

    g_signal_connect((gpointer) cfileRemoveFromPlaylist, "activate",
            G_CALLBACK(on_fileRemoveFromPlaylist_activate),
            NULL);

    return menu;

}


// ************************************************************************************************

/**
 * Create the Context Menu widget.
 * @return
 */
GtkWidget* create_windowMainColumnContextMenu(void) {
    GtkWidget* menu;
    menu = gtk_menu_new();

    cViewSize = gtk_check_menu_item_new_with_label(_("File Size"));
    gtk_widget_show(cViewSize);
    gtk_container_add(GTK_CONTAINER(menu), cViewSize);

    cViewType = gtk_check_menu_item_new_with_label(_("File Type"));
    gtk_widget_show(cViewType);
    gtk_container_add(GTK_CONTAINER(menu), cViewType);

    cViewTrackNumber = gtk_check_menu_item_new_with_label(_("Track Number"));
    gtk_widget_show(cViewTrackNumber);
    gtk_container_add(GTK_CONTAINER(menu), cViewTrackNumber);

    cViewTrackName = gtk_check_menu_item_new_with_label(_("Track Name"));
    gtk_widget_show(cViewTrackName);
    gtk_container_add(GTK_CONTAINER(menu), cViewTrackName);

    cViewArtist = gtk_check_menu_item_new_with_label(_("Artist"));
    gtk_widget_show(cViewArtist);
    gtk_container_add(GTK_CONTAINER(menu), cViewArtist);

    cViewAlbum = gtk_check_menu_item_new_with_label(_("Album"));
    gtk_widget_show(cViewAlbum);
    gtk_container_add(GTK_CONTAINER(menu), cViewAlbum);

    cViewYear = gtk_check_menu_item_new_with_label(_("Year"));
    gtk_widget_show(cViewYear);
    gtk_container_add(GTK_CONTAINER(menu), cViewYear);

    cViewGenre = gtk_check_menu_item_new_with_label(_("Genre"));
    gtk_widget_show(cViewGenre);
    gtk_container_add(GTK_CONTAINER(menu), cViewGenre);

    cViewDuration = gtk_check_menu_item_new_with_label(_("Duration"));
    gtk_widget_show(cViewDuration);
    gtk_container_add(GTK_CONTAINER(menu), cViewDuration);


    // Now our call backs.
    g_signal_connect((gpointer) cViewSize, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewTrackNumber, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewTrackName, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewType, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewArtist, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewAlbum, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewYear, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewGenre, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    g_signal_connect((gpointer) cViewDuration, "toggled",
            G_CALLBACK(on_view_activate),
            NULL);

    return menu;

}


// ************************************************************************************************

/**
 * Create the Context Menu widget.
 * @return
 */
GtkWidget* create_windowFolderContextMenu(void) {
    GtkWidget* menu;
    GtkWidget* menuseparator1;
    menu = gtk_menu_new();

    cfFolderAdd = gtk_menu_item_new_with_label(_("Create Folder"));
    gtk_widget_show(cfFolderAdd);
    gtk_container_add(GTK_CONTAINER(menu), cfFolderAdd);

    cfFolderRename = gtk_menu_item_new_with_label(_("Rename Folder"));
    gtk_widget_show(cfFolderRename);
    gtk_container_add(GTK_CONTAINER(menu), cfFolderRename);

    cfFolderDelete = gtk_menu_item_new_with_label(_("Delete Folder"));
    gtk_widget_show(cfFolderDelete);
    gtk_container_add(GTK_CONTAINER(menu), cfFolderDelete);

    cfFolderMove = gtk_menu_item_new_with_label(_("Move To..."));
    gtk_widget_show(cfFolderMove);
    gtk_container_add(GTK_CONTAINER(menu), cfFolderMove);

    menuseparator1 = gtk_separator_menu_item_new();
    gtk_widget_show(menuseparator1);
    gtk_container_add(GTK_CONTAINER(menu), menuseparator1);

    cfFolderRefresh = gtk_menu_item_new_with_label(_("Refresh Device"));
    gtk_widget_show(cfFolderRefresh);
    gtk_container_add(GTK_CONTAINER(menu), cfFolderRefresh);


    // Now our call backs.
    g_signal_connect((gpointer) cfFolderRefresh, "activate",
            G_CALLBACK(on_deviceRescan_activate),
            NULL);

    g_signal_connect((gpointer) cfFolderRename, "activate",
            G_CALLBACK(on_folderRenameFolder_activate),
            NULL);

    g_signal_connect((gpointer) cfFolderDelete, "activate",
            G_CALLBACK(on_folderRemoveFolder_activate),
            NULL);

    g_signal_connect((gpointer) cfFolderMove, "activate",
            G_CALLBACK(on_folderMoveFolder_activate),
            NULL);

    g_signal_connect((gpointer) cfFolderAdd, "activate",
            G_CALLBACK(on_folderNewFolder_activate),
            NULL);


    return menu;

}



// ************************************************************************************************

/**
 * Displays the playlist selection dialog used to auto add track to playlist option.
 * @return The playlist MTP Object ID of the selected playlist, or GMTP_NO_PLAYLIST if none selected.
 */
int32_t displayAddTrackPlaylistDialog(gboolean showNew /* = TRUE */) {
    GtkWidget *dialog, *hbox, *label, *buttonNewPlaylist;
    LIBMTP_playlist_t* tmpplaylist = NULL;
    gint selectedPlaylist = 0;

#if HAVE_GTK3 == 0
    GtkTooltips *tooltips;
    tooltips = gtk_tooltips_new();
#endif

    dialog = gtk_dialog_new_with_buttons(_("Playlists"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else 
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif

    gtk_widget_show(hbox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
#endif

    // Add in the label
    label = gtk_label_new(_("Playlist Name:"));
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(hbox), label);
    gtk_misc_set_padding(GTK_MISC(label), 5, 0);

    // Add in the combobox
#if HAVE_GTK3 == 0
    combobox_AddTrackPlaylist = gtk_combo_box_new_text();
#else
    combobox_AddTrackPlaylist = gtk_combo_box_text_new();
#endif
    gtk_widget_show(combobox_AddTrackPlaylist);
    gtk_box_pack_start(GTK_BOX(hbox), combobox_AddTrackPlaylist, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(combobox_AddTrackPlaylist), 5);

    // Add in the new playlist button.
#if HAVE_GTK3 == 0
    buttonNewPlaylist = gtk_button_new_from_stock(GTK_STOCK_ADD);
#else 
    buttonNewPlaylist = gtk_button_new_with_label(_("Add"));
#endif
    if (showNew == TRUE) {
        gtk_widget_show(buttonNewPlaylist);
        gtk_container_add(GTK_CONTAINER(hbox), buttonNewPlaylist);
        gtk_container_set_border_width(GTK_CONTAINER(buttonNewPlaylist), 5);
    }

#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, buttonNewPlaylist, _("Add New Playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(buttonNewPlaylist, _("Add New Playlist"));
#endif

    // Assign the callback for the new playlist button.
    g_signal_connect((gpointer) buttonNewPlaylist, "clicked",
            G_CALLBACK(on_TrackPlaylist_NewPlaylistButton_activate),
            NULL);

    // Populate the combobox with the current playlists.

    // We need to remove all entries in the combo box before starting.
    // This is a little bit of a hack - but does work.
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combobox_AddTrackPlaylist))));

    if (devicePlayLists != NULL) {
        // Populate the playlist dropdown box;
        //comboboxentry_playlist;
        tmpplaylist = devicePlayLists;
        while (tmpplaylist != NULL) {
            if (tmpplaylist->storage_id == DeviceMgr.devicestorage->id) {
#if HAVE_GTK3 == 0
                gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_AddTrackPlaylist), g_strdup(tmpplaylist->name));
#else
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox_AddTrackPlaylist), g_strdup(tmpplaylist->name));
#endif
            }
            tmpplaylist = tmpplaylist->next;
        }
    }
    if (devicePlayLists != NULL) {
        // Set our playlist to the first one.
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_AddTrackPlaylist), 0);
    }

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        // Get our playlist ID.
        selectedPlaylist = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox_AddTrackPlaylist));
        // Now cycle through the playlists to get the correct one.
        tmpplaylist = devicePlayLists;

        if (selectedPlaylist > 0) {
            while (selectedPlaylist--)
                if (tmpplaylist->next != NULL)
                    tmpplaylist = tmpplaylist->next;
        }
        gtk_widget_destroy(dialog);
        return tmpplaylist->playlist_id;
    } else {
        gtk_widget_destroy(dialog);
        return GMTP_NO_PLAYLIST;
    }
}


// ************************************************************************************************

/**
 * Set the title for the main window.
 * @param foldername - The foldername to be displayed in the application title bar.
 */
void setWindowTitle(gchar *foldername) {
    gchar *winTitle;

    if (foldername == NULL) {
        winTitle = g_strconcat(PACKAGE_NAME, NULL);
    } else {
        winTitle = g_strconcat(foldername, " - ", PACKAGE_NAME, NULL);
    }
    gtk_window_set_title(GTK_WINDOW(windowMain), (winTitle));
    g_free(winTitle);

}


// ************************************************************************************************

/**
 * Destroys a file listing object.
 * @param file - pointer to the FileListStruc object.
 */
void g_free_search(FileListStruc *file) {
    if (file != NULL) {
        if (file->filename != NULL) {
            g_free(file->filename);
        }
        if (file->location != NULL) {
            g_free(file->location);
        }
    }
    g_free(file);
}


// ************************************************************************************************

/**
 * Add a list of file to the nominated playlist.
 * @param List
 * @param PlaylistID
 * @return
 */
gboolean fileListAddToPlaylist(GList *List, uint32_t PlaylistID) {
    LIBMTP_playlist_t *playlist = NULL;
    LIBMTP_playlist_t *node = NULL;

    node = devicePlayLists;
    while (node != NULL) {
        if (node->playlist_id == PlaylistID) {
            playlist = node;
            node = NULL;
        } else {
            node = node->next;
        }
    }
    if (playlist != NULL) {
        g_list_foreach(List, (GFunc) __fileAddToPlaylist, (gpointer) & playlist);
    }
    return TRUE;
}




// ************************************************************************************************

/**
 * Displays a dialog box with all the device folders in it, and prompts the user to select one of
 * the folders.
 * @return The object ID of the selected folder.
 */
int64_t getTargetFolderLocation(void) {
    GtkWidget *dialog;
    GtkWidget *treeviewFoldersDialog;
    GtkTreeStore *folderListDialog;
    GtkTreeSelection *folderSelectionDialog;
    GtkWidget *scrolledwindowFoldersDialog;
    GtkTreeModel *folderListModelDialog;

    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;
    uint32_t objectID = 0;

    dialog = gtk_dialog_new_with_buttons(_("Move To..."), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_OK"), GTK_RESPONSE_OK,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 240, 400);

    // Actual folder list.
    scrolledwindowFoldersDialog = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindowFoldersDialog);
#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), scrolledwindowFoldersDialog);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindowFoldersDialog);
    gtk_widget_set_vexpand(scrolledwindowFoldersDialog, TRUE);
#endif

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindowFoldersDialog), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    treeviewFoldersDialog = gtk_tree_view_new();
    gtk_widget_show(treeviewFoldersDialog);
    gtk_container_add(GTK_CONTAINER(scrolledwindowFoldersDialog), treeviewFoldersDialog);
    gtk_container_set_border_width(GTK_CONTAINER(treeviewFoldersDialog), 5);
    folderSelectionDialog = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviewFoldersDialog));
    gtk_tree_selection_set_mode(folderSelectionDialog, GTK_SELECTION_SINGLE);

    folderListDialog = gtk_tree_store_new(NUM_FOL_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, GDK_TYPE_PIXBUF);
    setupFolderList(GTK_TREE_VIEW(treeviewFoldersDialog));

    folderListModelDialog = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(folderListDialog));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(folderListModelDialog),
            COL_FOL_NAME_HIDDEN, GTK_SORT_ASCENDING);

    gtk_tree_view_set_model(GTK_TREE_VIEW(treeviewFoldersDialog), GTK_TREE_MODEL(folderListModelDialog));

    folderListAddDialog(deviceFolders, NULL, folderListDialog);

    gtk_tree_view_expand_all(GTK_TREE_VIEW(treeviewFoldersDialog));
    g_object_unref(folderListDialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {

        // Get our selected row.
        sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFoldersDialog));

        if (gtk_tree_selection_get_selected(folderSelectionDialog, &sortmodel, &iter)) {
            gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(folderListDialog), &childiter, COL_FOL_ID, &objectID, -1);
            gtk_widget_destroy(dialog);
            return objectID;
        }
    }
    gtk_widget_destroy(dialog);
    return -1;
}


// ************************************************************************************************

/**
 * Add folders to the folder list in dialog window.
 */
gboolean folderListAddDialog(LIBMTP_folder_t *folders, GtkTreeIter *parent, GtkTreeStore *fl) {
    GtkTreeIter rowIter;
    GdkPixbuf *image = NULL;

    if (parent == NULL) {
        // Add in the root node.
        image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
        // Now add in the row information.
        gtk_tree_store_append(GTK_TREE_STORE(fl), &rowIter, parent);
        gtk_tree_store_set(GTK_TREE_STORE(fl), &rowIter,
                //COL_FOL_NAME, folders->name,
                COL_FOL_NAME_HIDDEN, "/",
                COL_FOL_ID, 0,
                COL_FOL_ICON, image,
                -1);

        // Indicate we are done with this image.
        g_object_unref(image);
        folderListAddDialog(folders, &rowIter, fl);
        return TRUE;
    }

    while (folders != NULL) {

        // Only add in folder if it's in the current storage device.
        if (folders->storage_id == DeviceMgr.devicestorage->id) {

            image = gdk_pixbuf_new_from_file(file_folder_png, NULL);
            // Now add in the row information.
            gtk_tree_store_append(GTK_TREE_STORE(fl), &rowIter, parent);
            gtk_tree_store_set(GTK_TREE_STORE(fl), &rowIter,
                    //COL_FOL_NAME, folders->name,
                    COL_FOL_NAME_HIDDEN, folders->name,
                    COL_FOL_ID, folders->folder_id,
                    COL_FOL_ICON, image,
                    -1);

            // Indicate we are done with this image.
            g_object_unref(image);
            if (folders->child != NULL) {
                // Call our child.
                folderListAddDialog(folders->child, &rowIter, fl);
            }
        }
        folders = folders->sibling;
    }
    return TRUE;
}

// ************************************************************************************************

/**
 * Get a string representation of the size 
 * @param value The value to convert to a string.
 * @return A string that represents the size in a friendly manner.
 */
gchar *calculateFriendlySize(const uint64_t value){
    if(value < KILOBYTE){
        return g_strdup_printf("%lu B", (long unsigned) value);
    };
    if(value < MEGABYTE){
        return g_strdup_printf("%.1f KB", value / 1024.00);
    }
    if(value < GIGABYTE){
        return g_strdup_printf("%.1f MB", value / (1024.00 * 1024.00));
    };
    return g_strdup_printf("%.1f GB", value / (1024.00 * 1024.00 * 1024.00));
}

