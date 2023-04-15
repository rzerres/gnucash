/*
 * dialog-coowner.h -- Dialog(s) for Employee search and entry
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

This file implements the UI dialogs needed to manage ::GncCoOwner
objects.

The UI is designed to group suitable informations in frames.

-# General coOwner information
   -# Identification
   -# Billing Address information
   -# Notes
 -# Billing Information
   -# Currency
   -# Terms
   -# Credit
   -# Discount
   -# Tax handling
-# Shipping Information
   -# Differing address for CoOwner communication
   -# Phone/Fax info
   -# Email Url

    @{ */
/** @file dialog-coowner.h
    @brief handling CoOwner dialog objects
    @author Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef GNC_DIALOG_COOWNER_H_
#define GNC_DIALOG_COOWNER_H_

/** \struct CoOwnerSelectWindow

@param  QofBook* Copy of the book pointer.
@param  QofQuery* Copy of the query pointer.
*/
typedef struct _coowner_select_window CoOwnerSelectWindow;

/** \struct CoOwnerWindow

@param  GtkWidget* The dialog entity.
@param  GtkWidget* Checkbutton handling activation status of a CoOwner entity.

@param  GtkWidget* Name in the billing address.
@param  GtkWidget* fist line in the billing address.
@param  GtkWidget* second line in the billing address.
@param  GtkWidget* third line in the billing address.
@param  GtkWidget* forth line in the billing address.
@param  GtkWidget* phone number in the billing address.
@param  GtkWidget* fax number in the billing address.
@param  GtkWidget* email url in the billing address.

@param  GtkWidget* The appartnment share value.
@param  GtkWidget* The appartment unit name.
@param  GtkWidget* The credit card account checkbutton.
@param  GtkWidget* The credit card selection box.
@param  GtkWidget* The currency assigned to CoOwner transactions.
@param  GtkWidget* The credit amount assinged to a CoOwner entity.
@param  GtkWidget* The discount amount assinged to a CoOwner entity.
@param  GtkWidget* The default distribution key assingend to a CoOwner entity.
@param  GtkWidget* The preferred language assigned to a CoOwner entity.

@param  GtkWidget* unique ID distinguishing the CoOwner entity.
@param  GtkWidget* Name assigned to the CoOwner entity.

@param  GtkWidget* Name in the shipping address.
@param  GtkWidget* fist line in the shipping address.
@param  GtkWidget* second line in the shipping address.
@param  GtkWidget* third line in the shipping address.
@param  GtkWidget* forth line in the shipping address.
@param  GtkWidget* phone number in the shipping address.
@param  GtkWidget* fax number in the shipping address.
@param  GtkWidget* email url in the shipping address.

@param  GtkWidget The tenant name assigned to the CoOwner entity.
@param  GtkWidget Notes saved with the CoOwner entity.

@param  GncBillTerm* Billing terms assigned to the CoOwner entity.
@param  GtkWidget* Box to select the valid billing term.
@param  GncTaxIncluded* Activate dedicated tax table.
@param  GtkWidget* Box to select the global tax table.
@param  GncTaxTable* The selected global tax table value.
@param  GtkWidget* Activate usage of dedicated tax table.
@param  GtkWidget* Combox to select the valid tax table entity.

@param  CoOwnerDialogType The dialog type.
@param  GncGUID The unique coowner GUID.
@param  gint The componet ID.
@param  QofBook* Copy of the book pointer.
@param  GncCoOwner* Pointer to the creaded CoOwner entity.

@param  QuickFill* Quickfill shipping address second line.
@param  QuickFill* Quickfill shipping address third line.
@param  QuickFill* Quickfill shipping address forth line.
@param  QuickFill* Quickfill shipping address second line.
@param  QuickFill* Quickfill shipping address third line.
@param  QuickFill* Quickfill shipping address forth line.
@param  gint  Address start selection marker.
@param  gint  Addres end selection marker.
@param  guint Address source ID for selection.
*/
typedef struct _coowner_window CoOwnerWindow;

#include "gncCoOwner.h"
#include "dialog-search.h"

/* Functions to edit and create coowners */

/** Edit and modify an exiting "CoOwner" object via the dialog page.
 *
 *  @param parent A pointer to the parent window.
 *  @param coowner A pointer to coowner object.
 */
CoOwnerWindow * gnc_ui_coowner_edit (GtkWindow *parent, GncCoOwner *coowner);

/** Create a new "CoOwner" object via the dialog page.
 *
 *  @param parent A pointer to the parent window.
 *  @param book A pointer to the assigned book object.
 */
CoOwnerWindow * gnc_ui_coowner_new (GtkWindow *parent, QofBook *book);

/* Search for an coowner */
/** Search for an existing "CoOwner" object.
 *
 *  @param parent A pointer to the parent window.
 *  @param start A pointer to a start value, that shoud match the GUID of the query.
 *  @param book A pointer to the assigned book object.
 */
GNCSearchWindow * gnc_coowner_search (GtkWindow *parent, GncCoOwner *start, QofBook *book);

/**
 * Convenience function. It will call 'gnc_coowner_search'.
 *
 *  @param parent A pointer to the parent window.
 *  @param start A pointer to a start value, that shoud match the GUID of the query.
 *  @param book A pointer to the assigned book object.
 */
GNCSearchWindow * gnc_coowner_search_select (GtkWindow *parent, gpointer start, gpointer book);

/** Open a UI to define search parameters when searching for existing "CoOwner" objects.
 *  It will call 'gnc_ui_coowner_edit'.
 *
 *  @param parent A pointer to the parent window.
 *  @param start A pointer to a start value, that shoud match the GUID of the query.
 *  @param book A pointer to the assigned book object.
 */
GNCSearchWindow * gnc_coowner_search_edit (GtkWindow *parent, gpointer start, gpointer book);

#endif /* GNC_DIALOG_COOWNER_H_ */
