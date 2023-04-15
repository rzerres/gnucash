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

This file implements the UI dialogs needed to manage ::Distribution Lists
objects.

The UI is designed to group suitable informations in frames.

-# Available distribution lists
-# Distribuion lists definitions
   -# Description
   -# List type

Right now, we handle just one type: Co-Ownership shares.
You may create, edit or delete entities.

A list is used to define attributes that are needed, when account
balances are to be split proportionally into co-owner settlements.

Each list defines the 100% share value (the numerator) as the
calculation base. You assign the appropriate unit share value inside
the co-owners objects, which in term is referenced to calculate the
costs to be settled to this apartment unit (the devisor).

Example:
Calculation base with 100% = 1000 units
Apartment unit 1 with 10% = 100 shares
Co-ownership share = 1000/100 * account balance

    @{ */
/** @file dialog-distriblists.h
    @brief handling distribution lists dialog objects
    @author Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef _DIALOG_DISTRIBUTION_LISTS_H
#define _DIALOG_DISTRIBUTION_LISTS_H

/** @struct DistributionListsWindow

@param  GtkWidget*  window - Pointer to the window The instance entity.
@param  GtkWidget*  entry_description - The description of the distributon list.
@param  GtkWidget*  label_type - Label of the distribution list type.
@param  GtkWidget*  entry_type - The type of a distribution list.
@param  GtkWidget*  vbox - The frame that handles distribution lists.
@param  GtkWidget*  view_lists - The frame that presents available distribution lists.
@param  GtkWidget*  vbox_definitions - The frame that presents details assigned to the active distribution list.
@param  DistributionListNotebook - notebook The box that presents the attributes of a distribution list.
@param  GncDistributionList* current_list - Copy of the current distribution list pointer.
@param  QofBook* book - Copy of the book pointer.
@param  gint component_id - A unique component identier.
@param  QofSession* session - Copy of the session pointer.
*/
typedef struct _distribution_lists_window DistributionListsWindow;

#include "qof.h"

/** @name Create/Destroy window Functions
 @{ */
DistributionListsWindow
*gnc_ui_distriblists_window_new (GtkWindow *parent, QofBook *book);

/* Destroy a distribution lists window */
//void
//gnc_ui_distriblists_window_destroy (DistriblistsWindow *ttw);

#endif /* _DIALOG_DISTRIBUTION_LISTS_H */
