/* 
 *
 *   File: albumart.h
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

#ifndef _ALBUMART_H
#define _ALBUMART_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    // AlbumArt Dialog global pointers
    GtkWidget *AlbumArtDialog;
    GtkWidget *AlbumArtImage;
    GtkWidget *buttonAlbumAdd;
    GtkWidget *buttonAlbumDownload;
    GtkWidget *buttonAlbumDelete;
    GtkWidget *textboxAlbumArt;
    
    // Set Album Art dialog;
#define ALBUM_SIZE 96

    void displayAddAlbumArtDialog(void);
    void AlbumArtUpdateImage(LIBMTP_album_t* selectedAlbum);
    void AlbumArtSetDefault(void);

    // Add Album Art Dialog
    void on_buttonAlbumArtAdd_activate(GtkWidget *button, gpointer user_data);
    void on_buttonAlbumArtDelete_activate(GtkWidget *button, gpointer user_data);
    void on_buttonAlbumArtDownload_activate(GtkWidget *button, gpointer user_data);
    void on_albumtextbox_activate(GtkComboBox *combobox, gpointer user_data);
    // Main menu callback
    void on_editAddAlbumArt_activate(GtkMenuItem *menuitem, gpointer user_data);
#ifdef  __cplusplus
}
#endif

#endif  /* _ALBUMART_H */
