/* 
 *
 *   File: properties.c
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

GtkWidget *windowPropDialog;

// ************************************************************************************************

/**
 * Create the Properties Dialog Box.
 * @return
 */
GtkWidget* create_windowProperties() {
    GtkWidget *windowDialog;
    GtkWidget *windowNotebook;
    GtkWidget *vbox1;
    GtkWidget *vbox2;
    GtkWidget *alignment2;
    GtkWidget *table2;
    GtkWidget *label15;
    GtkWidget *labelName;
    GtkWidget *labelModel;
    GtkWidget *labelSerial;
    GtkWidget *labelBattery;
    GtkWidget *labelManufacturer;
    GtkWidget *labelDeviceVer;
    GtkWidget *label26;
    GtkWidget *label29;
    GtkWidget *label28;
    GtkWidget *label27;
    GtkWidget *label17;
    GtkWidget *label25;
    GtkWidget *label18;
    GtkWidget *label19;
    GtkWidget *label16;
    GtkWidget *labelStorage;
    GtkWidget *labelSupportedFormat;
    GtkWidget *labelSecTime;
    GtkWidget *labelSyncPartner;
    GtkWidget *label2;
    GtkWidget *alignment1;
    GtkWidget *table1;
    GtkWidget *label3;
    GtkWidget *label4;
    GtkWidget *label5;
    GtkWidget *label6;
    GtkWidget *label7;
    GtkWidget *label8;
    GtkWidget *labelDeviceVendor;
    GtkWidget *labelDeviceProduct;
    GtkWidget *labelVenID;
    GtkWidget *labelProdID;
    GtkWidget *labelBusLoc;
    GtkWidget *labelDevNum;
    GtkWidget *label1;
    GtkWidget *hbox2;
    GtkWidget *buttonClose;

    GtkWidget *label50;

    GString *tmp_string2;
    gchar *tmp_string = NULL;

    windowDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gchar * winTitle;
    winTitle = g_strconcat(DeviceMgr.devicename->str, _(" Properties"), NULL);
    gtk_window_set_title(GTK_WINDOW(windowDialog), winTitle);
    gtk_window_set_modal(GTK_WINDOW(windowDialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(windowDialog), GTK_WINDOW(windowMain));
    gtk_window_set_position(GTK_WINDOW(windowDialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable(GTK_WINDOW(windowDialog), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(windowDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_free(winTitle);

    // Main Window
#if HAVE_GTK3 == 0
    vbox1 = gtk_vbox_new(FALSE, 5);
#else
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(windowDialog), vbox1);

    // Device Properties Pane
    label2 = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label2), _("<b>MTP Device Properties</b>"));
    gtk_widget_show(label2);

    alignment2 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment2);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment2), 0, 0, 12, 0);

    // Raw Device Information Pane
    label1 = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label1), _("<b>Raw Device Information</b>"));
    gtk_widget_show(label1);

    alignment1 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 0, 0, 12, 0);

    // Build the Notebook.
    windowNotebook = gtk_notebook_new();
    gtk_widget_show(windowNotebook);
    gtk_notebook_append_page(GTK_NOTEBOOK(windowNotebook), alignment2, label2);
    gtk_notebook_append_page(GTK_NOTEBOOK(windowNotebook), alignment1, label1);
    gtk_container_add(GTK_CONTAINER(vbox1), windowNotebook);

    // Start the Device Properties Pane.
#if HAVE_GTK3 == 0
    table2 = gtk_table_new(10, 2, FALSE);
    gtk_widget_show(table2);
    gtk_container_add(GTK_CONTAINER(alignment2), table2);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table2), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table2), 5);
#else
    table2 = gtk_grid_new();
    gtk_widget_show(table2);
    gtk_container_add(GTK_CONTAINER(alignment2), table2);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 5);
    gtk_grid_set_row_spacing(GTK_GRID(table2), 5);
    gtk_grid_set_column_spacing(GTK_GRID(table2), 5);
#endif


    label15 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label15), _("<b>Name:</b>"));
    gtk_widget_show(label15);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label15, 0, 1, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label15, 0, 0, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label15), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label15), 0, 1);

    labelName = gtk_label_new(("label20"));
    gtk_widget_show(labelName);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelName, 1, 2, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelName, 1, 0 , 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelName), 0, 0.5);

    labelModel = gtk_label_new(("label21"));
    gtk_widget_show(labelModel);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelModel, 1, 2, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else 
    gtk_grid_attach(GTK_GRID(table2), labelModel, 1, 1, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelModel), 0, 0.5);

    labelSerial = gtk_label_new(("label22"));
    gtk_widget_show(labelSerial);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelSerial, 1, 2, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelSerial, 1, 2, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelSerial), 0, 0.5);

    labelBattery = gtk_label_new(("label24"));
    gtk_widget_show(labelBattery);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelBattery, 1, 2, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelBattery, 1, 5, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelBattery), 0, 0.5);

    labelManufacturer = gtk_label_new(("label23"));
    gtk_widget_show(labelManufacturer);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelManufacturer, 1, 2, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelManufacturer, 1, 4, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelManufacturer), 0, 0.5);

    labelDeviceVer = gtk_label_new(("label26"));
    gtk_widget_show(labelDeviceVer);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelDeviceVer, 1, 2, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelDeviceVer, 1, 3, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelDeviceVer), 0, 0.5);

    label26 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label26), _("<b>Model Number:</b>"));
    gtk_widget_show(label26);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label26, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label26, 0, 1, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label26), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label26), 0, 1);

    label29 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label29), _("<b>Serial Number:</b>"));
    gtk_widget_show(label29);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label29, 0, 1, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label29, 0, 2, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label29), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label29), 0, 1);

    label28 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label28), _("<b>Device Version:</b>"));
    gtk_widget_show(label28);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label28, 0, 1, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label28, 0, 3, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label28), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label28), 0, 1);

    label27 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label27), _("<b>Manufacturer:</b>"));
    gtk_widget_show(label27);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label27, 0, 1, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label27, 0, 4, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label27), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label27), 0, 1);

    label17 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label17), _("<b>Battery Level:</b>"));
    gtk_widget_show(label17);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label17, 0, 1, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label17, 0, 5, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label17), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label17), 0, 1);

    label25 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label25), _("<b>Storage:</b>"));
    gtk_widget_show(label25);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label25, 0, 1, 6, 7,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label25, 0, 6, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label25), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label25), 0, 1);
#if HAVE_GTK3 == 0
    vbox2 = gtk_vbox_new(FALSE, 0);
#else
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show(vbox2);
    //gtk_container_add (GTK_CONTAINER (windowDialog), vbox1);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), vbox2, 0, 1, 7, 8,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), vbox2, 0, 7, 1, 1);
#endif
    label18 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label18), _("<b>Supported Formats:</b>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label18, FALSE, TRUE, 0);
    gtk_widget_show(label18);
    gtk_label_set_justify(GTK_LABEL(label18), GTK_JUSTIFY_RIGHT);
    //gtk_misc_set_alignment (GTK_MISC (label18), 1, 0);
    gtk_label_set_line_wrap(GTK_LABEL(label18), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label18), 0, 1);


    label50 = gtk_label_new((""));
    gtk_box_pack_start(GTK_BOX(vbox2), label50, FALSE, TRUE, 0);
    gtk_widget_show(label50);


    label19 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label19), _("<b>Secure Time:</b>"));
    gtk_widget_show(label19);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label19, 0, 1, 8, 9,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label19, 0, 8, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label19), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label19), 0, 1);

    label16 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label16), _("<b>Sync Partner:</b>"));
    gtk_widget_show(label16);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), label16, 0, 1, 9, 10,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), label16, 0, 9, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label16), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label16), 0, 1);

    labelStorage = gtk_label_new(("label30"));
    gtk_widget_show(labelStorage);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelStorage, 1, 2, 6, 7,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelStorage, 1, 6, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelStorage), 0, 0.5);

    labelSupportedFormat = gtk_label_new(("label31"));
    
    gtk_widget_show(labelSupportedFormat);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelSupportedFormat, 1, 2, 7, 8,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelSupportedFormat, 1, 7, 1, 1);
#endif
    gtk_label_set_line_wrap(GTK_LABEL(labelSupportedFormat), FALSE);
    gtk_misc_set_alignment(GTK_MISC(labelSupportedFormat), 0, 0.5);

    labelSecTime = gtk_label_new(("label32"));
    gtk_widget_show(labelSecTime);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelSecTime, 1, 2, 8, 9,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelSecTime, 1, 8, 1, 1);
#endif
    gtk_label_set_line_wrap(GTK_LABEL(labelSecTime), TRUE);
    gtk_misc_set_alignment(GTK_MISC(labelSecTime), 0, 0.5);

    labelSyncPartner = gtk_label_new(("label33"));
    gtk_widget_show(labelSyncPartner);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table2), labelSyncPartner, 1, 2, 9, 10,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table2), labelSyncPartner, 1, 9, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelSyncPartner), 0, 0.5);

    // Start the Raw Device Pane.
#if HAVE_GTK3 == 0
    table1 = gtk_table_new(6, 2, FALSE);
    gtk_widget_show(table1);
    gtk_container_add(GTK_CONTAINER(alignment1), table1);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 5);
#else
    table1 = gtk_grid_new();
    gtk_widget_show(table1);
    gtk_container_add(GTK_CONTAINER(alignment1), table1);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 5);
    gtk_grid_set_row_spacing(GTK_GRID(table1), 5);
    gtk_grid_set_column_spacing(GTK_GRID(table1), 5);
#endif

    label3 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label3), _("<b>Vendor:</b>"));
    gtk_widget_show(label3);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label3, 0, 1, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), label3, 0, 0, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label3), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label3), 0, 1);

    label4 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label4), _("<b>Product:</b>"));
    gtk_widget_show(label4);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label4, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), label4, 0, 1, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label4), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label4), 0, 1);

    label5 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label5), _("<b>Vendor ID:</b>"));
    gtk_widget_show(label5);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label5, 0, 1, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), label5, 0, 2, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label5), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label5), 0, 1);

    label6 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label6), _("<b>Product ID:</b>"));
    gtk_widget_show(label6);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label6, 0, 1, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), label6, 0, 3, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label6), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label6), 0, 1);

    label7 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label7), _("<b>Device Number:</b>"));
    gtk_widget_show(label7);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label7, 0, 1, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else 
    gtk_grid_attach(GTK_GRID(table1), label7, 0, 4, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label7), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label7), 0, 1);

    label8 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label8), _("<b>Bus Location:</b>"));
    gtk_widget_show(label8);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), label8, 0, 1, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), label8, 0, 5, 1, 1);
#endif
    gtk_label_set_justify(GTK_LABEL(label8), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label8), 0, 1);

    labelDeviceVendor = gtk_label_new(("label9"));
    gtk_widget_show(labelDeviceVendor);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelDeviceVendor, 1, 2, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else 
    gtk_grid_attach(GTK_GRID(table1), labelDeviceVendor, 1, 0, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelDeviceVendor), 0, 0.5);

    labelDeviceProduct = gtk_label_new(("label10"));
    gtk_widget_show(labelDeviceProduct);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelDeviceProduct, 1, 2, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), labelDeviceProduct, 1, 1, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelDeviceProduct), 0, 0.5);

    labelVenID = gtk_label_new(("label11"));
    gtk_widget_show(labelVenID);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelVenID, 1, 2, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), labelVenID, 1, 2, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelVenID), 0, 0.5);

    labelProdID = gtk_label_new(("label12"));
    gtk_widget_show(labelProdID);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelProdID, 1, 2, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), labelProdID, 1, 3, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelProdID), 0, 0.5);

    labelBusLoc = gtk_label_new(("label13"));
    gtk_widget_show(labelBusLoc);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelBusLoc, 1, 2, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), labelBusLoc, 1, 4, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelBusLoc), 0, 0.5);

    labelDevNum = gtk_label_new(("label14"));
    gtk_widget_show(labelDevNum);
#if HAVE_GTK3 == 0
    gtk_table_attach(GTK_TABLE(table1), labelDevNum, 1, 2, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
#else
    gtk_grid_attach(GTK_GRID(table1), labelDevNum, 1, 5, 1, 1);
#endif
    gtk_misc_set_alignment(GTK_MISC(labelDevNum), 0, 0.5);


#if HAVE_GTK3 == 0
    hbox2 = gtk_hbox_new(FALSE, 0);
#else
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_show(hbox2);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox2, TRUE, TRUE, 0);
#if HAVE_GTK3 == 0
    buttonClose = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
#else
    buttonClose = gtk_button_new_with_mnemonic(_("_Close"));
#endif
    gtk_widget_show(buttonClose);
    gtk_box_pack_end(GTK_BOX(hbox2), buttonClose, FALSE, FALSE, 0);

    g_signal_connect((gpointer) windowDialog, "destroy",
            G_CALLBACK(on_quitProp_activate),
            NULL);

    g_signal_connect((gpointer) buttonClose, "clicked",
            G_CALLBACK(on_quitProp_activate),
            NULL);

    // Now we need to update our strings for the information on the unit.
    gtk_label_set_text(GTK_LABEL(labelName), DeviceMgr.devicename->str);
    gtk_label_set_text(GTK_LABEL(labelModel), DeviceMgr.modelname->str);
    gtk_label_set_text(GTK_LABEL(labelSerial), DeviceMgr.serialnumber->str);
    if (DeviceMgr.maxbattlevel != 0) {
        tmp_string = g_strdup_printf("%d / %d (%d%%)", DeviceMgr.currbattlevel, DeviceMgr.maxbattlevel,
                (int) (((float) DeviceMgr.currbattlevel / (float) DeviceMgr.maxbattlevel) * 100.0));
    } else {
        tmp_string = g_strdup_printf("%d / %d", DeviceMgr.currbattlevel, DeviceMgr.maxbattlevel);
    }
    gtk_label_set_text(GTK_LABEL(labelBattery), tmp_string);
    gtk_label_set_text(GTK_LABEL(labelManufacturer), DeviceMgr.manufacturername->str);
    gtk_label_set_text(GTK_LABEL(labelDeviceVer), DeviceMgr.deviceversion->str);


    if (DeviceMgr.storagedeviceID == MTP_DEVICE_SINGLE_STORAGE) {
        tmp_string = g_strdup_printf(_("%d MB (free) / %d MB (total)"),
                (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE),
                (int) (DeviceMgr.devicestorage->MaxCapacity / MEGABYTE));
        gtk_label_set_text(GTK_LABEL(labelStorage), tmp_string);
    } else {
        tmp_string2 = g_string_new("");
        // Cycle through each storage device and list the name and capacity.
        LIBMTP_devicestorage_t* deviceStorage = DeviceMgr.device->storage;
        while (deviceStorage != NULL) {
            if (tmp_string2->len > 0)
                tmp_string2 = g_string_append(tmp_string2, "\n");
            if (deviceStorage->StorageDescription != NULL) {
                tmp_string2 = g_string_append(tmp_string2, deviceStorage->StorageDescription);
            } else {
                tmp_string2 = g_string_append(tmp_string2, deviceStorage->VolumeIdentifier);
            }
            tmp_string = g_strdup_printf(" : %d MB (free) / %d MB (total)",
                    (int) (deviceStorage->FreeSpaceInBytes / MEGABYTE),
                    (int) (deviceStorage->MaxCapacity / MEGABYTE));
            tmp_string2 = g_string_append(tmp_string2, tmp_string);
            deviceStorage = deviceStorage->next;
        }
        gtk_label_set_text(GTK_LABEL(labelStorage), tmp_string2->str);
        g_string_free(tmp_string2, TRUE);
    }

    tmp_string2 = g_string_new("");
    // Build a string for us to use.
    gint i = 0;
    for (i = 0; i < DeviceMgr.filetypes_len; i++) {
        if (tmp_string2->len > 0)
            tmp_string2 = g_string_append(tmp_string2, "\n");
        tmp_string2 = g_string_append(tmp_string2, LIBMTP_Get_Filetype_Description(DeviceMgr.filetypes[i]));
    }

    gtk_label_set_text(GTK_LABEL(labelSupportedFormat), tmp_string2->str);
    g_string_free(tmp_string2, TRUE);

    gtk_label_set_text(GTK_LABEL(labelSecTime), DeviceMgr.sectime->str);
    gtk_label_set_text(GTK_LABEL(labelSyncPartner), DeviceMgr.syncpartner->str);

    // This is our raw information.
    gtk_label_set_text(GTK_LABEL(labelDeviceVendor), DeviceMgr.Vendor->str);
    gtk_label_set_text(GTK_LABEL(labelDeviceProduct), DeviceMgr.Product->str);
    gtk_label_set_text(GTK_LABEL(labelVenID), g_strdup_printf("0x%x", DeviceMgr.VendorID));
    gtk_label_set_text(GTK_LABEL(labelProdID), g_strdup_printf("0x%x", DeviceMgr.ProductID));
    gtk_label_set_text(GTK_LABEL(labelBusLoc), g_strdup_printf("0x%x", DeviceMgr.BusLoc));
    gtk_label_set_text(GTK_LABEL(labelDevNum), g_strdup_printf("0x%x", DeviceMgr.DeviceID));

    return windowDialog;
}


// ************************************************************************************************

/**
 * on_deviceProperties_activate - Callback for displaying the device Properties Dialog box.
 * @param menuitem
 * @param user_data
 */
void on_deviceProperties_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gchar *tmp_string;

    // We confirm our device properties, this should setup the device structure information we use below.
    deviceProperties();

    // Update the status bar with our information.
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
            tmp_string = g_strdup_printf(_("Connected to %s - %d MB free"), DeviceMgr.devicename->str,
                    (int) (DeviceMgr.devicestorage->FreeSpaceInBytes / MEGABYTE));
        }
    }
    statusBarSet(tmp_string);
    g_free(tmp_string);

    // No idea how this could come about, but we should take it into account so we don't have a
    // memleak due to recreating the window multiple times.
    if (windowPropDialog != NULL) {
        gtk_widget_hide(windowPropDialog);
        gtk_widget_destroy(windowPropDialog);
    }

    // Create and show the dialog box.
    windowPropDialog = create_windowProperties();
    gtk_widget_show(GTK_WIDGET(windowPropDialog));
} // end on_deviceProperties_activate()

// ************************************************************************************************

/**
 * on_quitProp_activate - Callback used to close the Properties Dialog.
 * @param menuitem
 * @param user_data
 */
void on_quitProp_activate(GtkMenuItem *menuitem, gpointer user_data) {
    gtk_widget_hide(windowPropDialog);
    gtk_widget_destroy(windowPropDialog);
    windowPropDialog = NULL;
} // end on_quitProp_activate()
