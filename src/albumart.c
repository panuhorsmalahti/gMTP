/* 
 *
 *   File: albumart.c
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

// AlbumArt Dialog global pointers
GtkWidget *AlbumArtDialog;
GtkWidget *AlbumArtFilename;
GtkWidget *AlbumArtImage;
GtkWidget *buttonAlbumAdd;
GtkWidget *buttonAlbumDownload;
GtkWidget *buttonAlbumDelete;
GtkWidget *textboxAlbumArt;


// ************************************************************************************************

/**
 * Display the Add Album Art dialog box.
 * @return
 */
void displayAddAlbumArtDialog(void) {
    //Album_Struct* albumdetails;
    GtkWidget *hbox, *label;
    GtkWidget *buttonBox;
    LIBMTP_album_t *albuminfo = NULL;
    LIBMTP_album_t *album_orig = NULL;

    AlbumArtDialog = gtk_dialog_new_with_buttons(_("Album Art"), GTK_WINDOW(windowMain),
            (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            _("_Close"), GTK_RESPONSE_CLOSE,
            NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(AlbumArtDialog), GTK_RESPONSE_CLOSE);
    gtk_window_set_resizable(GTK_WINDOW(AlbumArtDialog), FALSE);
#if HAVE_GTK3 == 0
    gtk_dialog_set_has_separator(GTK_DIALOG(AlbumArtDialog), TRUE);
#endif

    // Set some nice 5px spacing.
#if HAVE_GTK3 == 0
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(AlbumArtDialog)->vbox), 10);
#else
    gtk_box_set_spacing(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(AlbumArtDialog))), 10);
#endif
#if HAVE_GTK3 == 0
    hbox = gtk_hbox_new(FALSE, 5);
#else
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
    gtk_widget_show(hbox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(AlbumArtDialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(AlbumArtDialog))), hbox);
#endif

    label = gtk_label_new(_("Album:"));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    // Now create the combo box.
#if HAVE_GTK3 == 0
    textboxAlbumArt = gtk_combo_box_new_text();
#else
    textboxAlbumArt = gtk_combo_box_text_new();
#endif
    gtk_widget_show(textboxAlbumArt);
    gtk_box_pack_start(GTK_BOX(hbox), textboxAlbumArt, TRUE, TRUE, 0);
    // Now add in our selection strings.
    albuminfo = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    // Better check to see if we actually have anything?
    if (albuminfo == NULL) {
        // we have no albums.
        displayInformation(_("No Albums available to set Album Art with. Either:\n1. You have no music files uploaded?\n2. Your device does not support Albums?\n3. Previous applications used to upload files do not autocreate albums for you or support the metadata for those files in order to create the albums for you?\n"));
        gtk_widget_destroy(AlbumArtDialog);
        AlbumArtImage = NULL;
        AlbumArtDialog = NULL;
        textboxAlbumArt = NULL;
        return;
    }

    album_orig = albuminfo;
    while (albuminfo != NULL) {
        if (albuminfo->name != NULL)
#if HAVE_GTK3 == 0
            gtk_combo_box_append_text(GTK_COMBO_BOX(textboxAlbumArt), albuminfo->name);
#else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(textboxAlbumArt), albuminfo->name);
#endif
        albuminfo = albuminfo->next;
    }
    // End add selection.
    gtk_combo_box_set_active(GTK_COMBO_BOX(textboxAlbumArt), 0);

    // Add in a image view of the current uploaded album art.
#if HAVE_GTK3 == 0
    AlbumArtImage = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG);
#else
    AlbumArtImage = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
#endif

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(AlbumArtDialog)->vbox), AlbumArtImage);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(AlbumArtDialog))), AlbumArtImage);
#endif
    gtk_widget_show(AlbumArtImage);


    // Add in the album art operations area.
#if HAVE_GTK3 == 0
    buttonBox = gtk_hbutton_box_new();
#else
    buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#endif
    gtk_box_set_spacing(GTK_BOX(buttonBox), 5);
    gtk_widget_show(buttonBox);

#if HAVE_GTK3 == 0
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(AlbumArtDialog)->vbox), buttonBox);
#else
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(AlbumArtDialog))), buttonBox);
#endif

    buttonAlbumAdd = gtk_button_new_with_mnemonic(_("Upload"));
    gtk_widget_show(buttonAlbumAdd);
    gtk_box_pack_start(GTK_BOX(buttonBox), buttonAlbumAdd, TRUE, TRUE, 0);

    buttonAlbumDelete = gtk_button_new_with_mnemonic(_("Remove"));
    gtk_widget_show(buttonAlbumDelete);
    gtk_box_pack_start(GTK_BOX(buttonBox), buttonAlbumDelete, TRUE, TRUE, 0);
    gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDelete), FALSE);

    buttonAlbumDownload = gtk_button_new_with_mnemonic(_("Download"));
    gtk_widget_show(buttonAlbumDownload);
    gtk_box_pack_start(GTK_BOX(buttonBox), buttonAlbumDownload, TRUE, TRUE, 0);
    gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDownload), FALSE);

    // Now, update the stock image with one from the selected album.
    AlbumArtUpdateImage(album_orig);

    g_signal_connect((gpointer) textboxAlbumArt, "changed",
            G_CALLBACK(on_albumtextbox_activate),
            NULL);

    g_signal_connect((gpointer) buttonAlbumAdd, "clicked",
            G_CALLBACK(on_buttonAlbumArtAdd_activate),
            NULL);

    g_signal_connect((gpointer) buttonAlbumDelete, "clicked",
            G_CALLBACK(on_buttonAlbumArtDelete_activate),
            NULL);

    g_signal_connect((gpointer) buttonAlbumDownload, "clicked",
            G_CALLBACK(on_buttonAlbumArtDownload_activate),
            NULL);

    gtk_dialog_run(GTK_DIALOG(AlbumArtDialog));

    gtk_widget_destroy(AlbumArtDialog);
    clearAlbumStruc(album_orig);

    //Clean up global pointers.
    AlbumArtImage = NULL;
    AlbumArtDialog = NULL;
    textboxAlbumArt = NULL;
}

// ************************************************************************************************

/**
 * Set the image in the AddAlbumDialog to be that supplied with the album information.
 * @return 
 */
void AlbumArtUpdateImage(LIBMTP_album_t* selectedAlbum) {
    LIBMTP_filesampledata_t *imagedata = NULL;
    GdkPixbufLoader *BufferLoader = NULL;
    GdkPixbuf *gdk_image = NULL;
    GdkPixbuf *gdk_image_scale = NULL;

    // Ensure our widget exists.
    if (AlbumArtImage == NULL)
        return;
    // Ensure we have a selected album.
    if (selectedAlbum == NULL) {
        AlbumArtSetDefault();
        return;
    }
    imagedata = albumGetArt(selectedAlbum);
    if (imagedata != NULL) {
        if (imagedata->size != 0) {
            // We have our image data.
            // Create a GdkPixbuf
            BufferLoader = gdk_pixbuf_loader_new();
            if (gdk_pixbuf_loader_write(BufferLoader, (const guchar*) imagedata->data, imagedata->size, NULL) == TRUE) {
                // Set the GtkImage to use that GdkPixbuf.
                gdk_image = gdk_pixbuf_loader_get_pixbuf(BufferLoader);
                gdk_pixbuf_loader_close(BufferLoader, NULL);
                gdk_image_scale = gdk_pixbuf_scale_simple(gdk_image, ALBUM_SIZE, ALBUM_SIZE, GDK_INTERP_BILINEAR);
                gtk_image_set_from_pixbuf(GTK_IMAGE(AlbumArtImage), gdk_image_scale);
                g_object_unref(gdk_image);
                g_object_unref(gdk_image_scale);

                // Set button states, so we can do stuff on the image.
                gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDownload), TRUE);
                gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDelete), TRUE);
            } else {
                AlbumArtSetDefault();
            }
        } else {
            AlbumArtSetDefault();
        }
        // Clean up the image buffer.
        LIBMTP_destroy_filesampledata_t(imagedata);
    } else {
        AlbumArtSetDefault();
    }
}

// ************************************************************************************************

/**
 * Set the album art to be the default image.
 */
void AlbumArtSetDefault(void) {
    //gtk_image_set_from_stock(GTK_IMAGE(AlbumArtImage), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG );
    GdkPixbuf *gdk_image = NULL;
    GdkPixbuf *gdk_image_scale = NULL;
#if HAVE_GTK3 == 0
    gdk_image = gtk_widget_render_icon(GTK_WIDGET(AlbumArtImage), GTK_STOCK_MISSING_IMAGE,
            GTK_ICON_SIZE_DIALOG, "gtk-missing-image");
#else 
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    gdk_image = gtk_icon_theme_load_icon(theme, "image-missing", ALBUM_SIZE, 0, NULL);
#endif
    gdk_image_scale = gdk_pixbuf_scale_simple(gdk_image, ALBUM_SIZE, ALBUM_SIZE, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(AlbumArtImage), gdk_image_scale);
    g_object_unref(gdk_image);
    g_object_unref(gdk_image_scale);

    // Disable the buttons, since we have a default image.
    gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDownload), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(buttonAlbumDelete), FALSE);
}


// ************************************************************************************************

/**
 * Callback to handle the Add Album Art menu option.
 * @param menuitem
 * @param user_data
 */
void on_editAddAlbumArt_activate(GtkMenuItem *menuitem, gpointer user_data) {
    // Get a filename of the album art.
    displayAddAlbumArtDialog();
} // end on_editAddAlbumArt_activate()

// ************************************************************************************************

/**
 * Callback to handle the select file button in the Add Album Art Dialog box.
 * @param button
 * @param user_data
 */
void on_buttonAlbumArtAdd_activate(GtkWidget *button, gpointer user_data) {
    // What we do here is display a find folder dialog, and save the resulting folder into the text wigdet and preferences item.
    //gchar *filename;
    gchar *filename = NULL;
    GtkWidget *FileDialog;
    FileDialog = gtk_file_chooser_dialog_new(_("Select Album Art File"),
            GTK_WINDOW(AlbumArtDialog), GTK_FILE_CHOOSER_ACTION_OPEN,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Open"), GTK_RESPONSE_ACCEPT,
            NULL);

    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(FileDialog), TRUE);

    // Set the default path to be the normal upload folder.
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileDialog), Preferences.fileSystemUploadPath->str);

    if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));

        if (filename != NULL) {
            // Upload the file to the selected album.
            gint selected = gtk_combo_box_get_active(GTK_COMBO_BOX(textboxAlbumArt));
            gint count = 0;
            LIBMTP_album_t *albumlist = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
            LIBMTP_album_t *albuminfo = albumlist;

            while (albuminfo != NULL) {
                if (count == selected) {
                    // Found our album, so update the image on the device, then update the display.
                    albumAddArt(albuminfo->album_id, filename);
                    AlbumArtUpdateImage(albuminfo);
                    clearAlbumStruc(albumlist);
                    g_free(filename);
                    gtk_widget_destroy(FileDialog);
                    return;
                }
                // Next album_entry
                albuminfo = albuminfo->next;
                count++;
            }
            // Set a default image as we didn't find our album.
            AlbumArtUpdateImage(NULL);
            clearAlbumStruc(albumlist);

            g_free(filename);
        }
    }
    gtk_widget_destroy(FileDialog);
} // end on_buttonAlbumArtAdd_activate()

// ************************************************************************************************

/**
 * Callback to handle removal of associated album art.
 * @param button
 * @param user_data
 */
void on_buttonAlbumArtDelete_activate(GtkWidget *button, gpointer user_data) {

    // Send a blank representation.
    gint selected = gtk_combo_box_get_active(GTK_COMBO_BOX(textboxAlbumArt));
    gint count = 0;
    LIBMTP_album_t *albumlist = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    LIBMTP_album_t *albuminfo = albumlist;

    while (albuminfo != NULL) {
        if (count == selected) {
            // Found our album, so update the image on the device, then update the display.
            albumDeleteArt(albuminfo->album_id);
            AlbumArtUpdateImage(NULL);
            clearAlbumStruc(albumlist);
            return;
        }
        // Next album_entry
        albuminfo = albuminfo->next;
        count++;
    }
    // Set a default image as we didn't find our album.
    AlbumArtUpdateImage(NULL);
    clearAlbumStruc(albumlist);

} // end on_buttonAlbumArtDelete_activate()

// ************************************************************************************************

/**
 * Retrieve the album art and attempt to save the file.
 * @param button
 * @param user_data
 */
void on_buttonAlbumArtDownload_activate(GtkWidget *button, gpointer user_data) {
    FILE* fd;
    gint selected = gtk_combo_box_get_active(GTK_COMBO_BOX(textboxAlbumArt));
    gint count = 0;
    GtkWidget *FileDialog = NULL;
    gchar *filename = NULL;
    LIBMTP_filesampledata_t *imagedata = NULL;
    LIBMTP_album_t *albumlist = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    LIBMTP_album_t *albuminfo = albumlist;

    // Scan our albums, looking for the correct one.
    while (albuminfo != NULL) {
        if (count == selected) {
            // Found our album, let's get our data..
            imagedata = albumGetArt(albuminfo);
            if (imagedata != NULL) {

                FileDialog = gtk_file_chooser_dialog_new(_("Save Album Art File"),
                        GTK_WINDOW(AlbumArtDialog), GTK_FILE_CHOOSER_ACTION_SAVE,
                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                        _("_Save"), GTK_RESPONSE_ACCEPT,
                        NULL);

                gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(FileDialog), TRUE);

                // Set the default path to be the normal download folder.
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileDialog),
                        Preferences.fileSystemDownloadPath->str);

                // Set a default name to be the album.JPG
                gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(FileDialog),
                        g_strdup_printf("%s.jpg", albuminfo->name));

                if (gtk_dialog_run(GTK_DIALOG(FileDialog)) == GTK_RESPONSE_ACCEPT) {
                    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileDialog));
                    if (filename != NULL) {
                        // The user has selected a file to save as, so do the deed.
                        fd = fopen(filename, "w");
                        if (fd == NULL) {
                            g_fprintf(stderr, _("Couldn't save image file %s\n"), filename);
                            displayError(_("Couldn't save image file\n"));
                        } else {
                            fwrite(imagedata->data, imagedata->size, 1, fd);
                            fclose(fd);
                        }
                        g_free(filename);
                    }
                }
                // Clean up our image data and dialog.
                LIBMTP_destroy_filesampledata_t(imagedata);
                gtk_widget_destroy(FileDialog);
            }
            clearAlbumStruc(albumlist);
            return;
        }
        // Next album_entry
        albuminfo = albuminfo->next;
        count++;
    }
    // Set a default image as we didn't find our album.
    clearAlbumStruc(albumlist);
    gtk_widget_destroy(FileDialog);
} // end on_buttonAlbumArtDownload_activate()

// ************************************************************************************************

/**
 * Update the Album Image in the Add Album Art Dialog Box.
 * @param menuitem
 * @param user_data
 */
void on_albumtextbox_activate(GtkComboBox *combobox, gpointer user_data) {
    gint selected = gtk_combo_box_get_active(combobox);
    gint count = 0;
    LIBMTP_album_t *albumlist = LIBMTP_Get_Album_List_For_Storage(DeviceMgr.device, DeviceMgr.devicestorage->id);
    LIBMTP_album_t *albuminfo = albumlist;

    while (albuminfo != NULL) {
        if (count == selected) {
            AlbumArtUpdateImage(albuminfo);
            clearAlbumStruc(albumlist);
            return;
        }
        // Text the album_entry
        albuminfo = albuminfo->next;
        count++;
    }
    // Set a default image
    AlbumArtUpdateImage(NULL);
    clearAlbumStruc(albumlist);
}

