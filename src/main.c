/* 
 *
 *   File: main.c
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

#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3
#include <gio/gio.h>
#else 
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#endif
#include <libmtp.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <id3tag.h>

#include "main.h"
#include "interface.h"
#include "callbacks.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"

// Global Widgets needed by various functions.
GtkWidget *windowMain;
GtkWidget *windowStatusBar;
GtkWidget *toolbuttonConnect;
GtkWidget *treeviewFiles;
GtkWidget *treeviewFolders;

// The device which we are connected with.
Device_Struct DeviceMgr;

// Device structures for files, folders, etc.
LIBMTP_file_t *deviceFiles = NULL;
LIBMTP_folder_t *deviceFolders = NULL;
LIBMTP_track_t *deviceTracks = NULL;
LIBMTP_playlist_t *devicePlayLists = NULL;
uint32_t currentFolderID = 0;
int32_t addTrackPlaylistID = GMTP_REQUIRE_PLAYLIST;

GQueue *stackFolderIDs = NULL;
GQueue *stackFolderNames = NULL;

// Paths to the application, and images used within the application.
gchar *applicationpath = NULL;
gchar *file_logo_png = NULL;
gchar *file_icon48_png = NULL;
gchar *file_icon16_png = NULL;
gchar *file_about_png = NULL;
gchar *file_format_png = NULL;
// File view Icons
gchar *file_audio_png = NULL;
gchar *file_video_png = NULL;
gchar *file_playlist_png = NULL;
gchar *file_album_png = NULL;
gchar *file_textfile_png = NULL;
gchar *file_generic_png = NULL;
gchar *file_folder_png = NULL;
gchar *file_image_png = NULL;


// ************************************************************************************************

/**
 * Main Function
 * @param argc - Number of arguments to the function
 * @param argv - Argument list
 * @return - exit code
 */
int main(int argc, char *argv[]) {
    setFilePaths(argc, argv);

#if GLIB_CHECK_VERSION(2,32,0)
    // only need g_thread_init on versions less than 2.32
#else
    g_thread_init(NULL);
#endif

#if HAVE_GTK3 == 0
    gtk_set_locale();
#endif
    g_set_prgname(PACKAGE_NAME);
    g_set_application_name(PACKAGE_NAME);

    gtk_init(&argc, &argv);
    
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE_NAME, g_strconcat(applicationpath, "/../share/locale", NULL));
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    textdomain(PACKAGE_NAME);
#endif

    // Initialise libmtp library
    LIBMTP_Init();

    // Create our main window for the application.
    windowMain = create_windowMain();
    gtk_widget_show(windowMain);

    //Set default state for application
    DeviceMgr.deviceConnected = FALSE;
    statusBarSet(_("No device attached"));
    SetToolbarButtonState(DeviceMgr.deviceConnected);

    setupPreferences();

    // If preference is to auto-connect then attempt to do so.
    if (Preferences.attemptDeviceConnectOnStart == TRUE)
        on_deviceConnect_activate(NULL, NULL);

    // If we do have a connected device, then do a rescan operation to fill in the filelist.
    if (DeviceMgr.deviceConnected == TRUE)
        deviceRescan();

    gtk_main();

    return EXIT_SUCCESS;
} // end main()

// ************************************************************************************************

/**
 * setFilePaths - set paths for image used within gMTP
 * @param argc
 * @param argv
 */
void setFilePaths(int argc, char *argv[]) {
    // Get our executable location.
    applicationpath = getRuntimePath(argc, argv);

    // Set our image locations.
    file_logo_png = g_strdup_printf("%s/../share/gmtp/logo.png", applicationpath);
    file_icon48_png = g_strdup_printf("%s/../share/gmtp/gmtpicon.png", applicationpath);
    file_icon16_png = g_strdup_printf("%s/../share/gmtp/icon-16.png", applicationpath);
    file_about_png = g_strdup_printf("%s/../share/gmtp/stock-about-16.png", applicationpath);
    file_format_png = g_strdup_printf("%s/../share/gmtp/view-refresh.png", applicationpath);

    file_audio_png = g_strdup_printf("%s/../share/gmtp/audio-x-mpeg.png", applicationpath);
    file_video_png = g_strdup_printf("%s/../share/gmtp/video-x-generic.png", applicationpath);
    file_playlist_png = g_strdup_printf("%s/../share/gmtp/audio-x-mp3-playlist.png", applicationpath);
    file_album_png = g_strdup_printf("%s/../share/gmtp/media-cdrom-audio.png", applicationpath);
    file_textfile_png = g_strdup_printf("%s/../share/gmtp/text-plain.png", applicationpath);
    file_generic_png = g_strdup_printf("%s/../share/gmtp/empty.png", applicationpath);
    file_folder_png = g_strdup_printf("%s/../share/gmtp/folder.png", applicationpath);
    file_image_png = g_strdup_printf("%s/../share/gmtp/image-x-generic.png", applicationpath);
} // end setFilePaths()

// ************************************************************************************************

/**
 * getRuntimePath - Returns the path which the application was run from
 * @param argc
 * @param argv
 * @return pointer to string with location of the binary.
 */
gchar *getRuntimePath(int argc, char *argv[]) {

    gchar *fullpath;
    gchar *filepath;
    gchar *foundpath = NULL;
    const char delimit[] = ";:";
    gchar *token;

    if (g_ascii_strcasecmp(PACKAGE_NAME, argv[0]) == 0) {
        // list each directory individually.
        fullpath = g_strdup(getenv("PATH"));
        token = strtok(fullpath, delimit);
        while ((token != NULL) && (foundpath == NULL)) {
            // Now test to see if we have it here...
            filepath = g_strdup(token);
            filepath = g_strconcat(filepath, "/", PACKAGE_NAME, NULL);
            if (access(filepath, F_OK) != -1) {
                foundpath = g_strdup(token);
            }
            token = strtok(NULL, delimit);
            g_free(filepath);
        }
    } else {
        // We were started with full file path.
        foundpath = g_strdup(dirname(argv[0]));
    }
    if (argc == 3) {
        // We have some other options, lets check for --datapath
        if (g_ascii_strcasecmp("--datapath", argv[1]) == 0) {
            // our first argument is --datapath, so set the path to argv[2];
            foundpath = g_strdup(argv[2]);
        }
    }
    return foundpath;
} // end getRuntimePath
