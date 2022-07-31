/*
 * dialog-invoice.c -- Dialog for Invoice entry
 * Copyright (C) 2001,2002,2006 Derek Atkins
 * Author: Derek Atkins <warlord@MIT.EDU>
 *
 * Copyright (c) 2005,2006 David Hampton <hampton@employees.org>
 * Changes:      2022 Ralf Zerres <ralf.zerres@mail.de>
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libguile.h>
#include "swig-runtime.h"

#include "qof.h"

#include "dialog-utils.h"
#include "gnc-component-manager.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "gnc-prefs.h"
#include "gnc-ui-util.h"
#include "gnc-date.h"
#include "gnc-date-edit.h"
#include "gnc-amount-edit.h"
#include "gnucash-sheet.h"
#include "gnucash-register.h"
#include "window-report.h"
#include "dialog-search.h"
#include "search-param.h"
#include "gnc-session.h"
#include "gncOwner.h"
#include "gncInvoice.h"
#include "gncInvoiceP.h"
#include <gnc-glib-utils.h>

#include "gncEntryLedger.h"

#include "gnc-plugin-page.h"
#include "gnc-general-search.h"
#include "business-gnome-utils.h"
#include "dialog-account.h"
#include "dialog-billterms.h"
#include "dialog-date-close.h"
#include "dialog-dup-trans.h"
#include "dialog-invoice.h"
#include "dialog-job.h"
#include "dialog-payment.h"
#include "dialog-tax-table.h"
#include "guile-mappings.h"

#include "dialog-query-view.h"

#include "gnc-plugin-business.h"
#include "gnc-plugin-page-invoice.h"
#include "gnc-plugin-page-report.h"
#include "gnc-main-window.h"
#include "gnc-state.h"

#include "dialog-doclink.h"
#include "dialog-doclink-utils.h"
#include "dialog-transfer.h"
#include "gnc-uri-utils.h"

/* Disable -Waddress.  GCC 4.2 warns (and fails to compile with -Werror) when
 * passing the address of a guid on the stack to QOF_BOOK_LOOKUP_ENTITY via
 * gncInvoiceLookup and friends.  When the macro gets inlined, the compiler
 * emits a warning that the guid null pointer test is always true.
 */
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 2)
#    pragma GCC diagnostic warning "-Waddress"
#endif

#define DIALOG_NEW_INVOICE_CM_CLASS "dialog-new-invoice"
#define DIALOG_VIEW_INVOICE_CM_CLASS "dialog-view-invoice"

#define GNC_PREFS_GROUP_SEARCH   "dialogs.business.invoice-search"
#define GNC_PREF_NOTIFY_WHEN_DUE "notify-when-due"
#define GNC_PREF_ACCUM_SPLITS    "accumulate-splits"
#define GNC_PREF_DAYS_IN_ADVANCE "days-in-advance"

void gnc_invoice_window_ok_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_window_cancel_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_window_ledger_cancel_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_window_help_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_type_toggled_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_id_changed_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_terms_changed_cb (GtkWidget *widget, gpointer data);

static gboolean doclink_button_cb (GtkLinkButton *button, InvoiceWindow *iw);
static void edit_invoice_cb (GtkWindow *dialog, gpointer inv, gpointer user_data);
static void multi_edit_invoice_cb (GtkWindow *dialog, GList *invoice_list, gpointer user_data);

#define ENUM_INVOICE_TYPE(_) \
  _(NEW_INVOICE, )  \
  _(MOD_INVOICE, )  \
  _(DUP_INVOICE, )  \
  _(EDIT_INVOICE, ) \
  _(VIEW_INVOICE, )

DEFINE_ENUM(InvoiceDialogType, ENUM_INVOICE_TYPE)
AS_STRING_DEC(InvoiceDialogType, ENUM_INVOICE_TYPE)
FROM_STRING_DEC(InvoiceDialogType, ENUM_INVOICE_TYPE)

FROM_STRING_FUNC(InvoiceDialogType, ENUM_INVOICE_TYPE)
AS_STRING_FUNC(InvoiceDialogType, ENUM_INVOICE_TYPE)

typedef enum
{
    DUE_FOR_COOWNER,    // show invoices due
    DUE_FOR_CUSTOMER,   // show invoices due
    DUE_FOR_VENDOR,     // show bills due
} GncWhichDueType;

struct _invoice_select_window
{
    QofBook *book;
    GncOwner *owner;
    QofQuery *q;
    GncOwner owner_def;
};

#define UNUSED_VAR     __attribute__ ((unused))

static QofLogModule UNUSED_VAR log_module = G_LOG_DOMAIN; //G_LOG_BUSINESS;

/** This data structure does double duty.  It is used to maintain
 *  information for the "New Invoice" dialog, and it is also used to
 *  maintain information for the "Invoice Entry" page that is embedded
 *  into a main window.  Beware, as not all fields are used by both windows.
 */
struct _invoice_window
{
    GtkBuilder *builder;
    GtkWidget *dialog;              /* Used by 'New Invoice Window' */
    GncPluginPage *page;            /* Used by 'Edit Invoice' Page */
    const gchar *page_state_name;   /* Used for loading open state information */

    /* Summary Bar Widgets */
    GtkWidget *total_label;
    GtkWidget *total_cash_label;
    GtkWidget *total_charge_label;
    GtkWidget *total_subtotal_label;
    GtkWidget *total_tax_label;

    /* Data Widgets */
    GtkWidget *label_invoice_info;
    GtkWidget *label_type;
    GtkWidget *selected_type_hbox;
    GtkWidget *selected_type;
    GtkWidget *label_invoice_id;
    GtkWidget *entry_invoice_id;
    GtkWidget *label_date_opened;
    GtkWidget *entry_date_opened;
    GtkWidget *label_date_posted;
    GtkWidget *entry_date_posted;
    GtkWidget *date_posted;
    GtkWidget *label_account_posted;
    GtkWidget *entry_account_posted;
    GtkWidget *label_active;
    GtkWidget *checkbutton_active;
    GtkWidget *label_paid;

    GtkWidget *label_owner;
    GtkWidget *entry_owner;
    GtkWidget *choice_owner;
    GtkWidget *label_job;
    GtkWidget *entry_job;
    GtkWidget *choice_job;
    GtkWidget *label_billing_id;
    GtkWidget *entry_billing_id;
    GtkWidget *label_terms;
    GtkWidget *entry_terms;

    GtkWidget *entry_notes;
    GtkWidget *button_doclink;

    /* Project Widgets (used for Bills only, really?) */
    GtkWidget *frame_project;
    GtkWidget *label_project_owner;
    GtkWidget *entry_project_owner;
    GtkWidget *choice_project_owner;
    GtkWidget *label_project_job;
    GtkWidget *entry_project_job;
    GtkWidget *choice_project_job;

    /* Expense Voucher Widgets (can't we use the project entries?) */
    GtkWidget *frame_charge_to;
    GtkWidget *entry_charge_to;

    gint width;

    GncBillTerm *terms;
    GnucashRegister *reg;
    GncEntryLedger *ledger;

    invoice_sort_type_t last_sort;

    InvoiceDialogType dialog_type;
    GncGUID invoice_guid;
    gboolean is_credit_note;
    gint component_id;
    QofBook *book;
    GncInvoice *created_invoice;
    GncOwner owner;
    GncOwner job;

    GncOwner project_owner;
    GncOwner project_job;

    /* the cached reportPage for this invoice. note this is not saved
       into .gcm file therefore the invoice editor->report link is lost
       upon restart. */
    GncPluginPage *reportPage;

    /* for Unposting */
    gboolean reset_tax_tables;
};

struct multi_duplicate_invoice_data
{
    GDate date;
    GtkWindow *parent;
};

/* Fuction prototypes */
void gnc_invoice_window_active_toggled_cb (GtkWidget *widget, gpointer data);
gboolean gnc_invoice_window_leave_notes_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data);
DialogQueryView *gnc_invoice_show_docs_due (GtkWindow *parent, QofBook *book, double days_in_advance, GncWhichDueType duetype);

#define INV_WIDTH_PREFIX "invoice_reg"
#define BILL_WIDTH_PREFIX "bill_reg"
#define SETTLEMENT_WIDTH_PREFIX "settlement_reg"
#define VOUCHER_WIDTH_PREFIX "voucher_reg"

static void gnc_invoice_update_window (InvoiceWindow *iw, GtkWidget *widget);
static InvoiceWindow * gnc_ui_invoice_modify (GtkWindow *parent, GncInvoice *invoice);

/*******************************************************************************/
/* FUNCTIONS FOR ACCESSING DATA STRUCTURE FIELDS */

static GtkWidget
*iw_get_window (InvoiceWindow *iw)
{
    if (iw->page)
        return gnc_plugin_page_get_window (iw->page);
    return iw->dialog;
}

GtkWidget
*gnc_invoice_get_register (InvoiceWindow *iw)
{
    if (iw)
        return (GtkWidget *)iw->reg;
    return NULL;
}

GtkWidget
*gnc_invoice_get_notes (InvoiceWindow *iw)
{
    if (iw)
        return (GtkWidget *)iw->entry_notes;
    return NULL;
}

/*******************************************************************************/
/* FUNCTIONS FOR UNPOSTING */

static gboolean
iw_ask_unpost (InvoiceWindow *iw)
{
    GtkWidget *dialog;
    GtkToggleButton *toggle;
    GtkBuilder *builder;
    gint response;
    const gchar *style_label= NULL;
    GncOwnerType owner_type = gncOwnerGetType (&iw->owner);


    builder = gtk_builder_new();
    gnc_builder_add_from_file (builder, "dialog-invoice.glade", "unpost_message_dialog");
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "unpost_message_dialog"));
    toggle = GTK_TOGGLE_BUTTON(gtk_builder_get_object (builder, "yes_tt_reset"));

    switch (owner_type)
    {
        case GNC_OWNER_COOWNER:
            style_label = "gnc-class-coowners";
            break;
        case GNC_OWNER_EMPLOYEE:
            style_label = "gnc-class-employees";
            break;
        case GNC_OWNER_VENDOR:
            style_label = "gnc-class-vendors";
            break;
        default:
            style_label = "gnc-class-customers";
            break;
    }
    // Set a secondary style context for this windowy so it can be easily manipulated with css
    gnc_widget_style_context_add_class (GTK_WIDGET(dialog), style_label);

    gtk_window_set_transient_for (GTK_WINDOW(dialog),
                                  GTK_WINDOW(iw_get_window(iw)));

    iw->reset_tax_tables = FALSE;

    gtk_widget_show_all(dialog);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
        iw->reset_tax_tables =
            gtk_toggle_button_get_active(toggle);

    gtk_widget_destroy(dialog);
    g_object_unref(G_OBJECT(builder));

    return (response == GTK_RESPONSE_OK);
}

/*******************************************************************************/
/* INVOICE WINDOW */

static GncInvoice
*iw_get_invoice (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return gncInvoiceLookup (iw->book, &iw->invoice_guid);
}

GncInvoice
*gnc_invoice_window_get_invoice (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return iw_get_invoice (iw);
}

GtkWidget
*gnc_invoice_window_get_doclink_button (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return iw->button_doclink;
}

static void
set_gncEntry_switch_type (gpointer data, gpointer user_data)
{
    GncEntry *entry = data;
    //g_warning("Modifying date for entry with desc=\"%s\"", gncEntryGetDescription(entry));

    gncEntrySetQuantity (entry, gnc_numeric_neg (gncEntryGetQuantity (entry)));
}

static void
set_gncEntry_date(gpointer data, gpointer user_data)
{
    GncEntry *entry = data;
    time64 new_date = *(time64*) user_data;
    //g_warning("Modifying date for entry with desc=\"%s\"", gncEntryGetDescription(entry));

    gncEntrySetDate(entry, gnc_time64_get_day_neutral (new_date));
    // gncEntrySetDateEntered(entry, *new_date); - don't modify this
    // because apparently it defines the ordering of the entries,
    // which we don't want to change.
}

static void
gnc_ui_to_invoice (InvoiceWindow *iw, GncInvoice *invoice)
{
    GtkTextBuffer* text_buffer;
    GtkTextIter start, end;
    gchar *text;
    time64 time;
    gboolean is_credit_note = gncInvoiceGetIsCreditNote (invoice);

    if (iw->dialog_type == VIEW_INVOICE)
        return;

    gnc_suspend_gui_refresh ();

    gncInvoiceBeginEdit (invoice);

    if (iw->checkbutton_active)
        gncInvoiceSetActive (invoice, gtk_toggle_button_get_active
                             (GTK_TOGGLE_BUTTON (iw->checkbutton_active)));

    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->entry_notes));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncInvoiceSetNotes (invoice, text);

    if (iw->entry_charge_to)
        gncInvoiceSetToChargeAmount (invoice,
            gnc_amount_edit_get_amount
            (GNC_AMOUNT_EDIT (iw->entry_charge_to)));

    time = gnc_date_edit_get_date (GNC_DATE_EDIT (iw->entry_date_opened));

    // Only set these values for NEW/MOD INVOICE types
    if (iw->dialog_type != EDIT_INVOICE)
    {
        gncInvoiceSetID (invoice, gtk_editable_get_chars (
            GTK_EDITABLE (iw->entry_invoice_id), 0, -1));
        gncInvoiceSetBillingID (invoice, gtk_editable_get_chars (
            GTK_EDITABLE (iw->entry_billing_id), 0, -1));
        gncInvoiceSetTerms (invoice, iw->terms);

        gncInvoiceSetDateOpened (invoice, time);

        gnc_owner_get_owner (iw->choice_owner, &(iw->owner));
        if (iw->choice_job)
            gnc_owner_get_owner (iw->choice_job, &(iw->job));

        // Only set the job if we've actually got one
        if (gncOwnerGetJob (&(iw->job)))
            gncInvoiceSetOwner (invoice, &(iw->job));
        else
            gncInvoiceSetOwner (invoice, &(iw->owner));

        // Set the invoice currency based on the owner
        gncInvoiceSetCurrency (invoice, gncOwnerGetCurrency (&iw->owner));

        // Only set the BillTo if we've actually got one
        if (gncOwnerGetJob (&iw->project_job))
            gncInvoiceSetBillTo (invoice, &iw->project_job);
        else
            gncInvoiceSetBillTo (invoice, &iw->project_owner);
    }

    // Document type can only be modified for a new or duplicated invoice/credit note
    if (iw->dialog_type == NEW_INVOICE || iw->dialog_type == DUP_INVOICE)
    {
        // Update the entry dates to match the invoice date. This only really
        // should happen for a duplicate invoice. However as a new invoice has
        // no entries we can run this unconditionally.
        g_list_foreach(gncInvoiceGetEntries(invoice),
                    &set_gncEntry_date, &time);


        gncInvoiceSetIsCreditNote (invoice, iw->is_credit_note);
    }

    // If the document type changed on a duplicated invoice,
    // its entries should be updated
    //
    if (iw->dialog_type == DUP_INVOICE && iw->is_credit_note != is_credit_note)
    {
        g_list_foreach(gncInvoiceGetEntries(invoice),
                       &set_gncEntry_switch_type, NULL);
    }

    gncInvoiceCommitEdit (invoice);
    gnc_resume_gui_refresh ();
}

static gboolean
gnc_invoice_window_verify_ok (InvoiceWindow *iw)
{
    const char *res;
    gchar *string;

    // save the current entry in the ledger?
    if (!gnc_entry_ledger_check_close (iw_get_window(iw), iw->ledger))
        return FALSE;

    // Check the Owner
    gnc_owner_get_owner (iw->choice_owner, &(iw->owner));
    res = gncOwnerGetName (&(iw->owner));
    if (res == NULL || g_strcmp0 (res, "") == 0)
    {
        gnc_error_dialog (GTK_WINDOW (iw_get_window(iw)), "%s",
            // Translators: In this context, 'Billing information'
            // maps to the owner label in the frame. The owner should
            // reflact to active owner type (coowner, customer,
            // employee, vendor).  We will use the name attribute of
            // that entity (e.g. company name) when generating the
            // invoice.
            _("You need to supply Billing Information."));
        return FALSE;
    }

    /* Check the ID; set one if necessary */
    res = gtk_entry_get_text (GTK_ENTRY (iw->entry_invoice_id));
    if (g_strcmp0 (res, "") == 0)
    {
      // Invoices and bills have separate counters.  Therefore we pass
      // the GncOwer to gncInvoiceNextID so it knows whether we
      // are creating a bill or an invoice. */
        string = gncInvoiceNextID(iw->book, &(iw->owner));
        gtk_entry_set_text (GTK_ENTRY (iw->entry_invoice_id), string);
        g_free(string);
    }

    return TRUE;
}

static gboolean
gnc_invoice_window_ok_save (InvoiceWindow *iw)
{
    if (!gnc_invoice_window_verify_ok (iw))
        return FALSE;

    {
        GncInvoice *invoice = iw_get_invoice (iw);
        if (invoice)
        {
            gnc_ui_to_invoice (iw, invoice);
        }
        // Save the invoice to return it later.
        iw->created_invoice = invoice;
    }
    return TRUE;
}

void
gnc_invoice_window_ok_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!gnc_invoice_window_ok_save (iw))
        return;

    // Ok, we don't need this anymore
    iw->invoice_guid = *guid_null ();

    // if this is a new or duplicated invoice, and created_invoice is NON-NULL,
    // then open up a new window with the invoice.  This used to be done
    // in gnc_ui_invoice_new() but cannot be done anymore
    //
    if ((iw->dialog_type == NEW_INVOICE || iw->dialog_type == DUP_INVOICE)
            && iw->created_invoice)
        gnc_ui_invoice_edit (gnc_ui_get_main_window (iw->dialog), iw->created_invoice);

    gnc_close_gui_component (iw->component_id);
}

void
gnc_invoice_window_cancel_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    gnc_close_gui_component (iw->component_id);
}

void
gnc_invoice_window_help_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    switch(iw->owner.type)
    {
        case GNC_OWNER_COOWNER:
           gnc_gnome_help (GTK_WINDOW(iw->dialog), HF_HELP, HL_USAGE_SETTLEMENT);
           break;
        case GNC_OWNER_CUSTOMER:
           gnc_gnome_help (GTK_WINDOW(iw->dialog), HF_HELP, HL_USAGE_INVOICE);
           break;
        case GNC_OWNER_VENDOR:
           gnc_gnome_help (GTK_WINDOW(iw->dialog), HF_HELP, HL_USAGE_BILL);
           break;
        default:
           gnc_gnome_help (GTK_WINDOW(iw->dialog), HF_HELP, HL_USAGE_VOUCHER);
           break;
    }
}

static const gchar
*gnc_invoice_window_get_state_group (InvoiceWindow *iw)
{
    switch (gncOwnerGetType (gncOwnerGetEndOwner (&iw->owner)))
    {
        case GNC_OWNER_COOWNER:
            return "Co-Owner documents";
            break;
        case GNC_OWNER_EMPLOYEE:
            return "Employee documents";
            break;
        case GNC_OWNER_VENDOR:
            return "Vendor documents";
            break;
        default:
            return "Customer documents";
            break;
    }
}

// Save user state layout information for Invoice/Bill/Voucher
// documents so it can be used for the default user set layout
void
gnc_invoice_window_save_document_layout_to_user_state (InvoiceWindow *iw)
{
    Table *table = gnc_entry_ledger_get_table (iw->ledger);
    const gchar *group = gnc_invoice_window_get_state_group (iw);

    gnc_table_save_state (table, group);
}

// Removes the user state layout information for Invoice/Bill/Voucher
// documents and also resets the current layout to the built-in defaults
void
gnc_invoice_window_reset_document_layout_and_clear_user_state (InvoiceWindow *iw)
{
    GnucashRegister *reg = iw->reg;
    const gchar *group = gnc_invoice_window_get_state_group (iw);

    gnucash_register_reset_sheet_layout (reg);
    gnc_state_drop_sections_for (group);
}

// Checks to see if there is user state layout information for
// Invoice/Bill/Voucher documents so it can be used for the
// default user layout
gboolean
gnc_invoice_window_document_has_user_state (InvoiceWindow *iw)
{
    GKeyFile *state_file = gnc_state_get_current ();
    const gchar *group = gnc_invoice_window_get_state_group (iw);
    return g_key_file_has_group (state_file, group);
}

// Invoice callback functions
void
gnc_invoice_window_destroy_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    gnc_suspend_gui_refresh ();

    if ((iw->dialog_type == NEW_INVOICE || iw->dialog_type == DUP_INVOICE)
         && invoice != NULL)
    {
        gncInvoiceRemoveEntries (invoice);
        gncInvoiceBeginEdit (invoice);
        gncInvoiceDestroy (invoice);
        iw->invoice_guid = *guid_null ();
    }

    gtk_widget_destroy(widget);
    gnc_entry_ledger_destroy (iw->ledger);
    gnc_unregister_gui_component (iw->component_id);
    g_object_unref (G_OBJECT (iw->builder));
    gnc_resume_gui_refresh ();

    g_free (iw);
}

void
gnc_invoice_window_edit_cb (GtkWindow *parent, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    if (invoice)
        gnc_ui_invoice_modify (parent, invoice);
}

void
gnc_invoice_window_duplicateInvoice_cb (GtkWindow *parent, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    if (invoice)
        gnc_ui_invoice_duplicate (parent, invoice, TRUE, NULL);
}

void gnc_invoice_window_entry_up_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_move_current_entry_updown(iw->ledger, TRUE);
}
void gnc_invoice_window_entry_down_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_move_current_entry_updown(iw->ledger, FALSE);
}

void
gnc_invoice_window_record_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    if (!gnc_entry_ledger_commit_entry (iw->ledger))
        return;

    gnucash_register_goto_next_virt_row (iw->reg);
}

void
gnc_invoice_window_ledger_cancel_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
}

void
gnc_invoice_window_delete_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncEntry *entry;

    if (!iw || !iw->ledger)
        return;

    // Grep the current entry based on cursor position
    entry = gnc_entry_ledger_get_current_entry (iw->ledger);
    if (!entry)
    {
        gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
        return;
    }

    // deleting the blank entry just cancels
    if (entry == gnc_entry_ledger_get_blank_entry (iw->ledger))
    {
        gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
        return;
    }

    // Verify that the user really wants to delete this entry
    {
        const char *message =
            _("Are you sure you want to delete the "
            "selected entry?");
        const char *order_warn =
            _("This entry is attached to an order and "
            "will be deleted from that as well!");
        char *msg;
        gboolean result;

        if (gncEntryGetOrder (entry))
            msg = g_strconcat (message, "\n\n", order_warn, (char *)NULL);
        else
            msg = g_strdup (message);

        result = gnc_verify_dialog (GTK_WINDOW (
            iw_get_window(iw)), FALSE, "%s", msg);
        g_free (msg);

        if (!result)
            return;
    }

    // Finally, delete the entry */
    gnc_entry_ledger_delete_current_entry (iw->ledger);
    return;
}

void
gnc_invoice_window_duplicate_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_duplicate_current_entry (iw->ledger);
}

void
gnc_invoice_window_blank_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    if (!gnc_entry_ledger_commit_entry (iw->ledger))
        return;

    {
        VirtualCellLocation vcell_loc;
        GncEntry *blank;

        blank = gnc_entry_ledger_get_blank_entry (iw->ledger);
        if (blank == NULL)
            return;

        if (gnc_entry_ledger_get_entry_virt_loc (iw->ledger, blank, &vcell_loc))
            gnucash_register_goto_virt_cell (iw->reg, vcell_loc);
    }
}

static GncPluginPage
*gnc_invoice_window_print_invoice(GtkWindow *parent, GncInvoice *invoice)
{
    SCM func, arg, arg2;
    SCM args = SCM_EOL;
    int report_id;
    const char *reportname = gnc_plugin_business_get_invoice_printreport();
    GncPluginPage *reportPage = NULL;

    g_return_val_if_fail (invoice, NULL);
    if (!reportname)
        // fallback if the option lookup failed
        reportname = "5123a759ceb9483abf2182d01c140e8d";

    func = scm_c_eval_string ("gnc:invoice-report-create");
    g_return_val_if_fail (scm_is_procedure (func), NULL);

    arg = SWIG_NewPointerObj(invoice, SWIG_TypeQuery("_p__gncInvoice"), 0);
    arg2 = scm_from_utf8_string(reportname);
    args = scm_cons2 (arg, arg2, args);

    // scm_gc_protect_object(func);

    arg = scm_apply (func, args, SCM_EOL);
    g_return_val_if_fail (scm_is_exact (arg), NULL);
    report_id = scm_to_int (arg);

    // scm_gc_unprotect_object(func);
    if (report_id >= 0)
    {
        reportPage = gnc_plugin_page_report_new (report_id);
        gnc_main_window_open_page (GNC_MAIN_WINDOW (parent), reportPage);
    }

    return reportPage;
}

static gboolean
equal_fn (gpointer find_data, gpointer elt_data)
{
    return (find_data && (find_data == elt_data));
}

// From the invoice editor, open the invoice report. This will reuse
// the invoice report if generated from the current invoice
// editor. Note the link is lost when GnuCash is restarted. This link
// may be restored by: scan the current session tabs, identify
// reports, checking whereby report's report-type matches an invoice
// report, and the report's invoice option value matches the current
// invoice.
void
gnc_invoice_window_print_cb (GtkWindow* parent, gpointer data)
{
    InvoiceWindow *iw = data;

    if (gnc_find_first_gui_component (WINDOW_REPORT_CM_CLASS, equal_fn,
                                      iw->reportPage))
        gnc_plugin_page_report_reload (GNC_PLUGIN_PAGE_REPORT (iw->reportPage));
    else
        iw->reportPage = gnc_invoice_window_print_invoice
            (parent, iw_get_invoice (iw));

    gnc_main_window_open_page (GNC_MAIN_WINDOW (iw->dialog), iw->reportPage);
}

static gboolean
gnc_dialog_post_invoice(
    InvoiceWindow *iw,
    char *message,
    time64 *ddue,
    time64 *postdate,
    char **memo,
    Account **acc,
    gboolean *accumulate)
{
    GncInvoice *invoice;
    char *ddue_label, *post_label, *acct_label, *question_label;
    GList * acct_types = NULL;
    GList * acct_commodities = NULL;
    QofInstance *owner_inst;
    EntryList *entries, *entries_iter;

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return FALSE;

    ddue_label = _("Due Date");
    post_label = _("Post Date");
    acct_label = _("Post to Account");
    question_label = _("Accumulate Splits?");

    // Determine the type of account to post to
    acct_types = gncOwnerGetAccountTypesList (&(iw->owner));

    // Determine which commodity we're working with
    acct_commodities = gncOwnerGetCommoditiesList(&(iw->owner));

    // Get the invoice entries
    entries = gncInvoiceGetEntries (invoice);

    // Find the most suitable post date:
    // * CoOwner -> Settlements: that would be today.
    // * Customer -> Invoices: that would be today.
    // * Vendor -> Bills: most recent invoice entry (fallback today).
    // * Employee -> Vouchers: most recent voucher entry (fallback today).
    *postdate = gnc_time(NULL);

    if (entries && (
           (gncInvoiceGetOwnerType (invoice) == GNC_OWNER_COOWNER) ||
           (gncInvoiceGetOwnerType (invoice) == GNC_OWNER_EMPLOYEE) ||
           (gncInvoiceGetOwnerType (invoice) == GNC_OWNER_VENDOR)))

    {
        *postdate = gncEntryGetDate ((GncEntry*)entries->data);
        for (entries_iter = entries; entries_iter != NULL; entries_iter = g_list_next(entries_iter))
        {
            time64 entrydate = gncEntryGetDate ((GncEntry*)entries_iter->data);
            if (entrydate > *postdate)
                *postdate = entrydate;
        }
    }

    // Get the due date and posted account
    *ddue = *postdate;
    *memo = NULL;
    {
    GncGUID *guid = NULL;
    owner_inst = qofOwnerGetOwner (gncOwnerGetEndOwner (&(iw->owner)));
    qof_instance_get (owner_inst,
                      "invoice-last-posted-account", &guid,
                      NULL);
    *acc = xaccAccountLookup (guid, iw->book);
    }
    // Get the default for the accumulate option
    *accumulate = gnc_prefs_get_bool(GNC_PREFS_GROUP_INVOICE, GNC_PREF_ACCUM_SPLITS);

    if (!gnc_dialog_dates_acct_question_parented (iw_get_window(iw), message, ddue_label,
            post_label, acct_label, question_label, TRUE, TRUE,
            acct_types, acct_commodities, iw->book, iw->terms,
            ddue, postdate, memo, acc, accumulate))
        return FALSE;

    return TRUE;
}

struct post_invoice_params
{
    time64 ddue;            /* Due date */
    time64 postdate;        /* Date posted */
    char *memo;             /* Memo for posting transaction */
    Account *acc;           /* Account to post to */
    gboolean accumulate;    /* Whether to accumulate splits */
    GtkWindow *parent;
};

static void
gnc_invoice_post(InvoiceWindow *iw, struct post_invoice_params *post_params)
{
    GncInvoice *invoice;
    char *message, *memo;
    Account *acc = NULL;
    time64 ddue, postdate;
    gboolean accumulate;
    QofInstance *owner_inst;
    const char *text;
    GHashTable *foreign_currs;
    GHashTableIter foreign_currs_iter;
    gpointer key, value;
    gboolean is_owner_doc;
    gboolean auto_pay;
    gboolean show_dialog = TRUE;
    gboolean post_ok = TRUE;

    // Make sure the invoice is ok
    if (!gnc_invoice_window_verify_ok (iw))
        return;

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    // Check that there is at least one Entry
    if (gncInvoiceGetEntries (invoice) == NULL)
    {
        gnc_error_dialog (GTK_WINDOW (iw_get_window(iw)), "%s",
            _("The Invoice must have at least one Entry."));
        return;
    }

    is_owner_doc = (
        gncInvoiceGetOwnerType (invoice) == GNC_OWNER_COOWNER ||
        gncInvoiceGetOwnerType (invoice) == GNC_OWNER_CUSTOMER);

    // Ok, we can post this invoice.  Ask for verification, set the due date,
    // post date, and posted account
    if (post_params)
    {
        ddue = post_params->ddue;
        postdate = post_params->postdate;
        // Dup it since it will be free later on
        memo = g_strdup (post_params->memo);
        acc = post_params->acc;
        accumulate = post_params->accumulate;
    }
    else
    {
        message = _("Do you really want to post the invoice?");
        if (!gnc_dialog_post_invoice(
               iw, message, &ddue, &postdate, &memo, &acc, &accumulate))
            return;
    }

    // Yep, we're posting.  So, save the invoice...
    // Note that we can safely ignore the return value since we checked
    // the verify_ok earlier. We assure it's ok.
    // Additionally make sure the invoice has the owner's currency
    // refer to https://bugs.gnucash.org/show_bug.cgi?id=728074
    gnc_suspend_gui_refresh ();
    gncInvoiceBeginEdit (invoice);
    gnc_invoice_window_ok_save (iw);
    gncInvoiceSetCurrency (invoice, gncOwnerGetCurrency (gncInvoiceGetOwner (invoice)));

    /* Fill in the conversion prices with feedback from the user */
    text = _("One or more of the entries are for accounts different "
             "from the invoice/bill currency. "
             "You will be asked a conversion rate for each.");

    // Ask the user for conversion rates for all foreign currencies
    // (relative to the invoice currency)
    foreign_currs = gncInvoiceGetForeignCurrencies (invoice);
    g_hash_table_iter_init (&foreign_currs_iter, foreign_currs);
    while (g_hash_table_iter_next (&foreign_currs_iter, &key, &value))
    {
        GNCPrice *convprice;
        gnc_commodity *account_currency = (gnc_commodity*)key;
        gnc_numeric *amount = (gnc_numeric*)value;
        XferDialog *xfer;
        gnc_numeric exch_rate;


        // Explain to the user we're about to ask for an exchange rate.
        // Only show this dialog once, right before the first xfer dialog pops up.
        if (show_dialog)
        {
            gnc_info_dialog(GTK_WINDOW (iw_get_window(iw)), "%s", text);
            show_dialog = FALSE;
        }

        // Note some twisted logic here:
        // We ask the exchange rate
        //  FROM invoice currency
        //  TO other account currency
        //  Because that's what happens logically.
        //  But the internal posting logic works backwards:
        //  It searches for an exchange rate
        //  FROM other account currency
        //  TO invoice currency
        //  So we will store the inverted exchange rate
        //

        // create the exchange-rate dialog
        xfer = gnc_xfer_dialog (iw_get_window(iw), acc);
        gnc_xfer_dialog_is_exchange_dialog(xfer, &exch_rate);
        gnc_xfer_dialog_select_to_currency(xfer, account_currency);
        gnc_xfer_dialog_set_date (xfer, postdate);
        // Even if amount is 0 ask for an exchange rate. It's required
        // for the transaction generating code. Use an amount of 1 in
        // that case as the dialog won't allow to specify an exchange
        // rate for 0.
        gnc_xfer_dialog_set_amount(xfer, gnc_numeric_zero_p (*amount) ?
                                         (gnc_numeric){1, 1} : *amount);
        // If we already had an exchange rate from a previous post operation,
        // set it here
        convprice = gncInvoiceGetPrice (invoice, account_currency);
        if (convprice)
        {
            exch_rate = gnc_price_get_value (convprice);
            /* Invert the exchange rate as explained above */
            if (!gnc_numeric_zero_p (exch_rate))
            {
                exch_rate = gnc_numeric_div ((gnc_numeric){1, 1}, exch_rate,
                            GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
                gnc_xfer_dialog_set_price_edit (xfer, exch_rate);
            }
        }

        // All we want is the exchange rate so prevent the user from thinking
        // it makes sense to mess with other stuff */
        gnc_xfer_dialog_set_from_show_button_active(xfer, FALSE);
        gnc_xfer_dialog_set_to_show_button_active(xfer, FALSE);
        gnc_xfer_dialog_hide_from_account_tree(xfer);
        gnc_xfer_dialog_hide_to_account_tree(xfer);
        if (gnc_xfer_dialog_run_until_done(xfer))
        {
          // User finished the transfer dialog successfully

            // Invert the exchange rate as explained above
            if (!gnc_numeric_zero_p (exch_rate))
                exch_rate = gnc_numeric_div (
                    (gnc_numeric){1, 1}, exch_rate,
                    GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
            convprice = gnc_price_create(iw->book);
            gnc_price_begin_edit (convprice);
            gnc_price_set_commodity (convprice, account_currency);
            gnc_price_set_currency (convprice, gncInvoiceGetCurrency (invoice));
            gnc_price_set_time64 (convprice, postdate);
            gnc_price_set_source (convprice, PRICE_SOURCE_TEMP);
            gnc_price_set_typestr (convprice, PRICE_TYPE_LAST);
            gnc_price_set_value (convprice, exch_rate);
            gncInvoiceAddPrice(invoice, convprice);
            gnc_price_commit_edit (convprice);
        }
        else
        {
            // User canceled the transfer dialog, abort posting
            post_ok = FALSE;
            goto cleanup;
        }
    }


    // Save account as last used account in the owner's
    // invoice-last-posted-account property.
    owner_inst = qofOwnerGetOwner (gncOwnerGetEndOwner (&(iw->owner)));
    {
    const GncGUID *guid = qof_instance_get_guid (QOF_INSTANCE (acc));
    qof_begin_edit (owner_inst);
    qof_instance_set (
        owner_inst, "invoice-last-posted-account", guid, NULL);
    qof_commit_edit (owner_inst);
    }

    // ... post it ...
    if (is_owner_doc)
        auto_pay = gnc_prefs_get_bool (GNC_PREFS_GROUP_INVOICE, GNC_PREF_AUTO_PAY);
    else
        auto_pay = gnc_prefs_get_bool (GNC_PREFS_GROUP_BILL, GNC_PREF_AUTO_PAY);

    gncInvoicePostToAccount (invoice, acc, postdate, ddue, memo, accumulate, auto_pay);

cleanup:
    gncInvoiceCommitEdit (invoice);
    g_hash_table_unref (foreign_currs);
    gnc_resume_gui_refresh ();

    if (memo)
        g_free (memo);

    if (post_ok)
    {
        // Reset the type; change to read-only!
        iw->dialog_type = VIEW_INVOICE;
        gnc_entry_ledger_set_readonly (iw->ledger, TRUE);
    }
    else
    {
        text = _("The post action was canceled "
                 "because not all exchange rates were given.");
        gnc_info_dialog(GTK_WINDOW (iw_get_window(iw)), "%s", text);
    }

    // ... and redisplay here.
    gnc_invoice_update_window (iw, NULL);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), FALSE);
}

void
gnc_invoice_window_post_cb (GtkWidget *unused_widget, gpointer data)
{
    InvoiceWindow *iw =data;
    gnc_invoice_post(iw, NULL);
}

void
gnc_invoice_window_unpost_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice;
    gboolean result;

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    // make sure the user REALLY wants to do this!
    result = iw_ask_unpost(iw);
    if (!result) return;

    // Attempt to unpost the invoice
    gnc_suspend_gui_refresh ();
    result = gncInvoiceUnpost (invoice, iw->reset_tax_tables);
    gnc_resume_gui_refresh ();
    if (!result) return;

    // if we get here, we succeeded in unposting -- reset the ledger and redisplay
    iw->dialog_type = EDIT_INVOICE;
    gnc_entry_ledger_set_readonly (iw->ledger, FALSE);
    gnc_invoice_update_window (iw, NULL);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), FALSE);
}

void
gnc_invoice_window_cut_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_cut_clipboard (iw->reg);
}

void
gnc_invoice_window_copy_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_copy_clipboard (iw->reg);
}

void
gnc_invoice_window_paste_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_paste_clipboard (iw->reg);
}

void
gnc_invoice_window_new_invoice_cb (GtkWindow *parent, gpointer data)
{
    InvoiceWindow *iw = data;

    if (gncOwnerGetJob (&iw->job))
    {
        // Handle a job: the job groups owner assigned invoices
        gnc_ui_invoice_new (parent, &iw->job, iw->book);
    }
    else
    {
        // Handle single owner assigned invoice
        gnc_ui_invoice_new (parent, &iw->owner, iw->book);
    }
}

void
gnc_business_call_owner_report (
    GtkWindow *parent,
    GncOwner *owner,
    Account *acc)
{
    gnc_business_call_owner_report_with_enddate (parent, owner, acc, INT64_MAX);
}

void
gnc_business_call_owner_report_with_enddate (
    GtkWindow *parent,
    GncOwner *owner,
    Account *acc,
    time64 enddate)
{
    int id;
    SCM args;
    SCM func;
    SCM arg;

    g_return_if_fail (owner);

    args = SCM_EOL;

    func = scm_c_eval_string ("gnc:owner-report-create-with-enddate");
    g_return_if_fail (scm_is_procedure (func));

    // set the enddate
    arg = (enddate != INT64_MAX) ? scm_from_int64 (enddate) : SCM_BOOL_F;
    args = scm_cons (arg, args);

    if (acc)
    {
        swig_type_info * qtype = SWIG_TypeQuery("_p_Account");
        g_return_if_fail (qtype);

        arg = SWIG_NewPointerObj(acc, qtype, 0);
        g_return_if_fail (arg != SCM_UNDEFINED);
        args = scm_cons (arg, args);
    }
    else
    {
        args = scm_cons (SCM_BOOL_F, args);
    }

    arg = SWIG_NewPointerObj(owner, SWIG_TypeQuery("_p__gncOwner"), 0);
    g_return_if_fail (arg != SCM_UNDEFINED);
    args = scm_cons (arg, args);

    // Apply the function to the args
    arg = scm_apply (func, args, SCM_EOL);
    g_return_if_fail (scm_is_exact (arg));
    id = scm_to_int (arg);

    if (id >= 0)
        reportWindow (id, parent);
}

void
gnc_invoice_window_report_owner_cb (GtkWindow *parent, gpointer data)
{
    InvoiceWindow *iw = data;
    gnc_business_call_owner_report (parent, &iw->owner, NULL);
}

void
gnc_invoice_window_payment_cb (GtkWindow *parent, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (gncOwnerGetJob (&iw->job))
        gnc_ui_payment_new_with_invoice (parent, &iw->job, iw->book, invoice);
    else
        gnc_ui_payment_new_with_invoice (parent, &iw->owner, iw->book, invoice);
}

// Sorting callbacks

void
gnc_invoice_window_sort (InvoiceWindow *iw, invoice_sort_type_t sort_code)
{
    QofQuery *query = gnc_entry_ledger_get_query (iw->ledger);
    GSList *p1 = NULL, *p2 = NULL, *p3 = NULL, *standard;

    if (iw->last_sort == sort_code)
        return;

    standard = g_slist_prepend (NULL, QUERY_DEFAULT_SORT);

    switch (sort_code)
    {
    case INVSORT_BY_STANDARD:
        p1 = standard;
        break;
    case INVSORT_BY_DATE:
        p1 = g_slist_prepend (p1, ENTRY_DATE);
        p2 = standard;
        break;
    case INVSORT_BY_DATE_ENTERED:
        p1 = g_slist_prepend (p1, ENTRY_DATE_ENTERED);
        p2 = standard;
        break;
    case INVSORT_BY_DESC:
        p1 = g_slist_prepend (p1, ENTRY_DESC);
        p2 = standard;
        break;
    case INVSORT_BY_QTY:
        p1 = g_slist_prepend (p1, ENTRY_QTY);
        p2 = standard;
        break;
    case INVSORT_BY_PRICE:
        p1 = g_slist_prepend (
            p1,
            ((iw->owner.type == GNC_OWNER_COOWNER ||
              iw->owner.type == GNC_OWNER_CUSTOMER)) ?
            ENTRY_IPRICE : ENTRY_BPRICE);
        p2 = standard;
        break;
    default:
        g_slist_free (standard);
        g_return_if_fail (FALSE);
        break;
    }

    qof_query_set_sort_order (query, p1, p2, p3);
    iw->last_sort = sort_code;
    gnc_entry_ledger_display_refresh (iw->ledger);
}

// Window configuration callbacks

void
gnc_invoice_window_active_toggled_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (!invoice) return;

    gncInvoiceSetActive (
        invoice, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

gboolean
gnc_invoice_window_leave_notes_cb (
    GtkWidget *widget,
    GdkEventFocus *event,
    gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);
    GtkTextBuffer* text_buffer;
    GtkTextIter start, end;
    gchar *text;

    if (!invoice) return FALSE;

    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->entry_notes));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncInvoiceSetNotes (invoice, text);
    return FALSE;
}

static gboolean
gnc_invoice_window_leave_to_charge_cb (
    GtkWidget *widget,
    GdkEventFocus *event,
    gpointer data)
{
    gnc_amount_edit_evaluate (GNC_AMOUNT_EDIT(data), NULL);
    return FALSE;
}

static void
gnc_invoice_window_changed_to_charge_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (!invoice) return;

    gncInvoiceSetToChargeAmount (invoice, gnc_amount_edit_get_amount
                                 (GNC_AMOUNT_EDIT (widget)));
}

static GtkWidget *
add_summary_label (GtkWidget *summarybar, const char *label_str)
{
    GtkWidget *entry_box;
    GtkWidget *label;

    entry_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_set_homogeneous (GTK_BOX (entry_box), FALSE);
    gtk_box_pack_start (GTK_BOX(summarybar), entry_box, FALSE, FALSE, 5);

    label = gtk_label_new (label_str);
    gnc_label_set_alignment (label, 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX(entry_box), label, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gnc_label_set_alignment (label, 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX(entry_box), label, FALSE, FALSE, 0);

    return label;
}

GtkWidget *
gnc_invoice_window_create_summary_bar (InvoiceWindow *iw)
{
    GtkWidget *summarybar;

    iw->total_label           = NULL;
    iw->total_cash_label      = NULL;
    iw->total_charge_label    = NULL;
    iw->total_subtotal_label  = NULL;
    iw->total_tax_label       = NULL;

    summarybar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_set_homogeneous (GTK_BOX (summarybar), FALSE);
    gtk_widget_set_name (summarybar, "gnc-id-summarybar");

    iw->total_label           = add_summary_label (summarybar, _("Total:"));

    switch (gncOwnerGetType (&iw->owner))
    {
    case GNC_OWNER_COOWNER:
    case GNC_OWNER_CUSTOMER:
    case GNC_OWNER_VENDOR:
        iw->total_subtotal_label = add_summary_label (summarybar, _("Subtotal:"));
        iw->total_tax_label     = add_summary_label (summarybar, _("Tax:"));
        break;

    case GNC_OWNER_EMPLOYEE:
        iw->total_cash_label    = add_summary_label (summarybar, _("Total Cash:"));
        iw->total_charge_label  = add_summary_label (summarybar, _("Total Charge:"));
        break;

    default:
        break;
    }

    gtk_widget_show_all(summarybar);
    return summarybar;
}

static int
gnc_invoice_job_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    char const *msg = "";

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gnc_owner_get_owner (iw->choice_job, &(iw->job));

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    msg = gncJobGetReference (gncOwnerGetJob (&(iw->job)));
    gtk_entry_set_text (GTK_ENTRY (iw->entry_billing_id), msg ? msg : "");

    return FALSE;

}

static GNCSearchWindow
*gnc_invoice_select_job_cb (
    GtkWindow *parent,
    gpointer jobp,
    gpointer user_data)
{
    GncJob *j = jobp;
    InvoiceWindow *iw = user_data;
    GncOwner owner;
    const GncOwner *ownerp;

    if (!iw) return NULL;

    if (j)
    {
        ownerp = gncJobGetOwner (j);
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerCopy (&(iw->owner), &owner);

    return gnc_job_search (parent, j, &owner, iw->book);
}

static void
gnc_invoice_update_job_choice (InvoiceWindow *iw)
{
    if (iw->choice_job)
        gtk_container_remove (GTK_CONTAINER (iw->entry_job), iw->choice_job);

    // If we don't have a real owner, then we obviously can't have a job
    if (iw->owner.owner.undefined == NULL)
    {
        iw->choice_job = NULL;
    }
    else
        switch (iw->dialog_type)
        {
        case VIEW_INVOICE:
        case EDIT_INVOICE:
            iw->choice_job =
                gnc_owner_edit_create (NULL, iw->entry_job, iw->book, &(iw->job));
            break;
        case NEW_INVOICE:
        case MOD_INVOICE:
        case DUP_INVOICE:
            iw->choice_job =
                gnc_general_search_new (GNC_JOB_MODULE_NAME, _("Select..."),
                    TRUE, gnc_invoice_select_job_cb, iw, iw->book);

            gnc_general_search_set_selected (
                GNC_GENERAL_SEARCH (iw->choice_job), gncOwnerGetJob (&iw->job));
            gnc_general_search_allow_clear (
                GNC_GENERAL_SEARCH (iw->choice_job), TRUE);
            gtk_box_pack_start (
                GTK_BOX (iw->entry_job), iw->choice_job, TRUE, TRUE, 0);

            g_signal_connect (G_OBJECT (iw->choice_job), "changed",
                G_CALLBACK (gnc_invoice_job_changed_cb), iw);
            break;
        }

    if (iw->choice_job)
        gtk_widget_show_all (iw->choice_job);
}

static GNCSearchWindow
*gnc_invoice_select_project_job_cb (
    GtkWindow *parent,
    gpointer jobp,
    gpointer user_data)
{
    GncJob *j = jobp;
    InvoiceWindow *iw = user_data;
    GncOwner owner;
    const GncOwner *ownerp = NULL;

    if (!iw) return NULL;

    if (j)
    {
        ownerp = gncJobGetOwner (j);
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerCopy (&(iw->project_owner), &owner);

    return gnc_job_search (parent, j, &owner, iw->book);
}

static int
gnc_invoice_project_job_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gnc_owner_get_owner (iw->choice_project_job, &(iw->project_job));
    return FALSE;
}

static void
gnc_invoice_update_project_job (InvoiceWindow *iw)
{
    if (iw->choice_project_job)
        gtk_container_remove (GTK_CONTAINER (iw->choice_project_job),
                              iw->choice_project_job);

    switch (iw->dialog_type)
    {
    case VIEW_INVOICE:
    case EDIT_INVOICE:
        iw->choice_project_job =
            gnc_owner_edit_create (NULL, iw->entry_project_job, iw->book, &(iw->project_job));
        break;
    case NEW_INVOICE:
    case MOD_INVOICE:
    case DUP_INVOICE:
        if (iw->project_owner.owner.undefined == NULL)
        {
            iw->choice_project_job = NULL;
        }
        else
        {
            iw->choice_project_job =
                gnc_general_search_new (GNC_JOB_MODULE_NAME, _("Select..."),
                   TRUE, gnc_invoice_select_project_job_cb, iw, iw->book);

            gnc_general_search_set_selected (
                GNC_GENERAL_SEARCH(iw->choice_project_job),
                gncOwnerGetJob (&iw->project_job));
            gnc_general_search_allow_clear (
                GNC_GENERAL_SEARCH (iw->choice_project_job), TRUE);
            gtk_box_pack_start (
               GTK_BOX (iw->choice_project_job), iw->choice_project_job,
                   TRUE, TRUE, 0);

            g_signal_connect (
                G_OBJECT (iw->choice_project_job), "changed",
                    G_CALLBACK (gnc_invoice_project_job_changed_cb), iw);
        }
        break;
    }

    if (iw->choice_project_job)
        gtk_widget_show_all (iw->choice_project_job);
}

static int
gnc_invoice_owner_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncBillTerm *term = NULL;
    GncOwner owner;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gncOwnerCopy (&(iw->owner), &owner);
    gnc_owner_get_owner (iw->choice_owner, &owner);

    // If owner has changed, then reset entities
    if (!gncOwnerEqual (&owner, &(iw->owner)))
    {
        gncOwnerCopy (&owner, &(iw->owner));
        gncOwnerInitJob (&(iw->job), NULL);
        gnc_entry_ledger_reset_query (iw->ledger);
    }

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    switch (gncOwnerGetType (&(iw->owner)))
    {
    case GNC_OWNER_COOWNER:
        term = gncCoOwnerGetTerms (gncOwnerGetCoOwner (&(iw->owner)));
        break;
    case GNC_OWNER_CUSTOMER:
        term = gncCustomerGetTerms (gncOwnerGetCustomer (&(iw->owner)));
        break;
    case GNC_OWNER_EMPLOYEE:
        term = NULL;
        break;
    case GNC_OWNER_VENDOR:
        term = gncVendorGetTerms (gncOwnerGetVendor (&(iw->owner)));
        break;
    default:
        g_warning ("Unknown owner type: %d\n", gncOwnerGetType (&(iw->owner)));
        break;
    }

    // Change the terms
    iw->terms = term;
    gnc_simple_combo_set_value (GTK_COMBO_BOX(iw->entry_terms), iw->terms);

    gnc_invoice_update_job_choice (iw);

    return FALSE;
}

static int
gnc_invoice_project_owner_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncOwner owner;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gncOwnerCopy (&(iw->project_owner), &owner);
    gnc_owner_get_owner (iw->choice_project_owner, &owner);

    // If this owner really changed, then reset ourselves
    if (!gncOwnerEqual (&owner, &(iw->project_owner)))
    {
        gncOwnerCopy (&owner, &(iw->project_owner));
        gncOwnerInitJob (&(iw->project_job), NULL);
    }

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    gnc_invoice_update_project_job (iw);

    return FALSE;
}

static void
gnc_invoice_dialog_close_handler (gpointer user_data)
{
    InvoiceWindow *iw = user_data;

    if (iw)
    {
        gtk_widget_destroy (iw->dialog);
    }
}

static void
gnc_invoice_window_close_handler (gpointer user_data)
{
    InvoiceWindow *iw = user_data;

    if (iw)
    {
        gnc_main_window_close_page(iw->page);
        iw->page = NULL;
    }
}

static void
gnc_invoice_reset_total_label (
    GtkLabel *label,
    gnc_numeric amt,
    gnc_commodity *com)
{
    char string[256];
    gchar *bidi_string;

    amt = gnc_numeric_convert (
        amt, gnc_commodity_get_fraction(com), GNC_HOW_RND_ROUND_HALF_UP);
    xaccSPrintAmount (string, amt, gnc_commodity_print_info (com, TRUE));

    bidi_string = gnc_wrap_text_with_bidi_ltr_isolate (string);
    gtk_label_set_text (label, bidi_string);
    g_free (bidi_string);
}

static void
gnc_invoice_redraw_all_cb (GnucashRegister *g_reg, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice * invoice;
    gnc_commodity * currency;
    gnc_numeric amount, to_charge_amt = gnc_numeric_zero();

    if (!iw)
        return;

    //  if (iw)
    //    gnc_invoice_update_window (iw, NULL);

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    currency = gncInvoiceGetCurrency (invoice);

    if (iw->total_label)
    {
        amount = gncInvoiceGetTotal (invoice);
        gnc_invoice_reset_total_label (
            GTK_LABEL (iw->total_label), amount, currency);
    }

    if (iw->total_subtotal_label)
    {
        amount = gncInvoiceGetTotalSubtotal (invoice);
        gnc_invoice_reset_total_label (
            GTK_LABEL (iw->total_subtotal_label), amount, currency);
    }

    if (iw->total_tax_label)
    {
        amount = gncInvoiceGetTotalTax (invoice);
        gnc_invoice_reset_total_label (
            GTK_LABEL (iw->total_tax_label), amount, currency);
    }

    // Deal with extra items for the expense voucher
    if (iw->entry_charge_to)
    {
        gnc_amount_edit_evaluate (
            GNC_AMOUNT_EDIT (iw->entry_charge_to), NULL);
        to_charge_amt = gnc_amount_edit_get_amount(
            GNC_AMOUNT_EDIT(iw->entry_charge_to));
    }

    if (iw->total_cash_label)
    {
        amount = gncInvoiceGetTotalOf (invoice, GNC_PAYMENT_CASH);
        amount = gnc_numeric_sub (amount, to_charge_amt,
            gnc_commodity_get_fraction (currency), GNC_HOW_RND_ROUND_HALF_UP);
        gnc_invoice_reset_total_label (
            GTK_LABEL (iw->total_cash_label), amount, currency);
    }

    if (iw->total_charge_label)
    {
        amount = gncInvoiceGetTotalOf (invoice, GNC_PAYMENT_CARD);
        amount = gnc_numeric_add (amount, to_charge_amt,
            gnc_commodity_get_fraction (currency), GNC_HOW_RND_ROUND_HALF_UP);
        gnc_invoice_reset_total_label (
            GTK_LABEL (iw->total_charge_label), amount, currency);
    }
}

void
gnc_invoice_window_changed (InvoiceWindow *iw, GtkWidget *window)
{
    gnc_entry_ledger_set_parent(iw->ledger, window);
}

gchar *
gnc_invoice_get_help (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return gnc_table_get_help (gnc_entry_ledger_get_table (iw->ledger));
}

static void
gnc_invoice_window_refresh_handler (GHashTable *changes, gpointer user_data)
{
    InvoiceWindow *iw = user_data;
    const EventInfo *info;
    GncInvoice *invoice = iw_get_invoice (iw);
    const GncOwner *owner;

    // If there isn't an invoice behind us, close down
    if (!invoice)
    {
        gnc_close_gui_component (iw->component_id);
        return;
    }

    // Close if a destroy event is triggered
    if (changes)
    {
        info = gnc_gui_get_entity_events (changes, &iw->invoice_guid);
        if (info && (info->event_mask & QOF_EVENT_DESTROY))
        {
            gnc_close_gui_component (iw->component_id);
            return;
        }
    }

    // Check the owners, and see if they have changed
    owner = gncInvoiceGetOwner (invoice);

    // Copy the owner information into our window
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (owner));

    // Copy the billto information into our window
    owner = gncInvoiceGetBillTo (invoice);
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &iw->project_owner);
    gncOwnerInitJob (&iw->project_job, gncOwnerGetJob (owner));

    // Finally, refresh ourselves
    gnc_invoice_update_window (iw, NULL);
}

/** Update the various widgets in the window/page based upon the data
 *  in the InvoiceWindow data structure.
 *
 *  @param iw A pointer to the InvoiceWindow data structure.
 *
 *  @param widget If set, this is the widget that will be used for the
 *  call to gtk_widget_show_all().  This is needed at window/page
 *  creation time when all of the iw/page linkages haven't been set up
 *  yet.
 */
static void
gnc_invoice_update_window (InvoiceWindow *iw, GtkWidget *widget)
{
    GtkWidget *entry_account_posted;
    GncInvoice *invoice;
    gboolean is_posted = FALSE;
    gboolean can_unpost = FALSE;

    invoice = iw_get_invoice (iw);

    if (iw->choice_owner)
        gtk_container_remove (GTK_CONTAINER (iw->entry_owner), iw->choice_owner);

    if (iw->choice_project_owner)
        gtk_container_remove (
            GTK_CONTAINER (iw->entry_project_owner),
            iw->choice_project_owner);

    switch (iw->dialog_type)
    {
    case VIEW_INVOICE:
    case EDIT_INVOICE:
        iw->choice_owner =
            gnc_owner_edit_create (
                iw->label_owner, iw->entry_owner, iw->book, &(iw->owner));
        iw->choice_project_owner =
            gnc_owner_edit_create (
                NULL, iw->entry_project_owner, iw->book, &(iw->project_owner));
        break;
    case NEW_INVOICE:
    case MOD_INVOICE:
    case DUP_INVOICE:
        iw->choice_owner =
            gnc_owner_select_create (
                iw->label_owner, iw->entry_owner, iw->book, &(iw->owner));
        iw->choice_project_owner =
            gnc_owner_select_create (
                NULL, iw->entry_project_owner, iw->book, &(iw->project_owner));

        g_signal_connect (
            G_OBJECT (iw->choice_owner), "changed",
            G_CALLBACK (gnc_invoice_owner_changed_cb), iw);

        g_signal_connect (
            G_OBJECT (iw->choice_project_owner), "changed",
            G_CALLBACK (gnc_invoice_project_owner_changed_cb), iw);

        break;
    }

    // Set the type label
    gtk_label_set_text (
        GTK_LABEL(iw->label_type), iw->is_credit_note ? _("Credit Note")
        : gtk_label_get_text (GTK_LABEL(iw->label_type)));

    if (iw->choice_owner)
        gtk_widget_show_all (iw->choice_owner);
    if (iw->choice_project_owner)
        gtk_widget_show_all (iw->choice_project_owner);

    gnc_invoice_update_job_choice (iw);
    gnc_invoice_update_project_job (iw);

    // Hide the project frame for coowner invoices
    if (iw->owner.type == GNC_OWNER_COOWNER)
        gtk_widget_hide (iw->frame_project);

    // Hide the project frame for customer invoices
    if (iw->owner.type == GNC_OWNER_CUSTOMER)
        gtk_widget_hide (iw->frame_project);

    // Hide the "job" label and entry for employee invoices
    if (iw->owner.type == GNC_OWNER_EMPLOYEE)
    {
        gtk_widget_hide (iw->label_job);
        gtk_widget_hide (iw->entry_job);
    }

    // Set the account name that posted the invoice
    entry_account_posted = GTK_WIDGET (
        gtk_builder_get_object (iw->builder, "invoice_entry_acct_posted"));

    // From here on, "invoice" (and "owner") entites exist
    {
        GtkTextBuffer* text_buffer;
        const char *string;
        gchar * tmp_string;
        time64 time;

        gtk_entry_set_text (GTK_ENTRY (
            iw->entry_invoice_id), gncInvoiceGetID (invoice));

        gtk_entry_set_text (GTK_ENTRY (
            iw->entry_billing_id), gncInvoiceGetBillingID (invoice));

        string = gncInvoiceGetNotes (invoice);
        text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->entry_notes));
        gtk_text_buffer_set_text (text_buffer, string, -1);

        if (iw->checkbutton_active)
            gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (iw->checkbutton_active),
                gncInvoiceGetActive (invoice));

        time = gncInvoiceGetDateOpened (invoice);
        if (time == INT64_MAX)
        {
            gnc_date_edit_set_time (GNC_DATE_EDIT (
                iw->entry_date_opened), gnc_time (NULL));
        }
        else
        {
            gnc_date_edit_set_time (GNC_DATE_EDIT (
                iw->entry_date_opened), time);
        }

        // fill in the terms text
        iw->terms = gncInvoiceGetTerms (invoice);

        switch (iw->dialog_type)
        {
            case NEW_INVOICE:
            case MOD_INVOICE:
            case DUP_INVOICE: //??
                gnc_simple_combo_set_value (
                    GTK_COMBO_BOX(iw->entry_terms), iw->terms);
                break;

            case EDIT_INVOICE:
            case VIEW_INVOICE:
                // Fill in the invoice view version
                if(gncBillTermGetName (iw->terms) != NULL)
                    gtk_entry_set_text (GTK_ENTRY (
                        iw->entry_terms), gncBillTermGetName (iw->terms));
                else
                    gtk_entry_set_text (GTK_ENTRY (iw->entry_terms),"None");
                break;

            default:
                break;
        }

        // Next, figure out if we've been posted, and if so set the
        // appropriate bits of information... Then work on hiding or
        // showing as necessary.
        is_posted = gncInvoiceIsPosted (invoice);
        if (is_posted)
        {
            Account *acct = gncInvoiceGetPostedAcc (invoice);

            // Can we unpost this invoice?
            // XXX: right now we always can, but there may be times
            // in the future when we cannot.
            can_unpost = TRUE;

            time = gncInvoiceGetDatePosted (invoice);
            gnc_date_edit_set_time (GNC_DATE_EDIT (iw->date_posted), time);

            tmp_string = gnc_account_get_full_name (acct);
            gtk_entry_set_text (GTK_ENTRY (entry_account_posted), tmp_string);
            g_free(tmp_string);
        }
    }

    gnc_invoice_id_changed_cb(NULL, iw);
    if (iw->dialog_type == NEW_INVOICE ||
            iw->dialog_type == DUP_INVOICE ||
            iw->dialog_type == MOD_INVOICE)
    {
        if (widget)
            gtk_widget_show (widget);
        else
            gtk_widget_show (iw_get_window(iw));
        return;
    }

    // Fill in the to_charge amount (only in VIEW/EDIT modes)
    {
        gnc_numeric amount;

        amount = gncInvoiceGetToChargeAmount (invoice);
        gnc_amount_edit_set_amount (GNC_AMOUNT_EDIT (iw->entry_charge_to), amount);
    }

    // Hide/show the appropriate widgets based on our posted/paid state

    {
        GtkWidget *hide, *show;

        if (is_posted)
        {
            show = GTK_WIDGET (gtk_builder_get_object (
                iw->builder, "label_date_posted"));
            gtk_widget_show (show);
            gtk_widget_show (iw->entry_date_posted);
            show = GTK_WIDGET (gtk_builder_get_object (
                iw->builder, "label_account_posted"));
            gtk_widget_show (show);
            gtk_widget_show (entry_account_posted);
        }
        else
        {
            //  we aren't posted */
            hide = GTK_WIDGET (gtk_builder_get_object (
                iw->builder, "label_posted"));
            gtk_widget_hide (hide);
            gtk_widget_hide (iw->entry_date_posted);

            hide = GTK_WIDGET (gtk_builder_get_object (
                iw->builder, "label_account_posted"));
            gtk_widget_hide (hide);
            gtk_widget_hide (entry_account_posted);
        }
    }

    // Set the toolbar widgets sensitivity
    if (iw->page)
        gnc_plugin_page_invoice_update_menus(
            iw->page, is_posted, can_unpost);

    // Set the to_charge widget
    gtk_widget_set_sensitive (iw->entry_charge_to, !is_posted);

    // Hide the to_charge frame for all non-employee invoices, or set
    // insensitive if the employee does not have a charge card
    if (iw->owner.type == GNC_OWNER_EMPLOYEE)
    {
        if (!gncEmployeeGetCCard (gncOwnerGetEmployee(&iw->owner)))
            gtk_widget_set_sensitive (iw->entry_charge_to, FALSE);
    }
    else
    {
        gtk_widget_hide (iw->frame_charge_to);
    }

    if (is_posted)
    {
        // Entries are already posted -> hide entries and
        // setup viewer for read-only access
        gtk_widget_set_sensitive (entry_account_posted, FALSE);
        gtk_widget_set_sensitive (iw->entry_invoice_id, FALSE); /* XXX: why set FALSE and then TRUE? */
        gtk_widget_set_sensitive (iw->entry_invoice_id, TRUE);
        gtk_widget_set_sensitive (iw->entry_terms, FALSE);
        gtk_widget_set_sensitive (iw->entry_owner, TRUE);
        gtk_widget_set_sensitive (iw->entry_job, TRUE);
        gtk_widget_set_sensitive (iw->entry_billing_id, FALSE);
        gtk_widget_set_sensitive (iw->entry_notes, TRUE);
    }
    else
    {
        // Entries needs to be posted ->
        // setup viewer for read-write access
        gtk_widget_set_sensitive (entry_account_posted, TRUE);
        gtk_widget_set_sensitive (iw->entry_terms, TRUE);
        gtk_widget_set_sensitive (iw->entry_owner, TRUE);
        gtk_widget_set_sensitive (iw->entry_job, TRUE);
        gtk_widget_set_sensitive (iw->entry_billing_id, TRUE);
        gtk_widget_set_sensitive (iw->entry_notes, TRUE);
    }

    // Translators: This is a label to show whether the invoice is paid or not.
    if(gncInvoiceIsPaid (invoice))
        gtk_label_set_text(GTK_LABEL(iw->label_paid),  _("PAID"));
    else
        gtk_label_set_text(GTK_LABEL(iw->label_paid),  _("UNPAID"));

    if (widget)
        gtk_widget_show (widget);
    else
        gtk_widget_show (iw_get_window(iw));
}

GncInvoiceType
gnc_invoice_get_type_from_window (InvoiceWindow *iw)
{
    // uses the same approach as gnc_invoice_get_title
    // not called gnc_invoice_get_type because of name collisions
    switch (gncOwnerGetType(&iw->owner))
    {
    case GNC_OWNER_COOWNER:
        return iw->is_credit_note ? GNC_INVOICE_COOWNER_CREDIT_NOTE
                                  : GNC_INVOICE_COOWNER_INVOICE;
        break;
    case GNC_OWNER_CUSTOMER:
        return iw->is_credit_note ? GNC_INVOICE_CUST_CREDIT_NOTE
                                  : GNC_INVOICE_CUST_INVOICE;
        break;
    case GNC_OWNER_EMPLOYEE:
        return iw->is_credit_note ? GNC_INVOICE_EMPL_CREDIT_NOTE
                                  : GNC_INVOICE_EMPL_INVOICE;
        break;
    case GNC_OWNER_VENDOR:
        return iw->is_credit_note ? GNC_INVOICE_VEND_CREDIT_NOTE
                                  : GNC_INVOICE_VEND_INVOICE;
        break;
    default:
        return GNC_INVOICE_UNDEFINED;
        break;
    }
}

gchar
*gnc_invoice_get_title (InvoiceWindow *iw)
{
    char *wintitle = NULL;
    const char *id = NULL;

    if (!iw) return NULL;

    switch (gncOwnerGetType (&iw->owner))
    {
    case GNC_OWNER_COOWNER:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = iw->is_credit_note ? _("New Credit Note")
                       : _("New Settlement");
            break;
        case MOD_INVOICE:
        case DUP_INVOICE:
        case EDIT_INVOICE:
            wintitle = iw->is_credit_note ? _("Edit Credit Note")
                       : _("Edit Settlement");
            break;
        case VIEW_INVOICE:
            wintitle = iw->is_credit_note ? _("View Credit Note")
                       : _("View Settlement");
            break;
        }
        break;
    case GNC_OWNER_CUSTOMER:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = iw->is_credit_note ? _("New Credit Note")
                       : _("New Invoice");
            break;
        case MOD_INVOICE:
        case DUP_INVOICE:
        case EDIT_INVOICE:
            wintitle = iw->is_credit_note ? _("Edit Credit Note")
                       : _("Edit Invoice");
            break;
        case VIEW_INVOICE:
            wintitle = iw->is_credit_note ? _("View Credit Note")
                       : _("View Invoice");
            break;
        }
        break;
    case GNC_OWNER_EMPLOYEE:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = iw->is_credit_note ? _("New Credit Note")
                       : _("New Expense Voucher");
            break;
        case MOD_INVOICE:
        case DUP_INVOICE:
        case EDIT_INVOICE:
            wintitle = iw->is_credit_note ? _("Edit Credit Note")
                       : _("Edit Expense Voucher");
            break;
        case VIEW_INVOICE:
            wintitle = iw->is_credit_note ? _("View Credit Note")
                       : _("View Expense Voucher");
            break;
        }
        break;
    case GNC_OWNER_VENDOR:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = iw->is_credit_note ? _("New Credit Note")
                       : _("New Bill");
            break;
        case MOD_INVOICE:
        case DUP_INVOICE:
        case EDIT_INVOICE:
            wintitle = iw->is_credit_note ? _("Edit Credit Note")
                       : _("Edit Bill");
            break;
        case VIEW_INVOICE:
            wintitle = iw->is_credit_note ? _("View Credit Note")
                       : _("View Bill");
            break;
        }
        break;
    default:
        break;
    }

    if (iw->entry_invoice_id)
        id = gtk_entry_get_text (GTK_ENTRY (iw->entry_invoice_id));
    if (id && *id)
        return g_strconcat (wintitle, " - ", id, (char *)NULL);
    return g_strdup (wintitle);
}

void
gnc_invoice_type_toggled_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw) return;
    iw->is_credit_note = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    /*
     * gtk_toggle_button_get_active()
     * TRUE  -> if the toggle button is pressed
     * FALSE -> in if it is raised
     */
    /* if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) */
    /* { */
    /*     iw->is_credit_note = TRUE; */
    /*     gtk_label_set_text (GTK_LABEL(iw->selected_type) , _("Credit Note")); */
    /* } */
    /* else */
    /* { */
    /*     iw->is_credit_note = FALSE; */
    /*     gtk_label_set_text (GTK_LABEL(iw->selected_type) , _("Invoice")); */
    /* }; */

}

void
gnc_invoice_id_changed_cb (GtkWidget *unused, gpointer data)
{
    InvoiceWindow *iw = data;
    gchar *title;

    if (!iw) return;
    if (iw->page)
    {
        gnc_plugin_page_invoice_update_title (iw->page);
    }
    else
    {
        title = gnc_invoice_get_title (iw);
        gtk_window_set_title (GTK_WINDOW (iw->dialog), title);
        g_free (title);
    }
}

void
gnc_invoice_terms_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkComboBox *cbox = GTK_COMBO_BOX (widget);
    InvoiceWindow *iw = data;

    if (!iw) return;
    if (!cbox) return;

    iw->terms = gnc_simple_combo_get_value (cbox);
}


static gboolean
find_handler (gpointer find_data, gpointer user_data)
{
    const GncGUID *invoice_guid = find_data;
    InvoiceWindow *iw = user_data;

    return(iw && guid_equal(&iw->invoice_guid, invoice_guid));
}

static InvoiceWindow
*gnc_invoice_new_page (
    QofBook *bookp,
    InvoiceDialogType type,
    GncInvoice *invoice,
    const GncOwner *owner,
    GncMainWindow *window,
    const gchar *group_name)
{
    InvoiceWindow *iw;
    GncOwner *billto;
    GncPluginPage *new_page;

    g_assert (type != NEW_INVOICE && type != MOD_INVOICE && type != DUP_INVOICE);
    g_assert (invoice != NULL);

    // Find an existing window for this invoice. If found, bring it
    // to the front.
    if (invoice)
    {
        GncGUID invoice_guid;

        invoice_guid = *gncInvoiceGetGUID (invoice);
        iw = gnc_find_first_gui_component (
            DIALOG_VIEW_INVOICE_CM_CLASS,
            find_handler, &invoice_guid);
        if (iw)
        {
            gnc_main_window_display_page(iw->page);
            return(iw);
        }
    }

    // No existing invoice window found. Build a new one.
    PINFO ("dialog-invoice: Create new invoice [dialog_type: \"%d\"]", type);

    iw = g_new0 (InvoiceWindow, 1);
    iw->book = bookp;
    iw->dialog_type = type;
    iw->invoice_guid = *gncInvoiceGetGUID (invoice);
    iw->is_credit_note = gncInvoiceGetIsCreditNote (invoice);
    iw->width = -1;
    iw->page_state_name = group_name;

    // Save this for later
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (owner));

    billto = gncInvoiceGetBillTo (invoice);
    gncOwnerCopy (gncOwnerGetEndOwner (billto), &(iw->project_owner));
    gncOwnerInitJob (&iw->project_job, gncOwnerGetJob (billto));

    // Now create the plugin page using the invoice structure data
    new_page = gnc_plugin_page_invoice_new (iw);

    if (!window)
        window = gnc_plugin_business_get_window ();

    // add the new invoice page to the main window
    gnc_main_window_open_page (window, new_page);

    // Initialize the summary bar
    gnc_invoice_redraw_all_cb(iw->reg, iw);

    return iw;
}

#define KEY_INVOICE_TYPE        "InvoiceType"
#define KEY_INVOICE_GUID        "InvoiceGUID"
#define KEY_OWNER_TYPE          "OwnerType"
#define KEY_OWNER_GUID          "OwnerGUID"

GncPluginPage
*gnc_invoice_recreate_page (
    GncMainWindow *window,
    GKeyFile *key_file,
    const gchar *group_name)
{
    InvoiceWindow *iw;
    GError *error = NULL;
    char *tmp_string = NULL;
    char *owner_type = NULL;
    InvoiceDialogType type;
    GncInvoice *invoice;
    GncGUID guid;
    QofBook *book;
    GncOwner owner = { 0 };

    // Get Invoice Type
    tmp_string = g_key_file_get_string(
        key_file, group_name, KEY_INVOICE_TYPE, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
            group_name, KEY_INVOICE_TYPE, error->message);
        goto give_up;
    }
    type = InvoiceDialogTypefromString(tmp_string);
    g_free(tmp_string);

    // Get Invoice GncGUID
    tmp_string = g_key_file_get_string(
        key_file, group_name, KEY_INVOICE_GUID, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
            group_name, KEY_INVOICE_GUID, error->message);
        goto give_up;
    }
    if (!string_to_guid(tmp_string, &guid))
    {
        g_warning("Invalid invoice guid: %s.", tmp_string);
        goto give_up;
    }
    book = gnc_get_current_book();
    invoice = gncInvoiceLookup(gnc_get_current_book(), &guid);
    if (invoice == NULL)
    {
        g_warning("Can't find invoice %s in current book.", tmp_string);
        goto give_up;
    }
    g_free(tmp_string);
    tmp_string = NULL;

    // Get Owner Type
    owner_type = g_key_file_get_string(
        key_file, group_name, KEY_OWNER_TYPE, &error);

    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
            group_name, KEY_OWNER_TYPE, error->message);
        goto give_up;
    }

    // Get Owner GncGUID
    tmp_string = g_key_file_get_string(
        key_file, group_name, KEY_OWNER_GUID, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
            group_name, KEY_OWNER_GUID, error->message);
        goto give_up;
    }
    if (!string_to_guid(tmp_string, &guid))
    {
        g_warning("Invalid owner guid: %s.", tmp_string);
        goto give_up;
    }

    if (!gncOwnerGetOwnerFromTypeGuid(book, &owner, owner_type, &guid))
    {
        g_warning("Can't find owner %s in current book.", tmp_string);
        goto give_up;
    }
    g_free(tmp_string);
    g_free(owner_type);

    iw = gnc_invoice_new_page (book, type, invoice, &owner, window, group_name);
    return iw->page;

give_up:
    g_warning("Giving up on restoring '%s'.", group_name);
    if (error)
        g_error_free(error);
    if (tmp_string)
        g_free(tmp_string);
    if (owner_type)
        g_free(owner_type);
    return NULL;
}

void
gnc_invoice_save_page (
    InvoiceWindow *iw,
    GKeyFile *key_file,
    const gchar *group_name)
{
    Table *table = gnc_entry_ledger_get_table (iw->ledger);
    gchar guidstr[GUID_ENCODING_LENGTH+1];
    guid_to_string_buff(&iw->invoice_guid, guidstr);
    g_key_file_set_string(key_file, group_name, KEY_INVOICE_TYPE,
        InvoiceDialogTypeasString(iw->dialog_type));
    g_key_file_set_string(key_file, group_name, KEY_INVOICE_GUID, guidstr);

    if (gncOwnerGetJob (&(iw->job)))
    {
        g_key_file_set_string(
            key_file, group_name, KEY_OWNER_TYPE, qofOwnerGetType(&iw->job));
        guid_to_string_buff(gncOwnerGetGUID(&iw->job), guidstr);
        g_key_file_set_string(key_file, group_name, KEY_OWNER_GUID, guidstr);
    }
    else
    {
        g_key_file_set_string(
            key_file, group_name, KEY_OWNER_TYPE, qofOwnerGetType(&iw->owner));
        guid_to_string_buff(gncOwnerGetGUID(&iw->owner), guidstr);
        g_key_file_set_string(key_file, group_name, KEY_OWNER_GUID, guidstr);
    }
    // save the open table layout
    gnc_table_save_state (table, group_name);
}

GtkWidget
*gnc_invoice_create_page (InvoiceWindow *iw, gpointer page)
{
    GtkBuilder *builder;
    GtkWidget *dialog;
    GtkWidget *entry_box;
    GncEntryLedger *entry_ledger = NULL;
    GncInvoice *invoice;
    gboolean is_credit_note = FALSE;
    GncEntryLedgerType ledger_type;
    GncOwnerType owner_type;
    const gchar *prefs_group = NULL;
    const gchar *style_label = NULL;
    const gchar *uri_doclink;

    invoice = gncInvoiceLookup (iw->book, &iw->invoice_guid);
    is_credit_note = gncInvoiceGetIsCreditNote (invoice);

    iw->page = page;

    // Read in glade dialog definition: "invoice_window"
    iw->builder = builder = gtk_builder_new();
    gnc_builder_add_from_file
        (builder, "dialog-invoice.glade", "invoice_liststore_terms");
    gnc_builder_add_from_file
        (builder, "dialog-invoice.glade", "invoice_vbox");
    dialog = GTK_WIDGET (
        gtk_builder_get_object
        (builder, "invoice_vbox"));

    // Grep the invoice information widgets
    g_warning ("[gnc_invoice_create_page] grep invoice information ui widgets\n");
    iw->label_invoice_info = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_invoice_info"));
    iw->label_type = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_type"));
    iw->selected_type = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_selecte_type"));

    // Grep the invoice entry values
    iw->label_invoice_id = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_invoice_id"));
    iw->entry_invoice_id = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_invoice_id"));

    // Grep the date values
    iw->label_date_opened = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_date_opened"));
    entry_box = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_date_opened"));
    iw->entry_date_opened = gnc_date_edit_new
        (gnc_time (NULL), FALSE, FALSE);
    gtk_widget_show(iw->entry_date_opened);
    gtk_box_pack_start (GTK_BOX(entry_box),
        iw->entry_date_opened, TRUE, TRUE, 0);

    iw->label_date_posted = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_date_posted"));
    iw->entry_date_posted = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_date_posted"));
    iw->date_posted = gnc_date_edit_new
        (gnc_time (NULL), FALSE, FALSE);
    gtk_widget_show(iw->date_posted);
    gtk_box_pack_start (GTK_BOX
        (iw->entry_date_posted), iw->date_posted,
        TRUE, TRUE, 0);

    // Grep the account posted values
    iw->label_account_posted = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_account_posted"));
    iw->entry_account_posted = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_account_posted"));

    // Grep active flag
    iw->label_active = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_active"));
    iw->checkbutton_active = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_checkbox_active"));
    iw->label_paid = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_paid"));

    // Set a style context for this label_paid, which can be easily
    // manipulated via css
    gnc_widget_style_context_add_class (GTK_WIDGET
        (iw->label_paid), "gnc-class-highlight");

    // Grep the invoice billing information widgets
    g_warning ("[gnc_invoice_create_page] grep billing information ui widgets\n");
    iw->label_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_owner"));
    iw->entry_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_owner"));

    iw->label_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_job"));
    iw->entry_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_job"));

    iw->entry_billing_id = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_billing_id"));
    iw->entry_terms = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_terms"));

    iw->entry_terms = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_terms"));


    // Grep the notes information widgets
    g_warning ("[gnc_invoice_create_page] grep notes ui widgets\n");
    iw->entry_notes = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_entry_notes"));

    // Set html-link to gnucash home-page
    iw->button_doclink = GTK_WIDGET(gtk_builder_get_object
        (builder, "invoice_button_doclink"));
    g_signal_connect (G_OBJECT (iw->button_doclink),
        "activate-link",
        G_CALLBACK (doclink_button_cb), iw);

    uri_doclink = gncInvoiceGetDocLink (invoice);
    if (uri_doclink)
    {
        gchar *uri_display = gnc_doclink_get_unescaped_just_uri (uri_doclink);
        gtk_button_set_label (GTK_BUTTON (iw->button_doclink),
            _("Open Linked Document:"));
        gtk_link_button_set_uri (GTK_LINK_BUTTON (iw->button_doclink),
            uri_display);
        gtk_widget_show (GTK_WIDGET (iw->button_doclink));
        g_free (uri_display);
    }
    else
        gtk_widget_hide (GTK_WIDGET (iw->button_doclink));

    /* grep the project chargeback information widgets */
    g_warning ("[gnc_invoice_create_page] grep project chargeback ui widgets\n");
    iw->frame_project = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_frame_project"));

    // chargeback -> Vouchers -> Employee
    //            -> Settlements -> Co-Owner
    //            -> Credit-Note -> Customer
    //            -> Credit-Note -> Vendor */
    iw->label_project_owner = GTK_WIDGET (gtk_builder_get_object
         (builder, "invoice_label_project_owner"));
    iw->entry_project_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_project_owner"));

    iw->label_project_job = GTK_WIDGET (gtk_builder_get_object
         (builder, "invoice_label_project_job"));
    iw->entry_project_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_hbox_project_job"));

    {
        GtkWidget *edit;

        gnc_commodity *currency = gncInvoiceGetCurrency (invoice);
        GNCPrintAmountInfo print_info;

        // Grep the charge_to widgets
        g_warning ("[gnc_invoice_create_page] grep the charge to ui widgets\n");
        iw->frame_charge_to = GTK_WIDGET (gtk_builder_get_object
            (builder, "invoice_frame_charge_to"));
        edit = gnc_amount_edit_new();
        print_info = gnc_commodity_print_info (currency, FALSE);
        gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (edit), TRUE);
        gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (edit), print_info);
        gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (edit),
            gnc_commodity_get_fraction (currency));
        iw->entry_charge_to = edit;
        gtk_widget_show (edit);
        entry_box = GTK_WIDGET (gtk_builder_get_object
            (builder, "invoice_hbox_charge_to"));
        gtk_box_pack_start (GTK_BOX (entry_box), edit, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(gnc_amount_edit_gtk_entry
            (GNC_AMOUNT_EDIT(edit))),
            "focus-out-event",
            G_CALLBACK(gnc_invoice_window_leave_to_charge_cb), edit);
        g_signal_connect(G_OBJECT(edit), "amount_changed",
            G_CALLBACK(gnc_invoice_window_changed_to_charge_cb), iw);
    }

    // Lock date and id entries to prohibit changes inside the invoice
    // dialog -> set status: insensitive)
    gtk_widget_set_sensitive (iw->entry_date_opened, FALSE);
    gtk_widget_set_sensitive (iw->date_posted, FALSE);
    gtk_widget_set_sensitive (iw->entry_invoice_id, FALSE);

    // Build the ledger
    ledger_type = GNCENTRY_INVOICE_VIEWER;
    owner_type = gncOwnerGetType (&iw->owner);
    switch (iw->dialog_type)
    {
    case EDIT_INVOICE:
        switch (owner_type)
        {
        case GNC_OWNER_COOWNER:
            ledger_type = is_credit_note ? GNCENTRY_COOWNER_CREDIT_NOTE_ENTRY
                          : GNCENTRY_SETTLEMENT_ENTRY;
            break;
        case GNC_OWNER_CUSTOMER:
            ledger_type = is_credit_note ? GNCENTRY_CUST_CREDIT_NOTE_ENTRY
                          : GNCENTRY_INVOICE_ENTRY;
            break;
        case GNC_OWNER_EMPLOYEE:
            ledger_type = is_credit_note ? GNCENTRY_EMPL_CREDIT_NOTE_ENTRY
                          : GNCENTRY_EXPVOUCHER_ENTRY;
            break;
        case GNC_OWNER_VENDOR:
            ledger_type = is_credit_note ? GNCENTRY_VEND_CREDIT_NOTE_ENTRY
                          : GNCENTRY_BILL_ENTRY;
            break;
        default:
            g_warning ("Invalid owner type");
            break;
        }
        break;
    case VIEW_INVOICE:
    default:
        switch (owner_type)
        {
        case GNC_OWNER_COOWNER:
            ledger_type = is_credit_note ? GNCENTRY_COOWNER_CREDIT_NOTE_VIEWER
                          : GNCENTRY_SETTLEMENT_VIEWER;
            prefs_group   = GNC_PREFS_GROUP_SETTLEMENT;
            break;
        case GNC_OWNER_CUSTOMER:
            ledger_type = is_credit_note ? GNCENTRY_CUST_CREDIT_NOTE_VIEWER
                          : GNCENTRY_INVOICE_VIEWER;
            prefs_group   = GNC_PREFS_GROUP_INVOICE;
            break;
        case GNC_OWNER_EMPLOYEE:
            ledger_type = is_credit_note ? GNCENTRY_EMPL_CREDIT_NOTE_VIEWER
                          : GNCENTRY_EXPVOUCHER_VIEWER;
            prefs_group   = GNC_PREFS_GROUP_BILL;
            break;
        case GNC_OWNER_VENDOR:
            ledger_type = is_credit_note ? GNCENTRY_VEND_CREDIT_NOTE_VIEWER
                          : GNCENTRY_BILL_VIEWER;
            prefs_group   = GNC_PREFS_GROUP_BILL;
            break;
        default:
            g_warning ("Invalid owner type");
            break;
        }
        break;
    }

    // Set a secondary style context for this page this can be easily
    // manipulated with css later on
    gnc_widget_style_context_add_class (GTK_WIDGET(dialog), style_label);

    // Connect all signals
    g_warning ("[gnc_invoice_create_page] gtk_builder_connect_signals_full");
    gtk_builder_connect_signals_full
        (builder, gnc_builder_connect_full_func, iw);

    // setup the leger -> updated via callback
    entry_ledger = gnc_entry_ledger_new (iw->book, ledger_type);
    iw->ledger = entry_ledger;

    // Set the entry_ledger's invoice
    gnc_entry_ledger_set_default_invoice (entry_ledger, invoice);

    // Set the preferences group
    gnc_entry_ledger_set_prefs_group (entry_ledger, prefs_group);

    // Set initial values
    iw->component_id =
        gnc_register_gui_component (DIALOG_VIEW_INVOICE_CM_CLASS,
                                    gnc_invoice_window_refresh_handler,
                                    gnc_invoice_window_close_handler,
                                    iw);

    gnc_gui_component_watch_entity_type (iw->component_id,
                                         GNC_INVOICE_MODULE_NAME,
                                         QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    // Create the register
    {
      GtkWidget *regWidget;
        GtkWidget *frame;
        GtkWidget *window;
        const gchar *default_group = gnc_invoice_window_get_state_group (iw);
        const gchar *group;

        // if this is from a page recreate, use those settings
        if (iw->page_state_name)
            group = iw->page_state_name;
        else
            group = default_group;

        // Watch the order of operations, here...
        regWidget = gnucash_register_new (
            gnc_entry_ledger_get_table (entry_ledger), group);
        gtk_widget_show(regWidget);

        frame = GTK_WIDGET (gtk_builder_get_object (builder, "invoice_frame_ledger"));
        gtk_container_add (GTK_CONTAINER (frame), regWidget);

        iw->reg = GNUCASH_REGISTER (regWidget);
        window = gnc_plugin_page_get_window(iw->page);
        gnucash_sheet_set_window (gnucash_register_get_sheet (iw->reg), window);

        g_signal_connect (G_OBJECT (regWidget), "activate_cursor",
                          G_CALLBACK (gnc_invoice_window_record_cb), iw);
        g_signal_connect (G_OBJECT (regWidget), "redraw_all",
                          G_CALLBACK (gnc_invoice_redraw_all_cb), iw);
    }

    gnc_table_realize_gui (gnc_entry_ledger_get_table (entry_ledger));

    // Now fill in a lot of the pieces and display properly
    gnc_invoice_update_window (iw, dialog);

    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), TRUE);

    // Show the dialog
    gtk_widget_show_all (dialog);

    return dialog;
}

void
gnc_invoice_update_doclink_for_window (
    GncInvoice *invoice,
    const gchar *uri)
{
    InvoiceWindow *iw = gnc_plugin_page_invoice_get_window (invoice);

    if (iw)
    {
        GtkWidget *button_doclink = gnc_invoice_window_get_doclink_button (iw);

        if (g_strcmp0 (uri, "") == 0)
        {
            // handle deleted uri
            GtkAction *uri_action;

            // update the menu actions
            uri_action = gnc_plugin_page_get_action (GNC_PLUGIN_PAGE(iw->page), "BusinessLinkOpenAction");
            gtk_action_set_sensitive (uri_action, FALSE);

            gtk_widget_hide (button_doclink);
        }
        else
        {
            gchar *uri_display = gnc_doclink_get_unescaped_just_uri (uri);
            gtk_link_button_set_uri (GTK_LINK_BUTTON (button_doclink),
                                     uri_display);
            gtk_widget_show (GTK_WIDGET (button_doclink));
            g_free (uri_display);
        }
    }
}

static InvoiceWindow
*gnc_invoice_window_new_invoice (
    GtkWindow *parent,
    InvoiceDialogType dialog_type,
    QofBook *bookp,
    const GncOwner *owner,
    GncInvoice *invoice)
{
    InvoiceWindow *iw;
    GtkBuilder *builder;
    GtkWidget *entry_box;
    GtkWidget *selected_invoice_type;
    GncOwner *billto;
    const GncOwner *start_owner;
    GncBillTerm *owner_terms = NULL;
    GncOwnerType owner_type;
    const gchar *style_label = NULL;

    g_assert (dialog_type == NEW_INVOICE ||
        dialog_type == MOD_INVOICE ||
        dialog_type == DUP_INVOICE);

    if (invoice)
    {
        // Since invoice is already existing, bring the corresponding
        // UI window to the front.
        GncGUID invoice_guid;

        invoice_guid = *gncInvoiceGetGUID (invoice);
        iw = gnc_find_first_gui_component (
            DIALOG_NEW_INVOICE_CM_CLASS,
            find_handler, &invoice_guid);
        if (iw)
        {
            gtk_window_set_transient_for (GTK_WINDOW(iw->dialog), parent);
            gtk_window_present (GTK_WINDOW(iw->dialog));
            return(iw);
        }
    }

    // No existing invoice window found.  Build a new one.
    iw = g_new0 (InvoiceWindow, 1);
    iw->dialog_type = dialog_type;

    switch (dialog_type)
    {
    case NEW_INVOICE:
        g_assert (bookp);

        invoice = gncInvoiceCreate (bookp);
        gncInvoiceSetCurrency (invoice, gnc_default_currency ());
        start_owner = owner;
        iw->book = bookp;

        g_warning ("[gnc_invoice_window_new_invoice] owner type '%d'.\n",
                   gncOwnerGetType (gncOwnerGetEndOwner (owner)));
        switch (gncOwnerGetType (gncOwnerGetEndOwner (owner)))
        {
        case GNC_OWNER_COOWNER:
            owner_terms = gncCoOwnerGetTerms (gncOwnerGetCoOwner (gncOwnerGetEndOwner (owner)));
            break;
        case GNC_OWNER_CUSTOMER:
            owner_terms = gncCustomerGetTerms (gncOwnerGetCustomer (gncOwnerGetEndOwner (owner)));
            break;
        case GNC_OWNER_VENDOR:
            owner_terms = gncVendorGetTerms (gncOwnerGetVendor (gncOwnerGetEndOwner (owner)));
            break;
        default:
            break;
        }
        if (owner_terms)
            gncInvoiceSetTerms (invoice, owner_terms);
        break;

    case MOD_INVOICE:
    case DUP_INVOICE:
        start_owner = gncInvoiceGetOwner (invoice);
        iw->book = gncInvoiceGetBook (invoice);
        break;
    default:
        // The assert at the beginning of this function should prevent
        // this switch case !
        return NULL;
    }

    // Save owner in local pointer for later use
    gncOwnerCopy (gncOwnerGetEndOwner(start_owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (start_owner));

    // Set bill information
    billto = gncInvoiceGetBillTo (invoice);

    // Set bill specific values for project-owner and project-job
    gncOwnerCopy (gncOwnerGetEndOwner (billto), &(iw->project_owner));
    gncOwnerInitJob (&iw->project_job, gncOwnerGetJob (billto));

    // Read in glade dialog definition: "invoice_new_dialog"
    g_warning ("[gnc_invoice_window_new_invoice] build 'invoice_new_dialog' page\n");
    iw->builder = builder = gtk_builder_new();
    gnc_builder_add_from_file (builder, "dialog-invoice.glade", "invoice_liststore_terms");
    gnc_builder_add_from_file (builder, "dialog-invoice.glade", "invoice_new_dialog");
    iw->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "invoice_new_dialog"));
    gtk_window_set_transient_for (GTK_WINDOW(iw->dialog), parent);

    // Set default project owner to correspond with owner
    gncOwnerCopy (gncOwnerGetEndOwner(start_owner), &(iw->project_owner));

    // Set the name for this dialog so we can be easily manipulated it via css
    gtk_widget_set_name (GTK_WIDGET(iw->dialog), "gnc-id-invoice");

    // Create the invoice dialog structure
    g_object_set_data (G_OBJECT (iw->dialog), "dialog_info", iw);

    // Grep the invoice information widgets
    g_warning ("[gnc_invoice_window_new_invoice] grep glade values for invoice information widgets\n");
    iw->label_invoice_info = GTK_WIDGET (
        gtk_builder_get_object (builder, "invoice_new_label_invoice_info"));
    iw->label_type = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_label_type"));
    iw->selected_type_hbox = GTK_WIDGET (
        gtk_builder_get_object (builder, "invoice_new_selected_type_hbox"));
    iw->selected_type = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_selected_type"));
    selected_invoice_type = GTK_WIDGET (
        gtk_builder_get_object (builder, "invoice_new_button_type_invoice"));

    /* TODO: If possible get the invoice type direcly from the active radio button label? */
    /*       Then we could update 'label_type' and eleminate the extra ui-field */
    /*       "invoice_new_selected_type" in "invoice_new_dialog" */
    /* g_warning ("Invoice Type: '%s'.\n", */
    /*            gtk_button_get_label (GTK_BUTTON (widget))); */
    /* gtk_label_set_text ( */
    /*     GTK_LABEL(iw->label_type), */
    /*     gtk_button_get_label (GTK_BUTTON (widget))); */

    // Grep the invoice id widgets
    iw->label_invoice_id = GTK_WIDGET (
        gtk_builder_get_object (builder, "invoice_new_label_invoice_id"));
    iw->entry_invoice_id = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_entry_invoice_id"));
    iw->checkbutton_active = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_checkbutton_active"));

    iw->label_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_label_owner"));
    iw->entry_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_hbox_owner"));
    iw->label_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_label_job"));
    iw->entry_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_hbox_job"));
    iw->entry_billing_id = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_entry_billing_id"));
    iw->entry_terms = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_combobox_terms"));
    iw->entry_notes = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_entry_notes"));
    iw->label_paid = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_label_paid"));

    entry_box = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_hbox_date_opened"));
    iw->entry_date_opened = gnc_date_edit_new (gnc_time (NULL), FALSE, FALSE);
    gtk_widget_show(iw->entry_date_opened);
    gtk_box_pack_start (GTK_BOX(entry_box),
        iw->entry_date_opened, TRUE, TRUE, 0);

    // Grep the project widgets
    g_warning ("[gnc_invoice_window_new_invoice] grep glade values for project information ui widgets\n");
    iw->frame_project = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_frame_project"));
    iw->label_project_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_label_project_owner"));
    iw->entry_project_owner = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_hbox_project_owner"));
    iw->label_project_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_label_project_owner"));
    iw->entry_project_job = GTK_WIDGET (gtk_builder_get_object
        (builder, "invoice_new_hbox_project_job"));

    // If this is a new invoice, reset the notes widget to read/write
    gtk_widget_set_sensitive (
        iw->entry_notes,
        (iw->dialog_type == NEW_INVOICE) ||
        (iw->dialog_type == DUP_INVOICE));

    // Default glade labels are for invoices, change them to comply
    // with active owner type.
    g_warning ("[gnc_invoice_window_new_invoice] adapt owner type specific labels\n");
    switch (iw->owner.type)
    {
        case GNC_OWNER_COOWNER:
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_info), _("Settlement Information"));
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_id), _("Settlement ID"));
            // TODO: does it make sense to chargeback to an owner type [customer, employee]?
            gtk_label_set_text (GTK_LABEL(iw->label_project_owner), _("Co-Owner"));
            gtk_label_set_text (GTK_LABEL(iw->label_type), _("Settlement"));
            style_label = "gnc-class-coowners";
            break;
        case GNC_OWNER_EMPLOYEE:
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_info), _("Voucher Information"));
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_id), _("Voucher ID"));
            gtk_label_set_text (GTK_LABEL(iw->label_owner), _("Employee"));
            // TODO: does it make sense to chargeback to an owner type [coowner, customer]?
            gtk_label_set_text (GTK_LABEL(iw->label_project_owner), _("Employee"));
            gtk_label_set_text (GTK_LABEL(iw->label_type), _("Voucher"));
            style_label = "gnc-class-employees";
            break;
        case GNC_OWNER_VENDOR:
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_info), _("Bill Information"));
            gtk_label_set_text (GTK_LABEL(iw->label_invoice_id), _("Bill ID"));
            gtk_label_set_text (GTK_LABEL(iw->label_owner), _("Vendor"));
            // TODO: does it make sense to chargeback to other owner type [coowner, customer, employee]?
            gtk_label_set_text (GTK_LABEL(iw->label_project_owner), _("Customer"));
            gtk_label_set_text (GTK_LABEL(iw->label_type), _("Bill"));
            style_label = "gnc-class-vendors";
            break;
        default:
            style_label = "gnc-class-customers";
            break;
    }

    // Restrict read/write status for ui-fileds to meet on given dialog and invoice type
    switch (dialog_type)
    {
    case NEW_INVOICE:
    case DUP_INVOICE:
        gtk_widget_show_all (iw->label_type);
        gtk_widget_hide (iw->selected_type_hbox);
        gtk_widget_hide (iw->selected_type);
        break;
    case MOD_INVOICE:
        gtk_widget_hide (iw->label_type);
        gtk_widget_show (iw->selected_type_hbox);
        gtk_widget_show (iw->selected_type);
        break;
    default:
        break;
    }

    if (dialog_type == DUP_INVOICE)
    {
        GtkWidget *cn_radio = GTK_WIDGET (gtk_builder_get_object (builder, "dialog_creditnote_type"));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cn_radio), gncInvoiceGetIsCreditNote (invoice));
    }

    // Set a secondary style context for this page this can be easily
    // manipulated with css later on
    gnc_widget_style_context_add_class (GTK_WIDGET(iw->dialog), style_label);

    // Connect all signals
    gtk_builder_connect_signals_full(
        builder,
        gnc_builder_connect_full_func,
        iw);

    // Set initial values
    g_warning ("[gnc_invoice_window_new_invoice] set initial values\n");
    iw->reportPage = NULL;
    iw->invoice_guid = *gncInvoiceGetGUID (invoice);
    iw->is_credit_note = gncInvoiceGetIsCreditNote (invoice);

    iw->component_id =
        gnc_register_gui_component (
            DIALOG_NEW_INVOICE_CM_CLASS,
            gnc_invoice_window_refresh_handler,
            gnc_invoice_dialog_close_handler,
            iw);

    gnc_gui_component_watch_entity_type (
        iw->component_id,
        GNC_INVOICE_MODULE_NAME,
        QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    // Now fill in a lot of the pieces and display properly
    switch(dialog_type)
    {
        case NEW_INVOICE:
        case MOD_INVOICE:
        case DUP_INVOICE:
            gnc_billterms_combo (GTK_COMBO_BOX(iw->entry_terms),
                iw->book, TRUE, iw->terms);
            break;
        case EDIT_INVOICE:
        case VIEW_INVOICE:
            // Fill in the invoice view version
            if(gncBillTermGetName (iw->terms) != NULL)
                gtk_entry_set_text (GTK_ENTRY (iw->entry_terms),
                    gncBillTermGetName (iw->terms));
            else
                gtk_entry_set_text (GTK_ENTRY (iw->entry_terms), "None");
        break;
    }

    gnc_invoice_update_window (iw, iw->dialog);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), TRUE);

    // The owner choice widget should have keyboard focus
    if (GNC_IS_GENERAL_SEARCH(iw->choice_owner))
    {
        gnc_general_search_grab_focus(GNC_GENERAL_SEARCH(iw->choice_owner));
    }

    return iw;
}

/* ========================================================
 * Dedicated UserInterface Functions
 * ======================================================== */
InvoiceWindow
*gnc_ui_invoice_edit (GtkWindow *parent, GncInvoice *invoice)
{
    InvoiceWindow *iw;
    InvoiceDialogType type;

    if (!invoice) return NULL;

    // Immutable once we've been posted
    if (gncInvoiceGetPostedAcc (invoice))
        type = VIEW_INVOICE;
    else
        type = EDIT_INVOICE;

    iw = gnc_invoice_new_page (
        gncInvoiceGetBook(invoice), type,
        invoice, gncInvoiceGetOwner (invoice),
        GNC_MAIN_WINDOW(gnc_ui_get_main_window (
            GTK_WIDGET (parent))), NULL);

    return iw;
}

InvoiceWindow
*gnc_ui_invoice_duplicate (
    GtkWindow *parent,
    GncInvoice *old_invoice,
    gboolean open_properties,
    const GDate *new_date)
{
    InvoiceWindow *iw = NULL;
    GncInvoice *new_invoice = NULL;
    time64 entry_date;

    g_assert(old_invoice);

    // Create a deep copy of the old invoice
    new_invoice = gncInvoiceCopy(old_invoice);

    // The new invoice is for sure active
    gncInvoiceSetActive(new_invoice, TRUE);

    // and unposted
    if (gncInvoiceIsPosted (new_invoice))
    {
        gboolean result = gncInvoiceUnpost(new_invoice, TRUE);
        if (!result)
        {
            g_warning("Oops, error when unposting the copied invoice; ignoring.");
        }
    }

    // Unset the invoice ID, let it get allocated later
    gncInvoiceSetID(new_invoice, "");

    // Modify the date to today
    if (new_date)
        entry_date = gnc_time64_get_day_neutral (gdate_to_time64 (*new_date));
    else
        entry_date = gnc_time64_get_day_neutral (gnc_time (NULL));
    gncInvoiceSetDateOpened(new_invoice, entry_date);

    // Also modify the date of all entries to today
    //g_warning("We have %d entries", g_list_length(gncInvoiceGetEntries(new_invoice)));
    g_list_foreach(
        gncInvoiceGetEntries(new_invoice),
        &set_gncEntry_date, &entry_date);


    if (open_properties)
    {
        // Open the "properties" pop-up for the invoice...
        iw = gnc_invoice_window_new_invoice
            (parent, DUP_INVOICE, NULL, NULL, new_invoice);
    }
    else
    {
         // Open the newly created invoice in the "edit" window
        iw = gnc_ui_invoice_edit (parent, new_invoice);
        // Check the ID; set one if necessary
        if (g_strcmp0 (gtk_entry_get_text
            (GTK_ENTRY (iw->entry_invoice_id)), "") == 0)
        {
            gncInvoiceSetID
                (new_invoice, gncInvoiceNextID(iw->book, &(iw->owner)));
        }
    }
    return iw;
}

static InvoiceWindow *
gnc_ui_invoice_modify (GtkWindow *parent, GncInvoice *invoice)
{
    InvoiceWindow *iw;
    if (!invoice) return NULL;

    iw = gnc_invoice_window_new_invoice
        (parent, MOD_INVOICE, NULL, NULL, invoice);
    return iw;
}

InvoiceWindow
*gnc_ui_invoice_new (GtkWindow *parent, GncOwner *owner, QofBook *book)
{
    InvoiceWindow *iw;
    GncOwner owner_invoice;

    if (owner)
    {
        gncOwnerCopy (owner, &owner_invoice);

        // Assure we have a working structure for the given owner type
        switch (owner->type)
        {
        case GNC_OWNER_NONE :
        case GNC_OWNER_UNDEFINED :
            /* This isn't valid */
            g_warning ("Invalid owner type, need to be \"GNC_OWNER_"
                       "[CoOwner, Customer, Employee, Job, Vendor]\".\n");
            break;
        case GNC_OWNER_COOWNER :
        {
            gncOwnerInitCoOwner (&owner_invoice, NULL);
            break;
        }
        case GNC_OWNER_CUSTOMER :
        {
            gncOwnerInitCustomer (&owner_invoice, NULL);
            break;
        }
        case GNC_OWNER_EMPLOYEE :
        {
            gncOwnerInitEmployee (&owner_invoice, NULL);
            break;
        }
        case GNC_OWNER_JOB :
        {
            gncOwnerInitJob (&owner_invoice, NULL);
            break;
        }
        case GNC_OWNER_VENDOR :
        {
            gncOwnerInitVendor (&owner_invoice, NULL);
            break;
        }
        }
    }
    else
    {
        // If nothing is preset: Customer is our default target
        gncOwnerInitCustomer (&owner_invoice, NULL);
    }

    // Assure the required book with the needed options exist
    if (!book) return NULL;

    // Prepare the data structure
    iw = gnc_invoice_window_new_invoice
        (parent, NEW_INVOICE, book, &owner_invoice, NULL);

    return iw;
}

/* ========================================================
 * Selection invoice widget Functions
 * ======================================================== */
static void
edit_invoice_direct (GtkWindow *dialog, gpointer invoice, gpointer user_data)
{
    g_return_if_fail (invoice);
    gnc_ui_invoice_edit
        (gnc_ui_get_main_window
        (GTK_WIDGET (dialog)), invoice);
}

struct multi_edit_invoice_data
{
    gpointer user_data;
    GtkWindow *parent;
};

static void
multi_duplicate_invoice_one(gpointer data, gpointer user_data)
{
    GncInvoice *old_invoice = data;
    struct multi_duplicate_invoice_data *dup_user_data = user_data;

    g_assert(dup_user_data);
    if (old_invoice)
    {
        GncInvoice *new_invoice;
        // In this simplest form, we just use the existing duplication
        // algorithm, only without opening the "edit invoice" window for editing
        // the number etc. for each of the invoices.
        InvoiceWindow *iw = gnc_ui_invoice_duplicate(
            dup_user_data->parent, old_invoice, FALSE, &dup_user_data->date);

        // FIXME: Now we could use this invoice and manipulate further data.
        g_assert(iw);
        new_invoice = iw_get_invoice(iw);
        g_assert(new_invoice);
    }
}

static void
multi_edit_invoice_one (gpointer inv, gpointer user_data)
{
    struct multi_edit_invoice_data *meid = user_data;
    edit_invoice_cb (meid->parent, inv, meid->user_data);
}

/* ========================================================
 * Widget Callback Functions
 * ======================================================== */
static gboolean
doclink_button_cb (GtkLinkButton *button, InvoiceWindow *iw)
{
    GncInvoice *invoice = gncInvoiceLookup (iw->book, &iw->invoice_guid);
    gnc_doclink_open_uri (GTK_WINDOW(iw->dialog), gncInvoiceGetDocLink (invoice));

    return TRUE;
}

static void
edit_invoice_cb (GtkWindow *dialog, gpointer inv, gpointer user_data)
{
    GncInvoice *invoice = inv;
    g_return_if_fail (invoice && user_data);
    edit_invoice_direct (dialog, invoice, user_data);
}

static void
multi_edit_invoice_cb (
    GtkWindow *dialog,
    GList *invoice_list,
    gpointer user_data)
{
    struct multi_edit_invoice_data meid;

    meid.user_data = user_data;
    meid.parent = dialog;
    g_list_foreach (invoice_list, multi_edit_invoice_one, &meid);
}

static void
pay_invoice_direct (GtkWindow *dialog, gpointer inv, gpointer user_data)
{
    GncInvoice *invoice = inv;

    g_return_if_fail (invoice);
    gnc_ui_payment_new_with_invoice (
        dialog, gncInvoiceGetOwner (invoice),
        gncInvoiceGetBook (invoice), invoice);
}

static void
pay_invoice_cb (GtkWindow *dialog, gpointer *invoice_p, gpointer user_data)
{
    g_return_if_fail (invoice_p && user_data);
    if (! *invoice_p)
        return;
    pay_invoice_direct (dialog, *invoice_p, user_data);
}

static void
multi_duplicate_invoice_cb (
    GtkWindow *dialog,
    GList *invoice_list,
    gpointer user_data)
{
    g_return_if_fail (invoice_list);
    switch (g_list_length(invoice_list))
    {
        case 0:
            return;
        case 1:
        {
            // Duplicate exactly one invoice
            GncInvoice *old_invoice = invoice_list->data;
            gnc_ui_invoice_duplicate(dialog, old_invoice, TRUE, NULL);
            return;
        }
        default:
        {
            // Duplicate multiple invoices. We ask for a date first.
            struct multi_duplicate_invoice_data dup_user_data;
            gboolean dialog_ok;

            // Default date: Today
            gnc_gdate_set_time64(&dup_user_data.date, gnc_time (NULL));
            dup_user_data.parent = dialog;
            dialog_ok = gnc_dup_date_dialog (
            GTK_WIDGET(dialog),
                _("Date of duplicated entries"),
                &dup_user_data.date);
            if (!dialog_ok)
            {
                // User pressed cancel, so don't duplicate anything here.
                return;
            }

            // Note: If we want to have a more sophisticated duplication,
            // we might want to ask for particular data right here, then
            // insert this data upon duplication.
            g_list_foreach(invoice_list, multi_duplicate_invoice_one, &dup_user_data);
            return;
        }
    }
}

static void
post_one_invoice_cb(gpointer data, gpointer user_data)
{
    GncInvoice *invoice = data;
    struct post_invoice_params *post_params = user_data;
    InvoiceWindow *iw = gnc_ui_invoice_edit(post_params->parent, invoice);
    gnc_invoice_post(iw, post_params);
}

static void
gnc_invoice_is_posted(gpointer inv, gpointer test_value)
{
    GncInvoice *invoice = inv;
    gboolean *test = (gboolean*)test_value;

    if (gncInvoiceIsPosted (invoice))
    {
        *test = TRUE;
    }
}


static void
multi_post_invoice_cb (
    GtkWindow *dialog,
    GList *invoice_list,
    gpointer user_data)
{
    struct post_invoice_params post_params;
    gboolean test;
    InvoiceWindow *iw;

    if (!gnc_list_length_cmp (invoice_list, 0))
        return;
    // Get the posting parameters for these invoices
    iw = gnc_ui_invoice_edit(dialog, invoice_list->data);
    test = FALSE;
    gnc_suspend_gui_refresh (); // Turn off GUI refresh for the duration.
    // Check if any of the selected invoices have already been posted.
    g_list_foreach(invoice_list, gnc_invoice_is_posted, &test);
    gnc_resume_gui_refresh ();
    if (test)
    {
        gnc_error_dialog (
            GTK_WINDOW (iw_get_window(iw)), "%s",
            _("One or more selected invoices have already been posted.\nRe-check your selection."));
        return;
    }

    if (!gnc_dialog_post_invoice(
        iw, _("Do you really want to post these invoices?"),
        &post_params.ddue, &post_params.postdate,
        &post_params.memo, &post_params.acc,
        &post_params.accumulate))
        return;
    post_params.parent = dialog;

    // Turn off GUI refresh for the duration.  This is more than just an
    // optimization.  If the search that got us here is based on the "posted"
    // status of an invoice, the updating the GUI will change the list we're
    // working on which leads to bad things happening.
    gnc_suspend_gui_refresh ();
    g_list_foreach(invoice_list, post_one_invoice_cb, &post_params);
    gnc_resume_gui_refresh ();
}

static void
print_one_invoice_cb(GtkWindow *dialog, gpointer data, gpointer user_data)
{
    GncInvoice *invoice = data;
    gnc_invoice_window_print_invoice (dialog, invoice);
}

static void
multi_print_invoice_one (gpointer data, gpointer user_data)
{
    struct multi_edit_invoice_data *meid = user_data;
    print_one_invoice_cb (gnc_ui_get_main_window (GTK_WIDGET(meid->parent)), data, meid->user_data);
}

static void
multi_print_invoice_cb (GtkWindow *dialog, GList *invoice_list, gpointer user_data)
{
    struct multi_edit_invoice_data meid;

    if (!gnc_list_length_cmp (invoice_list, 0))
        return;

    meid.user_data = user_data;
    meid.parent = dialog;
    g_list_foreach (invoice_list, multi_print_invoice_one, &meid);
}

static gpointer
new_invoice_cb (GtkWindow *dialog, gpointer user_data)
{
    struct _invoice_select_window *sw = user_data;
    InvoiceWindow *iw;

    g_return_val_if_fail (user_data, NULL);

    iw = gnc_ui_invoice_new (dialog, sw->owner, sw->book);
    return iw_get_invoice (iw);
}

static void
free_invoice_cb (gpointer user_data)
{
    struct _invoice_select_window *sw = user_data;

    g_return_if_fail (sw);

    qof_query_destroy (sw->q);
    g_free (sw);
}

GNCSearchWindow
*gnc_invoice_search (GtkWindow *parent, GncInvoice *start, GncOwner *owner, QofBook *book)
{
    QofIdType type = GNC_INVOICE_MODULE_NAME;
    struct _invoice_select_window *sw;
    QofQuery *q, *q2 = NULL;
    GncOwnerType owner_type = GNC_OWNER_CUSTOMER;
    static GList *bill_params = NULL;
    static GList *emp_params = NULL;
    static GList *inv_params = NULL;
    static GList *settlement_params = NULL;
    static GList *params;
    static GList *columns = NULL;
    const gchar *title, *label, *style_class;
    static GNCSearchCallbackButton *buttons;

    static GNCSearchCallbackButton bill_buttons[] =
    {
        { N_("View/Edit Bill"), NULL, multi_edit_invoice_cb, TRUE},
        { N_("Process Payment"), pay_invoice_cb, NULL, FALSE},
        { N_("Duplicate"), NULL, multi_duplicate_invoice_cb, FALSE},
        { N_("Post"), NULL, multi_post_invoice_cb, FALSE},
        { N_("Printable Report"), NULL, multi_print_invoice_cb, TRUE},
        { NULL },
    };
    static GNCSearchCallbackButton emp_buttons[] =
    {
        /* Translators: The terms 'Voucher' and 'Expense Voucher' are used
           interchangeably in gnucash and mean the same thing. */
        { N_("View/Edit Voucher"), NULL, multi_edit_invoice_cb, TRUE},
        { N_("Process Payment"), pay_invoice_cb, NULL, FALSE},
        { N_("Duplicate"), NULL, multi_duplicate_invoice_cb, FALSE},
        { N_("Post"), NULL, multi_post_invoice_cb, FALSE},
        { N_("Printable Report"), NULL, multi_print_invoice_cb, TRUE},
        { NULL },
    };
    static GNCSearchCallbackButton inv_buttons[] =
    {
        { N_("View/Edit Invoice"), NULL, multi_edit_invoice_cb, TRUE},
        { N_("Process Payment"), pay_invoice_cb, NULL, FALSE},
        { N_("Duplicate"), NULL, multi_duplicate_invoice_cb, FALSE},
        { N_("Post"), NULL, multi_post_invoice_cb, FALSE},
        { N_("Printable Report"), NULL, multi_print_invoice_cb, TRUE},
        { NULL },
    };
    static GNCSearchCallbackButton settlement_buttons[] =
    {
        { N_("View/Edit Settlement"), NULL, multi_edit_invoice_cb, TRUE},
        { N_("Process Payment"), pay_invoice_cb, NULL, FALSE},
        { N_("Duplicate"), NULL, multi_duplicate_invoice_cb, FALSE},
        { N_("Post"), NULL, multi_post_invoice_cb, FALSE},
        { N_("Printable Report"), NULL, multi_print_invoice_cb, TRUE},
        { NULL },
    };

    g_return_val_if_fail (book, NULL);

    /* Build parameter list in reverse order */
    if (bill_params == NULL)
    {
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Bill Owner"), NULL, type,
            INVOICE_OWNER, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Bill Notes"), NULL, type,
            INVOICE_NOTES, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Billing ID"), NULL, type,
            INVOICE_BILLINGID, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Is Paid?"), NULL, type,
            INVOICE_IS_PAID, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Date Posted"), NULL, type,
            INVOICE_POSTED, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Is Posted?"), NULL, type,
            INVOICE_IS_POSTED, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Date Opened"), NULL, type,
            INVOICE_OPENED, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Due Date"), NULL, type,
            INVOICE_DUE, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Company Name"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT,
            OWNER_NAME, NULL);
        bill_params = gnc_search_param_prepend (
            bill_params,
            _("Bill ID"), NULL, type,
            INVOICE_ID, NULL);
    }
    if (emp_params == NULL)
    {
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Billing ID"), NULL, type,
            INVOICE_BILLINGID, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Date Posted"), NULL, type,
            INVOICE_POSTED, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Date Opened"), NULL, type,
            INVOICE_OPENED, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Due Date"), NULL, type,
            INVOICE_DUE, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Employee Name"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT,
            OWNER_NAME, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Is Paid?"), NULL, type,
            INVOICE_IS_PAID, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Is Posted?"), NULL, type,
            INVOICE_IS_POSTED, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Voucher ID"), NULL, type,
            _("Voucher Owner"), NULL, type,
            INVOICE_ID, NULL);
        emp_params = gnc_search_param_prepend (
            emp_params,
            _("Voucher Notes"), NULL, type,
            INVOICE_NOTES, NULL);
    }
    if (inv_params == NULL)
    {
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Invoice Owner"), NULL, type,
            INVOICE_OWNER, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Invoice Notes"), NULL, type,
            INVOICE_NOTES, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Billing ID"), NULL, type,
            INVOICE_BILLINGID, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Is Paid?"), NULL, type,
            INVOICE_IS_PAID, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Date Posted"), NULL, type,
            INVOICE_POSTED, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Is Posted?"), NULL, type,
            INVOICE_IS_POSTED, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Date Opened"), NULL, type,
            INVOICE_OPENED, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Due Date"), NULL, type,
            INVOICE_DUE, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Company Name"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT,
            OWNER_NAME, NULL);
        inv_params = gnc_search_param_prepend (
            inv_params,
            _("Invoice ID"), NULL, type,
            INVOICE_ID, NULL);
    }
    if (settlement_params == NULL)
    {
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Settlement Owner"), NULL, type,
            INVOICE_OWNER, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Billing ID"), NULL, type,
            INVOICE_BILLINGID, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Is Paid?"), NULL, type,
            INVOICE_IS_PAID, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Date Posted"), NULL, type,
            INVOICE_POSTED, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Is Posted?"), NULL, type,
            INVOICE_IS_POSTED, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Date Opened"), NULL, type,
            INVOICE_OPENED, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Due Date"), NULL, type,
            INVOICE_DUE, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Settlement ID"), NULL, type,
            INVOICE_ID, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Settlement Owner"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT,
            OWNER_NAME, NULL);
        settlement_params = gnc_search_param_prepend (
            settlement_params,
            _("Settlement Notes"), NULL, type,
            INVOICE_NOTES, NULL);
    }

    /* Build the column list in reverse order */
    if (columns == NULL)
    {
        columns = gnc_search_param_prepend (
            columns, _("Billing ID"), NULL, type,
            INVOICE_BILLINGID, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Type"), NULL, type,
            INVOICE_TYPE_STRING, NULL);
        columns = gnc_search_param_prepend_with_justify (
            columns, _("Paid"),
            GTK_JUSTIFY_CENTER, NULL, type,
            INVOICE_IS_PAID, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Posted"), NULL, type,
            INVOICE_POSTED, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Company"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT,
            OWNER_NAME, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Due"), NULL, type,
            INVOICE_DUE, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Opened"), NULL, type,
            INVOICE_OPENED, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Num"), NULL, type,
            INVOICE_ID, NULL);
    }

    // Build the queries
    q = qof_query_create_for (type);
    qof_query_set_book (q, book);

    // If owner is supplied, limit all searches to invoices who's
    // owner or end-owner is the supplied owner!  Show all invoices by
    // this owner.  If a Job is supplied, search for all invoices for
    // that job, but if a Customer is supplied, search for all
    // invoices owned by that Customer or any of that Customer's Jobs.
    // In other words, match on <supplied-owner's guid> ==
    // Invoice->Owner->GncGUID or Invoice->owner->parentGUID.
    if (owner)
    {
        // First, figure out the type of owner here..
        owner_type = gncOwnerGetType (gncOwnerGetEndOwner (owner));

        // Then if there's an actual owner add it to the query and
        // limit the search to this owner If there's only a type,
        // limit the search to this type.
        if (gncOwnerGetGUID (owner))
        {
            q2 = qof_query_create ();
            qof_query_add_guid_match (
                q2, g_slist_prepend
                (g_slist_prepend (NULL, QOF_PARAM_GUID),
                 INVOICE_OWNER),
                gncOwnerGetGUID (owner), QOF_QUERY_OR);

            qof_query_add_guid_match (
                q2, g_slist_prepend
                (g_slist_prepend (NULL, OWNER_PARENTG),
                 INVOICE_OWNER),
                gncOwnerGetGUID (owner), QOF_QUERY_OR);
            qof_query_merge_in_place (q, q2, QOF_QUERY_AND);
            qof_query_destroy (q2);

            // Use this base query as pre-fill query.  This will
            // pre-fill the search dialog with the query results
            q2 = qof_query_copy (q);

        }
        else
        {
            QofQuery *q3 = qof_query_create ();
            QofQueryPredData *inv_type_pred = NULL;
            GList *type_list = NULL, *node = NULL;

            type_list = gncInvoiceGetTypeListForOwnerType(owner_type);
            for (node = type_list; node; node = node->next)
            {
                inv_type_pred = qof_query_int32_predicate(
                    QOF_COMPARE_EQUAL,
                    GPOINTER_TO_INT(node->data));
                qof_query_add_term (
                    q3, g_slist_prepend (NULL, INVOICE_TYPE),
                    inv_type_pred, QOF_QUERY_OR);
            }
            qof_query_merge_in_place (q, q3, QOF_QUERY_AND);
            qof_query_destroy (q3);

            // Don't set a pre-fill query in this case, the result set
            // would be too long */
            q2 = NULL;
        }
    }

    // Launch select dialog and return the result
    sw = g_new0 (struct _invoice_select_window, 1);

    if (owner)
    {
        gncOwnerCopy (owner, &(sw->owner_def));
        sw->owner = &(sw->owner_def);
    }
    sw->book = book;
    sw->q = q;

    switch (owner_type)
    {
    case GNC_OWNER_COOWNER:
        title = _("Find Settlement");
        label = _("Settlement");
        style_class = "gnc-class-settlement";
        params = emp_params;
        buttons = emp_buttons;
        break;
    case GNC_OWNER_EMPLOYEE:
        title = _("Find Expense Voucher");
        label = _("Expense Voucher");
        style_class = "gnc-class-vouchers";
        params = emp_params;
        buttons = emp_buttons;
        break;
    case GNC_OWNER_VENDOR:
        title = _("Find Bill");
        label = _("Bill");
        style_class = "gnc-class-bills";
        params = bill_params;
        buttons = bill_buttons;
        break;
    default:
        title = _("Find Invoice");
        label = _("Invoice");
        style_class = "gnc-class-invoices";
        params = inv_params;
        buttons = inv_buttons;
        break;
    }
    return gnc_search_dialog_create (
        parent, type, title, params, columns, q, q2,
        buttons, NULL, new_invoice_cb,
        sw, free_invoice_cb, GNC_PREFS_GROUP_SEARCH,
        label, style_class);
}

DialogQueryView
*gnc_invoice_show_docs_due (
    GtkWindow *parent,
    QofBook *book,
    double days_in_advance,
    GncWhichDueType duetype)
{
    QofIdType type = GNC_INVOICE_MODULE_NAME;
    Query *q;
    QofQueryPredData* pred_data;
    time64 end_date;
    GList *res;
    gchar *message, *title;
    DialogQueryView *dialog;
    gint len;
    static GList *param_list = NULL;
    static GNCDisplayViewButton vendorbuttons[] =
    {
        { N_("View/Edit Bill"), edit_invoice_direct },
        { N_("Process Payment"), pay_invoice_direct },
        { NULL },
    };
    static GNCDisplayViewButton customerbuttons[] =
    {
        { N_("View/Edit Invoice"), edit_invoice_direct },
        { N_("Process Payment"), pay_invoice_direct },
        { NULL },
    };

    if (!book)
    {
        PERR("No book, no due invoices.");
        return NULL;
    }

    /* Create the param list (in reverse order) */
    if (param_list == NULL)
    {
        param_list = gnc_search_param_prepend_with_justify (
            param_list, _("Amount"),
            GTK_JUSTIFY_RIGHT, NULL, type,
            INVOICE_POST_LOT, LOT_BALANCE, NULL);
        param_list = gnc_search_param_prepend (
            param_list, _("Type"), NULL, type,
            INVOICE_TYPE_STRING, NULL);
        param_list = gnc_search_param_prepend (
            param_list, _("Company"), NULL, type,
            INVOICE_OWNER, OWNER_PARENT, OWNER_NAME, NULL);
        param_list = gnc_search_param_prepend (
            param_list, _("Due"), NULL, type,
            INVOICE_DUE, NULL);
    }

    /* Create the query to search for invoices; set the book */
    q = qof_query_create();
    qof_query_search_for(q, GNC_INVOICE_MODULE_NAME);
    qof_query_set_book (q, book);

    // For vendor bills we want to find all invoices where:
    //      invoice -> is_posted == TRUE
    // AND  invoice -> lot -> is_closed? == FALSE
    // AND  invoice -> type != customer invoice
    // AND  invoice -> type != customer credit note
    // AND  invoice -> due <= (today + days_in_advance)

    // For customer invoices we want to find all invoices where:
    //      invoice -> is_posted == TRUE
    // AND  invoice -> lot -> is_closed? == FALSE
    // AND  invoice -> type != vendor bill
    // AND  invoice -> type != vendor credit note
    // AND  invoice -> type != employee voucher
    // AND  invoice -> type != employee credit note
    // AND  invoice -> due <= (today + days_in_advance)
    // This could probably also be done by searching for customer invoices OR customer credit notes
    // but that would make a more complicated query to compose.

    qof_query_add_boolean_match (
        q, g_slist_prepend(NULL, INVOICE_IS_POSTED), TRUE,
        QOF_QUERY_AND);

    qof_query_add_boolean_match (
        q, g_slist_prepend(g_slist_prepend(NULL, LOT_IS_CLOSED),
        INVOICE_POST_LOT), FALSE, QOF_QUERY_AND);


    if (duetype == DUE_FOR_VENDOR)
    {
        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_CUST_INVOICE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);

        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_CUST_CREDIT_NOTE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);
    }
    else
    {
        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_VEND_INVOICE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);

        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_VEND_CREDIT_NOTE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);

        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_EMPL_INVOICE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);

        pred_data = qof_query_int32_predicate
            (QOF_COMPARE_NEQ, GNC_INVOICE_EMPL_CREDIT_NOTE);
        qof_query_add_term (
            q, g_slist_prepend(NULL, INVOICE_TYPE),
            pred_data, QOF_QUERY_AND);
    }

    end_date = gnc_time (NULL);
    if (days_in_advance < 0)
        days_in_advance = 0;
    end_date += days_in_advance * 60 * 60 * 24;

    pred_data = qof_query_date_predicate
        (QOF_COMPARE_LTE, QOF_DATE_MATCH_NORMAL, end_date);
    qof_query_add_term (
        q, g_slist_prepend(NULL, INVOICE_DUE),
        pred_data, QOF_QUERY_AND);

    res = qof_query_run(q);
    len = g_list_length (res);
    if (!res || len <= 0)
    {
        qof_query_destroy(q);
        return NULL;
    }

    if (duetype == DUE_FOR_VENDOR)
    {
        message = g_strdup_printf
            (// Translators: %d is the number of bills/credit notes
             // due. This is a ngettext(3) message.
             ngettext("The following vendor document is due:",
                      "The following %d vendor documents are due:",
                      len),
             len);
        title = _("Due Bills Reminder");
    }
    else
    {
        message = g_strdup_printf
          (// Translators: %d is the number of invoices/credit notes
           // due. This is a ngettext(3) message. */
           ngettext("The following customer document is due:",
                    "The following %d customer documents are due:",
                    len),
           len);
        title = _("Due Invoices Reminder");
    }

    dialog = gnc_dialog_query_view_create(
        parent, param_list, q,
        title,
        message,
        TRUE, FALSE,
        1, GTK_SORT_ASCENDING,
        duetype == DUE_FOR_VENDOR ?
        vendorbuttons :
        customerbuttons, NULL);

    g_free(message);
    qof_query_destroy(q);
    return dialog;
}

void
gnc_invoice_remind_bills_due (GtkWindow *parent)
{
    QofBook *book;
    gint days;

    if (!gnc_current_session_exist()) return;
    book = qof_session_get_book(gnc_get_current_session());
    days = gnc_prefs_get_float(
        GNC_PREFS_GROUP_BILL,
        GNC_PREF_DAYS_IN_ADVANCE);

    gnc_invoice_show_docs_due (parent, book, days, DUE_FOR_VENDOR);
}

void
gnc_invoice_remind_invoices_due (GtkWindow *parent)
{
    QofBook *book;
    gint days;

    if (!gnc_current_session_exist()) return;
    book = qof_session_get_book(gnc_get_current_session());
    days = gnc_prefs_get_float(
        GNC_PREFS_GROUP_INVOICE,
        GNC_PREF_DAYS_IN_ADVANCE);

    gnc_invoice_show_docs_due (parent, book, days, DUE_FOR_CUSTOMER);
}

void
gnc_invoice_remind_bills_due_cb (void)
{
    if (!gnc_prefs_get_bool(
        GNC_PREFS_GROUP_BILL,
        GNC_PREF_NOTIFY_WHEN_DUE))
        return;

    gnc_invoice_remind_bills_due
        (GTK_WINDOW(gnc_ui_get_main_window (NULL)));
}

void
gnc_invoice_remind_invoices_due_cb (void)
{
    if (!gnc_prefs_get_bool
        (GNC_PREFS_GROUP_INVOICE, GNC_PREF_NOTIFY_WHEN_DUE))
        return;

    gnc_invoice_remind_invoices_due
        (GTK_WINDOW(gnc_ui_get_main_window (NULL)));
}
