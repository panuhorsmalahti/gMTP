/* 
 *
 *   File: preferences.h
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
 
 
#ifndef _PREFERENCES_H
#define _PREFERENCES_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    // Main preferences dialog if present.
    GtkWidget *windowPrefsDialog;

    // Main menu preference item.
    void on_preferences1_activate(GtkMenuItem *menuitem, gpointer user_data);

    // Create the dialog.
    GtkWidget* create_windowPreferences(void);

    // Widgets for preferences buttons;
    GtkWidget *comboboxToolbarStyle;
    GtkWidget *checkbuttonDeviceConnect;
    GtkWidget *entryDownloadPath;
    GtkWidget *entryUploadPath;
    GtkWidget *checkbuttonDownloadPath;
    GtkWidget *checkbuttonConfirmFileOp;
    GtkWidget *checkbuttonConfirmOverWriteFileOp;
    GtkWidget *checkbuttonAutoAddTrackPlaylist;
    GtkWidget *checkbuttonIgnorePathInPlaylist;
    GtkWidget *checkbuttonSuppressAlbumErrors;
    GtkWidget *checkbuttonAltAccessMethod;
    GtkWidget *checkbuttonAllMediaAsFiles;
 
    // Preferences Dialog callbacks
    void on_quitPrefs_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsDevice_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsAskDownload_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsDownloadPath_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsUploadPath_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsConfirmDelete_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsConfirmOverWriteFileOp_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsAutoAddTrackPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsIgnorePathInPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsSuppressAlbumError_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsUseAltAccessMethod_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsAllMediaAsFiles_activate(GtkMenuItem *menuitem, gpointer user_data);
    void on_PrefsToolbarStyle_activate(GtkComboBox *combobox, gpointer user_data);
#ifdef  __cplusplus
}
#endif

#endif  /* _PREFERENCES_H */
