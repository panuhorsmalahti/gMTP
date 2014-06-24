/* 
 *
 *   File: progress.c
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

// Public indicator if the dialog has been killed by the user.
gboolean progressDialog_killed = FALSE;

// Widget for Progress Bar Dialog box.
GtkWidget *progressDialog;
GtkWidget *progressDialog_Text;
GtkWidget *progressDialog_Bar;
gchar *progressDialog_filename;

// ************************************************************************************************

/**
 * Create a Upload/Download Progress Window.
 * @param msg Default message to be displayed.
 * @return
 */
GtkWidget* create_windowProgressDialog(gchar* msg) {
    GtkWidget *window1;
    GtkWidget *vbox1;
    GtkWidget *hbox1;
    GtkWidget *label_FileProgress;
    GtkWidget *label1;
    GtkWidget *cancelButton;
    GtkWidget *progressbar_Main;

    window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gchar * winTitle;
    winTitle = g_strconcat(PACKAGE_NAME, NULL);
    gtk_window_set_title(GTK_WINDOW(window1), winTitle);
    gtk_window_set_position(GTK_WINDOW(window1), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_modal(GTK_WINDOW(window1), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window1), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(window1), GTK_WINDOW(windowMain));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window1), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window1), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_free(winTitle);
#if HAVE_GTK3 == 0
    vbox1 = gtk_vbox_new(FALSE, 0);
#else
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(window1), vbox1);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 10);
    gtk_box_set_spacing(GTK_BOX(vbox1), 5);

    label1 = gtk_label_new(NULL);
    winTitle = g_strconcat("<b><big>", msg, "</big></b>", NULL);
    gtk_label_set_markup(GTK_LABEL(label1), winTitle);
    gtk_widget_show(label1);
    gtk_box_pack_start(GTK_BOX(vbox1), label1, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label1), 0, 5);
    gtk_misc_set_alignment(GTK_MISC(label1), 0, 0);
    g_free(winTitle);

    label_FileProgress = gtk_label_new(_("file = ( x / x ) x %"));
    gtk_label_set_line_wrap(GTK_LABEL(label_FileProgress), TRUE);
    
#if GTK_CHECK_VERSION(2,10,0)
    // If we can, word wrap the label.
    gtk_label_set_line_wrap_mode(GTK_LABEL(label_FileProgress), PANGO_WRAP_WORD);
#endif
    gtk_widget_set_size_request(label_FileProgress, 320, -1);
    gtk_widget_show(label_FileProgress);
    gtk_box_pack_start(GTK_BOX(vbox1), label_FileProgress, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label_FileProgress), 0, 5);

    progressbar_Main = gtk_progress_bar_new();
    gtk_widget_show(progressbar_Main);
    gtk_box_pack_start(GTK_BOX(vbox1), progressbar_Main, TRUE, TRUE, 0);

    // Insert a cancel button.
#if HAVE_GTK3 == 0
    hbox1 = gtk_hbox_new(FALSE, 0);
#else
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_show(hbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
    cancelButton = gtk_button_new_with_mnemonic(_("_Cancel"));
    gtk_widget_show(cancelButton);
    gtk_box_pack_end(GTK_BOX(hbox1), cancelButton, FALSE, FALSE, 0);

    progressDialog = window1;
    progressDialog_Text = label_FileProgress;
    progressDialog_Bar = progressbar_Main;

    g_signal_connect((gpointer) cancelButton, "clicked",
            G_CALLBACK(on_progressDialog_Cancel),
            NULL);

    return window1;
}

// ************************************************************************************************

/**
 * Display the File Progress Window.
 * @param msg Message to be displayed.
 */
void displayProgressBar(gchar *msg) {
    // No idea how this could come about, but we should take it into account so we don't have a memleak
    // due to recreating the window multiple times.
    if (progressDialog != NULL) {
        destroyProgressBar();
    }
    // create our progress window.
    progressDialog = create_windowProgressDialog(msg);
    progressDialog_killed = FALSE;
    // Attach a callback to get notification that it has closed.
    g_signal_connect((gpointer) progressDialog, "destroy",
            G_CALLBACK(on_progressDialog_Close),
            NULL);

    // Show the progress window.
    gtk_widget_show_all(progressDialog);
}

// ************************************************************************************************

/**
 * Destroy the Progress Window.
 */
void destroyProgressBar(void) {
    if (progressDialog_killed == FALSE) {
        gtk_widget_hide(progressDialog);
        gtk_widget_destroy(progressDialog);
    }
    g_free(progressDialog_filename);
    progressDialog = NULL;
    progressDialog_Text = NULL;
    progressDialog_Bar = NULL;
    progressDialog_killed = FALSE;
}

// ************************************************************************************************

/**
 * Update the filename displayed in the Progress Window.
 * @param filename The filename to be displayed in the dialog
 */
void setProgressFilename(gchar* filename) {
    progressDialog_filename = g_strdup(filename);
}

// ************************************************************************************************

/**
 * Callback to handle updating the Progress Window.
 * @param sent
 * @param total
 * @param data
 * @return
 */
int fileprogress(const uint64_t sent, const uint64_t total, void const * const data) {
    gchar* tmp_string;
    gchar* tmp_sent;
    gchar* tmp_total;
    gint percent = (sent * 100) / total;

    // See if our dialog box was killed, and if so, just return which also kill our download/upload...
    if (progressDialog_killed == TRUE)
        return TRUE;

    // Now update the progress dialog.
    if (progressDialog != NULL) {
        tmp_sent = calculateFriendlySize(sent);
        tmp_total = calculateFriendlySize(total);

        if (progressDialog_filename != NULL) {
            tmp_string = g_strdup_printf(_("%s\n%s of %s (%d%%)"), progressDialog_filename,
                    tmp_sent, tmp_total, percent);
        } else {
            tmp_string = g_strdup_printf(_("%s of %s (%d%%)"),
                    tmp_sent, tmp_total, percent);
        }
        gtk_label_set_text(GTK_LABEL(progressDialog_Text), tmp_string);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressDialog_Bar), (double) percent / 100.00);

        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
        g_free(tmp_string);
        g_free(tmp_sent);
        g_free(tmp_total);
    }
    return 0;
}



// ************************************************************************************************

/**
 * Callback to handle when a user closes the Progress Dialog box, via the X button.
 * @param window
 * @param user_data
 */
void on_progressDialog_Close(GtkWidget *window, gpointer user_data) {
    // Set the global flag that the user has done it.
    progressDialog_killed = TRUE;
} // end on_progressDialog_Close()

// ************************************************************************************************

/**
 * Callback to handle when a user presses the Cancel button in the Progress Dialog box
 * @param window
 * @param user_data
 */
void on_progressDialog_Cancel(GtkWidget *button, gpointer user_data) {
    // Set the global flag that the user has done it.
    on_progressDialog_Close(NULL, NULL);

    // Destroy the dialog box.
    gtk_widget_hide(progressDialog);
    gtk_widget_destroy(progressDialog);
} // end on_progressDialog_Cancel()
