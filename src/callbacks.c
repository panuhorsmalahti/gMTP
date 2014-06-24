/* 
 *
 *   File: callbacks.c
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
#include "formatdevice.h"

/**
 * on_quit1_activate - Call back for Quit toolbar and menu option.
 * @param menuitem
 * @param user_data
 */
void on_quit1_activate(GtkMenuItem *menuitem, gpointer user_data) {
    // Disconnect device.
    if (DeviceMgr.deviceConnected == TRUE) {
        on_deviceConnect_activate(NULL, NULL);
    }
    savePreferences();
#if HAVE_GTK3 == 0
    gtk_exit(EXIT_SUCCESS);
#else
    exit(EXIT_SUCCESS);
#endif
} // end on_quit1_activate()

// ************************************************************************************************

/**
 * on_about1_activate - Call back for displaying the About Dialog Box
 * @param menuitem
 * @param user_data
 */
void on_about1_activate(GtkMenuItem *menuitem, gpointer user_data) {
    displayAbout();
} // end on_about1_activate()



// ************************************************************************************************

/**
 * on_deviceRescan_activate - Callback to rescan the device properties and update the main
 * application window.
 * @param menuitem
 * @param user_data
 */
void on_deviceRescan_activate(GtkMenuItem *menuitem, gpointer user_data) {
    deviceRescan();
} // end on_deviceRescan_activate()

// ************************************************************************************************

/**
 * on_filesAdd_activate - Callback to initiate an Add Files operation.
 * @param menuitem
 * @param user_data
 */
void on_filesAdd_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GSList* files;
    int64_t targetFol = 0;
    //uint32_t tmpFolderID = 0;
    // Set the Playlist ID to be asked if needed.
    if (Preferences.auto_add_track_to_playlist == TRUE) {
        addTrackPlaylistID = GMTP_REQUIRE_PLAYLIST;
    } else {
        addTrackPlaylistID = GMTP_NO_PLAYLIST;
    }
    // Get the files, and add them.
    files = getFileGetList2Add();

    // See if a folder is selected in the folder view, and if so add the files to that folder.
    if ((targetFol = folderListGetSelection()) != -1) {
        //tmpFolderID = currentFolderID;
        currentFolderID = (uint32_t) targetFol;
    }
    AlbumErrorIgnore = FALSE;
    if (files != NULL){
	displayProgressBar(_("File Upload"));
        g_slist_foreach(files, (GFunc) __filesAdd, NULL);
	destroyProgressBar();
    }

    // Now clear the GList;
    g_slist_foreach(files, (GFunc) g_free, NULL);
    g_slist_free(files);

    // Restore the current folder ID is we added to another folder.
    if (targetFol != -1) {
        // Disable this, so the user is taken to the folder in which the files were added to.
        //currentFolderID = tmpFolderID;
    }
    // Now do a device rescan to see the new files.
    deviceRescan();
    deviceoverwriteop = MTP_ASK;
} // end on_filesAdd_activate()

// ************************************************************************************************

/**
 * Callback to handle the Rename Device menu option.
 * @param menuitem
 * @param user_data
 */
void on_fileRenameFile_activate(GtkMenuItem *menuitem, gpointer user_data) {

    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *newfilename = NULL;
    gchar *filename = NULL;
    gboolean isFolder;
    uint32_t ObjectID = 0;

    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {

        // See if anything is selected in the folder view, if so use that as our source.
        if (folderListGetSelection() != -1) {
            on_folderRenameFolder_activate(menuitem, user_data);
        } else {
            displayInformation(_("No files/folders selected?"));
        }
        return;
    }
    GList *List = fileListGetSelection();

    // We only care about the first entry.
    // convert the referenece to a path and retrieve the iterator;
    path = gtk_tree_row_reference_get_path(List->data);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(fileList), &iter, path);
    // We have our Iter now.
    // Before we download, is it a folder ?
    gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_FILENAME_ACTUAL, &filename, COL_ISFOLDER, &isFolder,
            COL_FILEID, &ObjectID, -1);

    // Make sure we are not attempting to edit the parent link folder.
    if (g_ascii_strcasecmp(filename, "..") == 0) {
        g_fprintf(stderr, _("Unable to rename parent folder\n"));
        displayInformation(_("Unable to rename this folder"));
        return;
    }
    // Get our new device name.
    newfilename = displayRenameFileDialog(filename);

    // If the user supplied something, then update the name of the device.
    if (newfilename != NULL) {
        filesRename(newfilename, ObjectID);
        g_free(newfilename);
        deviceRescan();
    }
} // end on_editDeviceName_activate()

// ************************************************************************************************

/**
 * Callback to handle the Move File menu option.
 * @param menuitem
 * @param user_data
 */
void on_fileMoveFile_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GList *List = NULL;
    int64_t targetfolder = 0;

    // If using alternate connection mode, this is disabled.
    if (Preferences.use_alt_access_method) {
        displayInformation(_("The move function is disabled when using the alternate access method for your device."));
        return;
    }

    // Let's check to see if we have anything selected in our treeview?
    if ((List = fileListGetSelection()) == NULL) {
        if (folderListGetSelection() != -1) {
            on_folderRemoveFolder_activate(menuitem, user_data);
        } else {
            displayInformation(_("No files/folders selected?"));
        }
        return;
    }

    // Prompt for the target folder location.
    targetfolder = getTargetFolderLocation();
    if ((targetfolder == -1) || (targetfolder == currentFolderID)) {
        // If the user didn't select a folder, or the target folder is the current selected folder
        // then do nothing.
        return;
    }
    fileMoveTargetFolder = targetfolder;
    fileListClearSelection();
    // List is a list of Iter's to be moved
    g_list_foreach(List, (GFunc) __fileMove, NULL);
    // We have 2 options, manually scan the file structure for that file and manually fix up...
    // or do a rescan...
    // I'll be cheap, and do a full rescan of the device.
    deviceRescan();
}


// ************************************************************************************************

/**
 * on_filesDelete_activate - Callback to initiate a Delete Files operation.
 * @param menuitem
 * @param user_data
 */
void on_filesDelete_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dialog;

    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {
        if (folderListGetSelection() != -1) {
            on_folderRemoveFolder_activate(menuitem, user_data);
        } else {
            displayInformation(_("No files/folders selected?"));
        }
        return;
    }

    // Now we prompt to confirm delete?
    if (Preferences.confirm_file_delete_op == FALSE) {
        // Now download the actual file from the MTP device.
        fileListRemove(fileListGetSelection());
    } else {
        dialog = gtk_message_dialog_new(GTK_WINDOW(windowMain),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_WARNING,
                GTK_BUTTONS_YES_NO,
                _("Are you sure you want to delete these files?"));
        gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm Delete"));

        // Run the Dialog and get our result.
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        if (result == GTK_RESPONSE_YES)
            fileListRemove(fileListGetSelection());

        // Destroy the dialog box.
        gtk_widget_destroy(dialog);
    }
} // on_filesDelete_activate()

// ************************************************************************************************

/**
 * on_filesDownload_activate - Callback to initiate a download files operation.
 * @param menuitem
 * @param user_data
 */
void on_filesDownload_activate(GtkMenuItem *menuitem, gpointer user_data) {
    int64_t targetfolder = 0;
    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {
        if ((targetfolder = folderListGetSelection()) != -1) {
            displayProgressBar(_("File download"));
            folderListDownload(folderListGetSelectionName(), targetfolder);
            destroyProgressBar();
        } else {
            displayInformation(_("No files/folders selected?"));
        }
        return;
    }

    // Download the selected files.
    displayProgressBar(_("File download"));
    fileListDownload(fileListGetSelection());
    destroyProgressBar();
} // end on_filesDownload_activate()

// ************************************************************************************************

/**
 * on_deviceConnect_activate - Callback used to connect a device to the application.
 * @param menuitem
 * @param user_data
 */
void on_deviceConnect_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *tmp_string;
    GtkWidget *menuText;

    deviceConnect();
    //g_printf("Device connect/disconnect code = %d\n", result);
    // Update our label to indicate current condition.
    if (DeviceMgr.deviceConnected == TRUE) {
        // Set up our properties.
        deviceProperties();
        deviceRescan();

        // Update the toolbar to show a disconnect string.
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbuttonConnect), _("Disconnect"));

        // Now update the status bar;
        if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
            tmp_string = g_strdup_printf(_("Connected to %s - %d MB free"), DeviceMgr.devicename->str,
                    (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
        } else {
            if (DeviceMgr.devicestorage->StorageDescription != NULL) {
                tmp_string = g_strdup_printf(_("Connected to %s (%s) - %d MB free"),
                        DeviceMgr.devicename->str,
                        DeviceMgr.devicestorage->StorageDescription,
                        (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
            } else {
                tmp_string = g_strdup_printf(_("Connected to %s - %d MB free"),
                        DeviceMgr.devicename->str,
                        (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
            }
        }
        statusBarSet(tmp_string);
        g_free(tmp_string);

        // Now update the filemenu;
        menuText = gtk_bin_get_child(GTK_BIN(fileConnect));
        gtk_label_set_text(GTK_LABEL(menuText), _("Disconnect Device"));

        // Enable the Drag'n'Drop interface for the main window and folder window.
        gmtp_drag_dest_set(scrolledwindowMain);
        gmtp_drag_dest_set(treeviewFolders);

    } else {

        // Update the toolbar to show the Connect String.
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbuttonConnect), _("Connect"));

        // Now update the status bar;
        statusBarSet(_("No device attached"));

        // Now update the filemenu;
        menuText = gtk_bin_get_child(GTK_BIN(fileConnect));
        gtk_label_set_text(GTK_LABEL(menuText), _("Connect Device"));

        // Now update the file list area and disable Drag'n'Drop.
        fileListClear();
        folderListClear();
        gtk_drag_dest_unset(scrolledwindowMain);
        gtk_drag_dest_unset(treeviewFolders);
        setWindowTitle(NULL);
        // Hide the find toolbar if open and force search mode to false.
        gtk_widget_hide(findToolbar);
        gtk_widget_set_sensitive(GTK_WIDGET(cfileAdd), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(cfileNewFolder), TRUE);
        inFindMode = FALSE;
    }

    // Update the Toolbar and Menus enabling/disabling the menu items.
    SetToolbarButtonState(DeviceMgr.deviceConnected);
} // on_deviceConnect_activate()

// ************************************************************************************************

/**
 * Callback to handle double click on item in main window. If it's a folder, then change to it,
 * other attempt to download the file(s).
 * @param treeview
 * @param path
 * @param column
 * @param data
 */
void fileListRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data) {
    GtkTreeModel *model;
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;

    gchar *filename = NULL;
    gboolean isFolder;
    uint32_t objectID;

    GtkWidget *FileDialog;
    gchar *savepath = NULL;

    // Obtain the iter, and the related objectID.
    sortmodel = gtk_tree_view_get_model(treeview);
    model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sortmodel));
    if (gtk_tree_model_get_iter(model, &iter, gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(sortmodel), path))) {
        gtk_tree_model_get(GTK_TREE_MODEL(fileList), &iter, COL_ISFOLDER, &isFolder, COL_FILENAME_ACTUAL, &filename, COL_FILEID, &objectID, -1);
        if (isFolder == FALSE) {
            // Now download the actual file from the MTP device.
            displayProgressBar(_("File download"));
            savepath = g_malloc0(8192);
            // Let's confirm our download path.
            if (Preferences.ask_download_path == TRUE) {
                FileDialog = gtk_file_chooser_dialog_new(_("Select Path to Download"),
                        GTK_WINDOW(windowMain), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                        _("_Open"), GTK_RESPONSE_ACCEPT,
                        NULL);

                gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(FileDialog), Preferences.fileSystemDownloadPath->str);
                if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
                    savepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
                    // Save our download path.
                    Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, savepath);
                    // We do the deed.
                    displayProgressBar(_("File download"));
                    filesDownload(filename, objectID);
                    destroyProgressBar();
                }
                gtk_widget_destroy(FileDialog);
            } else {
                // We do the deed.
                displayProgressBar(_("File download"));
                filesDownload(filename, objectID);
                destroyProgressBar();
            }
            destroyProgressBar();
            g_free(savepath);

        } else {
            // Maintain the stack of folder IDs and names for alt access mode.
            if (Preferences.use_alt_access_method) {
                if (g_ascii_strcasecmp(filename, "..") == 0) {
                    // going down a level.
                    g_free(g_queue_pop_tail(stackFolderIDs));
                    g_free(g_queue_pop_tail(stackFolderNames));
                } else {
                    // going up a level
                    guint *currentFld = g_malloc(sizeof (guint));
                    *currentFld = currentFolderID;
                    g_queue_push_tail(stackFolderIDs, currentFld);
                    g_queue_push_tail(stackFolderNames, g_strdup(filename));
                }
            }

            // We have a folder so change to it?
            currentFolderID = objectID;
            on_editFindClose_activate(NULL, NULL);
        }
    }
    g_free(filename);
} // end fileListRowActivated()


// ************************************************************************************************

/**
 * Callback to handle double click on item in folder main window.
 * @param treeview
 * @param path
 * @param column
 * @param data
 */
void folderListRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data) {
    GtkTreeModel *model;
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;

    uint32_t objectID;

    // Obtain the iter, and the related objectID.
    sortmodel = gtk_tree_view_get_model(treeview);
    model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sortmodel));
    if (gtk_tree_model_get_iter(model, &iter, gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(sortmodel), path))) {
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, COL_FOL_ID, &objectID, -1);
        // We have a folder so change to it?
        currentFolderID = objectID;
        on_editFindClose_activate(NULL, NULL);
    }
} // end folderListRowActivated()


// ************************************************************************************************

/**
 * Callback to handle selecting NewFolder from menu or toolbar.
 * @param menuitem
 * @param user_data
 */
void on_fileNewFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *foldername = NULL;
    if (folderListGetSelection() != -1) {
        on_folderNewFolder_activate(menuitem, user_data);
        return;
    }
    // Get the folder name by displaying a dialog.
    foldername = displayFolderNewDialog();
    if (foldername != NULL) {
        // Add in folder to MTP device.
        folderAdd(foldername);
        g_free(foldername);
        deviceRescan();
    }
} // end on_fileNewFolder_activate()


// ************************************************************************************************

/**
 * Callback to handle selecting NewFolder from menu or toolbar.
 * @param menuitem
 * @param user_data
 */
void on_folderNewFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *foldername = NULL;
    uint32_t tmpFolderID = 0;
    // Get the folder name by displaying a dialog.
    foldername = displayFolderNewDialog();
    if (foldername != NULL) {
        // Let's see if we have anything selected in the folder view, and if not, then we add the
        // folder to the current Folder.
        if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
            // Add in folder to MTP device.
            folderAdd(foldername);
        } else {
            // We have selected a folder in the folder view, so let's get it's ID.
            tmpFolderID = currentFolderID;
            currentFolderID = folderListGetSelection();
            folderAdd(foldername);
            currentFolderID = tmpFolderID;
        }
        g_free(foldername);
        deviceRescan();
    }
} // end on_folderNewFolder_activate()


// ************************************************************************************************

/**
 * Callback to handle selecting RemoveFolder from menu or toolbar.
 * @param menuitem
 * @param user_data
 */
void on_folderRemoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dialog;
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;

    uint32_t objectID;

    // Let's see if we have anything selected in the folder view, and if not let the user know, and return
    if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
        // Add in folder to MTP device.
        displayInformation(_("No files/folders selected?"));
        return;
    } else {
        // We have selected a folder in the folder view, so let's get it's ID.
        sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFiles));
        gtk_tree_selection_get_selected(folderSelection, &sortmodel, &iter);
        gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
        gtk_tree_model_get(GTK_TREE_MODEL(folderList), &childiter, COL_FOL_ID, &objectID, -1);

        // Now we prompt to confirm delete?
        if (Preferences.confirm_file_delete_op == FALSE) {
            // Now download the actual file from the MTP device.
            folderDelete(getCurrentFolderPtr(deviceFolders, objectID), 0);
        } else {
            dialog = gtk_message_dialog_new(GTK_WINDOW(windowMain),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_WARNING,
                    GTK_BUTTONS_YES_NO,
                    _("Are you sure you want to delete this folder (and all contents)?"));
            gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm Delete"));
            gint result = gtk_dialog_run(GTK_DIALOG(dialog));
            if (result == GTK_RESPONSE_YES)
                folderDelete(getCurrentFolderPtr(deviceFolders, objectID), 0);
            gtk_widget_destroy(dialog);
        }

        //folderDelete(getCurrentFolderPtr(deviceFolders, objectID), 0);
    }
    deviceRescan();
} // end on_folderRemoveFolder_activate()


// ************************************************************************************************

/**
 * Callback to handle selecting MoveFolder from context menu.
 * @param menuitem
 * @param user_data
 */
void on_folderMoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;
    int64_t targetfolder = 0;
    uint32_t objectID;

    LIBMTP_folder_t *currentFolder = NULL;
    LIBMTP_folder_t *newFolder = NULL;
    int error;

    // Let's see if we have anything selected in the folder view, and if not let the user know, and return
    if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
        // Add in folder to MTP device.
        displayInformation(_("No files/folders selected?"));
        return;
    } else {
        // We have selected a folder in the folder view, so let's get it's ID.
        sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFiles));
        gtk_tree_selection_get_selected(folderSelection, &sortmodel, &iter);
        gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
        gtk_tree_model_get(GTK_TREE_MODEL(folderList), &childiter, COL_FOL_ID, &objectID, -1);

        // Prompt for the target folder location.
        targetfolder = getTargetFolderLocation();
        if (targetfolder == -1) {
            // If the user didn't select a folder, or the target folder is the current selected folder
            // then do nothing.
            return;
        }
        fileMoveTargetFolder = targetfolder;
        gtk_tree_selection_unselect_all(folderSelection);

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
    deviceRescan();
} // end on_folderMoveFolder_activate()


// ************************************************************************************************

/**
 * Callback to handle selecting Rename Folder from menu
 * @param menuitem
 * @param user_data
 */
void on_folderRenameFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *newfilename = NULL;
    gchar *filename = NULL;
    GtkTreeModel *sortmodel;
    GtkTreeIter iter;
    GtkTreeIter childiter;

    uint32_t objectID;

    // Let's see if we have anything selected in the folder view, and if not let the user know, and return
    if (gtk_tree_selection_count_selected_rows(folderSelection) == 0) {
        // Add in folder to MTP device.
        displayInformation(_("No files/folders selected?"));
        return;
    } else {
        // We have selected a folder in the folder view, so let's get it's ID.
        sortmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviewFiles));
        gtk_tree_selection_get_selected(folderSelection, &sortmodel, &iter);
        gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sortmodel), &childiter, &iter);
        gtk_tree_model_get(GTK_TREE_MODEL(folderList), &childiter, COL_FOL_ID, &objectID,
                COL_FOL_NAME_HIDDEN, &filename, -1);

        // Get our new folder name.
        newfilename = displayRenameFileDialog(filename);

        // If the user supplied something, then update the name of the device.
        if (newfilename != NULL) {
            filesRename(newfilename, objectID);
            g_free(newfilename);
            deviceRescan();
        }
    }
} // end on_folderRenameFolder_activate()


// ************************************************************************************************

/**
 * Callback handle to handle deleting a folder menu option.
 * @param menuitem
 * @param user_data
 */
void on_fileRemoveFolder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dialog;

    // Let's check to see if we have anything selected in our treeview?
    if (fileListGetSelection() == NULL) {
        if (folderListGetSelection() != -1) {
            on_folderRemoveFolder_activate(menuitem, user_data);
        } else {
            displayInformation(_("No files/folders selected?"));
        }
        return;
    }

    // Now we prompt to confirm delete?
    if (Preferences.confirm_file_delete_op == FALSE) {
        // Now download the actual file from the MTP device.
        folderListRemove(fileListGetSelection());
    } else {
        dialog = gtk_message_dialog_new(GTK_WINDOW(windowMain),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_WARNING,
                GTK_BUTTONS_YES_NO,
                _("Are you sure you want to delete this folder (and all contents)?"));
        gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm Delete"));
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        if (result == GTK_RESPONSE_YES)
            folderListRemove(fileListGetSelection());
        gtk_widget_destroy(dialog);
    }
} // end on_fileRemoveFolder_activate()

// ************************************************************************************************

/**
 * Callback to handle the Rename Device menu option.
 * @param menuitem
 * @param user_data
 */
void on_editDeviceName_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *devicename = NULL;
    gchar *tmp_string = NULL;

    // Get our new device name.
    devicename = displayChangeDeviceNameDialog(DeviceMgr.devicename->str);

    // If the user supplied something, then update the name of the device.
    if (devicename != NULL) {
        // add change to MTP device.
        setDeviceName(devicename);
        g_free(devicename);
        // Attempt to read it back as confirmation that something may of happened.
        tmp_string = LIBMTP_Get_Friendlyname(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.devicename = g_string_new(_("N/A"));
        } else {
            DeviceMgr.devicename = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Perform a device Rescan operation to reset all device parameters.
        deviceRescan();
    }
} // end on_editDeviceName_activate()


// ************************************************************************************************

/**
 * Callback to handle the displaying of the context menu.
 * @param widget
 * @param event
 * @return
 */
gboolean on_windowMainContextMenu_activate(GtkWidget *widget, GdkEvent *event) {
    GtkMenu *menu;
    GdkEventButton *event_button;
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(GTK_IS_MENU(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    /* The "widget" is the menu that was supplied when
     * g_signal_connect_swapped() was called.
     */
    menu = GTK_MENU(widget);
    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
                    event_button->button, event_button->time);
            return TRUE;
        }
    }
    return FALSE;
} // end on_windowMainContextMenu_activate()


// ************************************************************************************************

/**
 * Callback to handle the displaying of the context menu.
 * @param widget
 * @param event
 * @return
 */
gboolean on_windowViewContextMenu_activate(GtkWidget *widget, GdkEvent *event) {
    GtkMenu *menu;
    GdkEventButton *event_button;
    g_return_val_if_fail(event != NULL, FALSE);

    /* The "widget" is the menu that was supplied when
     * g_signal_connect_swapped() was called.
     */
    menu = GTK_MENU(contextMenuColumn);
    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
                    event_button->button, event_button->time);
            return TRUE;
        }
    }
    return FALSE;
} // end on_windowMainContextMenu_activate()

// ************************************************************************************************

/**
 * Callback to handle the Find menu option.
 * @param menuitem
 * @param user_data
 */
void on_editFind_activate(GtkMenuItem *menuitem, gpointer user_data) {
    if (inFindMode == FALSE) {

        gtk_widget_show(findToolbar);
        gtk_widget_hide(scrolledwindowFolders);

        fileListClear();
        //folderListClear();
        inFindMode = TRUE;
        statusBarSet(_("Please enter search item."));
        setWindowTitle(_("Search"));
        gtk_tree_view_column_set_visible(column_Location, TRUE);

        //Disable some of the menu options, while in search mode.
        gtk_widget_set_sensitive(GTK_WIDGET(cfileAdd), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(cfileNewFolder), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(fileAdd), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(fileNewFolder), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonAddFile), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(menu_view_folders), FALSE);
        // Get focus on text entry box.

        gtk_widget_grab_focus(GTK_WIDGET(FindToolbar_entry_FindText));
    } else {
        on_editFindClose_activate(menuitem, user_data);
    }

} // end on_editFind_activate()


// ************************************************************************************************

/**
 * Callback to handle the Find menu option.
 * @param menuitem
 * @param user_data
 */
void on_editSelectAll_activate(GtkMenuItem *menuitem, gpointer user_data) {
    fileListSelectAll();
} // end on_editSelectAll_activate()

// ************************************************************************************************

/**
 * Callback to handle the Find toolbar close option.
 * @param menuitem
 * @param user_data
 */
void on_editFindClose_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar* tmp_string;

    gtk_widget_hide(findToolbar);
    if (Preferences.view_folders == TRUE) {
        gtk_widget_show(scrolledwindowFolders);
    }
    fileListClear();
    inFindMode = FALSE;
    gtk_tree_view_column_set_visible(column_Location, FALSE);

    // First we clear the file and folder list...
    fileListClear();
    folderListClear();
    // Refresh the file listings.

    // If using alternate access method, then get our next list of files for the current folder id.
    if (Preferences.use_alt_access_method) {
        filesUpateFileList();
    }

    fileListAdd();
    folderListAdd(deviceFolders, NULL);

    // Update the status bar.
    if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
        tmp_string = g_strdup_printf(_("Connected to %s - %d MB free"), DeviceMgr.devicename->str,
                (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
    } else {
        if (DeviceMgr.devicestorage->StorageDescription != NULL) {
            tmp_string = g_strdup_printf(_("Connected to %s (%s) - %d MB free"), DeviceMgr.devicename->str,
                    DeviceMgr.devicestorage->StorageDescription,
                    (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
        } else {
            tmp_string = g_strdup_printf(_("Connected to %s - %d MB free"), DeviceMgr.devicename->str,
                    (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
        }
    }
    statusBarSet(tmp_string);
    g_free(tmp_string);

    // Now clear the Search GList;
    if (searchList != NULL) {
        g_slist_foreach(searchList, (GFunc) g_free_search, NULL);
        g_slist_free(searchList);
        searchList = NULL;
    }
    //Enable some of the menu options, while in search mode.
    gtk_widget_set_sensitive(GTK_WIDGET(cfileAdd), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(cfileNewFolder), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(fileAdd), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(fileNewFolder), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(toolbuttonAddFile), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(menu_view_folders), !Preferences.use_alt_access_method);

} // end on_editFindClose_activate()


// ************************************************************************************************

/**
 * Callback to handle the actual searching of files/folders.
 * @param menuitem
 * @param user_data
 */
void on_editFindSearch_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *searchstring = NULL;
    gboolean searchfiles = FALSE;
    gboolean searchmeta = FALSE;

    statusBarSet(_("Searching..."));
    // Now clear the Search GList;
    if (searchList != NULL) {
        g_slist_foreach(searchList, (GFunc) g_free_search, NULL);
        g_slist_free(searchList);
        searchList = NULL;
    }

    // Set to upper case to perform case insensitive searches.
    searchstring = g_utf8_strup(gtk_entry_get_text(GTK_ENTRY(FindToolbar_entry_FindText)), -1);
    searchfiles = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FindToolbar_checkbutton_FindFiles));
    searchmeta = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FindToolbar_checkbutton_TrackInformation));
    // disable search metadata if treatallmediaasfiles or in alternate access mode is set.
    if(Preferences.allmediaasfiles == TRUE || Preferences.use_alt_access_method == TRUE){
        searchmeta = FALSE;
    }
    // Let's start our search.
    searchList = filesSearch(searchstring, searchfiles, searchmeta);
    inFindMode = TRUE;
    fileListClear();
    fileListAdd();
    g_free(searchstring);
} // end on_editFindSearch_activate()







// ************************************************************************************************

/**
 * Callback to handle the change of columns viewable in the main window.
 * @param menuitem
 * @param user_data
 */
void on_view_activate(GtkMenuItem *menuitem, gpointer user_data) {
#if HAVE_GTK3 == 0
    gchar *gconf_path = NULL;
    gboolean state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
    // Main menu.
    if ((void *) menuitem == (void *) menu_view_filesize) gconf_path = g_strdup("/apps/gMTP/viewFileSize");
    if ((void *) menuitem == (void *) menu_view_filetype) gconf_path = g_strdup("/apps/gMTP/viewFileType");
    if ((void *) menuitem == (void *) menu_view_track_number) gconf_path = g_strdup("/apps/gMTP/viewTrackNumber");
    if ((void *) menuitem == (void *) menu_view_title) gconf_path = g_strdup("/apps/gMTP/viewTitle");
    if ((void *) menuitem == (void *) menu_view_artist) gconf_path = g_strdup("/apps/gMTP/viewArtist");
    if ((void *) menuitem == (void *) menu_view_album) gconf_path = g_strdup("/apps/gMTP/viewAlbum");
    if ((void *) menuitem == (void *) menu_view_year) gconf_path = g_strdup("/apps/gMTP/viewYear");
    if ((void *) menuitem == (void *) menu_view_genre) gconf_path = g_strdup("/apps/gMTP/viewGenre");
    if ((void *) menuitem == (void *) menu_view_duration) gconf_path = g_strdup("/apps/gMTP/viewDuration");
    if ((void *) menuitem == (void *) menu_view_folders) gconf_path = g_strdup("/apps/gMTP/viewFolders");
    if ((void *) menuitem == (void *) menu_view_toolbar) gconf_path = g_strdup("/apps/gMTP/viewtoolbar");
    // context menu
    if ((void *) menuitem == (void *) cViewSize) gconf_path = g_strdup("/apps/gMTP/viewFileSize");
    if ((void *) menuitem == (void *) cViewType) gconf_path = g_strdup("/apps/gMTP/viewFileType");
    if ((void *) menuitem == (void *) cViewTrackNumber) gconf_path = g_strdup("/apps/gMTP/viewTrackNumber");
    if ((void *) menuitem == (void *) cViewTrackName) gconf_path = g_strdup("/apps/gMTP/viewTitle");
    if ((void *) menuitem == (void *) cViewArtist) gconf_path = g_strdup("/apps/gMTP/viewArtist");
    if ((void *) menuitem == (void *) cViewAlbum) gconf_path = g_strdup("/apps/gMTP/viewAlbum");
    if ((void *) menuitem == (void *) cViewYear) gconf_path = g_strdup("/apps/gMTP/viewYear");
    if ((void *) menuitem == (void *) cViewGenre) gconf_path = g_strdup("/apps/gMTP/viewGenre");
    if ((void *) menuitem == (void *) cViewDuration) gconf_path = g_strdup("/apps/gMTP/viewDuration");

    if ((gconfconnect != NULL) && (gconf_path != NULL)) {
        gconf_client_set_bool(gconfconnect, gconf_path, state, NULL);
        g_free(gconf_path);
    }
#else
    gchar *gsetting_path = NULL;
    gboolean state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
    // main menu
    if ((void *) menuitem == (void *) menu_view_filesize) gsetting_path = g_strdup("viewfilesize");
    if ((void *) menuitem == (void *) menu_view_filetype) gsetting_path = g_strdup("viewfiletype");
    if ((void *) menuitem == (void *) menu_view_track_number) gsetting_path = g_strdup("viewtracknumber");
    if ((void *) menuitem == (void *) menu_view_title) gsetting_path = g_strdup("viewtitle");
    if ((void *) menuitem == (void *) menu_view_artist) gsetting_path = g_strdup("viewartist");
    if ((void *) menuitem == (void *) menu_view_album) gsetting_path = g_strdup("viewalbum");
    if ((void *) menuitem == (void *) menu_view_year) gsetting_path = g_strdup("viewyear");
    if ((void *) menuitem == (void *) menu_view_genre) gsetting_path = g_strdup("viewgenre");
    if ((void *) menuitem == (void *) menu_view_duration) gsetting_path = g_strdup("viewduration");
    if ((void *) menuitem == (void *) menu_view_folders) gsetting_path = g_strdup("viewfolders");
    if ((void *) menuitem == (void *) menu_view_toolbar) gsetting_path = g_strdup("viewtoolbar");
    //context menu.
    if ((void *) menuitem == (void *) cViewSize) gsetting_path = g_strdup("viewfilesize");
    if ((void *) menuitem == (void *) cViewType) gsetting_path = g_strdup("viewfiletype");
    if ((void *) menuitem == (void *) cViewTrackNumber) gsetting_path = g_strdup("viewtracknumber");
    if ((void *) menuitem == (void *) cViewTrackName) gsetting_path = g_strdup("viewtitle");
    if ((void *) menuitem == (void *) cViewArtist) gsetting_path = g_strdup("viewartist");
    if ((void *) menuitem == (void *) cViewAlbum) gsetting_path = g_strdup("viewalbum");
    if ((void *) menuitem == (void *) cViewYear) gsetting_path = g_strdup("viewyear");
    if ((void *) menuitem == (void *) cViewGenre) gsetting_path = g_strdup("viewgenre");
    if ((void *) menuitem == (void *) cViewDuration) gsetting_path = g_strdup("viewduration");

    if ((gsettings_connect != NULL) && (gsetting_path != NULL)) {
        g_settings_set_boolean(gsettings_connect, gsetting_path, state);
        g_settings_sync();
        g_free(gsetting_path);
    }
#endif
} // end on_view_activate()


// ************************************************************************************************

/**
 * Callback to handle user asking to create a new playlist from the AutoAddTrack to Playlist option.
 * @param button
 * @param user_data
 */
void on_TrackPlaylist_NewPlaylistButton_activate(GtkWidget *button, gpointer user_data) {
    gchar *playlistname = NULL;
    gint combobox_entries = 0;

    playlistname = displayPlaylistNewDialog();
    if (playlistname != NULL) {
        // Add in playlist to MTP device.
        playlistAdd(playlistname);
        // Refresh our playlist information.
        devicePlayLists = getPlaylists();

        // Add it to our combobox
#if HAVE_GTK3 == 0
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_AddTrackPlaylist), g_strdup(playlistname));
#else
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox_AddTrackPlaylist), g_strdup(playlistname));
#endif
        g_free(playlistname);

        // Set the active combobox item.
        combobox_entries = gtk_tree_model_iter_n_children(gtk_combo_box_get_model(GTK_COMBO_BOX(combobox_AddTrackPlaylist)), NULL);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_AddTrackPlaylist), combobox_entries - 1);
    }
}


// ************************************************************************************************

/**
 * Callback to handle when a row is selected in the folder list.
 * @param treeselection
 * @param user_data
 */
void on_treeviewFolders_rowactivated(GtkTreeSelection *treeselection, gpointer user_data) {
    // Block the handler from running ...
    g_signal_handler_block((gpointer) fileSelection, fileSelectHandler);
    g_signal_handler_block((gpointer) folderSelection, folderSelectHandler);

    if ((void *) treeselection == (void *) fileSelection) {
        gtk_tree_selection_unselect_all(folderSelection);
    } else {
        gtk_tree_selection_unselect_all(fileSelection);
    }

    // Unblock the handler from running ...
    g_signal_handler_unblock((gpointer) fileSelection, fileSelectHandler);
    g_signal_handler_unblock((gpointer) folderSelection, folderSelectHandler);
}
