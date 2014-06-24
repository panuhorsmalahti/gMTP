/* 
 *
 *   File: playlist.h
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

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    GtkWidget *windowPlaylistDialog;
    // Playlist
    GtkWidget *treeview_Avail_Files;
    GtkWidget *treeview_Playlist_Files;
    GtkWidget *comboboxentry_playlist;
    GtkListStore *playlist_TrackList;
    GtkListStore *playlist_PL_List;

    // Buttons for playlist
    GtkWidget *button_Del_Playlist;
    GtkWidget *button_Export_Playlist;
    GtkWidget *button_File_Move_Up;
    GtkWidget *button_File_Move_Down;
    GtkWidget *button_Del_File;
    GtkWidget *button_Add_Files;
    
    // Create playlist dialog
    GtkWidget* create_windowPlaylist(void);
    
    // Main menu callback
    void on_editPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileAddToPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_fileRemoveFromPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    
    // Playlists

    gint playlist_number;
    gint comboboxentry_playlist_entries;

    void displayPlaylistDialog(void);
    void setupTrackList(GtkTreeView *treeviewFiles);
    void setup_PL_List(GtkTreeView *treeviewFiles);
    void SetPlaylistButtonState(gboolean state);
    void setPlayListComboBox(void);
    void setPlaylistField(gint PlayListID);
    gchar* displayPlaylistNewDialog(void);

    gboolean playlist_PL_ListClearSelection();
    GList* playlist_PL_ListGetSelection();
    gboolean playlist_PL_ListRemove(GList *List);
    void __playlist_PL_Remove(GtkTreeRowReference *Row);

    GList* playlist_TrackList_GetSelection();
    gboolean playlist_TrackList_Add(GList *List);
    void __playlist_TrackList_Add(GtkTreeRowReference *Row);

    gboolean playlist_move_files(gint direction);
    void __playlist_move_files_up(GtkTreeRowReference *Row);
    void __playlist_move_files_down(GtkTreeRowReference *Row);

    void playlist_SavePlaylist(gint PlayListID);

    gboolean fileListAddToPlaylist(GList *List, uint32_t PlaylistID);
    gboolean fileListRemoveFromPlaylist(GList *List, uint32_t PlaylistID);
    void __fileAddToPlaylist(GtkTreeRowReference *Row, LIBMTP_playlist_t **playlist);
    void __fileRemoveFromPlaylist(GtkTreeRowReference *Row, LIBMTP_playlist_t **playlist);

    
    // Playlist Dialog
    void on_quitPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_NewPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_ImportPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_ExportPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_DelPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_DelFileButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_AddFileButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_FileUpButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_FileDownButton_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_Playlist_Combobox_activate(GtkComboBox *combobox, gpointer user_data);

#ifdef  __cplusplus
}
#endif

#endif  /* _PLAYLIST_H */
