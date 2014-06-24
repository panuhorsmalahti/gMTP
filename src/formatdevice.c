/* 
 *
 *   File: formatdevice.c
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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#include <gdk/gdkkeysyms-compat.h>
#endif
#include <libgen.h>
#include <libmtp.h>
#include <id3tag.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "progress.h"
#include "properties.h"
#include "preferences.h"
#include "playlist.h"
#include "albumart.h"
#include "formatdevice.h"

// Flag to check if the tread is running.
static volatile gboolean formatThreadWorking = TRUE;

// Widget for formatDevice progress bar.
GtkWidget *formatDialog_progressBar;

// ************************************************************************************************

/**
 * Creates the Format Device Dialog box
 * @return Widget of completed dialog box.
 */
GtkWidget* create_windowFormat(void) {
    GtkWidget* windowFormat;
    GtkWidget* label1;
    GtkWidget* vbox1;

    windowFormat = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gchar * winTitle;
    winTitle = g_strconcat(PACKAGE_NAME, NULL);
    gtk_window_set_title(GTK_WINDOW(windowFormat), winTitle);
    gtk_window_set_position(GTK_WINDOW(windowFormat), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_modal(GTK_WINDOW(windowFormat), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(windowFormat), GTK_WINDOW(windowMain));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(windowFormat), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(windowFormat), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_default_size(GTK_WINDOW(windowFormat), 200, 60);
    g_free(winTitle);
#if HAVE_GTK3 == 0
    vbox1 = gtk_vbox_new(FALSE, 0);
#else
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(windowFormat), vbox1);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 10);
    gtk_box_set_spacing(GTK_BOX(vbox1), 5);

    label1 = gtk_label_new("Formatting...");
    gtk_widget_show(label1);
    gtk_box_pack_start(GTK_BOX(vbox1), label1, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label1), 0, 5);
    gtk_misc_set_alignment(GTK_MISC(label1), 0, 0);

    formatDialog_progressBar = gtk_progress_bar_new();
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(formatDialog_progressBar), 0.05);
    gtk_widget_show(formatDialog_progressBar);
    gtk_box_pack_start(GTK_BOX(vbox1), formatDialog_progressBar, TRUE, TRUE, 0);

    return windowFormat;
}


// ************************************************************************************************

/**
 * Callback to format the current storage device.
 * @param menuitem
 * @param user_data
 */
void on_editFormatDevice_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(windowMain),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_YES_NO,
            _("Are you sure you want to format this device?"));
    gtk_window_set_title(GTK_WINDOW(dialog), _("Format Device"));
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(GTK_WIDGET(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_YES) {
        dialog = create_windowFormat();
        // Show progress dialog.
        gtk_widget_show_all(dialog);
        // Ensure GTK redraws the window.

        formatThreadWorking = TRUE;

#if GLIB_CHECK_VERSION(2,32,0)
	GThread *th = g_thread_new("format", (GThreadFunc) formatDevice_thread, NULL);
#else
        g_thread_create((GThreadFunc) formatDevice_thread, NULL, FALSE, NULL);
#endif
        while (formatThreadWorking) {
            while (gtk_events_pending())
                gtk_main_iteration();

            if (formatDialog_progressBar != NULL) {
                gtk_progress_bar_pulse(GTK_PROGRESS_BAR(formatDialog_progressBar));
                g_usleep(G_USEC_PER_SEC * 0.1);
            }

        }
        // The worker thread has finished so let's continue.
#if GLIB_CHECK_VERSION(2,32,0)
	g_thread_unref(th);
#endif
        // Disconnect and reconnect the device.
        on_deviceConnect_activate(NULL, NULL);
        // Sleep for 2 secs to allow the device to settle itself
        g_usleep(G_USEC_PER_SEC * 2);
        on_deviceConnect_activate(NULL, NULL);
        // Close progress dialog.
        gtk_widget_hide(dialog);
        gtk_widget_destroy(dialog);
        formatDialog_progressBar = NULL;
    }
    //
} // end on_editFormatDevice_activate()

// ************************************************************************************************

/**
 * Worker thread for on_editFormatDevice_activate();
 */
void formatDevice_thread(void) {
    formatStorageDevice();
    // Add a 5 sec wait so the device has time to settle itself.
    g_usleep(G_USEC_PER_SEC * 5);
    formatThreadWorking = FALSE;
    g_thread_exit(NULL);
}

