/* 
 *
 *   File: main.h
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

#ifndef _MAIN_H
#define _MAIN_H

#ifdef  __cplusplus
extern "C" {
#endif

    // Main Device information struct.

    typedef struct {
        gboolean deviceConnected;
        gint numrawdevices;
        gint rawdeviceID;
        gint storagedeviceID;

        LIBMTP_raw_device_t * rawdevices;
        LIBMTP_mtpdevice_t *device;
        LIBMTP_devicestorage_t *devicestorage;
        LIBMTP_error_number_t err;

        GString *devicename;
        GString *manufacturername;
        GString *modelname;
        GString *serialnumber;
        GString *deviceversion;
        GString *syncpartner;
        GString *sectime;
        GString *devcert;

        // Raw device
        GString *Vendor;
        GString *Product;
        uint32_t VendorID;
        uint32_t ProductID;
        uint32_t DeviceID;
        uint32_t BusLoc;

        uint16_t *filetypes;
        uint16_t filetypes_len;
        uint8_t maxbattlevel;
        uint8_t currbattlevel;

    } Device_Struct;

    // Main Window Widgets.
    GtkWidget *windowMain;
    GtkWidget *scrolledwindowMain;

    GtkWidget *windowStatusBar;
    GtkWidget *toolbuttonConnect;
    GtkWidget *treeviewFiles;
    GtkWidget *treeviewFolders;
    // Folder view;
    GtkWidget *scrolledwindowFolders;
    GtkTreeSelection *folderSelection;

    // Device information struct
    Device_Struct DeviceMgr;

    // File/Folder/Track/Playlist pointers
    LIBMTP_file_t *deviceFiles;
    LIBMTP_folder_t *deviceFolders;
    LIBMTP_track_t *deviceTracks;
    LIBMTP_playlist_t *devicePlayLists;
    uint32_t currentFolderID; // This is the ID of the current folder....
    int32_t addTrackPlaylistID;

    GQueue *stackFolderIDs;
    GQueue *stackFolderNames;

    // Icon file locations.
    gchar *file_logo_png;
    gchar *file_icon48_png;
    gchar *file_icon16_png;
    gchar *file_about_png;
    gchar *file_format_png;
    // File view Icons
    gchar *file_audio_png;
    gchar *file_video_png;
    gchar *file_playlist_png;
    gchar *file_album_png;
    gchar *file_textfile_png;
    gchar *file_generic_png;
    gchar *file_folder_png;
    gchar *file_image_png;

    // Misc Utility function;
    void setFilePaths(int argc, char *argv[]);
    gchar *getRuntimePath(int argc, char *argv[]);

    // Common magic numbers.

#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824
    
#define GMTP_REQUIRE_PLAYLIST -2
#define GMTP_NO_PLAYLIST -1
#define GMTP_MAX_STRING 8192

#ifdef  __cplusplus
}
#endif

#endif  /* _MAIN_H */
