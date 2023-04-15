/*
 * dialog-job.h -- Dialog(s) for Job search and entry
 * Copyright (C) 2001,2002 Derek Atkins
 * Author: Derek Atkins <warlord@MIT.EDU>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */


#ifndef GNC_DIALOG_JOB_H_
#define GNC_DIALOG_JOB_H_

typedef struct _job_window JobWindow;

#include "gncJob.h"
#include "gncOwner.h"
#include "dialog-search.h"

/* Create or Edit a job */
JobWindow *gnc_ui_job_edit (GtkWindow *parent, GncOwner *owner, GncJob *job);
JobWindow *gnc_ui_job_new (GtkWindow *parent, GncOwner *owner, QofBook *book);

/** Create a new job as a duplicate of the given existing job.
 *
 * \param old_job The job which is being duplicated
 * \param open_properties If TRUE, open the "job properties" dialog window after creating the new job
 * \param new_date If non-NULL, use this date as the date for the "opening date" and also as date for all job entries.
 *
 * \return The Job Window structure that contains a whole lot of things,
 * among others the "created_job" as a GncJob* pointer on the newly
 * created job.
 */
JobWindow* gnc_ui_job_duplicate (GtkWindow* parent, GncJob* old_job, gboolean open_properties, const GDate* new_date);

/** Create a new job as a duplicate of the given existing job.
 *
 * \param old_job The job which is being duplicated
 * \param open_properties If TRUE, open the "job properties" dialog window after creating the new job
 * \param new_date If non-NULL, use this date as the date for the "opening date" and also as date for all job entries.
 *
 * \return The Job Window structure that contains a whole lot of things,
 * among others the "created_job" as a GncJob* pointer on the newly
 * created job.
 */
JobWindow* gnc_ui_job_duplicate (GtkWindow* parent, GncJob* old_job, gboolean open_properties, const GDate* new_date);

/* Search for Jobs */
GNCSearchWindow * gnc_job_search (GtkWindow *parent, GncJob *start,
                                  GncOwner *owner, QofBook *book);

/*
 * These callbacks are for use with the gnc_general_search widget
 *
 * select() provides a Select Dialog and returns it.
 * edit() opens the existing customer for editing and returns NULL.
 */
GNCSearchWindow * gnc_job_search_select (GtkWindow *parent, gpointer start, gpointer book);
GNCSearchWindow * gnc_job_search_edit (GtkWindow *parent, gpointer start, gpointer book);

/* definitions for CB functions */
void gnc_job_name_changed_cb (GtkWidget *widget, gpointer data);
void gnc_job_type_toggled_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_cancel_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_destroy_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_help_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_ok_cb (GtkWidget *widget, gpointer data);

#endif /* GNC_DIALOG_JOB_H_ */
