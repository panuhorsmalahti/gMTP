/* 
 *
 *   File: callbacks.h
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

#ifndef _CALLBACKS_H
#define _CALLBACKS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    // Main window functions.
    void on_quit1_activate(GtkMenuItem *menuitem, gpointer user_data);
    
    void on_about1_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_deviceConnect_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_deviceRescan_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_filesAdd_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_filesDelete_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_filesDownload_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileNewFolder_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileRemoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileRenameFile_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileMoveFile_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_editDeviceName_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_editFind_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_editSelectAll_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_view_activate(GtkMenuItem *menuitem, gpointer user_data);

    // Treeview handling.
    void fileListRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
    gboolean on_windowMainContextMenu_activate(GtkWidget *widget, GdkEvent *event);
    gboolean on_windowViewContextMenu_activate(GtkWidget *widget, GdkEvent *event);
    void on_treeviewFolders_rowactivated(GtkTreeSelection *treeselection, gpointer user_data);

    // Folder Treeview handling.
    void folderListRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
    void on_folderNewFolder_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_folderRemoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_folderRenameFolder_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_folderMoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data);

    // Add Track to Playlist option.
    void on_TrackPlaylist_NewPlaylistButton_activate(GtkWidget *button, gpointer user_data);

    // Search function;
    void on_editFindSearch_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_editFindClose_activate(GtkMenuItem *menuitem, gpointer user_data);

#ifdef  __cplusplus
}
#endif

#endif  /* _CALLBACKS_H */
