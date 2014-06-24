/* 
 *
 *   File: playlist.c
 *
 *   Copyright (C) 2009-2014 Darran Kartaschew
 *
 *   This file is part of the gMTP package.
 *
 *   gMTP is free software; you can redistribute it and/or modify
 *   it under the terms of the BSD License as included within the
 *   file 'COPYING' located in the root directory
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#endif
#include <gtk/gtk.h>
#include <libmtp.h>
#include <id3tag.h>
#include <stdlib.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "about.h"
#include "progress.h"
#include "properties.h"
#include "preferences.h"
#include "playlist.h"

GtkWidget *windowPlaylistDialog;
// Playlist

GtkWidget *comboboxentry_playlist;
gint playlist_number = 0;
gint comboboxentry_playlist_entries = 0;
gint playlist_track_count = 0;
gboolean runPlaylistHandler = TRUE;

GtkWidget *treeview_Avail_Files;
GtkWidget *treeview_Playlist_Files;

GtkListStore *playlist_TrackList;
GtkTreeSelection *playlist_TrackSelection;
GList *playlist_Selection_TrackRowReferences = NULL;

GtkListStore *playlist_PL_List;
GtkTreeSelection *playlist_PL_Selection;
GList *playlist_Selection_PL_RowReferences = NULL;

// Buttons for playlist
GtkWidget *button_Del_Playlist;
GtkWidget *button_Export_Playlist;
GtkWidget *button_File_Move_Up;
GtkWidget *button_File_Move_Down;
GtkWidget *button_Del_File;
GtkWidget *button_Add_Files;

// ************************************************************************************************

// Playlist support

/**
 * Create the Playlist Editor Window.
 * @return
 */
GtkWidget* create_windowPlaylist(void) {
    GtkWidget *window_playlist;
    GtkWidget *vbox1;
    GtkWidget *hbox1;
    GtkWidget *label_Playlist;

    GtkWidget *button_Add_Playlist;
    GtkWidget *button_Import_Playlist;
    GtkWidget *alignment2;
    GtkWidget *hbox3;
    GtkWidget *image2;
    GtkWidget *label3;

    GtkWidget *alignment1;
    GtkWidget *hbox2;
    GtkWidget *image1;
    GtkWidget *label2;
    GtkWidget *hbox4;
    GtkWidget *scrolledwindow2;

    GtkWidget *vbuttonbox1;

    GtkWidget *alignment6;
    GtkWidget *hbox8;
    GtkWidget *image6;
    GtkWidget *label10;

    GtkWidget *alignment7;
    GtkWidget *hbox9;
    GtkWidget *image7;
    GtkWidget *label11;
    GtkWidget *scrolledwindow3;

    GtkWidget *vbuttonbox2;

    GtkWidget *hbuttonbox1;
    GtkWidget *button_Close;


#if HAVE_GTK3 == 0
    GtkTooltips *tooltips;
    tooltips = gtk_tooltips_new();
#endif

    window_playlist = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gchar * winTitle;
    winTitle = g_strconcat(PACKAGE_NAME, _(" Playlists"), NULL);
    gtk_window_set_title(GTK_WINDOW(window_playlist), winTitle);
    gtk_window_set_modal(GTK_WINDOW(window_playlist), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window_playlist), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window_playlist), 760, 400);
    gtk_window_set_transient_for(GTK_WINDOW(window_playlist), GTK_WINDOW(windowMain));
    gtk_window_set_position(GTK_WINDOW(window_playlist), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window_playlist), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window_playlist), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_free(winTitle);
#if HAVE_GTK3 == 0
    vbox1 = gtk_vbox_new(FALSE, 0);
#else
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(window_playlist), vbox1);
#if HAVE_GTK3 == 0
    hbox1 = gtk_hbox_new(FALSE, 5);
#else
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, TRUE, 5);

    label_Playlist = gtk_label_new(_("Current Playlist: "));
    gtk_widget_show(label_Playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), label_Playlist, FALSE, FALSE, 5);
    gtk_misc_set_padding(GTK_MISC(label_Playlist), 5, 0);

#if HAVE_GTK3 == 0
    comboboxentry_playlist = gtk_combo_box_new_text();
#else
    comboboxentry_playlist = gtk_combo_box_text_new();
#endif
    gtk_widget_show(comboboxentry_playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), comboboxentry_playlist, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(comboboxentry_playlist), 5);

    button_Add_Playlist = gtk_button_new();
    gtk_widget_show(button_Add_Playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), button_Add_Playlist, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(button_Add_Playlist), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Add_Playlist, _("Add New Playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Add_Playlist, _("Add New Playlist"));
#endif

    alignment2 = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(alignment2);
    gtk_container_add(GTK_CONTAINER(button_Add_Playlist), alignment2);
#if HAVE_GTK3 == 0
    hbox3 = gtk_hbox_new(FALSE, 2);
#else
    hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#endif
    gtk_widget_show(hbox3);
    gtk_container_add(GTK_CONTAINER(alignment2), hbox3);
#if HAVE_GTK3 == 0
    image2 = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
#else 
    image2 = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
#endif
    gtk_widget_show(image2);
    gtk_box_pack_start(GTK_BOX(hbox3), image2, FALSE, FALSE, 0);

    label3 = gtk_label_new_with_mnemonic(_("Add"));
    gtk_widget_show(label3);
    gtk_box_pack_start(GTK_BOX(hbox3), label3, FALSE, FALSE, 0);

    button_Del_Playlist = gtk_button_new();
    gtk_widget_show(button_Del_Playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), button_Del_Playlist, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(button_Del_Playlist), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Del_Playlist, _("Remove Current Selected Playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Del_Playlist, _("Remove Current Selected Playlist"));
#endif

    alignment1 = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(alignment1);
    gtk_container_add(GTK_CONTAINER(button_Del_Playlist), alignment1);
#if HAVE_GTK3 == 0
    hbox2 = gtk_hbox_new(FALSE, 2);
#else
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#endif

    gtk_widget_show(hbox2);
    gtk_container_add(GTK_CONTAINER(alignment1), hbox2);
#if HAVE_GTK3 == 0
    image1 = gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON);
#else
    image1 = gtk_image_new_from_icon_name("edit-delete", GTK_ICON_SIZE_BUTTON);
#endif
    gtk_widget_show(image1);
    gtk_box_pack_start(GTK_BOX(hbox2), image1, FALSE, FALSE, 0);

    label2 = gtk_label_new_with_mnemonic(_("Del"));
    gtk_widget_show(label2);
    gtk_box_pack_start(GTK_BOX(hbox2), label2, FALSE, FALSE, 0);

    // Import Button
#if HAVE_GTK3 == 0
    button_Import_Playlist = gtk_button_new_from_stock(GTK_STOCK_OPEN);
#else
    button_Import_Playlist = gtk_button_new_with_label(_("Open"));
#endif
    gtk_widget_show(button_Import_Playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), button_Import_Playlist, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(button_Import_Playlist), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Import_Playlist, _("Import Playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Import_Playlist, _("Import Playlist"));
#endif

    // Export Button
#if HAVE_GTK3 == 0
    button_Export_Playlist = gtk_button_new_from_stock(GTK_STOCK_SAVE_AS);
#else
    button_Export_Playlist = gtk_button_new_with_label(_("Save"));
#endif
    gtk_widget_show(button_Export_Playlist);
    gtk_box_pack_start(GTK_BOX(hbox1), button_Export_Playlist, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(button_Export_Playlist), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Export_Playlist, _("Export Playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Export_Playlist, _("Export Playlist"));
#endif

    // Scrolled Window.
#if HAVE_GTK3 == 0
    hbox4 = gtk_hbox_new(FALSE, 5);
#else
    hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox4);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox4, TRUE, TRUE, 0);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(hbox4), scrolledwindow2, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow2), 5);

    treeview_Avail_Files = gtk_tree_view_new();
    gtk_widget_show(treeview_Avail_Files);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview_Avail_Files);
    gtk_container_set_border_width(GTK_CONTAINER(treeview_Avail_Files), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, treeview_Avail_Files, _("Device Audio Tracks"), NULL);
#else
    gtk_widget_set_tooltip_text(treeview_Avail_Files, _("Device Audio Tracks"));
#endif

    playlist_TrackSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_Avail_Files));
    gtk_tree_selection_set_mode(playlist_TrackSelection, GTK_SELECTION_MULTIPLE);

    playlist_TrackList = gtk_list_store_new(NUM_TCOLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);
    setupTrackList(GTK_TREE_VIEW(treeview_Avail_Files));
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_Avail_Files), GTK_TREE_MODEL(playlist_TrackList));
    g_object_unref(playlist_TrackList);
#if HAVE_GTK3 == 0
    vbuttonbox1 = gtk_vbutton_box_new();
#else
    vbuttonbox1 = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
#endif
    gtk_widget_show(vbuttonbox1);
    gtk_box_pack_start(GTK_BOX(hbox4), vbuttonbox1, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox1), GTK_BUTTONBOX_SPREAD);

    button_Add_Files = gtk_button_new();
    gtk_widget_show(button_Add_Files);
    gtk_container_add(GTK_CONTAINER(vbuttonbox1), button_Add_Files);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Add_Files, _("Add file to playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Add_Files, _("Add file to playlist"));
#endif

    alignment6 = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(alignment6);
    gtk_container_add(GTK_CONTAINER(button_Add_Files), alignment6);
#if HAVE_GTK3 == 0
    hbox8 = gtk_hbox_new(FALSE, 2);
#else
    hbox8 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#endif
    gtk_widget_show(hbox8);
    gtk_container_add(GTK_CONTAINER(alignment6), hbox8);
#if HAVE_GTK3 == 0
    image6 = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
#else
    image6 = gtk_image_new_from_icon_name("go-next", GTK_ICON_SIZE_BUTTON);
#endif
    gtk_widget_show(image6);
    gtk_box_pack_start(GTK_BOX(hbox8), image6, FALSE, FALSE, 0);

    label10 = gtk_label_new_with_mnemonic(_("Add File"));
    gtk_widget_show(label10);
    gtk_box_pack_start(GTK_BOX(hbox8), label10, FALSE, FALSE, 0);

    button_Del_File = gtk_button_new();
    gtk_widget_show(button_Del_File);
    gtk_container_add(GTK_CONTAINER(vbuttonbox1), button_Del_File);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_Del_File, _("Remove file from playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_Del_File, _("Remove file from playlist"));
#endif

    alignment7 = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(alignment7);
    gtk_container_add(GTK_CONTAINER(button_Del_File), alignment7);
#if HAVE_GTK3 == 0
    hbox9 = gtk_hbox_new(FALSE, 2);
#else
    hbox9 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#endif
    gtk_widget_show(hbox9);
    gtk_container_add(GTK_CONTAINER(alignment7), hbox9);
#if HAVE_GTK3 == 0
    image7 = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
#else
    image7 = gtk_image_new_from_icon_name("go-previous", GTK_ICON_SIZE_BUTTON);
#endif
    gtk_widget_show(image7);
    gtk_box_pack_start(GTK_BOX(hbox9), image7, FALSE, FALSE, 0);

    label11 = gtk_label_new_with_mnemonic(_("Del File"));
    gtk_widget_show(label11);
    gtk_box_pack_start(GTK_BOX(hbox9), label11, FALSE, FALSE, 0);

    scrolledwindow3 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow3);
    gtk_box_pack_start(GTK_BOX(hbox4), scrolledwindow3, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow3), 5);

    treeview_Playlist_Files = gtk_tree_view_new();
    gtk_widget_show(treeview_Playlist_Files);
    gtk_container_add(GTK_CONTAINER(scrolledwindow3), treeview_Playlist_Files);
    gtk_container_set_border_width(GTK_CONTAINER(treeview_Playlist_Files), 5);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, treeview_Playlist_Files, _("Playlist Audio Tracks"), NULL);
#else
    gtk_widget_set_tooltip_text(treeview_Playlist_Files, _("Playlist Audio Tracks"));
#endif

    playlist_PL_Selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_Playlist_Files));
    gtk_tree_selection_set_mode(playlist_PL_Selection, GTK_SELECTION_MULTIPLE);

    playlist_PL_List = gtk_list_store_new(NUM_PL_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);
    setup_PL_List(GTK_TREE_VIEW(treeview_Playlist_Files));
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_Playlist_Files), GTK_TREE_MODEL(playlist_PL_List));
    g_object_unref(playlist_PL_List);
#if HAVE_GTK3 == 0
    vbuttonbox2 = gtk_vbutton_box_new();
#else
    vbuttonbox2 = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
#endif
    gtk_widget_show(vbuttonbox2);
    gtk_box_pack_start(GTK_BOX(hbox4), vbuttonbox2, FALSE, FALSE, 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox2), GTK_BUTTONBOX_SPREAD);
#if HAVE_GTK3 == 0
    button_File_Move_Up = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
#else
#if GTK_CHECK_VERSION(3,10,0)
    button_File_Move_Up = gtk_button_new_from_icon_name("go-up", GTK_ICON_SIZE_BUTTON);
#else
    button_File_Move_Up = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
#endif
#endif
    gtk_widget_show(button_File_Move_Up);
    gtk_container_add(GTK_CONTAINER(vbuttonbox2), button_File_Move_Up);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_File_Move_Up, _("Move selected file up in the playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_File_Move_Up, _("Move selected file up in the playlist"));
#endif

#if HAVE_GTK3 == 0
    button_File_Move_Down = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
#else 
#if GTK_CHECK_VERSION(3,10,0)
    button_File_Move_Down = gtk_button_new_from_icon_name("go-down", GTK_ICON_SIZE_BUTTON);
#else
    button_File_Move_Down = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
#endif
#endif
    gtk_widget_show(button_File_Move_Down);
    gtk_container_add(GTK_CONTAINER(vbuttonbox2), button_File_Move_Down);
#if HAVE_GTK3 == 0
    gtk_tooltips_set_tip(tooltips, button_File_Move_Down, _("Move selected file down in the playlist"), NULL);
#else
    gtk_widget_set_tooltip_text(button_File_Move_Down, _("Move selected file down in the playlist"));
#endif
#if HAVE_GTK3 == 0
    hbuttonbox1 = gtk_hbutton_box_new();
#else
    hbuttonbox1 = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#endif
    gtk_widget_show(hbuttonbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox1, FALSE, FALSE, 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1), GTK_BUTTONBOX_END);
#if HAVE_GTK3 == 0
    button_Close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
#else
    button_Close = gtk_button_new_with_label(_("Close"));
#endif
    gtk_widget_show(button_Close);
    gtk_container_add(GTK_CONTAINER(hbuttonbox1), button_Close);
    gtk_container_set_border_width(GTK_CONTAINER(button_Close), 5);

    g_signal_connect((gpointer) window_playlist, "destroy",
            G_CALLBACK(on_quitPlaylist_activate),
            NULL);

    g_signal_connect((gpointer) button_Close, "clicked",
            G_CALLBACK(on_quitPlaylist_activate),
            NULL);

    g_signal_connect((gpointer) button_Add_Playlist, "clicked",
            G_CALLBACK(on_Playlist_NewPlaylistButton_activate),
            NULL);

    g_signal_connect((gpointer) button_Import_Playlist, "clicked",
            G_CALLBACK(on_Playlist_ImportPlaylistButton_activate),
            NULL);

    g_signal_connect((gpointer) button_Export_Playlist, "clicked",
            G_CALLBACK(on_Playlist_ExportPlaylistButton_activate),
            NULL);

    g_signal_connect((gpointer) button_Del_Playlist, "clicked",
            G_CALLBACK(on_Playlist_DelPlaylistButton_activate),
            NULL);

    g_signal_connect((gpointer) button_Del_File, "clicked",
            G_CALLBACK(on_Playlist_DelFileButton_activate),
            NULL);

    g_signal_connect((gpointer) button_Add_Files, "clicked",
            G_CALLBACK(on_Playlist_AddFileButton_activate),
            NULL);

    g_signal_connect((gpointer) button_File_Move_Up, "clicked",
            G_CALLBACK(on_Playlist_FileUpButton_activate),
            NULL);

    g_signal_connect((gpointer) button_File_Move_Down, "clicked",
            G_CALLBACK(on_Playlist_FileDownButton_activate),
            NULL);

    g_signal_connect((gpointer) comboboxentry_playlist, "changed",
            G_CALLBACK(on_Playlist_Combobox_activate),
            NULL);

    return window_playlist;
}

// ************************************************************************************************

/**
 * Callback to hanlde the Playlist menu/toolbar operations.
 * @param menuitem
 * @param user_data
 */
void on_editPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data) {
    displayPlaylistDialog();
} // end on_editPlaylist_activate()


// ************************************************************************************************

/**
 * Callback to handle adding file to playlist.
 * @param menuitem
 * @param user_data
 */
void on_fileAddToPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data) {
    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {
        displayInformation(_("No files/folders selected?"));
        return;
    }
    // Display the select playlist dialog;
    int32_t addTrackPlaylistID = displayAddTrackPlaylistDialog(TRUE);

    // Now add the actual files from the MTP device.
    if (addTrackPlaylistID != GMTP_NO_PLAYLIST) {
        fileListAddToPlaylist(fileListGetSelection(), addTrackPlaylistID);
    }
}


// ************************************************************************************************

/**
 * Callback to handle removing file from playlist.
 * @param menuitem
 * @param user_data
 */
void on_fileRemoveFromPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data) {
    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {
        displayInformation(_("No files/folders selected?"));
        return;
    }
    // Display the select playlist dialog;
    int32_t addTrackPlaylistID = displayAddTrackPlaylistDialog(FALSE);

    // Now remove the actual files from the MTP device.
    if (addTrackPlaylistID != GMTP_NO_PLAYLIST) {
        fileListRemoveFromPlaylist(fileListGetSelection(), addTrackPlaylistID);
    }
}

// ************************************************************************************************

/**
 * Remove a list of files to the nominated playlist.
 * @param List
 * @param PlaylistID
 * @return
 */
gboolean fileListRemoveFromPlaylist(GList *List, uint32_t PlaylistID) {
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
        g_list_foreach(List, (GFunc) __fileRemoveFromPlaylist, (gpointer) & playlist);
    }
    return TRUE;
}


// ************************************************************************************************

/**
 * Helper to add a single row to playlist.
 * @param Row
 * @param playlist
 */
void __fileAddToPlaylist(GtkTreeRowReference *Row, LIBMTP_playlist_t **playlist) {
    GtkTreePath *path;
    GtkTreeIter iter;
    uint32_t objectID;
    gboolean isFolder;
    LIBMTP_track_t *tracks = deviceTracks;
    LIBMTP_track_t *node = NULL;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILEID, &objectID, -1);
    if (isFolder == FALSE) {
        // Now add the file to the playlist.
        // We need the playlist pointer, and the **track** pointer;
        while (tracks != NULL) {
            if (tracks->item_id == objectID) {
                node = tracks;
                tracks = NULL;
            } else {
                tracks = tracks->next;
            }
        }
        if (node != NULL) {
            playlistAddTrack(*(playlist), node);
        }
    }
}


// ************************************************************************************************

/**
 * Helper to remove a single row from a playlist.
 * @param Row
 * @param playlist
 */
void __fileRemoveFromPlaylist(GtkTreeRowReference *Row, LIBMTP_playlist_t **playlist) {
    GtkTreePath *path;
    GtkTreeIter iter;
    uint32_t objectID;
    gboolean isFolder;
    LIBMTP_track_t *tracks = deviceTracks;
    LIBMTP_track_t *node = NULL;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILEID, &objectID, -1);
    if (isFolder == FALSE) {
        // Now add the file to the playlist.
        // We need the playlist pointer, and the **track** pointer;
        while (tracks != NULL) {
            if (tracks->item_id == objectID) {
                node = tracks;
                tracks = NULL;
            } else {
                tracks = tracks->next;
            }
        }
        if (node != NULL) {
            playlistRemoveTrack(*(playlist), node, MTP_PLAYLIST_FIRST_INSTANCE);
        }
    }
}


// ************************************************************************************************

/**
 * Display the Playlist Editor.
 */
void displayPlaylistDialog(void) {
    //LIBMTP_playlist_t* tmpplaylist;
    LIBMTP_track_t* tmptrack;
    GtkTreeIter rowIter;
    gchar * tmp_string;

    if (windowPlaylistDialog != NULL) {
        gtk_widget_hide(windowPlaylistDialog);
        gtk_widget_destroy(windowPlaylistDialog);
    }
    windowPlaylistDialog = create_windowPlaylist();
    playlist_number = 0;
    // Clear the track and playlist lists;
    gtk_list_store_clear(GTK_LIST_STORE(playlist_PL_List));
    gtk_list_store_clear(GTK_LIST_STORE(playlist_TrackList));
    // Populate the playlist changebox.
    devicePlayLists = getPlaylists();
    deviceTracks = getTracks();
    setPlayListComboBox();

    // Populate the available track list.
    if (deviceTracks != NULL) {
        // Populate the track list;
        tmptrack = deviceTracks;
        while (tmptrack != NULL) {
            if ((tmptrack->storage_id == DeviceMgr.devicestorage->id) && (LIBMTP_FILETYPE_IS_AUDIO(tmptrack->filetype))) {
                gtk_list_store_append(GTK_LIST_STORE(playlist_TrackList), &rowIter);
                tmp_string = g_strdup_printf("%d:%.2d", (int) ((tmptrack->duration / 1000) / 60), (int) ((tmptrack->duration / 1000) % 60));
                gtk_list_store_set(GTK_LIST_STORE(playlist_TrackList), &rowIter, COL_ARTIST, tmptrack->artist, COL_ALBUM, tmptrack->album,
                        COL_TRACKID, tmptrack->item_id, COL_TRACKNAME, tmptrack->title, COL_TRACKDURATION, tmp_string, -1);
                g_free(tmp_string);
                tmp_string = NULL;
            }
            tmptrack = tmptrack->next;
        }
    }
    gtk_widget_show(GTK_WIDGET(windowPlaylistDialog));
    // Save the current selected playlist if needed.
}

// ************************************************************************************************

/**
 * Setup the display for the Playlist.
 * @param treeviewFiles
 */
void setupTrackList(GtkTreeView *treeviewFiles) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Artist
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Artist"), renderer,
            "text", COL_ARTIST,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_sort_column_id(column, COL_ARTIST);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    // Album column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Album"), renderer,
            "text", COL_ALBUM,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_sort_column_id(column, COL_ALBUM);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    // Folder/FileID column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Object ID", renderer,
            "text", COL_TRACKID,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // Track column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Track"), renderer,
            "text", COL_TRACKNAME,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_sort_column_id(column, COL_TRACKNAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_visible(column, TRUE);

    // Track Duration
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Duration"), renderer,
            "text", COL_TRACKDURATION,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_visible(column, TRUE);


}

// ************************************************************************************************

/**
 * Setup the list of tracks in the current playlist.
 * @param treeviewFiles
 */
void setup_PL_List(GtkTreeView *treeviewFiles) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Order Num
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Num"), renderer,
            "text", COL_PL_ORDER_NUM,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    //gtk_tree_view_column_set_sort_column_id(column, COL_PL_ORDER_NUM);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    // Artist
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Artist"), renderer,
            "text", COL_PL_ARTIST,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    //gtk_tree_view_column_set_sort_column_id(column, COL_PL_ARTIST);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    // Album column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Album"), renderer,
            "text", COL_PL_ALBUM,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    //gtk_tree_view_column_set_sort_column_id(column, COL_PL_ALBUM);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_spacing(column, 5);

    // Folder/FileID column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Object ID", renderer,
            "text", COL_PL_TRACKID,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_visible(column, FALSE);

    // Track column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Track"), renderer,
            "text", COL_PL_TRACKNAME,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    //gtk_tree_view_column_set_sort_column_id(column, COL_TRACKNAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_visible(column, TRUE);

    // Track Duration
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Duration"), renderer,
            "text", COL_PL_TRACKDURATION,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeviewFiles), column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_visible(column, TRUE);
}

// ************************************************************************************************

/**
 * Set the state of the buttons within the playlist editor.
 * @param state
 */
void SetPlaylistButtonState(gboolean state) {
    gtk_widget_set_sensitive(GTK_WIDGET(button_Del_Playlist), state);
    gtk_widget_set_sensitive(GTK_WIDGET(button_Export_Playlist), state);
    gtk_widget_set_sensitive(GTK_WIDGET(button_File_Move_Up), state);
    gtk_widget_set_sensitive(GTK_WIDGET(button_File_Move_Down), state);
    gtk_widget_set_sensitive(GTK_WIDGET(button_Del_File), state);
    gtk_widget_set_sensitive(GTK_WIDGET(button_Add_Files), state);
    gtk_widget_set_sensitive(GTK_WIDGET(treeview_Avail_Files), state);
    gtk_widget_set_sensitive(GTK_WIDGET(treeview_Playlist_Files), state);
}

// ************************************************************************************************

/**
 * Setup the Playlist selection Combo Box in the playlist editor.
 */
void setPlayListComboBox(void) {
    LIBMTP_playlist_t* tmpplaylist = NULL;
    
    // We need to remove all entries in the combo box before starting.
    // This is a little bit of a hack - but does work.
    runPlaylistHandler = FALSE; // disable the handler from running on the control.
    // For some reason the "changed" event is triggered on a remove_all operation.
#if HAVE_GTK3 == 0
    //gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(comboboxentry_playlist))));
    while(comboboxentry_playlist_entries-- > 0){
        gtk_combo_box_remove_text(GTK_COMBO_BOX(comboboxentry_playlist), 0);
    }
#else
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(comboboxentry_playlist));
#endif
    comboboxentry_playlist_entries = 0;
    
    if (devicePlayLists != NULL) {
        // Populate the playlist dropdown box;
        //comboboxentry_playlist;
        tmpplaylist = devicePlayLists;
        while (tmpplaylist != NULL) {
            if (tmpplaylist->storage_id == DeviceMgr.devicestorage->id) {
#if HAVE_GTK3 == 0
                gtk_combo_box_append_text(GTK_COMBO_BOX(comboboxentry_playlist), g_strdup(tmpplaylist->name));
#else
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry_playlist), g_strdup(tmpplaylist->name));
#endif
                comboboxentry_playlist_entries++;
            }
            tmpplaylist = tmpplaylist->next;
        }
    }
    if (devicePlayLists != NULL) {
        // Set our playlist to the first one.
        gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxentry_playlist), 0);
        playlist_number = 0;
        // Now populate the playlist screen with it's details.
        setPlaylistField(0);
    } else {
        playlist_number = -1;
    }
    // If no playlists set parts of dialog to disabled.
    if (devicePlayLists == NULL) {
        SetPlaylistButtonState(FALSE);
    } else {
        SetPlaylistButtonState(TRUE);
    }
    runPlaylistHandler = TRUE;
}

// ************************************************************************************************

/**
 * Setup the list of tracks in the current selected playlist.
 * @param PlayListID
 */
void setPlaylistField(gint PlayListID) {
    // This function will populate the playlist_PL_List widget with the
    // details of the selected playlist.
    LIBMTP_playlist_t* tmpplaylist = devicePlayLists;
    gint tmpplaylistID = PlayListID;
    guint trackID = 0;
    GtkTreeIter rowIter;
    gchar * tmp_string = NULL;

    playlist_track_count = 0;

    gtk_list_store_clear(GTK_LIST_STORE(playlist_PL_List));

    if (PlayListID > 0) {
        while (tmpplaylistID--)
            if (tmpplaylist->next != NULL)
                tmpplaylist = tmpplaylist->next;
    }
    // tmpplaylist points to our playlist;
    for (trackID = 0; trackID < tmpplaylist->no_tracks; trackID++) {
        LIBMTP_track_t *trackinfo;
        trackinfo = LIBMTP_Get_Trackmetadata(DeviceMgr.device, tmpplaylist->tracks[trackID]);
        if (trackinfo != NULL) {
            playlist_track_count++;
            gtk_list_store_append(GTK_LIST_STORE(playlist_PL_List), &rowIter);
            tmp_string = g_strdup_printf("%d:%.2d", (int) ((trackinfo->duration / 1000) / 60), (int) ((trackinfo->duration / 1000) % 60));
            gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &rowIter, COL_PL_ORDER_NUM, playlist_track_count,
                    COL_PL_ARTIST, trackinfo->artist,
                    COL_PL_ALBUM, trackinfo->album, COL_PL_TRACKID, trackinfo->item_id,
                    COL_PL_TRACKNAME, trackinfo->title, COL_PL_TRACKDURATION, tmp_string, -1);
            g_free(tmp_string);
            tmp_string = NULL;

            LIBMTP_destroy_track_t(trackinfo);
        } else {
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
    }
}

// ************************************************************************************************

/**
 * Display the New Playlist Dialog box.
 * @return The name of the new playlist.
 */
gchar* displayPlaylistNewDialog(void) {
    GtkWidget *dialog, *hbox, *label, *textbox;
    gchar* textfield;

    dialog = gtk_dialog_new_with_buttons(_("New Playlist"), GTK_WINDOW(windowMain),
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

    label = gtk_label_new(_("Playlist Name:"));
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
 * Get the list of selected tracks in the playlist editor.
 * @return
 */
GList* playlist_PL_ListGetSelection() {
    GList *selectedFiles, *ptr;
    GtkTreeRowReference *ref;
    GtkTreeModel *model;
    // Lets clear up the old list.
    g_list_free(playlist_Selection_PL_RowReferences);
    playlist_Selection_PL_RowReferences = NULL;

    if (gtk_tree_selection_count_selected_rows(playlist_PL_Selection) == 0) {
        // We have no rows.
        return NULL;
    }
    // So now we must convert each selection to a row reference and store it in a new GList variable
    // which we will return below.
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_Playlist_Files));
    selectedFiles = gtk_tree_selection_get_selected_rows(playlist_PL_Selection, &model);
    ptr = selectedFiles;
    while (ptr != NULL) {
        ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist_PL_List), (GtkTreePath*) ptr->data);
        playlist_Selection_PL_RowReferences = g_list_append(playlist_Selection_PL_RowReferences, gtk_tree_row_reference_copy(ref));
        gtk_tree_row_reference_free(ref);
        ptr = ptr->next;
    }
    g_list_foreach(selectedFiles, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedFiles);
    return playlist_Selection_PL_RowReferences;
}

// ************************************************************************************************

/**
 * Clear the selection of tracks in the playlist.
 * @return
 */
gboolean playlist_PL_ListClearSelection() {
    if (playlist_PL_Selection != NULL)
        gtk_tree_selection_unselect_all(playlist_PL_Selection);
    return TRUE;
}

// ************************************************************************************************

/**
 * Remove the selected tracks from the playlist.
 * @param List
 * @return
 */
gboolean playlist_PL_ListRemove(GList *List) {
    GtkTreeIter iter;
    gint tracknumber = 1;

    playlist_PL_ListClearSelection();
    g_list_foreach(List, (GFunc) __playlist_PL_Remove, NULL);

    // Now reorder all tracks in this playlist.
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
        gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &iter, COL_PL_ORDER_NUM, tracknumber, -1);
        tracknumber++;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
            gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &iter, COL_PL_ORDER_NUM, tracknumber, -1);
            tracknumber++;
        }
    }
    return TRUE;
}

// ************************************************************************************************

/**
 * Remove the track from the current playlist.
 * @param Row
 */
void __playlist_PL_Remove(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_PL_List), &iter, path);
    // We have our Iter now.
    gtk_list_store_remove(GTK_LIST_STORE(playlist_PL_List), &iter);
    playlist_track_count--;
}

// ************************************************************************************************

/**
 * Get the selection of tracks in the current playlist.
 * @return
 */
GList* playlist_TrackList_GetSelection() {
    GList *selectedFiles, *ptr;
    GtkTreeRowReference *ref;
    GtkTreeModel *model;
    // Lets clear up the old list.
    g_list_free(playlist_Selection_TrackRowReferences);
    playlist_Selection_TrackRowReferences = NULL;

    if (gtk_tree_selection_count_selected_rows(playlist_TrackSelection) == 0) {
        // We have no rows.
        return NULL;
    }
    // So now we must convert each selection to a row reference and store it in a new GList variable
    // which we will return below.
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_Avail_Files));
    selectedFiles = gtk_tree_selection_get_selected_rows(playlist_TrackSelection, &model);
    ptr = selectedFiles;
    while (ptr != NULL) {
        ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist_TrackList), (GtkTreePath*) ptr->data);
        playlist_Selection_TrackRowReferences = g_list_append(playlist_Selection_TrackRowReferences, gtk_tree_row_reference_copy(ref));
        gtk_tree_row_reference_free(ref);
        ptr = ptr->next;
    }
    g_list_foreach(selectedFiles, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedFiles);
    return playlist_Selection_TrackRowReferences;
}

// ************************************************************************************************

/**
 * Add the list of tracks to the selected playlist.
 * @param List
 * @return
 */
gboolean playlist_TrackList_Add(GList *List) {
    g_list_foreach(List, (GFunc) __playlist_TrackList_Add, NULL);
    return TRUE;
}

// ************************************************************************************************

/**
 * Add the individual track to the playlist.
 * @param Row
 */
void __playlist_TrackList_Add(GtkTreeRowReference *Row) {
    GtkTreePath *path = NULL;
    GtkTreeIter iter;
    GtkTreeIter PL_rowIter;
    gchar* artist = NULL;
    gchar* album = NULL;
    gchar* title = NULL;
    gint item_id = 0;
    gchar * duration = NULL;

    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_TrackList), &iter, path);
    // We have our Iter now, so get the required information from the track treeview.
    gtk_tree_model_get(GTK_TREE_MODEL(playlist_TrackList), &iter, COL_ARTIST, &artist, COL_ALBUM, &album,
            COL_TRACKID, &item_id, COL_TRACKNAME, &title, COL_TRACKDURATION, & duration, -1);
    // Now store our information in the playlist treeview.
    playlist_track_count++;
    gtk_list_store_append(GTK_LIST_STORE(playlist_PL_List), &PL_rowIter);
    gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &PL_rowIter, COL_PL_ORDER_NUM, playlist_track_count, COL_PL_ARTIST, artist,
            COL_PL_ALBUM, album, COL_PL_TRACKID, item_id, COL_PL_TRACKNAME, title, COL_PL_TRACKDURATION, duration, -1);

    //Need to free our string values
    g_free(artist);
    g_free(album);
    g_free(title);
    g_free(duration);
}

// ************************************************************************************************

/**
 * Reorder the tracks within the playlist.
 * @param direction
 * @return
 */
gboolean playlist_move_files(gint direction) {
    GList * playlist_files = NULL;
    GtkTreeIter iter;
    gint tracknumber = 1;
    // Get our files...
    playlist_files = playlist_PL_ListGetSelection();
    if (playlist_files == NULL)
        return FALSE;

    // If we are moving files down we need to reverse the rows references...
    if (direction == 1) {
        playlist_files = g_list_reverse(playlist_files);
        g_list_foreach(playlist_files, (GFunc) __playlist_move_files_down, NULL);
    } else {
        g_list_foreach(playlist_files, (GFunc) __playlist_move_files_up, NULL);
    }
    // Now reorder all tracks in this playlist.
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
        gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &iter, COL_PL_ORDER_NUM, tracknumber, -1);
        tracknumber++;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
            gtk_list_store_set(GTK_LIST_STORE(playlist_PL_List), &iter, COL_PL_ORDER_NUM, tracknumber, -1);
            tracknumber++;
        }
    }
    return TRUE;
}

// ************************************************************************************************

/**
 * Move the selected track up in the playlist.
 * @param Row
 */
void __playlist_move_files_up(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeIter iter2;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_PL_List), &iter, path);
    // We have our Iter now.
    // Now get it's prev path and turn it into a iter
    if (gtk_tree_path_prev(path) == TRUE) {
        // we have a previous entry...
        gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_PL_List), &iter2, path);
        gtk_list_store_swap(GTK_LIST_STORE(playlist_PL_List), &iter, &iter2);
    }

}

// ************************************************************************************************

/**
 * Move the selected track down in the playlist.
 * @param Row
 */
void __playlist_move_files_down(GtkTreeRowReference *Row) {
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeIter iter2;
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(Row);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_PL_List), &iter, path);
    // We have our Iter now.
    iter2 = iter;
    if (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_PL_List), &iter2) == TRUE) {
        // we have something to swap with...
        gtk_list_store_swap(GTK_LIST_STORE(playlist_PL_List), &iter, &iter2);
    }
}

// ************************************************************************************************

/**
 * Save the current selected playlist to the device.
 * @param PlayListID
 */
void playlist_SavePlaylist(gint PlayListID) {
    LIBMTP_playlist_t* tmpplaylist = devicePlayLists;
    gint tmpplaylistID = PlayListID;
    gint item_id = 0;
    GtkTreeIter iter;
    uint32_t *tmp = NULL;

    if (PlayListID > 0) {
        while (tmpplaylistID--)
            if (tmpplaylist->next != NULL)
                tmpplaylist = tmpplaylist->next;
    }
    // tmpplaylist points to our playlist;
    // So all we need to do is - update our current structure with the new details

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playlist_PL_List), &iter, COL_PL_TRACKID, &item_id, -1);
        tmpplaylist->no_tracks = 1;

        // item_id = our track num... so append to tmpplaylist->tracks
        if ((tmp = g_realloc(tmpplaylist->tracks, sizeof (uint32_t) * (tmpplaylist->no_tracks))) == NULL) {
            g_fprintf(stderr, _("realloc in savePlayList failed\n"));
            displayError(_("Updating playlist failed? 'realloc in savePlayList'\n"));
            return;
        }
        tmpplaylist->tracks = tmp;
        tmpplaylist->tracks[(tmpplaylist->no_tracks - 1)] = item_id;
        //tmpplaylist->no_tracks++;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_PL_List), &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playlist_PL_List), &iter, COL_PL_TRACKID, &item_id, -1);
            tmpplaylist->no_tracks++;
            // item_id = our track num... so append to tmpplaylist->tracks
            if ((tmp = g_realloc(tmpplaylist->tracks, sizeof (uint32_t) * (tmpplaylist->no_tracks))) == NULL) {
                g_fprintf(stderr, _("realloc in savePlayList failed\n"));
                displayError(_("Updating playlist failed? 'realloc in savePlayList'\n"));
                return;
            }
            tmpplaylist->tracks = tmp;
            tmpplaylist->tracks[(tmpplaylist->no_tracks - 1)] = item_id;
            //tmpplaylist->no_tracks++;

        }
    }
    // get libmtp to save it.
    playlistUpdate(tmpplaylist);
    // Update our own metadata.
    devicePlayLists = getPlaylists();
}


// Playlist Callbacks.
// ************************************************************************************************

/**
 * Callback to handle closing the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_quitPlaylist_activate(GtkMenuItem *menuitem, gpointer user_data) {
    // Save our current selected playlist!
    if (devicePlayLists != NULL)
        playlist_SavePlaylist(playlist_number);
    // Kill our window
    gtk_widget_hide(windowPlaylistDialog);
    gtk_widget_destroy(windowPlaylistDialog);
    windowPlaylistDialog = NULL;
    // Do a device rescan to show the new playlists in the file window
    deviceRescan();
} // end on_quitPlaylist_activate()

// ************************************************************************************************

/**
 * Callback to handle the new Playlist button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_NewPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    //g_printf("Clicked on new playlist button\n");
    gchar *playlistname = NULL;

    // Save our current selected playlist!
    if (devicePlayLists != NULL)
        playlist_SavePlaylist(playlist_number);

    playlistname = displayPlaylistNewDialog();
    if (playlistname != NULL) {
        // Add in playlist to MTP device.
        playlistAdd(playlistname);
        // Refresh our playlist information.
        devicePlayLists = getPlaylists();
        gtk_list_store_clear(GTK_LIST_STORE(playlist_PL_List));
        // Add it to our combobox

#if HAVE_GTK3 == 0
        gtk_combo_box_append_text(GTK_COMBO_BOX(comboboxentry_playlist), g_strdup(playlistname));
#else
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry_playlist), g_strdup(playlistname));
#endif
        g_free(playlistname);

        // Set the active combobox item.
        comboboxentry_playlist_entries++;
        playlist_number = comboboxentry_playlist_entries - 1;
        gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxentry_playlist), comboboxentry_playlist_entries - 1);
        SetPlaylistButtonState(TRUE);
        setPlaylistField(playlist_number);
    }
} // end on_Playlist_NewPlaylistButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Import Playlist button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_ImportPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    //g_printf("Clicked on new playlist button\n");
    gchar *playlistfilename = NULL;
    gchar *playlistname = NULL;
    GtkWidget *FileDialog;
    GtkFileFilter *OpenFormFilter, *OpenFormFilter2;

    // Save our current selected playlist!
    if (devicePlayLists != NULL)
        playlist_SavePlaylist(playlist_number);

    // Get our filename...

    FileDialog = gtk_file_chooser_dialog_new(_("Select Playlist to Import"),
            GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_OPEN,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Open"), GTK_RESPONSE_ACCEPT,
            NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(FileDialog), FALSE);
    OpenFormFilter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(OpenFormFilter, "*.m3u");
    gtk_file_filter_set_name(OpenFormFilter, "m3u Playlists");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(FileDialog), OpenFormFilter);
    OpenFormFilter2 = gtk_file_filter_new();
    gtk_file_filter_add_pattern(OpenFormFilter2, "*");
    gtk_file_filter_set_name(OpenFormFilter2, "All Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(FileDialog), OpenFormFilter2);

    if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
        playlistfilename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
    }

    gtk_widget_hide(FileDialog);
    gtk_widget_destroy(FileDialog);

    if (playlistfilename != NULL) {
        // Add in playlist to MTP device.
        playlistname = playlistImport(playlistfilename);

        // If our name is NULL, then the import failed...
        if (playlistname != NULL) {
            // Refresh our playlist information.
            devicePlayLists = getPlaylists();
            gtk_list_store_clear(GTK_LIST_STORE(playlist_PL_List));
            // Add it to our combobox

#if HAVE_GTK3 == 0
            gtk_combo_box_append_text(GTK_COMBO_BOX(comboboxentry_playlist), g_strdup(playlistname));
#else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry_playlist), g_strdup(playlistname));
#endif

            // Set the active combobox item.
            comboboxentry_playlist_entries++;
            playlist_number = comboboxentry_playlist_entries - 1;
            gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxentry_playlist), comboboxentry_playlist_entries - 1);
            SetPlaylistButtonState(TRUE);
            setPlaylistField(playlist_number);

            // Clean up fields.
            g_free(playlistname);
        } else {
            // Let the user know the import failed.
            g_fprintf(stderr, _("The playlist failed to import correctly.\n"));
            displayError(_("The playlist failed to import correctly.\n"));
        }
        // Clean up fields.
        g_free(playlistfilename);
    }
} // end on_Playlist_ImportPlaylistButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Export Playlist button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_ExportPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *playlistfilename = NULL;
    GtkWidget *FileDialog;

    // Save our current selected playlist!
    if (devicePlayLists != NULL)
        playlist_SavePlaylist(playlist_number);

    gint PlayListID = gtk_combo_box_get_active(GTK_COMBO_BOX(comboboxentry_playlist));

    if (PlayListID != -1) {
        // We have something selected so lets do the dance.
        LIBMTP_playlist_t* tmpplaylist = devicePlayLists;
        if (PlayListID > 0) {
            while (PlayListID--)
                if (tmpplaylist->next != NULL)
                    tmpplaylist = tmpplaylist->next;
        }
        // We should be in the correct playlist LIBMTP structure.

        playlistfilename = g_strdup_printf("%s.%s", tmpplaylist->name, "m3u");

        FileDialog = gtk_file_chooser_dialog_new(_("Save as..."),
                GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_SAVE,
                _("_Cancel"), GTK_RESPONSE_CANCEL,
                _("_Open"), GTK_RESPONSE_ACCEPT,
                NULL);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(FileDialog), playlistfilename);

        if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
            playlistfilename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
        }

        gtk_widget_hide(FileDialog);
        gtk_widget_destroy(FileDialog);

        if (playlistfilename != NULL) {
            playlistExport(playlistfilename, tmpplaylist);
            g_free(playlistfilename);
        }
    }
} // end on_Playlist_ExportPlaylistButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Delete Playlist button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_DelPlaylistButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gint PlayListID = gtk_combo_box_get_active(GTK_COMBO_BOX(comboboxentry_playlist));
    if (PlayListID != -1) {
        // We have something selected so lets do the dance.
        LIBMTP_playlist_t* tmpplaylist = devicePlayLists;
        if (PlayListID > 0) {
            while (PlayListID--)
                if (tmpplaylist->next != NULL)
                    tmpplaylist = tmpplaylist->next;
        }
        // We should be in the correct playlist LIBMTP structure.
        playlistDelete(tmpplaylist);
        // Clear the PL list view box
        gtk_list_store_clear(GTK_LIST_STORE(playlist_PL_List));
        // Rebuild the playlist structure and combobox.
        devicePlayLists = getPlaylists();
        setPlayListComboBox();
    }
} // end on_Playlist_DelPlaylistButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Delete Track button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_DelFileButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    if (playlist_PL_ListGetSelection() == NULL)
        return;
    playlist_PL_ListRemove(playlist_PL_ListGetSelection());
} // end on_Playlist_DelFileButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Add Track button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_AddFileButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    //g_printf("Clicked on add file in playlist button\n");
    if (playlist_TrackList_GetSelection() == NULL)
        return;
    playlist_TrackList_Add(playlist_TrackList_GetSelection());
} // end on_Playlist_AddFileButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Move Track Up button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_FileUpButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    playlist_move_files(-1);
} // end on_Playlist_FileUpButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the Move Track Down button in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_FileDownButton_activate(GtkMenuItem *menuitem, gpointer user_data) {
    playlist_move_files(1);
} // end on_Playlist_FileDownButton_activate()

// ************************************************************************************************

/**
 * Callback to handle the change of Playlist selection in the Playlist editor dialog.
 * @param menuitem
 * @param user_data
 */
void on_Playlist_Combobox_activate(GtkComboBox *combobox, gpointer user_data) {
    if(runPlaylistHandler == TRUE){
        // Save our current selected playlist
        playlist_SavePlaylist(playlist_number);
        // Get our new playlist ID, and display the contents of it.
        playlist_number = gtk_combo_box_get_active(GTK_COMBO_BOX(comboboxentry_playlist));
        setPlaylistField(playlist_number);
    }
} // end on_Playlist_Combobox_activate()

