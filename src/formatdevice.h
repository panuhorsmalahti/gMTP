/* 
 *
 *   File: formatdevice.h
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

#ifndef _FORMATDEVICE_H
#define _FORMATDEVICE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

    // Widget for formatDevice progress bar.
    GtkWidget* create_windowFormat(void);
    
    // Main menu callback
    void on_editFormatDevice_activate(GtkMenuItem *menuitem, gpointer user_data);

    // Format Device Progress Bar.
    void formatDevice_thread(void);

#ifdef  __cplusplus
}
#endif

#endif  /* _FORMATDEVICE_H */
