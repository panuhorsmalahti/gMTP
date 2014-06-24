/* 
 *
 *   File: dnd.c
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
/*
        This file contains all the Drag and Drop Functionality for gMTP
 */

#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#endif
#include <sys/types.h>
#include <libgen.h>
#include <string.h>
#include <id3tag.h>
#include <libmtp.h>

#include "main.h"
#include "interface.h"
#include "callbacks.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "progress.h"

GtkTargetEntry _gmtp_drop_types[] = {
    {"text/plain", 0, GMTP_DROP_PLAINTEXT},
    {"text/uri-list", 0, GMTP_DROP_URLENCODED},
    {"STRING", 0, GMTP_DROP_STRING}
};

// ************************************************************************************************

/**
 * Callback to handle the initial drop of data into gmtp.
 * @param widget
 * @param context
 * @param x
 * @param y
 * @param selection_data
 * @param info
 * @param time
 * @param user_data
 */
void gmtp_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
        GtkSelectionData *selection_data, guint info, guint time, gpointer user_data) {
#if HAVE_GTK3 == 0
    if (selection_data->data)
#else
    if (gtk_selection_data_get_data(selection_data))
#endif
    {
        GSList* files;
        displayProgressBar(_("File Upload"));
#if HAVE_GTK3 == 0
        files = getFilesListURI((gchar *) selection_data->data);
#else
        files = getFilesListURI((gchar *) gtk_selection_data_get_data(selection_data));
#endif
        // Set the Playlist ID to be asked if needed.
        if (Preferences.auto_add_track_to_playlist == TRUE) {
            addTrackPlaylistID = GMTP_REQUIRE_PLAYLIST;
        } else {
            addTrackPlaylistID = GMTP_NO_PLAYLIST;
        }
        AlbumErrorIgnore = FALSE;
        // Add the files.
        if (files != NULL) {
            g_slist_foreach(files, (GFunc) __filesAdd, NULL);
        }
        destroyProgressBar();
        // Now clear the GList;
        g_slist_foreach(files, (GFunc) g_free, NULL);
        g_slist_free(files);
        // Now do a device rescan to see the new files.
        deviceRescan();
        deviceoverwriteop = MTP_ASK;
    }
} // gmtp_drag_data_received()


// ************************************************************************************************

/**
 * Callback to handle the initial drop of data into gmtp.
 * @param widget
 * @param context
 * @param x
 * @param y
 * @param selection_data
 * @param info
 * @param time
 * @param user_data
 */

void gmtpfolders_drag_data_received(GtkWidget * widget, GdkDragContext * context, gint x, gint y,
        GtkSelectionData * selection_data, guint info, guint time, gpointer user_data) {

    //uint32_t mainFolderID = 0;
    int64_t targetFolderID = 0;

    g_signal_stop_emission_by_name((gpointer) treeviewFolders, "drag-data-received");

#if HAVE_GTK3 == 0
    if (selection_data->data)
#else
    if (gtk_selection_data_get_data(selection_data))
#endif
    {

        //mainFolderID = currentFolderID;

        // Get our target folder ID...
        targetFolderID = folderListGetSelection();
        if (targetFolderID != -1) {
            currentFolderID = targetFolderID;

            GSList* files;
            displayProgressBar(_("File Upload"));
#if HAVE_GTK3 == 0
            files = getFilesListURI((gchar *) selection_data->data);
#else
            files = getFilesListURI((gchar *) gtk_selection_data_get_data(selection_data));
#endif
            // Set the Playlist ID to be asked if needed.
            if (Preferences.auto_add_track_to_playlist == TRUE) {
                addTrackPlaylistID = GMTP_REQUIRE_PLAYLIST;
            } else {
                addTrackPlaylistID = GMTP_NO_PLAYLIST;
            }
            AlbumErrorIgnore = FALSE;
            // Add the files.
            if (files != NULL) {
                //displayProgressBar(_("File Upload"));
                g_slist_foreach(files, (GFunc) __filesAdd, NULL);
                //destroyProgressBar();
            }
            destroyProgressBar();
            // Now clear the GList;
            g_slist_foreach(files, (GFunc) g_free, NULL);
            g_slist_free(files);
            // Now do a device rescan to see the new files.

            // Restore our current Folder ID.
            //currentFolderID = mainFolderID;
        }

        deviceRescan();
        deviceoverwriteop = MTP_ASK;
    }
}


// ************************************************************************************************

/**
 * Handle drag motion across the folder widget.
 * @param widget
 * @param context
 * @param x
 * @param y
 * @param time
 */
void gmtpfolders_drag_motion_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time) {
    GtkTreePath *path;
    GtkTreeViewDropPosition pos;

    if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &pos)) {

        // Highlight the current row...
        gtk_tree_selection_select_path(folderSelection, path);

    }
}

// ************************************************************************************************

/**
 * Take the raw list of filenames as given via a Drop operation, and return a nicely formatted
 * list of filenames.
 * @param rawdata
 * @return GList of all individual filenames.
 */
GSList* getFilesListURI(gchar* rawdata) {
    // The data is just the data in string form
    // Files are in the URI form of file:///filename\n so just look for those,
    // and if found see if a folder or not?
    // Then create a slist of those and use filesAdd() to add them in .
    GSList* filelist;
    gchar* tmpstring;
    gchar *fullpath;
    gchar *filepath;
    gchar *token;

    const char delimit[] = "\n\r";

    filelist = NULL;

    fullpath = g_strdup(rawdata);

    token = strtok(fullpath, delimit);
    while ((token != NULL)) {
        // Now test to see if we have it here...
        filepath = g_strdup(token);
        // See if we have a local file URI, otherwise discard.
        if (!g_ascii_strncasecmp(filepath, "file://", 7)) {
            tmpstring = g_filename_from_uri(filepath, NULL, NULL);

            // See if we have a file or a folder?
            if (g_file_test(tmpstring, G_FILE_TEST_IS_REGULAR) == TRUE) {
                // Add the file to the list
                filelist = g_slist_append(filelist, g_strdup(tmpstring));
            } else {
                // Otherwise we have a folder, so add the folder and all it's contents.
                addFilesinFolder(tmpstring);
            }
            g_free(tmpstring);
        }
        token = strtok(NULL, delimit);
        g_free(filepath);
    }
    g_free(fullpath);
    return filelist;
} // end getFilesListURI()

// ************************************************************************************************

/**
 * Process a folder looking for all files in that folder, and upload all those files.
 * @param foldername
 */
void addFilesinFolder(gchar* foldername) {
    // foldername is the name of the folder as the absolute path with leading /
    // We save the currentFolderID, create a new folder on the device,
    // and set currentFolderID to the new folders ID.
    // Then scan the folder (on the filesystem) adding in files as needed.
    // Found folders are always added first, so files are copied from
    // the deepest level of the folder hierarchy first as well, and we
    // work our way back down towards to the initial folder that was
    // dragged in.
    // Lastly we restore the currentFolderID back to what it was.
    GDir *fileImageDir;
    GSList* filelist;
    const gchar *filename;
    gchar* relative_foldername;
    gchar *tmpstring;
    uint32_t oldFolderID;

    filelist = NULL;
    // Save our current working folder.
    oldFolderID = currentFolderID;
    // Get just the folder name, as we are given a full absolute path.
    relative_foldername = basename(foldername);
    if (relative_foldername != NULL) {

        // Determine if the folder already exists in the current location, otherwise create it.

        if(folderExists(relative_foldername, currentFolderID)){
            currentFolderID = getFolder(relative_foldername, currentFolderID);        
        } else {
            // Add our folder to the mtp device and set our new current working folder ID.
            currentFolderID = folderAdd(relative_foldername);
        }    
    }

    // Start scanning the folder on the filesystem for our new files/folders.
    fileImageDir = g_dir_open(foldername, 0, NULL);
    // Now parse that directory looking for JPEG/PNG files (based on settings).
    // If we find one, we create a new GString and add it to the list.
    if (fileImageDir != NULL) {
        filename = g_dir_read_name(fileImageDir);
        while (filename != NULL) {
            // See if a file or a folder?
            tmpstring = g_strconcat(foldername, "/", filename, NULL);
            if (g_file_test(tmpstring, G_FILE_TEST_IS_REGULAR) == TRUE) {
                // We have a regular file. So add it to the list.
                filelist = g_slist_append(filelist, g_strdup(tmpstring));
            } else {
                if (g_file_test(tmpstring, G_FILE_TEST_IS_DIR) == TRUE) {
                    // We have another folder so recursively call ourselves...
                    addFilesinFolder(tmpstring);
                }
            }
            filename = g_dir_read_name(fileImageDir);
            g_free(tmpstring);
        }
    }
    // Set the Playlist ID to be asked if needed.
    if (Preferences.auto_add_track_to_playlist == TRUE) {
        addTrackPlaylistID = GMTP_REQUIRE_PLAYLIST;
    } else {
        addTrackPlaylistID = GMTP_NO_PLAYLIST;
    }
    AlbumErrorIgnore = FALSE;
    // Upload our given files in the current selected folder.
    if (filelist != NULL) {
        g_slist_foreach(filelist, (GFunc) __filesAdd, NULL);
    }
    // Now clear the GList;
    g_slist_foreach(filelist, (GFunc) g_free, NULL);
    g_slist_free(filelist);

    if (fileImageDir != NULL)
        g_dir_close(fileImageDir);
    // Restore our current working folder.
    currentFolderID = oldFolderID;
} // end addFilesinFolder()
