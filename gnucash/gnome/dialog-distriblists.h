/*
 * dialog-distriblists.h -- Dialog to handle distribution lists
 * Copyright (C) 2022 Ralf Zerres
 * Author: Ralf Zerres <ralf.zerres@mail.de>
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
/** @addtogroup Business
    @{ */
/** @addtogroup CoOwner

This file implements the UI dialogs needed to manage ::Distribution Lists.

Distribution lists offer the possibility to define reusable rules,
to calculate split values for assigend owner objects. Split values
are useful in reports (e.g settlements) where the distribution list rules
are consumed to calculate the proportional value for each assigned owner object.

The UI is designed to group suitable informations in frames.

-# Available distribution lists
-# Distribuion lists definitions
   -# Description
   -# List type

You may create, edit or delete distribution list entities.

Within a list you define a list type. Each list type will hold the
100% share value (the numerator). The numerator is an absolute or
a percentage value.

Assigned owner objects will offer their appropriate unit share value
as an object attribute. To calculate the settlement costs, the share
value is referenced as the devisor.

Co-Owner example:
Calculation base with 100% = 1000 units (numerator)
Apartment unit 1 with 10% = 100 shares (devisor)
Co-Owner settlement value = 1000/100 * account balance (owner object)
    @{ */
/** @file dialog-distriblists.h
    @brief handling distribution lists dialog objects
    @author Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef _DIALOG_DISTRIBUTION_LISTS_H
#define _DIALOG_DISTRIBUTION_LISTS_H

/** @struct DistributionListsWindow

@param  GtkWidget*  window - Pointer to the window.
@param  GtkWidget*  label_definition - The window label string for the distribution list.
@param  GtkWidget*  label_description - The label string of the distribution list.
@param  GtkWidget*  entry_description - The description of the distribution list.
@param  GtkWidget*  label_type - Label of the distribution list type.
@param  GtkWidget*  entry_type - The type of a distribution list.
@param  GtkWidget*  vbox - The frame that handles distribution lists.
@param  GtkWidget*  vbox_definitions - The frame that presents details assigned to the active distribution list.
@param  GtkWidget*  view_lists - The frame that presents available distribution lists.
@param  DistributionListNotebook - notebook The box that presents the attributes of a distribution list.
@param  DistributionListOwners - owners The box that presents the owner specific attributes of a distribution list.
@param  GncDistributionList* current_list - Copy of the current distribution list pointer.
@param  QofBook* book - Copy of the book pointer.
@param  gint component_id - A unique component identier.
@param  QofSession* session - Copy of the session pointer.
*/
typedef struct _distribution_lists_window DistributionListsWindow;

/** @struct DistributionListsNotebook

@param  GtkWidget*  dialog - Pointer to the notebook dialog.
@param  GtkWidget*  notebook - Pointer to the notebook entry.
@param  GtkWidget*  parent - Pointer to the parent of the notebook dialog.
*/
typedef struct _distribution_lists_notebook DistributionListsNotebook;

/** @struct DistributionListsOwners

@param  GtkWidget*  dialog - Pointer to the owners dialog.
@param  GtkWidget*  owners - Pointer to the owners entry.
@param  GtkWidget*  parent - Pointer to the parent of the owners dialog.
*/
typedef struct _distribution_lists_owners DistributionListsOwners;

#include "qof.h"

/** @name Create/Destroy window Functions
 @{ */

/** Create a new DistribuionList window that list the available
 *distribution list objects.
 *
 * It offers callbacks to create, modify or delete lists objects. */
DistributionListsWindow
*gnc_ui_distriblists_window_new (GtkWindow *parent, QofBook *book);
/** @} */

/* Destroy a distribution lists window */
//void
//gnc_ui_distriblists_window_destroy (DistriblistsWindow *ttw);

/** @name Callback Functions
 @{ */
void
distriblists_list_delete_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void
distriblists_list_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void
distriblists_list_new_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void
distriblists_type_changed_cb (
    GtkComboBox *combobox,
    gpointer data);
void
distriblists_window_close_cb (
    GtkWidget *widget,
    gpointer data);
void
distriblists_window_destroy_cb (
    GtkWidget *widget,
    gpointer data);
gboolean
distriblists_window_key_press_cb (
    GtkWidget *widget, GdkEventKey *event,
    gpointer data);
void
owners_assign_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void
owners_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void owners_ok_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void
owners_remove_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
/** @} */

#endif /* _DIALOG_DISTRIBUTION_LISTS_H */
/** @} */
/** @} */
