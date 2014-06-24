/* 
 *
 *   File: about.c
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

// ************************************************************************************************

/**
 * Display the About Dialog Box.
 */
void displayAbout(void) {

#if GTK_CHECK_VERSION(2,12,0)
    GtkWidget *dialog;
    const char *authors[] = {
        "Development",
        "Darran Kartaschew (chewy509@mailcity.com)",
        "\nTranslations",
        "English - Darran Kartaschew",
        "English (Australia) - Darran Kartaschew",
        "Italian - Francesca Ciceri",
        "French - 'Coug'",
        "German - Laurenz Kamp",
        "Spanish - Google Translate",
        "Danish - Cai Andersen",
        "Russian - PuppyRus Linux team",
        NULL
    };
    dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), PACKAGE_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), PACKAGE_VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
            "Copyright 2009-2014, Darran Kartaschew\nReleased under the BSD License");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
            _("A simple MTP Client for Solaris 10\nand other UNIX / UNIX-like systems\n"));
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog),
            "gMTP License\n"
            "------------\n\n"
            "Copyright (C) 2009-2014, Darran Kartaschew.\n"
            "All rights reserved.\n\n"
            "Redistribution and use in source and binary forms, with or without "
            "modification, are permitted provided that the following conditions are met:\n\n"
            "*  Redistributions of source code must retain the above copyright notice, "
            "this list of conditions and the following disclaimer.\n\n"
            "*  Redistributions in binary form must reproduce the above copyright notice, "
            "this list of conditions and the following disclaimer in the documentation "
            "and/or other materials provided with the distribution. \n\n"
            "*  Neither the name of \"gMTP Development Team\" nor the names of its  "
            "contributors may be used to endorse or promote products derived from this  "
            "software without specific prior written permission. \n\n"
            "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" "
            "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE "
            "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
            "ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE "
            "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR "
            "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF "
            "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS "
            "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN "
            "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) "
            "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE "
            "POSSIBILITY OF SUCH DAMAGE.");
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), TRUE);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://gmtp.sourceforge.net");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), gdk_pixbuf_new_from_file(file_logo_png, NULL));
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
#else
    GtkWidget *dialog, *vbox, *label, *label2, *label3, *label4, *label5, *image;
    gchar *version_string;
    gchar *gtk_version_string;

    dialog = gtk_dialog_new_with_buttons(_("About gMTP"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_Close"), GTK_RESPONSE_CLOSE,
            NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

#if HAVE_GTK3 == 0
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
#endif
#if HAVE_GTK3 == 0
    vbox = gtk_vbox_new(FALSE, 5);
#else
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
    gtk_widget_show(vbox);
#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);
#endif

    // Add in our icon.
    image = gtk_image_new_from_file(file_logo_png);
    gtk_widget_show(image);
    gtk_container_add(GTK_CONTAINER(vbox), image);

    version_string = g_strconcat("<span size=\"xx-large\"><b>", PACKAGE_NAME, " v", PACKAGE_VERSION, "</b></span>", NULL);

    label = gtk_label_new(version_string);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(vbox), label);

    label2 = gtk_label_new(_("A simple MTP Client for Solaris 10\nand other UNIX / UNIX-like systems\n"));
    gtk_label_set_use_markup(GTK_LABEL(label2), TRUE);
    gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_CENTER);
    gtk_misc_set_padding(GTK_MISC(label2), 5, 0);
    gtk_widget_show(label2);
    gtk_container_add(GTK_CONTAINER(vbox), label2);

    label5 = gtk_label_new("http://gmtp.sourceforge.net\n");
    gtk_label_set_use_markup(GTK_LABEL(label5), TRUE);
    gtk_widget_show(label5);
    gtk_container_add(GTK_CONTAINER(vbox), label5);

    label3 = gtk_label_new(_("<small>Copyright 2009-2014, Darran Kartaschew</small>\n<small>Released under the BSD License</small>"));
    gtk_label_set_use_markup(GTK_LABEL(label3), TRUE);
    gtk_widget_show(label3);
    gtk_container_add(GTK_CONTAINER(vbox), label3);

    gtk_version_string = g_strdup_printf("<small>Built with GTK v%d.%d.%d</small>\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    label4 = gtk_label_new(gtk_version_string);
    gtk_label_set_use_markup(GTK_LABEL(label4), TRUE);
    gtk_widget_show(label4);
    gtk_container_add(GTK_CONTAINER(vbox), label4);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(version_string);
    g_free(gtk_version_string);
#endif
}
