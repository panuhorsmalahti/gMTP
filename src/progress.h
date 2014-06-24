/* 
 *
 *   File: progress.h
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

#ifndef _PROGRESS_H
#define _PROGRESS_H

#ifdef  __cplusplus
extern "C" {
#endif

    gboolean progressDialog_killed;

    // Progress Dialog
    GtkWidget* create_windowProgressDialog(gchar* msg);
    void displayProgressBar(gchar* msg);
    void destroyProgressBar(void);
    void setProgressFilename(gchar* filename_stripped);
    int fileprogress(const uint64_t sent, const uint64_t total, void const * const data);

   // Progress Dialog
    void on_progressDialog_Close(GtkWidget *window, gpointer user_data);
    void on_progressDialog_Cancel(GtkWidget *window, gpointer user_data);

#ifdef  __cplusplus
}
#endif

#endif  /* _PROGRESS_H */
