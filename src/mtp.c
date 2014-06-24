/* 
 *
 *   File: mtp.c
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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#endif
#include <libmtp.h>
#include <libgen.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <id3tag.h>
#include <stdio.h>
#include <unistd.h>
#include <FLAC/all.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "metatag_info.h"
#include "progress.h"


// Array with file extensions matched to internal libmtp file types;
// See find_filetype() for usage;
MTP_file_ext_struct file_ext[] = {
    {"wav", LIBMTP_FILETYPE_WAV},
    {"mp3", LIBMTP_FILETYPE_MP3},
    {"wma", LIBMTP_FILETYPE_WMA},
    {"ogg", LIBMTP_FILETYPE_OGG},
    {"mp4", LIBMTP_FILETYPE_MP4},
    {"wmv", LIBMTP_FILETYPE_WMV},
    {"avi", LIBMTP_FILETYPE_AVI},
    {"mpeg", LIBMTP_FILETYPE_MPEG},
    {"mpg", LIBMTP_FILETYPE_MPEG},
    {"asf", LIBMTP_FILETYPE_ASF},
    {"qt", LIBMTP_FILETYPE_QT},
    {"mov", LIBMTP_FILETYPE_QT},
    {"wma", LIBMTP_FILETYPE_WMA},
    {"jpg", LIBMTP_FILETYPE_JPEG},
    {"jpeg", LIBMTP_FILETYPE_JPEG},
    {"jfif", LIBMTP_FILETYPE_JFIF},
    {"tif", LIBMTP_FILETYPE_TIFF},
    {"tiff", LIBMTP_FILETYPE_TIFF},
    {"bmp", LIBMTP_FILETYPE_BMP},
    {"gif", LIBMTP_FILETYPE_GIF},
    {"pic", LIBMTP_FILETYPE_PICT},
    {"pict", LIBMTP_FILETYPE_PICT},
    {"png", LIBMTP_FILETYPE_PNG},
    {"wmf", LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT},
    {"ics", LIBMTP_FILETYPE_VCALENDAR2},
    {"exe", LIBMTP_FILETYPE_WINEXEC},
    {"com", LIBMTP_FILETYPE_WINEXEC},
    {"bat", LIBMTP_FILETYPE_WINEXEC},
    {"dll", LIBMTP_FILETYPE_WINEXEC},
    {"sys", LIBMTP_FILETYPE_WINEXEC},
    {"txt", LIBMTP_FILETYPE_TEXT},
    {"aac", LIBMTP_FILETYPE_AAC},
    {"mp2", LIBMTP_FILETYPE_MP2},
    {"flac", LIBMTP_FILETYPE_FLAC},
    {"m4a", LIBMTP_FILETYPE_M4A},
    {"doc", LIBMTP_FILETYPE_DOC},
    {"xml", LIBMTP_FILETYPE_XML},
    {"xls", LIBMTP_FILETYPE_XLS},
    {"ppt", LIBMTP_FILETYPE_PPT},
    {"mht", LIBMTP_FILETYPE_MHT},
    {"jp2", LIBMTP_FILETYPE_JP2},
    {"jpx", LIBMTP_FILETYPE_JPX},
    {"bin", LIBMTP_FILETYPE_FIRMWARE},
    {"vcf", LIBMTP_FILETYPE_VCARD3},
    {"alb", LIBMTP_FILETYPE_ALBUM},
    {"pla", LIBMTP_FILETYPE_PLAYLIST}
};

static gchar* blank_ext = "";

// Ignore Album errors?

gboolean AlbumErrorIgnore = FALSE;

// ************************************************************************************************

/**
 * Attempt to connect to a device.
 * @return 0 if successful, otherwise error code.
 */
guint deviceConnect() {
    gint error;
    if (DeviceMgr.deviceConnected == TRUE) {
        // We must be wanting to disconnect the device.
        return deviceDisconnect();
    } else {
        error = LIBMTP_Detect_Raw_Devices(&DeviceMgr.rawdevices, &DeviceMgr.numrawdevices);
        switch (error) {
            case LIBMTP_ERROR_NONE:
                break;
            case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
                g_fprintf(stderr, _("Detect: No raw devices found.\n"));
                displayError(_("Detect: No raw devices found.\n"));
                return MTP_GENERAL_FAILURE;
            case LIBMTP_ERROR_CONNECTING:
                g_fprintf(stderr, _("Detect: There has been an error connecting. \n"));
                displayError(_("Detect: There has been an error connecting. \n"));
                return MTP_GENERAL_FAILURE;
            case LIBMTP_ERROR_MEMORY_ALLOCATION:
                g_fprintf(stderr, _("Detect: Encountered a Memory Allocation Error. \n"));
                displayError(_("Detect: Encountered a Memory Allocation Error. \n"));
                return MTP_GENERAL_FAILURE;
            default:
                // Some other generic error, so let's exit.
                g_fprintf(stderr, _("Detect: There has been an error connecting. \n"));
                displayError(_("Detect: There has been an error connecting. \n"));
                return MTP_GENERAL_FAILURE;
        }
        // We have at least 1 raw device, so we connect to the first device.
        if (DeviceMgr.numrawdevices > 1) {
            DeviceMgr.rawdeviceID = displayMultiDeviceDialog();
            if (!Preferences.use_alt_access_method) {
                DeviceMgr.device = LIBMTP_Open_Raw_Device(&DeviceMgr.rawdevices[DeviceMgr.rawdeviceID]);
            } else {
                DeviceMgr.device = LIBMTP_Open_Raw_Device_Uncached(&DeviceMgr.rawdevices[DeviceMgr.rawdeviceID]);
            }
        } else {
            // Connect to the first device.
            if (!Preferences.use_alt_access_method) {
                DeviceMgr.device = LIBMTP_Open_Raw_Device(&DeviceMgr.rawdevices[0]);
            } else {
                DeviceMgr.device = LIBMTP_Open_Raw_Device_Uncached(&DeviceMgr.rawdevices[0]);
            }
            DeviceMgr.rawdeviceID = 0;
        }
        if (DeviceMgr.device == NULL) {
            g_fprintf(stderr, _("Detect: Unable to open raw device?\n"));
            displayError(_("Detect: Unable to open raw device?\n"));
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
            DeviceMgr.deviceConnected = FALSE;
            return MTP_GENERAL_FAILURE;
        }

        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
        DeviceMgr.deviceConnected = TRUE;

        // We have a successful device connect, but lets check for multiple storageIDs.
        if (DeviceMgr.device->storage == NULL) {
            g_fprintf(stderr, _("Detect: No available Storage found on device?\n"));
            displayError(_("Detect: No available Storage found on device?\n"));
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
            deviceDisconnect();
            return MTP_GENERAL_FAILURE;
        }
        if (DeviceMgr.device->storage->next != NULL) {
            // Oops we have multiple storage IDs.
            DeviceMgr.storagedeviceID = displayDeviceStorageDialog();
        } else {
            DeviceMgr.storagedeviceID = MTP_DEVICE_SINGLE_STORAGE;
        }
        currentFolderID = 0;
        DeviceMgr.devicename = NULL;
        DeviceMgr.manufacturername = NULL;
        DeviceMgr.modelname = NULL;
        DeviceMgr.serialnumber = NULL;
        DeviceMgr.deviceversion = NULL;
        DeviceMgr.syncpartner = NULL;
        DeviceMgr.sectime = NULL;
        DeviceMgr.devcert = NULL;
        DeviceMgr.Vendor = NULL;
        DeviceMgr.Product = NULL;
        DeviceMgr.devicestorage = NULL;

        // if in alt connection mode;
        if (Preferences.use_alt_access_method) {
            if (stackFolderIDs != NULL) {
                g_queue_free(stackFolderIDs);
            }
            stackFolderIDs = g_queue_new();
            if (stackFolderNames != NULL) {
                g_queue_free(stackFolderNames);
            }
            stackFolderNames = g_queue_new();
        }
        return MTP_SUCCESS;
    }
}

// ************************************************************************************************

/**
 * Disconnect from the currently connected device.
 * @return 0 if successful, otherwise error code.
 */
guint deviceDisconnect() {
    if (DeviceMgr.deviceConnected == FALSE) {
        DeviceMgr.deviceConnected = FALSE;
        return MTP_NO_DEVICE;
    } else {
        DeviceMgr.deviceConnected = FALSE;
        LIBMTP_Release_Device(DeviceMgr.device);
        g_free(DeviceMgr.rawdevices);
        // Now clean up the dymanic data in struc that get's loaded when displaying the properties dialog.
        if (DeviceMgr.devicename != NULL) g_string_free(DeviceMgr.devicename, TRUE);
        if (DeviceMgr.manufacturername != NULL) g_string_free(DeviceMgr.manufacturername, TRUE);
        if (DeviceMgr.modelname != NULL) g_string_free(DeviceMgr.modelname, TRUE);
        if (DeviceMgr.serialnumber != NULL) g_string_free(DeviceMgr.serialnumber, TRUE);
        if (DeviceMgr.deviceversion != NULL) g_string_free(DeviceMgr.deviceversion, TRUE);
        if (DeviceMgr.syncpartner != NULL) g_string_free(DeviceMgr.syncpartner, TRUE);
        if (DeviceMgr.sectime != NULL) g_string_free(DeviceMgr.sectime, TRUE);
        if (DeviceMgr.devcert != NULL) g_string_free(DeviceMgr.devcert, TRUE);
        if (DeviceMgr.Vendor != NULL) g_string_free(DeviceMgr.Vendor, TRUE);
        if (DeviceMgr.Product != NULL) g_string_free(DeviceMgr.Product, TRUE);
        g_free(DeviceMgr.filetypes);
        return MTP_SUCCESS;
    }
}

// ************************************************************************************************

/**
 * Get the properties of the connected device. These properties are stored in 'DeviceMgr'.
 */
void deviceProperties() {
    gint ret;
    gchar *tmp_string;

    // We first see if we have a connected device, and then extract the information from it.
    if (DeviceMgr.deviceConnected == TRUE) {
        // Lets get our information. Let's start with the raw information.
        if (DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.vendor == NULL) {
            DeviceMgr.Vendor = g_string_new(_("Unknown"));
        } else {
            DeviceMgr.Vendor = g_string_new(DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.vendor);
        }
        if (DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.product == NULL) {
            DeviceMgr.Product = g_string_new(_("Unknown"));
        } else {
            DeviceMgr.Product = g_string_new(DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.product);
        }
        DeviceMgr.VendorID = DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.vendor_id;
        DeviceMgr.ProductID = DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].device_entry.product_id;
        DeviceMgr.BusLoc = DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].bus_location;
        DeviceMgr.DeviceID = DeviceMgr.rawdevices[DeviceMgr.rawdeviceID].devnum;

        // Now lets get our other information.
        // Nice name:
        tmp_string = LIBMTP_Get_Friendlyname(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.devicename = g_string_new(_("N/A"));
        } else {
            DeviceMgr.devicename = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Sync Partner
        tmp_string = LIBMTP_Get_Syncpartner(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.syncpartner = g_string_new(_("N/A"));
        } else {
            DeviceMgr.syncpartner = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Battery Level
        ret = LIBMTP_Get_Batterylevel(DeviceMgr.device, &DeviceMgr.maxbattlevel, &DeviceMgr.currbattlevel);
        if (ret != 0) {
            // Silently ignore. Some devices does not support getting the
            // battery level.
            DeviceMgr.maxbattlevel = 0;
            DeviceMgr.currbattlevel = 0;
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
        // Manufacturer Name.
        tmp_string = LIBMTP_Get_Manufacturername(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.manufacturername = g_string_new(_("N/A"));
        } else {
            DeviceMgr.manufacturername = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Model Number,
        tmp_string = LIBMTP_Get_Modelname(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.modelname = g_string_new(_("N/A"));
        } else {
            DeviceMgr.modelname = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Serial Number.
        tmp_string = LIBMTP_Get_Serialnumber(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.serialnumber = g_string_new(_("N/A"));
        } else {
            DeviceMgr.serialnumber = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Device Version.
        tmp_string = LIBMTP_Get_Deviceversion(DeviceMgr.device);
        if (tmp_string == NULL) {
            DeviceMgr.deviceversion = g_string_new(_("N/A"));
        } else {
            DeviceMgr.deviceversion = g_string_new(tmp_string);
            g_free(tmp_string);
        }
        // Secure Time
        ret = LIBMTP_Get_Secure_Time(DeviceMgr.device, &tmp_string);
        if (ret == 0 && tmp_string != NULL) {
            // tmp_string is a XML fragment, and we need just the date/time out of it.
            DeviceMgr.sectime = g_string_new(tmp_string);
            g_free(tmp_string);
        } else {
            // Silently ignore - there may be devices not supporting secure time.
            DeviceMgr.sectime = g_string_new(_("N/A"));
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }

        // Storage.
        if (DeviceMgr.devicestorage == NULL) {
            if (LIBMTP_Get_Storage(DeviceMgr.device, 0) < 0) {
                // We have an error getting our storage, so let the user know and then disconnect the device.
                displayError("Failed to get storage parameters from the device - need to disconnect.");
                on_deviceConnect_activate(NULL, NULL);
                return;
            }
            if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
                DeviceMgr.devicestorage = DeviceMgr.device->storage;
            } else {
                DeviceMgr.devicestorage = getCurrentDeviceStoragePtr(DeviceMgr.storagedeviceID);
            }
            // Supported filetypes;
            ret = LIBMTP_Get_Supported_Filetypes(DeviceMgr.device, &DeviceMgr.filetypes, &DeviceMgr.filetypes_len);
            if (ret != 0) {
                LIBMTP_Dump_Errorstack(DeviceMgr.device);
                LIBMTP_Clear_Errorstack(DeviceMgr.device);
            }
        }
    } else {
        // Set to to none.
        g_fprintf(stderr, _("DevicePropeties: How did I get called?\n"));
        DeviceMgr.device = NULL;
    }
}

// ************************************************************************************************

/**
 * Deallocates the complete chain of the filelist.
 * @param filelist
 */
void clearDeviceFiles(LIBMTP_file_t * filelist) {
    if (filelist != NULL) {
        if (filelist->next != NULL) {
            clearDeviceFiles(filelist->next);
            filelist->next = NULL;
        }
        LIBMTP_destroy_file_t(filelist);
    }
}

// ************************************************************************************************

/**
 * Deallocates the complete chain of Album information.
 * @param albumlist
 */
void clearAlbumStruc(LIBMTP_album_t * albumlist) {
    if (albumlist != NULL) {
        if (albumlist->next != NULL) {
            clearAlbumStruc(albumlist->next);
            albumlist->next = NULL;
        }
        LIBMTP_destroy_album_t(albumlist);
    }
}

// ************************************************************************************************

/**
 * Deallocates the complete chain of all Playlists.
 * @param playlist_list
 */
void clearDevicePlaylist(LIBMTP_playlist_t * playlist_list) {
    if (playlist_list != NULL) {
        if (playlist_list->next != NULL) {
            clearDevicePlaylist(playlist_list->next);
            playlist_list->next = NULL;
        }
        LIBMTP_destroy_playlist_t(playlist_list);
    }
}

// ************************************************************************************************

/**
 * Deallocates the complete chain of all Track information.
 * @param tracklist
 */
void clearDeviceTracks(LIBMTP_track_t * tracklist) {
    if (tracklist != NULL) {
        if (tracklist->next != NULL) {
            clearDeviceTracks(tracklist->next);
            tracklist->next = NULL;
        }
        LIBMTP_destroy_track_t(tracklist);
    }
}

// ************************************************************************************************

void printFolders(LIBMTP_folder_t *fold) {
    while (fold != NULL) {
        g_printf("folder: %s\n", fold->name);
        if (fold->child != NULL) {
            printFolders(fold->child);
        }
        fold = fold->sibling;
    }
}

/**
 * Perform a rescan of the device, recreating any device properties or device information.
 */
void deviceRescan() {
    gchar* tmp_string;
    //g_print("You selected deviceRescan\n");
    // First we clear the file and folder list...
    fileListClear();
    folderListClear();
    // Now clear the folder/file structures.
    if (deviceFolders != NULL)
        LIBMTP_destroy_folder_t(deviceFolders);
    if (deviceFiles != NULL)
        clearDeviceFiles(deviceFiles);
    // Add in track, playlist globals as well.
    deviceFolders = NULL;
    deviceFiles = NULL;
    // Now get started.
    if (DeviceMgr.deviceConnected) {
        // Get a list of folder on the device. (Note: this may fail on some devices, and we end up with zero folders being listed)
        deviceFolders = LIBMTP_Get_Folder_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
        if (deviceFolders == NULL) {
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
        // Now get a list of files from the device.
        if (!Preferences.use_alt_access_method) {
            deviceFiles = LIBMTP_Get_Filelisting_With_Callback(DeviceMgr.device, NULL, NULL);
        } else {
            // Alternate access method ONLY gets the files for the CURRENT FOLDER ONLY. This should help some Android based devices.
            deviceFiles = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, currentFolderID);
        }
        if (deviceFiles == NULL) {
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
        devicePlayLists = getPlaylists();
        deviceTracks = getTracks();
        fileListAdd();
        folderListAdd(deviceFolders, NULL);
        // Now update the storage...
        if (DeviceMgr.devicestorage == NULL) {
            if (LIBMTP_Get_Storage(DeviceMgr.device, 0) < 0) {
                // We have an error getting our storage, so let the user know and then disconnect the device.
                displayError(_("Failed to get storage parameters from the device - need to disconnect."));
                on_deviceConnect_activate(NULL, NULL);
                return;
            }
            if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
                DeviceMgr.devicestorage = DeviceMgr.device->storage;
            } else {
                DeviceMgr.devicestorage = getCurrentDeviceStoragePtr(DeviceMgr.storagedeviceID);
            }
        }
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
    } else {
        g_fprintf(stderr, _("Rescan: How did I get called?\n"));
    }
}

// ************************************************************************************************

/**
 * Find the ptr to the current storage structure.
 * @param StorageID
 * @return
 */
LIBMTP_devicestorage_t* getCurrentDeviceStoragePtr(gint StorageID) {
    LIBMTP_devicestorage_t* deviceStorage = DeviceMgr.device->storage;
    gint i = 0;
    // This is easy, as the gint is the number of hops we need to do to get to our target.
    if (StorageID == MTP_DEVICE_SINGLE_STORAGE)
        return DeviceMgr.device->storage;
    if (StorageID == 0)
        return DeviceMgr.device->storage;
    for (i = 0; i < StorageID; i++) {
        deviceStorage = deviceStorage->next;
        if (deviceStorage == NULL) // Oops, off the end
            return DeviceMgr.device->storage;
    }
    return deviceStorage;
}

// ************************************************************************************************

/**
 * Find the ID of the parent folder.
 * @param tmpfolder
 * @param currentFolderID
 * @return
 */
uint32_t getParentFolderID(LIBMTP_folder_t *tmpfolder, uint32_t currentFolderID) {
    uint32_t parentID = 0;
    if (tmpfolder == NULL) {
        return 0;
    }
    if (tmpfolder->folder_id == currentFolderID) {
        parentID = tmpfolder->parent_id;
        return parentID;
    }
    parentID = getParentFolderID(tmpfolder->child, currentFolderID);
    if (parentID != 0)
        return parentID;
    parentID = getParentFolderID(tmpfolder->sibling, currentFolderID);
    return parentID;
}

// ************************************************************************************************

/**
 * Find the structure of the parent MTP Folder based on the currentID.
 * @param tmpfolder
 * @param currentFolderID
 * @return
 */
LIBMTP_folder_t* getParentFolderPtr(LIBMTP_folder_t *tmpfolder, uint32_t currentFolderID) {
    LIBMTP_folder_t* parentID = NULL;
    if (tmpfolder == NULL) {
        return tmpfolder;
    }
    if (tmpfolder->parent_id == currentFolderID) {
        return tmpfolder;
    }
    parentID = getParentFolderPtr(tmpfolder->child, currentFolderID);
    if (parentID != NULL)
        return parentID;
    parentID = getParentFolderPtr(tmpfolder->sibling, currentFolderID);
    return parentID;
}

// ************************************************************************************************

/**
 * Find the structure of the MTP Folder based on the currentID.
 * @param tmpfolder
 * @param FolderID
 * @return
 */
LIBMTP_folder_t* getCurrentFolderPtr(LIBMTP_folder_t *tmpfolder, uint32_t FolderID) {
    LIBMTP_folder_t* parentID = NULL;
    if (tmpfolder == NULL) {
        return tmpfolder;
    }
    if (tmpfolder->folder_id == FolderID) {
        return tmpfolder;
    }
    parentID = getCurrentFolderPtr(tmpfolder->child, FolderID);
    if (parentID != NULL)
        return parentID;
    parentID = getCurrentFolderPtr(tmpfolder->sibling, FolderID);
    return parentID;
}

// ************************************************************************************************

/**
 * Get the list of files for the device.
 */
void filesUpateFileList() {
    if (deviceFiles != NULL) {
        clearDeviceFiles(deviceFiles);
    }
    deviceFiles = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, currentFolderID);
    if (deviceFiles == NULL) {
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
}


// ************************************************************************************************

/**
 * Add a single file to the current connected device.
 * @param filename
 */
void filesAdd(gchar* filename) {
    uint64_t filesize = 0;
    gchar *filename_stripped;
    struct stat sb;
    LIBMTP_file_t *genfile = NULL;
    LIBMTP_track_t *trackfile = NULL;
    LIBMTP_album_t *albuminfo = NULL;
    LIBMTP_playlist_t* tmpplaylist = NULL;
    gint ret;

    // Maybe something went wrong, so we disconnected. If so, then simple exit....
    if (DeviceMgr.deviceConnected == FALSE){
        return;
    }

    if(progressDialog_killed == TRUE){
	    return;
    }

    if (stat(filename, &sb) == -1) {
        perror("stat");
        return;
    }

    filesize = sb.st_size;
    if (filesize > DeviceMgr.devicestorage->FreeSpaceInBytes) {
        g_fprintf(stderr, _("Unable to add %s due to insufficient space: filesize = %llu, freespace = %llu\n"),
                filename, 
                (unsigned long long int)filesize, 
                (unsigned long long int)DeviceMgr.devicestorage->FreeSpaceInBytes);
        displayError(_("Unable to add file due to insufficient space"));
        return;
    }

    filename_stripped = basename(filename);
    //displayProgressBar(_("File Upload"));
    setProgressFilename(g_strdup(filename_stripped));

    // What we need to do is work what type of file we are sending
    // and either use the general file send, or
    // use the track send function.
    ret = find_filetype(filename_stripped);

    gboolean useTrack = (ret == LIBMTP_FILETYPE_MP3) ||
            (ret == LIBMTP_FILETYPE_OGG) ||
            (ret == LIBMTP_FILETYPE_FLAC) ||
            (ret == LIBMTP_FILETYPE_WMA);
    
    if(Preferences.allmediaasfiles){
        useTrack = FALSE;
    }
    
    if (useTrack) {
        // We have an MP3/Ogg/FLAC/WMA file.
        trackfile = LIBMTP_new_track_t();

        trackfile->filesize = filesize;
        trackfile->filename = g_strdup(filename_stripped);
        trackfile->filetype = find_filetype(filename_stripped);
        trackfile->parent_id = currentFolderID;
        trackfile->storage_id = DeviceMgr.devicestorage->id;
        trackfile->album = NULL;
        trackfile->title = NULL;
        trackfile->artist = NULL;
        trackfile->date = NULL;
        trackfile->genre = NULL;
        trackfile->tracknumber = 0;

        albuminfo = LIBMTP_new_album_t();
        albuminfo->parent_id = currentFolderID;
        albuminfo->storage_id = DeviceMgr.devicestorage->id;
        albuminfo->album_id = 0;
        // Let's collect our metadata from the file, typically id3 tag data.
        switch (ret) {
            case LIBMTP_FILETYPE_MP3:
                // We have an MP3 file, so use id3tag to read the metadata.
                get_id3_tags(filename, trackfile);
                break;
            case LIBMTP_FILETYPE_OGG:
                get_ogg_tags(filename, trackfile);
                break;
            case LIBMTP_FILETYPE_FLAC:
                get_flac_tags(filename, trackfile);
                break;
            case LIBMTP_FILETYPE_WMA:
                get_asf_tags(filename, trackfile);
                break;
                //break;

        }
        // Add some data if it's all blank so we don't freak out some players.
        if (trackfile->album == NULL)
            trackfile->album = NULL;
        if (trackfile->title == NULL)
            trackfile->title = g_strdup(filename_stripped);
        if (trackfile->artist == NULL)
            trackfile->artist = g_strdup(_("<Unknown>"));
        if (trackfile->date == NULL) {
            trackfile->date = g_strdup("");
        } else {
            if (strlen(trackfile->date) == 4) {
                // only have year part, so extend it.
                trackfile->date = g_strconcat(trackfile->date, "0101T000000", NULL);
            }
        }
        if (trackfile->genre == NULL)
            trackfile->genre = g_strdup(_("<Unknown>"));

        // Update our album info, if we actually have an album.
        if (trackfile->album != NULL) {
            albuminfo->name = g_strdup(trackfile->album);
            albuminfo->artist = g_strdup(trackfile->artist);
            albuminfo->composer = NULL;
            albuminfo->genre = g_strdup(trackfile->genre);
        }

        // If we need a playlist, then ask for it.
        if (addTrackPlaylistID == GMTP_REQUIRE_PLAYLIST) {
            addTrackPlaylistID = displayAddTrackPlaylistDialog(TRUE);
        }

        // Now send the track
        ret = LIBMTP_Send_Track_From_File(DeviceMgr.device, filename, trackfile, fileprogress, NULL);
        if (ret != 0) {
            // Report the error in sending the file.
            g_fprintf(stderr, _("Error sending track.\n"));
            displayError(g_strdup_printf(_("Error code %d sending track to device: <b>%s</b>"), ret, filename));
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        } else {
            // Adjust device storage.
            DeviceMgr.devicestorage->FreeSpaceInBytes -= filesize;
            DeviceMgr.devicestorage->FreeSpaceInObjects--;

            // Only update Album data if transfer was successful.
            if (trackfile->album != NULL) {
                albumAddTrackToAlbum(albuminfo, trackfile);
            }

            // Now add to playlist if needed...
            if ((addTrackPlaylistID != GMTP_REQUIRE_PLAYLIST) && (addTrackPlaylistID != GMTP_NO_PLAYLIST)) {
                // addTrackPlaylistID has the ID of the playlist, and trackfile is the track we need to
                // add to that playlist.

                // Find the playlist.
                tmpplaylist = devicePlayLists;
                while (tmpplaylist != NULL) {
                    if (tmpplaylist->playlist_id != (uint32_t) addTrackPlaylistID) {
                        // Don't have it.
                        tmpplaylist = tmpplaylist->next;
                    } else {
                        //We have found it.
                        playlistAddTrack(tmpplaylist, trackfile);
                        tmpplaylist = NULL;
                    }
                }
            }
        }
        LIBMTP_destroy_track_t(trackfile);
        LIBMTP_destroy_album_t(albuminfo);
    } else {
        // Generic file upload.
        genfile = LIBMTP_new_file_t();
        genfile->filesize = filesize;
        genfile->filename = g_strdup(filename_stripped);
        genfile->filetype = find_filetype(filename_stripped);
        genfile->parent_id = currentFolderID;
        genfile->storage_id = DeviceMgr.devicestorage->id;

        // Only import the file if it's not a Playlist or Album. (Bad mojo if this happens).
        if ((genfile->filetype != LIBMTP_FILETYPE_ALBUM) && (genfile->filetype != LIBMTP_FILETYPE_PLAYLIST)) {
            ret = LIBMTP_Send_File_From_File(DeviceMgr.device, filename, genfile, fileprogress, NULL);
            if (ret != 0) {
                g_fprintf(stderr, _("Error sending file %s.\n"), filename);
                displayError(g_strconcat(_("Error sending file:"), " <b>", filename, "</b>", NULL));
                LIBMTP_Dump_Errorstack(DeviceMgr.device);
                LIBMTP_Clear_Errorstack(DeviceMgr.device);
            } else {
                // Adjust device storage.
                DeviceMgr.devicestorage->FreeSpaceInBytes -= filesize;
                DeviceMgr.devicestorage->FreeSpaceInObjects--;
            }
        }
        LIBMTP_destroy_file_t(genfile);
    }


    //destroyProgressBar();
    // Now update the storage...
    if (DeviceMgr.devicestorage == NULL) {
        if (LIBMTP_Get_Storage(DeviceMgr.device, 0) < 0) {
            // We have an error getting our storage, so let the user know and then disconnect the device.
            displayError("Failed to get storage parameters from the device - need to disconnect.");
            on_deviceConnect_activate(NULL, NULL);
            return;
        }
        if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
            DeviceMgr.devicestorage = DeviceMgr.device->storage;
        } else {
            DeviceMgr.devicestorage = getCurrentDeviceStoragePtr(DeviceMgr.storagedeviceID);
        }
    }
}

// ************************************************************************************************

/**
 * Delete a single file from the connected device.
 * @param filename
 * @param objectID
 */
void filesDelete(gchar* filename, uint32_t objectID) {
    gint ret = 1;
    GSList *node;
    FileListStruc *fileptr;
    uint64_t filesize = 0;
    // Maybe something went wrong, so we disconnected. If so, then simple exit....
    if (DeviceMgr.deviceConnected == FALSE)
        return;

    // Now remove the item from the searchlist if we are in search mode.
    if (inFindMode == TRUE) {
        // searchList
        node = searchList;
        while (node != NULL) {
            fileptr = node->data;
            if (fileptr->itemid == objectID) {
                // remove this node from the main list;
                searchList = g_slist_delete_link(searchList, node);
                g_free_search(fileptr);
                node = NULL;
            } else {
                node = node->next;
            }
        }
    }
    // Get the filesize of the object.
    LIBMTP_file_t * files = deviceFiles;
    while (files != NULL) {
        if (files->item_id == objectID) {
            filesize = files->filesize;
            files = NULL;
        } else {
            files = files->next;
        }
    }

    // Delete the file based on the object ID.
    ret = LIBMTP_Delete_Object(DeviceMgr.device, objectID);
    if (ret != 0) {
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
        g_fprintf(stderr, _("\nFailed to delete file %s\n"), filename);
        displayError(g_strconcat(_("Failed to delete file"), " <b>", filename, "</b>", NULL));
    } else {
        // Adjust device storage.
        DeviceMgr.devicestorage->FreeSpaceInBytes += filesize;
        DeviceMgr.devicestorage->FreeSpaceInObjects++;
    }
}

// ************************************************************************************************

/**
 * Download a file from the device to local storage.
 * @param filename
 * @param objectID
 */
void filesDownload(gchar* filename, uint32_t objectID) {
    gchar* fullfilename = NULL;

    // Maybe something went wrong, so we disconnected. If so, then simple exit....
    if (DeviceMgr.deviceConnected == FALSE){
        return;
    }
    if(progressDialog_killed == TRUE){
	    return;
    }

    //displayProgressBar(_("File download"));
    setProgressFilename(filename);
    // Download the file based on the objectID.
    fullfilename = g_strdup_printf("%s/%s", Preferences.fileSystemDownloadPath->str, filename);
    if (LIBMTP_Get_File_To_File(DeviceMgr.device, objectID, fullfilename, fileprogress, NULL) != 0) {
        g_fprintf(stderr, _("\nError getting file from MTP device.\n"));
        displayError(_("Error getting file from MTP device."));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    //destroyProgressBar();
    g_free(fullfilename);
}

// ************************************************************************************************

/**
 * Rename a single file on the device.
 * @param filename - New name to be given to the file.
 * @param ObjectID - The ID of the object.
 */
void filesRename(gchar* filename, uint32_t ObjectID) {
    // We must first determine, if this is a file, a folder, playlist or album
    // and use the correct API.
    LIBMTP_file_t *genfile = NULL;
    LIBMTP_album_t *albuminfo = NULL;
    LIBMTP_album_t *albumlist = NULL;
    LIBMTP_playlist_t *playlist = NULL;
    LIBMTP_folder_t *folder = NULL;
    GSList *node;
    FileListStruc *fileptr;

    if (filename == NULL) {
        return;
    }
    if (ObjectID == 0) {
        return;
    }

    // Now remove the item from the searchlist if we are in search mode.
    if (inFindMode == TRUE) {
        // searchList
        node = searchList;
        while (node != NULL) {
            fileptr = node->data;
            if (fileptr->itemid == ObjectID) {
                // update the filename appropriately;
                g_free(fileptr->filename);
                fileptr->filename = g_strdup(filename);
                node = NULL;
            } else {
                node = node->next;
            }
        }
    }

    // Lets scan files first.
    genfile = deviceFiles;
    while (genfile != NULL) {
        if (genfile->item_id == ObjectID) {
            // We have our file, so update it.
            LIBMTP_Set_File_Name(DeviceMgr.device, genfile, filename);
            deviceRescan();
            return;
        }
        genfile = genfile->next;
    }

    // Lets scan our albums.
    albuminfo = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    albumlist = albuminfo;
    while (albuminfo != NULL) {
        if (albuminfo->album_id == ObjectID) {
            LIBMTP_Set_Album_Name(DeviceMgr.device, albuminfo, filename);
            deviceRescan();
            clearAlbumStruc(albumlist);
            return;
        }
        albuminfo = albuminfo->next;
    }
    clearAlbumStruc(albumlist);

    // Let's scan our playlists.
    playlist = devicePlayLists;
    while (playlist != NULL) {
        if (playlist->playlist_id == ObjectID) {
            // We have our playlist, so update it.
            LIBMTP_Set_Playlist_Name(DeviceMgr.device, playlist, filename);
            deviceRescan();
            return;
        }
        playlist = playlist->next;
    }

    // Lets scan our folders;
    folder = deviceFolders;
    folder = LIBMTP_Find_Folder(folder, ObjectID);
    if (folder != NULL) {
        LIBMTP_Set_Folder_Name(DeviceMgr.device, folder, filename);
        deviceRescan();
        return;
    }

}

// ************************************************************************************************

/**
 * Create a folder on the current connected device.
 * @param foldername
 * @return Object ID of new folder, otherwise 0 if failed.
 */
guint32 folderAdd(gchar* foldername) {
    guint32 res = LIBMTP_Create_Folder(DeviceMgr.device, foldername, currentFolderID, DeviceMgr.devicestorage->id);
    if (res == 0) {
        g_fprintf(stderr, _("Folder creation failed: %s\n"), foldername);
        displayError(g_strconcat(_("Folder creation failed:"), foldername, NULL));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    return res;
}

// ************************************************************************************************

/**
 * Delete a single folder from the currently connected device.
 * @param folderptr
 * @param level Set to 0 as default.
 */
void folderDelete(LIBMTP_folder_t* folderptr, guint level) {

    GSList *node;
    FileListStruc *fileptr;

    if (folderptr == NULL) {
        // Sanity check for rogue data or exit here operation, that is no child/sibling to work on.
        return;
    }
    // This is fun, as we have to find all child folders, delete those, as well as all files contained within...
    // First iteratate through all child folders and remove those files in those folders.
    // But first we need to get the folder structure pointer based on the objectID, so we know where to start.
    // So now we have our structure to the current select folder, so we need to cycle through all children and remove any files contained within.
    folderDeleteChildrenFiles(folderptr->folder_id);
    // Now cycle through folders contained in this folder and delete those;
    folderDelete(folderptr->child, level + 1);
    if (level != 0)
        folderDelete(folderptr->sibling, level + 1);
    // That should clear all the children.

    // Now remove the item from the searchlist if we are in search mode.
    if (inFindMode == TRUE) {
        // searchList
        node = searchList;
        while (node != NULL) {
            fileptr = node->data;
            if (fileptr->itemid == folderptr->folder_id) {
                // remove this node from the main list;
                searchList = g_slist_delete_link(searchList, node);
                g_free_search(fileptr);
                node = NULL;
            } else {
                node = node->next;
            }
        }
    }
    // Now do self.
    guint res = LIBMTP_Delete_Object(DeviceMgr.device, folderptr->folder_id);
    if (res != 0) {
        g_fprintf(stderr, _("Couldn't delete folder %s (%x)\n"), folderptr->name, folderptr->folder_id);
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    } else {
        // Adjust device storage.
        DeviceMgr.devicestorage->FreeSpaceInObjects++;
    }
}

// ************************************************************************************************

/**
 * Delete all files from the specified folder on the device.
 * @param folderID
 */
void folderDeleteChildrenFiles(guint folderID) {
    LIBMTP_file_t* files = deviceFiles;
    while (files != NULL) {
        if (files->parent_id == folderID) {
            filesDelete(files->filename, files->item_id);
        }
        files = files->next;
    }
}

// ************************************************************************************************

/**
 * Download the defined folder to the local device.
 * @param foldername Name of the folder to be downloaded
 * @param folderID ID of the folder on the device
 * @param isParent TRUE, if this is the parent folder to download (eg ignore any folder siblings).
 */
void folderDownload(gchar *foldername, uint32_t folderID, gboolean isParent) {
    gchar* fullfilename = NULL;
    LIBMTP_folder_t* currentFolder = NULL;
    LIBMTP_file_t* tmpFiles = NULL;

    // Store our current path for safe keeping and generate our new path.
    GString *currentdownload_folder = g_string_new(Preferences.fileSystemDownloadPath->str);
    fullfilename = g_strdup_printf("%s/%s", Preferences.fileSystemDownloadPath->str, foldername);

    // See if folder exists, if not create it.
    if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR) == FALSE) {
        if (mkdir(fullfilename, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
            g_fprintf(stderr, _("Folder creation failed: %s\n"), fullfilename);
            displayError(g_strconcat(_("Folder creation failed:"), " <b>", fullfilename, "</b>", NULL));
            // Since we can't create that directory, then we simple return from this call...
            return;
        }
    }

    // First we scan for all folders in the current folder, and do those first...
    if (folderID != 0) {
        currentFolder = getCurrentFolderPtr(deviceFolders, folderID);
    } else {
        // If our id = 0 then we are dealing with the root device.
        currentFolder = deviceFolders;
    }

    if (currentFolder == NULL) {
        // This means we don't exist, so bail out.
        return;
    }

    if (currentFolder->child != NULL) {
        // Now we can simple set our download path to the new path, and save all folders/files there.
        Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, fullfilename);

        // Call to download all folder/files in that folder...
        folderDownload(currentFolder->child->name, currentFolder->child->folder_id, FALSE);

        // Restore the old download path;
        Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, currentdownload_folder->str);
    }

    if ((isParent == FALSE) || (folderID == 0)) {
        if (currentFolder->sibling != NULL) {
            folderDownload(currentFolder->sibling->name, currentFolder->sibling->folder_id, FALSE);
        }
    }
    // Now download all the files whose parentID = folderID;

    // Now we can simple set our download path to the new path, and save all folders/files there.
    Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, fullfilename);

    // we no longer need the full folder path, so free it's variable.
    g_free(fullfilename);

    // Start processing all our files.
    tmpFiles = deviceFiles;
    while (tmpFiles != NULL) {
        if ((tmpFiles->parent_id == folderID) && (tmpFiles->storage_id == DeviceMgr.devicestorage->id)) {
            // We have a file in this folder, so download it...
            // But first check to see if it exists, before overwriting it...
            fullfilename = g_strdup_printf("%s/%s", Preferences.fileSystemDownloadPath->str, tmpFiles->filename);
            // Check if file exists?
            if (access(fullfilename, F_OK) != -1) {
                // We have that file already?
                if (Preferences.prompt_overwrite_file_op == TRUE) {
                    if (fileoverwriteop == MTP_ASK) {
                        fileoverwriteop = displayFileOverwriteDialog(tmpFiles->filename);
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
                            filesDownload(tmpFiles->filename, tmpFiles->item_id);
                            fileoverwriteop = MTP_ASK;
                            break;
                        case MTP_OVERWRITE_ALL:
                            filesDownload(tmpFiles->filename, tmpFiles->item_id);
                            break;
                    }
                } else {
                    filesDownload(tmpFiles->filename, tmpFiles->item_id);
                }
            } else {
                filesDownload(tmpFiles->filename, tmpFiles->item_id);
            }
            // Free our file name...
            g_free(fullfilename);
        }
        // Start working on the next file...
        tmpFiles = tmpFiles->next;
    }

    // Restore the old download path;
    Preferences.fileSystemDownloadPath = g_string_assign(Preferences.fileSystemDownloadPath, currentdownload_folder->str);

    // Clean up any tmp items;
    g_string_free(currentdownload_folder, TRUE);
}

// ************************************************************************************************

/**
 * Find the file type based on extension
 * @param filename
 * @return
 */
LIBMTP_filetype_t find_filetype(const gchar * filename) {
    LIBMTP_filetype_t filetype = LIBMTP_FILETYPE_UNKNOWN;
    gchar *fileext;
    gint i;
    gint j = sizeof (file_ext) / sizeof (MTP_file_ext_struct);

    fileext = rindex(filename, '.');
    // This accounts for the case with a filename without any "." (period).
    if (!fileext) {
        fileext = "";
    } else {
        ++fileext;
    }
    // Now cycle through the array of extensions, and get the associated
    // libmtp filetype.
    for (i = 0; i < j; i++) {
        if (g_ascii_strcasecmp(fileext, file_ext[i].file_extension) == 0) {
            filetype = file_ext[i].file_type;
            break;
        }
    }
    //if (filetype == 0) {
    //    filetype = LIBMTP_FILETYPE_UNKNOWN;
    //}
    return filetype;
}

// ************************************************************************************************

/**
 * Get the file extension  based on filetype
 * @param filetype
 * @return
 */
gchar* find_filetype_ext(LIBMTP_filetype_t filetype) {
    gint i = 0;
    gint j = sizeof (file_ext) / sizeof (MTP_file_ext_struct);
    for (i = 0; i < j; i++) {
        if (filetype == file_ext[i].file_type) {
            return file_ext[i].file_extension;
        }
    }
    return blank_ext;
}

// ************************************************************************************************

/**
 * Set the friendly name of the current connected device.
 * @param devicename
 */
void setDeviceName(gchar* devicename) {
    gint res = 0;
    if (DeviceMgr.deviceConnected == TRUE) {
        if (devicename != NULL)
            res = LIBMTP_Set_Friendlyname(DeviceMgr.device, devicename);
        if (res != 0) {
            g_fprintf(stderr, _("Error: Couldn't set device name to %s\n"), devicename);
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
    } else {
        // Set to to none.
        g_fprintf(stderr, _("setDeviceName: How did I get called?\n"));
    }
}

// ************************************************************************************************

/**
 * Check to see if this file already exists within the current folder on the device.
 * @param filename
 * @return
 */
gboolean fileExists(gchar* filename) {
    // What we have to go is scan the entire file tree looking for
    // entries in the same folder as the current and the same
    // storage pool, then we do a string compare (since doing a string
    // compare is so much slower than comparing a few numbers).
    LIBMTP_file_t* tmpfile;
    if(Preferences.use_alt_access_method){
        tmpfile = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, currentFolderID);
    } else {
        tmpfile = deviceFiles;
    }
    while (tmpfile != NULL) {
        // Check for matching folder ID and storage ID.
        if ((tmpfile->parent_id == currentFolderID) && (tmpfile->storage_id == DeviceMgr.devicestorage->id)) {
            // Now test for the file name (do case insensitive cmp for those odd devices);
            if (g_ascii_strcasecmp(filename, tmpfile->filename) == 0)
                return TRUE;
        }
        tmpfile = tmpfile->next;
    }
    return FALSE;
}

// ************************************************************************************************

/**
 * Check to see if this file already exists within the current folder on the device.
 * @param filename
 * @return
 */
uint32_t getFile(gchar* filename, uint32_t folderID) {
    // What we have to go is scan the entire file tree looking for
    // entries in the same folder as the current and the same
    // storage pool, then we do a string compare (since doing a string
    // compare is so much slower than comparing a few numbers).
    LIBMTP_file_t* tmpfile;
    if(Preferences.use_alt_access_method){
        tmpfile = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, folderID);
    } else {
        tmpfile = deviceFiles;
    }
    while (tmpfile != NULL) {
        // Check for matching folder ID and storage ID.
        if ((tmpfile->parent_id == currentFolderID) && (tmpfile->storage_id == DeviceMgr.devicestorage->id)) {
            // Now test for the file name (do case insensitive cmp for those odd devices);
            if (g_ascii_strcasecmp(filename, tmpfile->filename) == 0)
                return tmpfile->item_id;
        }
        tmpfile = tmpfile->next;
    }
    return -1;
}

// ************************************************************************************************


/**
 * Determine if the folder name exists in the given folder (based on ID)
 * @param foldername The name of the folder.
 * @param folderID The ID of the folder whose contents are to be checked.
 */
gboolean folderExists(gchar *foldername, uint32_t folderID ){
    if(Preferences.use_alt_access_method){
        LIBMTP_file_t* tmpfile = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, folderID);
        while (tmpfile != NULL) {
            // Check for matching folder ID and storage ID.
            if ((tmpfile->parent_id == currentFolderID) && (tmpfile->storage_id == DeviceMgr.devicestorage->id)) {
                // Now test for the file name (do case insensitive cmp for those odd devices);
                if (g_ascii_strcasecmp(foldername, tmpfile->filename) == 0)
                    return TRUE;
            }
            tmpfile = tmpfile->next;
        }
    } else {

        LIBMTP_folder_t* folder = getCurrentFolderPtr(deviceFolders, folderID);
        if(folder == NULL){
            return FALSE;
        }
        // Scan for child folders with the same name.
        LIBMTP_folder_t* child = folder->child;
        while(child != NULL){
            if (g_ascii_strcasecmp(foldername, child->name) == 0)
                return TRUE;
            child = child->sibling;
        }
    }
    return FALSE;
}

// ************************************************************************************************

/**
 * Get the folderID based on the folder name that exists in the given folder (based on ID)
 * @param foldername The name of the folder.
 * @param folderID The ID of the folder whose contents are to be checked.
 * @return The ID of the folder.
 */
uint32_t getFolder(gchar *foldername, uint32_t folderID){
    if(Preferences.use_alt_access_method){
        LIBMTP_file_t* tmpfile = LIBMTP_Get_Files_And_Folders(DeviceMgr.device, DeviceMgr.devicestorage->id, folderID);
        while (tmpfile != NULL) {
            // Check for matching folder ID and storage ID.
            if ((tmpfile->parent_id == currentFolderID) && (tmpfile->storage_id == DeviceMgr.devicestorage->id)) {
                // Now test for the file name (do case insensitive cmp for those odd devices);
                if (g_ascii_strcasecmp(foldername, tmpfile->filename) == 0)
                    return tmpfile->item_id;
            }
            tmpfile = tmpfile->next;
        }
    } else {
        LIBMTP_folder_t* folder = getCurrentFolderPtr(deviceFolders, folderID);
        if(folder == NULL){
            return FALSE;
        }
        // Scan for child folders with the same name.
        LIBMTP_folder_t* child = folder->child;
        while(child != NULL){
            if (g_ascii_strcasecmp(foldername, child->name) == 0)
                return child->folder_id;
            child = child->sibling;
        }
    }
    return -1;
}

// ************************************************************************************************

/**
 * Add the specified track to an album, and return that album information.
 * @param albuminfo
 * @param trackinfo
 */
void albumAddTrackToAlbum(LIBMTP_album_t* albuminfo, LIBMTP_track_t* trackinfo) {
    LIBMTP_album_t *album = NULL;
    LIBMTP_album_t *found_album = NULL;
    LIBMTP_album_t *album_orig = NULL;
    gint ret = 0;

    // Quick sanity check.
    if ((albuminfo->name == NULL) || (albuminfo->artist == NULL))
        return;

    // Lets try to find the album.
    album = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    album_orig = album;
    while ((album != NULL) && (found_album == NULL)) {
        if ((album->name != NULL) && (album->artist != NULL)) {
            // Lets test it. We attempt to match both album name and artist.
            if ((g_ascii_strcasecmp(album->name, albuminfo->name) == 0) &&
                    (g_ascii_strcasecmp(album->artist, albuminfo->artist) == 0)) {
                found_album = album;
            }
        }
        album = album->next;
    }
    // Some devices ignore all other fields and only retain the ablum name - so test for this as well!
    album = album_orig;
    if (found_album == NULL) {
        while ((album != NULL) && (found_album == NULL)) {
            if (album->name != NULL) {
                // Lets test it. We attempt to match both album name and artist.
                if (g_ascii_strcasecmp(album->name, albuminfo->name) == 0) {
                    found_album = album;
                }
            }
            album = album->next;
        }
    }

    if (found_album != NULL) {
        // The album already exists.
        uint32_t *tracks;
        tracks = (uint32_t *) g_malloc0((found_album->no_tracks + 1) * sizeof (uint32_t));
        if (!tracks) {
            g_fprintf(stderr, _("ERROR: Failed memory allocation in albumAddTrackToAlbum()\n"));
            return;
        }
        found_album->no_tracks++;
        if (found_album->tracks != NULL) {
            memcpy(tracks, found_album->tracks, found_album->no_tracks * sizeof (uint32_t));
            free(found_album->tracks);
        }
        tracks[found_album->no_tracks - 1] = trackinfo->item_id; // This ID is only set once the track is on the device.
        found_album->tracks = tracks;
        ret = LIBMTP_Update_Album(DeviceMgr.device, found_album);
        g_free(tracks);
        found_album->tracks = NULL;
    } else {
        // New album.
        uint32_t *trackid;
        trackid = (uint32_t *) g_malloc0(sizeof (uint32_t));
        *trackid = trackinfo->item_id;
        albuminfo->tracks = trackid;
        albuminfo->no_tracks = 1;
        albuminfo->storage_id = DeviceMgr.devicestorage->id;
        ret = LIBMTP_Create_New_Album(DeviceMgr.device, albuminfo);
        g_free(trackid);
        albuminfo->tracks = NULL;
    }
    if (ret != 0) {

        if (Preferences.suppress_album_errors == FALSE) {
            if (AlbumErrorIgnore == FALSE) {
                displayError(_("Error creating or updating album.\n(This could be due to that your device does not support albums.)\n"));
                g_fprintf(stderr, _("Error creating or updating album.\n(This could be due to that your device does not support albums.)\n"));
            }
        }
        // Displayed the message once already per transfer...
        AlbumErrorIgnore = TRUE;
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    clearAlbumStruc(album_orig);
}

// ************************************************************************************************

/**
 * Add album artfile to be associated with an album.
 * @param album_id
 * @param filename
 */
void albumAddArt(guint32 album_id, gchar* filename) {
    LIBMTP_filesampledata_t *albumart;
    gint ret;
    uint64_t filesize;
    uint8_t *imagedata = NULL;
    struct stat statbuff;
    FILE* fd;

    if (stat(filename, &statbuff) == -1) {
        perror("stat");
        return;
    }
    filesize = (uint64_t) statbuff.st_size;
    imagedata = g_malloc(filesize * sizeof (uint8_t));
    if (imagedata == NULL) {
        g_fprintf(stderr, _("ERROR: Failed memory allocation in albumAddArt()\n"));
        return;
    }
    fd = fopen(filename, "r");
    if (fd == NULL) {
        g_fprintf(stderr, _("Couldn't open image file %s\n"), filename);
        g_free(imagedata);
        return;
    } else {
        size_t i = fread(imagedata, filesize, 1, fd);
        fclose(fd);
        if(i != 1 ){
            g_fprintf(stderr, _("Couldn't open image file %s\n"), filename);
            g_free(imagedata);
            return;
        }
    }

    albumart = LIBMTP_new_filesampledata_t();
    albumart->data = (gchar *) imagedata;
    albumart->size = filesize;
    albumart->filetype = find_filetype(basename(filename));

    ret = LIBMTP_Send_Representative_Sample(DeviceMgr.device, album_id, albumart);
    if (ret != 0) {
        g_fprintf(stderr, _("Couldn't send album art\n"));
        displayError(_("Couldn't send album art\n"));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    g_free(imagedata);
    albumart->data = NULL;

    // Adjust device storage.
    DeviceMgr.devicestorage->FreeSpaceInBytes -= filesize;
    DeviceMgr.devicestorage->FreeSpaceInObjects--;
    LIBMTP_destroy_filesampledata_t(albumart);
}

// ************************************************************************************************

/**
 * Retrieves the raw image data for the selected album
 * @return Pointer to the image data.
 */
LIBMTP_filesampledata_t * albumGetArt(LIBMTP_album_t* selectedAlbum) {
    LIBMTP_filesampledata_t *albumart = LIBMTP_new_filesampledata_t();
    gint ret;
    // Attempt to get some data
    ret = LIBMTP_Get_Representative_Sample(DeviceMgr.device, selectedAlbum->album_id, albumart);
    if (ret != 0) {
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
        LIBMTP_destroy_filesampledata_t(albumart);
        return NULL;
    }
    if (albumart == NULL) {
        // Something went wrong;
        return NULL;
    }
    return albumart;
}

// ************************************************************************************************

/**
 * Retrieves the raw image data for the selected album
 * @return Pointer to the image data.
 */
void albumDeleteArt(guint32 album_id) {
    LIBMTP_filesampledata_t *albumart = LIBMTP_new_filesampledata_t();
    // Attempt to send a null representative sample.
    albumart->data = NULL;
    albumart->size = 0;
    albumart->filetype = LIBMTP_FILETYPE_UNKNOWN;

    gint ret = LIBMTP_Send_Representative_Sample(DeviceMgr.device, album_id, albumart);
    if (ret != 0) {
        g_fprintf(stderr, _("Couldn't remove album art\n"));
        displayError(_("Couldn't remove album art\n"));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    LIBMTP_destroy_filesampledata_t(albumart);
}

// ************************************************************************************************

/**
 * Retrieve all Playlists from the device.
 * @return Linked list of all playlists.
 */
LIBMTP_playlist_t* getPlaylists(void) {

    if (devicePlayLists != NULL)
        clearDevicePlaylist(devicePlayLists);

    devicePlayLists = LIBMTP_Get_Playlist_List(DeviceMgr.device);
    if (devicePlayLists == NULL) {
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    return devicePlayLists;
}

// ************************************************************************************************

/**
 * Retrieve all Tracks from the device.
 * @return Linked list of all tracks.
 */
LIBMTP_track_t* getTracks(void) {
    if (deviceTracks != NULL)
        clearDeviceTracks(deviceTracks);

    deviceTracks = LIBMTP_Get_Tracklisting_With_Callback(DeviceMgr.device, NULL, NULL);
    if (deviceTracks == NULL) {
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    return deviceTracks;
}

// ************************************************************************************************

/**
 * Create a new playlist on the device.
 * @param playlistname
 */
void playlistAdd(gchar* playlistname) {

    LIBMTP_playlist_t *playlist = LIBMTP_new_playlist_t();

    playlist->name = g_strdup(playlistname);
    playlist->no_tracks = 0;
    playlist->tracks = NULL;
    playlist->parent_id = DeviceMgr.device->default_playlist_folder;
    playlist->storage_id = DeviceMgr.devicestorage->id;

    gint ret = LIBMTP_Create_New_Playlist(DeviceMgr.device, playlist);

    if (ret != 0) {
        displayError(_("Couldn't create playlist object\n"));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
    LIBMTP_destroy_playlist_t(playlist);
}

// ************************************************************************************************

/**
 * Delete the selected playlist from the device.
 * @param tmpplaylist
 */
void playlistDelete(LIBMTP_playlist_t * tmpplaylist) {
    guint res = LIBMTP_Delete_Object(DeviceMgr.device, tmpplaylist->playlist_id);
    if (res != 0) {
        displayError(_("Deleting playlist failed?\n"));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
}

// ************************************************************************************************

/**
 * Update the selected playlist with the new information.
 * @param tmpplaylist
 */
void playlistUpdate(LIBMTP_playlist_t * tmpplaylist) {
    guint res = LIBMTP_Update_Playlist(DeviceMgr.device, tmpplaylist);
    if (res != 0) {
        displayError(_("Updating playlist failed?\n"));
        LIBMTP_Dump_Errorstack(DeviceMgr.device);
        LIBMTP_Clear_Errorstack(DeviceMgr.device);
    }
}

// ************************************************************************************************

/**
 * Import a playlist into the current device.
 * @param filename Filename of the playlist to import.
 * @return The Playlist Name as stored in the file. Caller to free when no longer needed.
 */
gchar* playlistImport(gchar * filename) {
    FILE* fd;
    gchar* playlistname = NULL;
    gchar* fileString = NULL;
    gchar* needle = NULL;
    uint32_t *tracktmp = NULL;
    uint32_t fileobject = 0;
    gboolean ignorepath = Preferences.ignore_path_in_playlist_import;

    // Build basic playlist object
    LIBMTP_playlist_t *playlist = LIBMTP_new_playlist_t();
    playlist->name = NULL;
    playlist->no_tracks = 0;
    playlist->tracks = NULL;
    playlist->parent_id = DeviceMgr.device->default_playlist_folder;
    playlist->storage_id = DeviceMgr.devicestorage->id;

    // Load the file, and parse.
    fd = fopen(filename, "r");
    if (fd == NULL) {
        g_fprintf(stderr, _("Couldn't open playlist file %s\n"), filename);
        displayError(_("Couldn't open playlist file\n"));
        LIBMTP_destroy_playlist_t(playlist);
        return NULL;
    } else {
        fileString = g_malloc0(GMTP_MAX_STRING);
        // Read file until EOF
        while (fgets(fileString, GMTP_MAX_STRING, fd) != NULL) {
            // Strip any trailing '\n' from the string...
            fileString = g_strchomp(fileString);
            if (g_ascii_strncasecmp(fileString, "#GMTPPLA: ", 10) == 0) {
                // We have a playlist name marker...
                playlistname = g_strdup((fileString + 10));
                playlist->name = g_strdup(playlistname);
            } else {
                // We should have a file?
                // But ignore ANY line starting with a # as this is a comment line.
                if ((*fileString != '#') && (*fileString != '\0')) {
                    fileobject = getFileID(fileString, ignorepath);
                    if (fileobject != 0) {
                        // We have a file within our device!
                        playlist->no_tracks++;
                        if ((tracktmp = g_realloc(playlist->tracks, sizeof (uint32_t) * (playlist->no_tracks))) == NULL) {
                            g_fprintf(stderr, _("realloc in playlistImport failed\n"));
                            displayError(_("Updating playlist failed? 'realloc in playlistImport'\n"));
                            return NULL;
                        }
                        playlist->tracks = tracktmp;
                        playlist->tracks[(playlist->no_tracks - 1)] = fileobject;
                    }
                }
            }
        }
        g_free(fileString);
        fclose(fd);
    }
    if ((playlistname == NULL) && (playlist->no_tracks > 0)) {
        // We have some tracks, but no playlist name.
        // So derive the playlist name from the filename...
        playlistname = g_path_get_basename(filename);
        // Now chop off the file extension?
        needle = g_strrstr(playlistname, ".");
        if (needle != NULL) {
            *needle = '\0';
        }
        // And set the name...
        playlist->name = g_strdup(playlistname);
    }

    // If we have something?
    if ((playlistname != NULL) && (playlist->no_tracks > 0)) {
        // Store the playlist on the device...
        gint ret = LIBMTP_Create_New_Playlist(DeviceMgr.device, playlist);
        if (ret != 0) {
            displayError(_("Couldn't create playlist object\n"));
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
            g_free(playlistname);
            playlistname = NULL;
        } else {
            displayInformation(_("Playlist imported.\n"));
        }
    } else {
        // Let the user know we found zero tracks, so didn't bother to import it.
        displayInformation(_("Found no tracks within the playlist that exist on this device. Did not import the playlist.\n"));
        g_fprintf(stderr, _("Found no tracks within the playlist that exist on this device. Did not import the playlist.\n"));
        // Clean up the playlist name, since we don't need it.
        if (playlistname != NULL) {
            g_free(playlistname);
            playlistname = NULL;
        }
    }

    // Clean up our playlist data structure.
    LIBMTP_destroy_playlist_t(playlist);

    // Return to caller.
    return playlistname;
}

// ************************************************************************************************

/**
 * Export a playlist.
 * @param filename Filename of the playlist to import.
 * @param playlist Pointer to Playlist to export.
 */
void playlistExport(gchar * filename, LIBMTP_playlist_t * playlist) {
    FILE* fd;
    uint32_t numtracks = playlist->no_tracks;
    uint32_t *tracks = playlist->tracks;
    uint32_t trackid = 0;
    gchar* trackname = NULL;

    // Open the file to save it to...
    fd = fopen(filename, "w");
    if (fd == NULL) {
        g_fprintf(stderr, _("Couldn't save playlist file %s\n"), filename);
        displayError(_("Couldn't save playlist file\n"));
    } else {
        fprintf(fd, "#GMTPPLA: %s\n", playlist->name);
        fflush(fd);
        while (numtracks--) {
            trackid = *tracks++;
            // We now have our track id. Let's form the complete path to the file including the filename.
            // Then store that string in the file...
            trackname = getFullFilename(trackid);
            if (trackname != NULL) {
                fprintf(fd, "%s\n", trackname);
                g_free(trackname);
                trackname = NULL;
            }
        }
        fclose(fd);
    }
}

// ************************************************************************************************

/**
 * Format the current active device/storage partition.
 */
void formatStorageDevice() {
    if (DeviceMgr.deviceConnected) {
        guint res = LIBMTP_Format_Storage(DeviceMgr.device, DeviceMgr.devicestorage);
        if (res != 0) {
            displayError(_("Format Device failed?\n"));
            LIBMTP_Dump_Errorstack(DeviceMgr.device);
            LIBMTP_Clear_Errorstack(DeviceMgr.device);
        }
    } else {
        g_fprintf(stderr, ("formatStorageDevice: How did I get called?\n"));
    }
}

// ************************************************************************************************

/**
 * Add the assigned track to the nominated playlist.
 * @param playlist
 * @param track
 */
void playlistAddTrack(LIBMTP_playlist_t* playlist, LIBMTP_track_t* track) {
    LIBMTP_playlist_t* tmpplaylist = playlist;
    uint32_t *tmp = NULL;

    tmpplaylist->no_tracks++;
    // item_id = our track num... so append to tmpplaylist->tracks
    if ((tmp = g_realloc(tmpplaylist->tracks, sizeof (uint32_t) * (tmpplaylist->no_tracks))) == NULL) {
        g_fprintf(stderr, _("realloc in savePlayList failed\n"));
        displayError(_("Updating playlist failed? 'realloc in savePlayList'\n"));
        return;
    }
    tmpplaylist->tracks = tmp;
    tmpplaylist->tracks[(tmpplaylist->no_tracks - 1)] = track->item_id;
    playlistUpdate(tmpplaylist);
}


// ************************************************************************************************

/**
 * Remove the assigned track from a playlist.
 * @param playlist
 * @param track
 */
void playlistRemoveTrack(LIBMTP_playlist_t* playlist, LIBMTP_track_t* track, uint32_t instances) {
    LIBMTP_playlist_t* tmpplaylist = playlist;
    uint32_t *tmp = NULL;

    uint32_t numtracks = tmpplaylist->no_tracks;
    int32_t count;
    int32_t count2;
    // item_id = our track num... so remove to tmpplaylist->tracks
    if ((instances == MTP_PLAYLIST_ALL_INSTANCES) || (instances == MTP_PLAYLIST_FIRST_INSTANCE)) {
        for (count = 0; count < (int32_t) numtracks; count++) {
            if (tmpplaylist->tracks[count] == track->item_id) {
                // move all ones up one.
                for (count2 = count; count2 < (int32_t) numtracks; count2++) {
                    if ((count2 + 1) != (int32_t) numtracks) {
                        tmpplaylist->tracks[count2] = tmpplaylist->tracks[count2 + 1];
                    }
                }
                // exit the for loop if only doing the first instance.
                if (instances == MTP_PLAYLIST_FIRST_INSTANCE) {
                    count = numtracks;
                }
                tmpplaylist->no_tracks--;
            }
        }
    } else {
        for (count = numtracks - 1; count >= 0; count--) {
            if (tmpplaylist->tracks[count] == track->item_id) {
                // move all ones up one.
                for (count2 = count; count2 < (int32_t) numtracks; count2++) {
                    if ((count2 + 1) != (int32_t) numtracks) {
                        tmpplaylist->tracks[count2] = tmpplaylist->tracks[count2 + 1];
                    }
                }
                // exit the for loop if only doing the first instance.
                if (instances == MTP_PLAYLIST_LAST_INSTANCE) {
                    count = numtracks;
                }
                tmpplaylist->no_tracks--;
            }
        }
    }
    // And redo the memory allocation.
    if ((tmp = g_realloc(tmpplaylist->tracks, sizeof (uint32_t) * (tmpplaylist->no_tracks))) == NULL) {
        g_fprintf(stderr, _("realloc in savePlayList failed\n"));
        displayError(_("Updating playlist failed? 'realloc in savePlayList'\n"));
        return;
    }
    tmpplaylist->tracks = tmp;
    playlistUpdate(tmpplaylist);
}


// ************************************************************************************************

/**
 * Return the full filename including path of the selected MTP file object
 * @param trackid MTP file objectID
 * @return
 */
gchar* getFullFilename(uint32_t item_id) {
    gchar* fullfilename = NULL;
    gchar* tmpfilename = NULL;
    uint32_t parent_id = 0;
    LIBMTP_file_t* tmpfile = deviceFiles;
    LIBMTP_folder_t* tmpfolder = deviceFolders;

    // Find our file...
    while (tmpfile != NULL) {
        if (tmpfile->item_id == item_id) {
            fullfilename = g_strdup(tmpfile->filename);
            parent_id = tmpfile->parent_id;
            tmpfile = NULL;
        } else {
            tmpfile = tmpfile->next;
        }
    }
    // Let's see if we have a filename?
    if (fullfilename != NULL) {
        // Now let's prepend the parent folder names to it...
        while (tmpfolder != NULL) {
            tmpfolder = getCurrentFolderPtr(deviceFolders, parent_id);
            if (tmpfolder != NULL) {
                // We have something.
                tmpfilename = g_strdup_printf("%s/%s", tmpfolder->name, fullfilename);
                g_free(fullfilename);
                fullfilename = tmpfilename;
                parent_id = tmpfolder->parent_id;
            }
        }
    }
    return fullfilename;
}

// ************************************************************************************************

/**
 * Find the file within the device.
 * @param filename Name of the file to search for.
 * @param ignorepath Ignore any path information that may be present.
 * @return Object ID or 0 if not found.
 */
uint32_t getFileID(gchar* filename, gboolean ignorepath) {
    LIBMTP_file_t* files = deviceFiles;
    uint32_t folderID = 0;

    // Separate into filename and path...
    gchar* basefilename = g_path_get_basename(filename);
    gchar* dirfilename = g_path_get_dirname(filename);

    // If we are ignoring all path information, then simply scan all files for the filename.
    if (ignorepath == TRUE) {
        while (files != NULL) {
            // Ensure we only check if we are on the current storage device...
            if (files->storage_id == DeviceMgr.devicestorage->id) {
                // See if our filename is the same.
                if (g_ascii_strcasecmp(basefilename, files->filename) == 0) {
                    // We found our file...
                    g_free(basefilename);
                    g_free(dirfilename);
                    return files->item_id;
                }
            }
            files = files->next;
        }
    } else {
        // Lets find the folderid of the path we have, so it makes searching a lot easier...
        folderID = getFolderID(deviceFolders, dirfilename);
        if ((int32_t) folderID == -1) {
            // We don't have this path on the device, so no need to continue checking.
            g_free(basefilename);
            g_free(dirfilename);
            return 0;
        }
        while (files != NULL) {
            // Ensure we only check if we are on the current storage device AND the file is in the correct folder...
            if ((files->storage_id == DeviceMgr.devicestorage->id) && (files->parent_id == folderID)) {
                // See if our filename is the same.
                if (g_ascii_strcasecmp(basefilename, files->filename) == 0) {
                    // We found our file...
                    g_free(basefilename);
                    g_free(dirfilename);
                    return files->item_id;
                }
            }
            files = files->next;
        }
    }
    g_free(basefilename);
    g_free(dirfilename);
    return 0;
}

// ************************************************************************************************

/**
 * Find the folder within the device.
 * @param folderptr Folder Structure to search in.
 * @param foldername Name of the folder to find.
 * @return Object ID or -1 if not found.
 */
uint32_t getFolderID(LIBMTP_folder_t* folderptr, gchar* foldername) {
    gchar** pathcomponents;
    if (g_ascii_strcasecmp(foldername, ".") == 0) {
        // We have a root directory...
        return 0;
    }
    // Get the first component of the foldername.
    pathcomponents = g_strsplit(foldername, "/", 2);
    while (folderptr != NULL) {
        if (g_ascii_strcasecmp(pathcomponents[0], folderptr->name) == 0) {
            // We have found our path...
            // If we have of the path to process then...
            if (pathcomponents[1] != NULL) {
                return (getFolderID(folderptr->child, pathcomponents[1]));
            } else {
                return folderptr->folder_id;
            }
        } else {
            folderptr = folderptr->sibling;
        }
    }
    return -1;
}


// ************************************************************************************************

/**
 * Returns the string holding the full path name for the given folderid.
 * @param folderid - the selected path to be returned.
 * @return The full path name.
 */
gchar* getFullFolderPath(uint32_t folderid) {

    gchar* fullfilename = g_strdup("");
    gchar* tmpfilename = NULL;
    uint32_t parent_id = folderid;
    guint stringlength = 0;
    LIBMTP_folder_t* tmpfolder = deviceFolders;
    if(!Preferences.use_alt_access_method){
        // Legacy search since we have a complete folder structure in memory.
        while (tmpfolder != NULL) {
            tmpfolder = getCurrentFolderPtr(deviceFolders, parent_id);
            if (tmpfolder != NULL) {
                // We have something.
                tmpfilename = g_strdup_printf("%s/%s", tmpfolder->name, fullfilename);
                g_free(fullfilename);
                fullfilename = tmpfilename;
                parent_id = tmpfolder->parent_id;
            }
        }
        
       
    } else {
        // We are using alt access mode, so we need to manually query the device for
        // the parent folder.
        LIBMTP_file_t * f = LIBMTP_Get_Filemetadata(DeviceMgr.device, parent_id);
        while(f != NULL){
            // we have the entry
            tmpfilename = g_strdup_printf("%s/%s", f->filename, fullfilename);
            g_free(fullfilename);
            fullfilename = tmpfilename;
            parent_id = f->parent_id;
            LIBMTP_destroy_file_t(f);
            f = LIBMTP_Get_Filemetadata(DeviceMgr.device, parent_id);
        }
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
    return fullfilename;
}


// ************************************************************************************************

/**
 * Returns a list of files/folders that have been found on the device.
 * @param searchstring - string to search
 * @param searchfiles - search files/folder names.
 * @param searchmeta - search file metadata.
 * @return
 */
GSList *filesSearch(gchar *searchstring, gboolean searchfiles, gboolean searchmeta) {
    GSList *list = NULL;
    GPatternSpec *pspec = g_pattern_spec_new(searchstring);
    LIBMTP_file_t *files = deviceFiles;
    GSList *folderIDs = NULL;
    LIBMTP_track_t *tracks = deviceTracks;
    FileListStruc *filestruc = NULL;
    LIBMTP_track_t *trackinfo;
    gchar *tmpstring1 = NULL;
    gchar *tmpstring2 = NULL;
    gchar *tmpstring3 = NULL;
    gchar *tmpstring4 = NULL;

    uint32_t tmpFolderID = currentFolderID;

    // if using alt method, we cycle through all folders on the current storage device...
    if (Preferences.use_alt_access_method) {
        currentFolderID = 0;
        filesUpateFileList();
        files = deviceFiles;
        //buildFolderIDs(&folderIDs, deviceFolders);
    }

    if (searchfiles == TRUE) {
        // Search folders
        // ignore folder search in alt access mode...
        if (!Preferences.use_alt_access_method) {
            folderSearch(pspec, &list, deviceFolders);
        }
        // Search files.
        while (files != NULL) {
            if (files->storage_id == DeviceMgr.devicestorage->id) {
                if (files->filetype == LIBMTP_FILETYPE_FOLDER) {
                    // Add this folder to the list to be searched.
                    folderIDs = g_slist_append(folderIDs, &files->item_id);
                }
                // Make search case insensitive.
                tmpstring1 = g_utf8_strup(files->filename, -1);
                if (g_pattern_match_string(pspec, tmpstring1) == TRUE) {
                    g_free(tmpstring1);
                    tmpstring1 = NULL;
                    // We have found a matching string...
                    filestruc = g_malloc(sizeof (FileListStruc));
                    if (filestruc == NULL) {
                        g_fprintf(stderr, _("malloc in filesSearch failed\n"));
                        displayError(_("Failed searching? 'malloc in filesSearch'\n"));
                        return list;
                    }
                    filestruc->filename = g_strdup(files->filename);
                    filestruc->filesize = files->filesize;
                    if (files->filetype == LIBMTP_FILETYPE_FOLDER) {
                        filestruc->isFolder = TRUE;
                    } else {
                        filestruc->isFolder = FALSE;
                    }
                    filestruc->itemid = files->item_id;
                    filestruc->filetype = files->filetype;
                    filestruc->location = getFullFolderPath(files->parent_id);
                    list = g_slist_append(list, filestruc);
                } else if (searchmeta == TRUE) {
                    // Now if it's a track type file, eg OGG, WMA, MP3 or FLAC, get it's metadata.
                    // search case insensitive.
                    if ((files->filetype == LIBMTP_FILETYPE_MP3) ||
                            (files->filetype == LIBMTP_FILETYPE_OGG) ||
                            (files->filetype == LIBMTP_FILETYPE_FLAC) ||
                            (files->filetype == LIBMTP_FILETYPE_WMA)) {
                        trackinfo = LIBMTP_Get_Trackmetadata(DeviceMgr.device, files->item_id);
                        if (trackinfo != NULL) {
                            // search case insensitive.
                            if (tmpstring1 != NULL) {
                                g_free(tmpstring1);
                                tmpstring1 = NULL;
                            }
                            tmpstring1 = g_utf8_strup(trackinfo->album, -1);
                            tmpstring2 = g_utf8_strup(trackinfo->artist, -1);
                            tmpstring3 = g_utf8_strup(trackinfo->genre, -1);
                            tmpstring4 = g_utf8_strup(trackinfo->title, -1);
                            if ((g_pattern_match_string(pspec, tmpstring1) == TRUE) ||
                                    (g_pattern_match_string(pspec, tmpstring2) == TRUE) ||
                                    (g_pattern_match_string(pspec, tmpstring3) == TRUE) ||
                                    (g_pattern_match_string(pspec, tmpstring4) == TRUE)) {
                                // We have found a matching string...
                                filestruc = g_malloc(sizeof (FileListStruc));
                                if (filestruc == NULL) {
                                    if (tmpstring1 != NULL) {
                                        g_free(tmpstring1);
                                        tmpstring1 = NULL;
                                    }
                                    if (tmpstring2 != NULL) {
                                        g_free(tmpstring2);
                                        tmpstring2 = NULL;
                                    }
                                    if (tmpstring3 != NULL) {
                                        g_free(tmpstring3);
                                        tmpstring3 = NULL;
                                    }
                                    if (tmpstring4 != NULL) {
                                        g_free(tmpstring4);
                                        tmpstring4 = NULL;
                                    }
                                    g_fprintf(stderr, _("malloc in filesSearch failed\n"));
                                    displayError(_("Failed searching? 'malloc in filesSearch'\n"));
                                    return list;
                                }
                                filestruc->filename = g_strdup(files->filename);
                                filestruc->filesize = files->filesize;
                                filestruc->isFolder = FALSE;
                                filestruc->itemid = files->item_id;
                                filestruc->filetype = files->filetype;
                                filestruc->location = getFullFolderPath(files->parent_id);
                                list = g_slist_append(list, filestruc);
                            }
                            if (tmpstring1 != NULL) {
                                g_free(tmpstring1);
                                tmpstring1 = NULL;
                            }
                            if (tmpstring2 != NULL) {
                                g_free(tmpstring2);
                                tmpstring2 = NULL;
                            }
                            if (tmpstring3 != NULL) {
                                g_free(tmpstring3);
                                tmpstring3 = NULL;
                            }
                            if (tmpstring4 != NULL) {
                                g_free(tmpstring4);
                                tmpstring4 = NULL;
                            }
                            LIBMTP_destroy_track_t(trackinfo);
                        }
                    }
                }
                if (tmpstring1 != NULL) {
                    g_free(tmpstring1);
                    tmpstring1 = NULL;
                }
            }
            files = files->next;

            // Update the file list IFF we are using the alt access method
            if ((files == NULL) && (Preferences.use_alt_access_method)) {
                // reached the end of the current folder, so lets move onto the next folder.
                while (files == NULL) {
                    // exit searching for file if we run out of folders.
                    if (folderIDs == NULL) {
                        break;
                    }
                    currentFolderID = *((uint32_t*) folderIDs->data);
                    //printf("Searching folder: %d\n", currentFolderID);
                    filesUpateFileList();
                    folderIDs = folderIDs->next;
                    files = deviceFiles;
                }
            }
        }
    } else if (searchmeta == TRUE) {
        // Search using the Track information only.
        while (tracks != NULL) {
            // Make search case insensitive.
            tmpstring1 = g_utf8_strup(tracks->album, -1);
            tmpstring2 = g_utf8_strup(tracks->artist, -1);
            tmpstring3 = g_utf8_strup(tracks->genre, -1);
            tmpstring4 = g_utf8_strup(tracks->title, -1);
            if ((g_pattern_match_string(pspec, tmpstring1) == TRUE) ||
                    (g_pattern_match_string(pspec, tmpstring2) == TRUE) ||
                    (g_pattern_match_string(pspec, tmpstring3) == TRUE) ||
                    (g_pattern_match_string(pspec, tmpstring4) == TRUE)) {
                // We have found a matching string...
                filestruc = g_malloc(sizeof (FileListStruc));
                if (filestruc == NULL) {
                    if (tmpstring1 != NULL) {
                        g_free(tmpstring1);
                        tmpstring1 = NULL;
                    }
                    if (tmpstring2 != NULL) {
                        g_free(tmpstring2);
                        tmpstring2 = NULL;
                    }
                    if (tmpstring3 != NULL) {
                        g_free(tmpstring3);
                        tmpstring3 = NULL;
                    }
                    if (tmpstring4 != NULL) {
                        g_free(tmpstring4);
                        tmpstring4 = NULL;
                    }
                    g_fprintf(stderr, _("malloc in filesSearch failed\n"));
                    displayError(_("Failed searching? 'malloc in filesSearch'\n"));
                    return list;
                }
                filestruc->filename = g_strdup(tracks->filename);
                filestruc->filesize = tracks->filesize;
                filestruc->isFolder = FALSE;
                filestruc->itemid = tracks->item_id;
                filestruc->filetype = tracks->filetype;
                filestruc->location = getFullFolderPath(tracks->parent_id);
                list = g_slist_append(list, filestruc);
            }
            if (tmpstring1 != NULL) {
                g_free(tmpstring1);
                tmpstring1 = NULL;
            }
            if (tmpstring2 != NULL) {
                g_free(tmpstring2);
                tmpstring2 = NULL;
            }
            if (tmpstring3 != NULL) {
                g_free(tmpstring3);
                tmpstring3 = NULL;
            }
            if (tmpstring4 != NULL) {
                g_free(tmpstring4);
                tmpstring4 = NULL;
            }
            tracks = tracks->next;
        }
    }
    // restore the current folder ID.
    if (Preferences.use_alt_access_method) {
        currentFolderID = tmpFolderID;
    }
    return list;
}

// ************************************************************************************************

/**
 * Generate a list of all the folder IDs in the current storage pool.
 * @return 
 */
void buildFolderIDs(GSList **list, LIBMTP_folder_t * folderptr) {
    while (folderptr != NULL) {
        *(list) = g_slist_append(*(list), &folderptr->folder_id);
        if (folderptr->child != NULL) {
            buildFolderIDs(list, folderptr->child);
        }
        folderptr = folderptr->sibling;
    }
}


// ************************************************************************************************

/**
 * Search the folder hier for the foldername.
 * @param pspec - foldername to search
 * @param list - the GSList to append found items.
 * @param folderptr - the MTP folder struc to search.
 */
void folderSearch(GPatternSpec *pspec, GSList **list, LIBMTP_folder_t * folderptr) {
    FileListStruc *filestruc = NULL;
    gchar *tmpstring = NULL;
    while (folderptr != NULL) {
        // Make search case insensitive.
        tmpstring = g_utf8_strup(folderptr->name, -1);
        if (g_pattern_match_string(pspec, tmpstring) == TRUE) {
            // We have found a matching string...
            filestruc = g_malloc(sizeof (FileListStruc));
            if (filestruc == NULL) {
                if (tmpstring != NULL) {
                    g_free(tmpstring);
                    tmpstring = NULL;
                }
                g_fprintf(stderr, _("malloc in foldersearch failed\n"));
                displayError(_("Failed searching? 'malloc in foldersearch'\n"));
                return;
            }
            filestruc->filename = g_strdup(folderptr->name);
            filestruc->filesize = 0;
            filestruc->isFolder = TRUE;
            filestruc->itemid = folderptr->folder_id;
#if defined(LIBMTP_FILETYPE_FOLDER)
            filestruc->filetype = LIBMTP_FILETYPE_FOLDER;
#else
            filestruc->filetype = LIBMTP_FILETYPE_UNKNOWN;
#endif
            filestruc->location = getFullFolderPath(folderptr->parent_id);
            *(list) = g_slist_append(*(list), filestruc);
        }
        if (tmpstring != NULL) {
            g_free(tmpstring);
            tmpstring = NULL;
        }
        // Search child if present;
        if (folderptr->child != NULL) {
            folderSearch(pspec, list, folderptr->child);
        }
        folderptr = folderptr->sibling;
    }
}


// ************************************************************************************************

/**
 * Updates the parent FolderID for a given object within the device.
 * @param objectID - The object to be updated.
 * @param folderID - The new parent folder ID for the object.
 * @return 0 on success, any other value means failure.
 */
int setNewParentFolderID(uint32_t objectID, uint32_t folderID) {
    return LIBMTP_Set_Object_u32(DeviceMgr.device, objectID, LIBMTP_PROPERTY_ParentObject, folderID);
}
