/* 
 *
 *   File: properties.h
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

#ifndef _PROPERTIES_H
#define _PROPERTIES_H

#ifdef  __cplusplus
extern "C" {
#endif

    GtkWidget* create_windowProperties(void);

    // Main menu callbacks.
    void on_deviceProperties_activate(GtkMenuItem *menuitem, gpointer user_data);

    // Properties Dialog
    void on_quitProp_activate(GtkMenuItem *menuitem, gpointer user_data);

#ifdef  __cplusplus
}
#endif

#endif  /* _PROPERTIES_H */
