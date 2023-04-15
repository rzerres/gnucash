/*
 * dialog-coowner.c -- Dialog for Co-Owner entry
 * Copyright (C) 2022 Ralf Zerres
 * Author: Ralf Zerres <ralf.zerres@mail.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published the Free Software Foundation; either version 2 of
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
#include <gdk/gdkkeysyms.h>

#include "dialog-utils.h"
#include "gnc-amount-edit.h"
#include "gnc-currency-edit.h"
#include "gnc-component-manager.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "gnc-ui-util.h"
#include "qof.h"

#include "dialog-search.h"
#include "search-param.h"
#include "gnc-account-sel.h"
#include "QuickFill.h"
#include "gnc-addr-quickfill.h"
#include "gnc-account-sel.h"

#include "gncAddress.h"
#include "gncCoOwner.h"
#include "gncCoOwnerP.h"
#include "gncOwner.h"

#include "business-gnome-utils.h"
#include "dialog-coowner.h"
#include "dialog-invoice.h"
#include "dialog-job.h"
#include "dialog-order.h"
#include "dialog-payment.h"

#define DIALOG_NEW_COOWNER_CM_CLASS "dialog-new-coowner"
#define DIALOG_EDIT_COOWNER_CM_CLASS "dialog-edit-coowner"

#define ADDR_QUICKFILL "GncAddress-Quickfill"
#define GNC_PREFS_GROUP_SEARCH "dialogs.business.coowner-search"


/*************************************************\
 * Function prototypes declaration
 * (assume C11 semantics, where order matters)
\*************************************************/

void
gnc_coowner_apt_share_changed_cb (GtkWidget *widget, gpointer data);
void
gnc_coowner_ccard_acct_toggled_cb (GtkToggleButton *button, gpointer data);

void gnc_coowner_billaddr2_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_billaddr3_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_billaddr4_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);

gboolean gnc_coowner_billaddr2_key_press_cb(
    GtkEntry *entry, GdkEventKey *event,
    gpointer user_data );
gboolean gnc_coowner_billaddr3_key_press_cb(
    GtkEntry *entry, GdkEventKey *event,
    gpointer user_data );
gboolean gnc_coowner_billaddr4_key_press_cb(
    GtkEntry *entry, GdkEventKey *event,
    gpointer user_data );

static GncCoOwner
*ow_get_coowner (CoOwnerWindow *ow);
void
gnc_coowner_name_changed_cb (GtkWidget *widget, gpointer data);
static void
gnc_ui_to_coowner (CoOwnerWindow *ow, GncCoOwner *coowner);

void gnc_coowner_shipaddr2_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_shipaddr3_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_shipaddr4_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);

gboolean gnc_coowner_shipaddr2_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );
gboolean gnc_coowner_shipaddr3_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );
gboolean gnc_coowner_shipaddr4_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );

void
gnc_coowner_taxincluded_changed_cb (GtkWidget *widget, gpointer data);
void
gnc_coowner_taxtable_changed_cb (GtkWidget *widget, gpointer data);
void
gnc_coowner_taxtable_check_cb (GtkToggleButton *togglebutton,
    gpointer user_data);

void
gnc_coowner_tenant_name_changed_cb (GtkWidget *widget, gpointer data);
void gnc_coowner_tenantaddr2_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_tenantaddr3_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);
void gnc_coowner_tenantaddr4_insert_cb(GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data);

gboolean gnc_coowner_tenantaddr2_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );
gboolean gnc_coowner_tenantaddr3_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );
gboolean gnc_coowner_tenantaddr4_key_press_cb( GtkEntry *entry,
    GdkEventKey *event, gpointer user_data );

void
gnc_coowner_terms_changed_cb (GtkWidget *widget, gpointer data);


void
gnc_coowner_window_cancel_cb (GtkWidget *widget, gpointer data);
static void
gnc_coowner_window_close_handler (gpointer user_data);
void
gnc_coowner_window_destroy_cb (GtkWidget *widget, gpointer data);
void
gnc_coowner_window_help_cb (GtkWidget *widget, gpointer data);
static CoOwnerWindow
*gnc_coowner_window_new (
    GtkWindow *parent,
    QofBook *bookp, GncCoOwner *coowner);
void
gnc_coowner_window_ok_cb (GtkWidget *widget, gpointer data);
static void
gnc_coowner_window_refresh_handler (GHashTable *changes,
    gpointer user_data);

/* Helper function prototypes */
static gboolean check_edit_amount (GtkWidget *amount, gnc_numeric *min,
    gnc_numeric *max, const char *error_message);
static gboolean check_entry_nonempty (GtkWidget *entry,
    const char *error_message);
static void edit_coowner_cb (GtkWindow *dialog, gpointer *coowner_p,
    gpointer user_data);
static gboolean find_handler (gpointer find_data, gpointer user_data);
static void free_coowner_cb (gpointer user_data);
static void invoice_coowner_cb (GtkWindow *dialog, gpointer *coowner_p,
    gpointer user_data);
static void jobs_coowner_cb (GtkWindow *dialog, gpointer *coowner_p,
    gpointer user_data);
static gpointer new_coowner_cb (GtkWindow *dialog, gpointer user_data);
static void order_coowner_cb (GtkWindow *dialog, gpointer *coowner_p,
    gpointer user_data);
static GncCoOwner *ow_get_coowner (CoOwnerWindow *ow);
static void payment_coowner_cb (GtkWindow *dialog, gpointer *coowner_p,
    gpointer user_data);


/*************************************************\
 * enumerations and type definitions
\*************************************************/

typedef enum
{
    NEW_COOWNER,
    EDIT_COOWNER
} CoOwnerDialogType;

struct _coowner_select_window
{
    QofBook  *book;
    QofQuery *q;
};

struct _coowner_window
{
    GtkWidget *dialog;

    GtkWidget *coowner_checkbutton_active;
    GtkWidget *coowner_checkbutton_tenant_active;

    GtkWidget *coowner_entry_coowner_addrname;
    GtkWidget *coowner_entry_coowner_addr1;
    GtkWidget *coowner_entry_coowner_addr2;
    GtkWidget *coowner_entry_coowner_addr3;
    GtkWidget *coowner_entry_coowner_addr4;
    GtkWidget *coowner_entry_coowner_mobile;
    GtkWidget *coowner_entry_coowner_phone;
    GtkWidget *coowner_entry_coowner_fax;
    GtkWidget *coowner_entry_coowner_email;

    GtkWidget *coowner_entry_coowner_id;
    GtkWidget *coowner_entry_coowner_name;
    GtkWidget *coowner_entry_coowner_text;

    GtkWidget *coowner_entry_apt_share;
    GtkWidget *coowner_entry_apt_unit;
    GtkWidget *ccard_acct_check;
    GtkWidget *ccard_acct_sel;
    GtkWidget *coowner_entry_currency;
    GtkWidget *coowner_entry_credit;
    GtkWidget *coowner_entry_discount;
    GtkWidget *coowner_entry_distribution_key;
    GtkWidget *coowner_entry_language;

    GtkWidget *coowner_entry_ship_addrname;
    GtkWidget *coowner_entry_ship_addr1;
    GtkWidget *coowner_entry_ship_addr2;
    GtkWidget *coowner_entry_ship_addr3;
    GtkWidget *coowner_entry_ship_addr4;
    GtkWidget *coowner_entry_ship_mobile;
    GtkWidget *coowner_entry_ship_phone;
    GtkWidget *coowner_entry_ship_fax;
    GtkWidget *coowner_entry_ship_email;

    GtkWidget *coowner_entry_tenant_id;
    GtkWidget *coowner_entry_tenant_name;
    GtkWidget *coowner_entry_tenant_addrname;
    GtkWidget *coowner_entry_tenant_addr1;
    GtkWidget *coowner_entry_tenant_addr2;
    GtkWidget *coowner_entry_tenant_addr3;
    GtkWidget *coowner_entry_tenant_addr4;
    GtkWidget *coowner_entry_tenant_phone;
    GtkWidget *coowner_entry_tenant_mobile;
    GtkWidget *coowner_entry_tenant_fax;
    GtkWidget *coowner_entry_tenant_email;
    GtkWidget *coowner_entry_tenant_language;
    GtkWidget *coowner_entry_tenant_text;

    //GncLanguage *languages;
    GncBillTerm *coowner_terms;
    GtkWidget *coowner_combobox_terms;
    GncTaxIncluded taxincluded;
    GtkWidget *coowner_combobox_taxincluded;
    GncTaxTable *coowner_entry_taxtable;
    GtkWidget *coowner_button_taxtable;
    GtkWidget *coowner_combobox_taxtable;

    CoOwnerDialogType dialog_type;
    GncGUID coowner_guid;
    gint component_id;
    gint tenant_id;
    QofBook *book;
    GncCoOwner *created_coowner;

    /* stored data for the description quickfill selection function */
    QuickFill *coowner_addr2_quickfill;
    QuickFill *coowner_addr3_quickfill;
    QuickFill *coowner_addr4_quickfill;
    QuickFill *ship_addr2_quickfill;
    QuickFill *ship_addr3_quickfill;
    QuickFill *ship_addr4_quickfill;
    QuickFill *tenant_addr2_quickfill;
    QuickFill *tenant_addr3_quickfill;
    QuickFill *tenant_addr4_quickfill;
    gint addrX_start_selection;
    gint addrX_end_selection;
    guint addrX_selection_source_id;
};


/*************************************************\
 * Functions
\*************************************************/

static GncCoOwner *
ow_get_coowner (CoOwnerWindow *ow)
{
    if (!ow)
        return NULL;

    return gncCoOwnerLookup (ow->book, &ow->coowner_guid);
}

/* CoOwner callback functions */
void
gnc_coowner_apt_share_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkComboBox *cbox = GTK_COMBO_BOX (widget);
    CoOwnerWindow *ow = data;

    if (!ow) return;
    if (!cbox) return;

    ow->coowner_entry_apt_share = gnc_simple_combo_get_value (cbox);
}

void
gnc_coowner_ccard_acct_toggled_cb (GtkToggleButton *button, gpointer data)
{
    CoOwnerWindow *ow = data;

    if (!ow)
        return;

    if (gtk_toggle_button_get_active (button))
    {
        gtk_widget_set_sensitive (ow->ccard_acct_sel, TRUE);
        gtk_widget_show (ow->ccard_acct_sel);
    }
    else
    {
        gtk_widget_set_sensitive (ow->ccard_acct_sel, TRUE);
        gtk_widget_hide (ow->ccard_acct_sel);
    }
}

void
gnc_coowner_name_changed_cb (GtkWidget *widget, gpointer data)
{
    CoOwnerWindow *ow = data;
    char *fullname, *title;
    const char *id, *name;

    if (!ow)
        return;

    name = gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_name));
    if (!name || *name == '\0')
        name = _("<No Co-Owner name>");

    id = gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_id));

    fullname = g_strconcat (name, " (", id, ")", (char *)NULL);

    if (ow->dialog_type == EDIT_COOWNER)
        title = g_strconcat (_("Edit Co-Owner"), " - ", fullname, (char *)NULL);
    else
        title = g_strconcat (_("New Co-Owner"), " - ", fullname, (char *)NULL);

    gtk_window_set_title (GTK_WINDOW (ow->dialog), title);

    g_free (fullname);
    g_free (title);
}

void
gnc_coowner_taxincluded_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkComboBox *cbox = GTK_COMBO_BOX (widget);
    CoOwnerWindow *ow = data;

    if (!ow) return;
    if (!cbox) return;

    ow->taxincluded = GPOINTER_TO_INT (gnc_simple_combo_get_value (cbox));
}

void
gnc_coowner_taxtable_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkComboBox *cbox = GTK_COMBO_BOX (widget);
    CoOwnerWindow *ow = data;

    if (!ow) return;
    if (!cbox) return;

    ow->coowner_entry_taxtable = gnc_simple_combo_get_value (cbox);
}

void
gnc_coowner_taxtable_check_cb (GtkToggleButton *togglebutton,
                                gpointer user_data)
{
    CoOwnerWindow *ow = user_data;

    if (gtk_toggle_button_get_active (togglebutton))
        gtk_widget_set_sensitive (ow->coowner_combobox_taxtable, TRUE);
    else
        gtk_widget_set_sensitive (ow->coowner_combobox_taxtable, FALSE);
}

void
gnc_coowner_tenant_name_changed_cb (GtkWidget *widget, gpointer data)
{
    CoOwnerWindow *ow = data;
    char *fullname;
    const char *id;
    const char *name;

    if (!ow)
        return;

    name = gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_name));
    if (!name || *name == '\0')
        name = _("<No tenant name>");

    id = gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_id));

    fullname = g_strconcat (name, " (", id, ")", (char *)NULL);

    g_free (fullname);
}

void
gnc_coowner_terms_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkComboBox *cbox = GTK_COMBO_BOX (widget);
    CoOwnerWindow *ow = data;

    if (!ow) return;
    if (!cbox) return;

    ow->coowner_terms = gnc_simple_combo_get_value (cbox);
}

static void
gnc_coowner_window_close_handler (gpointer user_data)
{
    CoOwnerWindow *ow = user_data;

    gtk_widget_destroy (ow->dialog);
}

void
gnc_coowner_window_cancel_cb (GtkWidget *widget, gpointer data)
{
    CoOwnerWindow *ow = data;

    gnc_close_gui_component (ow->component_id);
}

void
gnc_coowner_window_destroy_cb (GtkWidget *widget, gpointer data)
{
    CoOwnerWindow *ow = data;
    GncCoOwner *coowner = ow_get_coowner (ow);

    gnc_suspend_gui_refresh ();

    if (ow->dialog_type == NEW_COOWNER && coowner != NULL)
    {
        gncCoOwnerBeginEdit (coowner);
        gncCoOwnerDestroy (coowner);
        ow->coowner_guid = *guid_null ();
    }

    gnc_unregister_gui_component (ow->component_id);
    gnc_resume_gui_refresh ();

    g_free (ow);
}

void
gnc_coowner_window_help_cb (GtkWidget *widget, gpointer data)
{
    CoOwnerWindow *ow = data;
    gnc_gnome_help (GTK_WINDOW(ow->dialog), DF_MANUAL, DL_USAGE_COOWNER);
}

void
gnc_coowner_window_ok_cb (GtkWidget *widget, gpointer data)
{
    gnc_commodity *currency;
    gnc_numeric min, max;
    CoOwnerWindow *ow = data;
    GNCPrintAmountInfo print_info;
    gnc_numeric share_min, share_max;
    gchar *string;

    /*
     * Co-Owner Frame
     */

    /* Set the coowner id if one hasn't been chosen */
    if (g_strcmp0 (gtk_entry_get_text
        (GTK_ENTRY (ow->coowner_entry_coowner_id)), "") == 0)
    {
        string = gncCoOwnerNextID (ow->book);
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_id), string);
        g_free(string);
    }

    /* Check for valid coowner id */
    if (check_entry_nonempty (
         ow->coowner_entry_coowner_name,
         _("The Co-Owner name field cannot be left blank, please "
           "enter a coowner's name associated to the property.")))
        return;

    /* Check for valid coowner address name */
    if (check_entry_nonempty (
        ow->coowner_entry_coowner_addrname,
        _("The Co-Owner name field inside the address section "
          "cannot be left blank, please "
          "enter a coowner's name associated as contact for the apartment.")))
        return;

    /* Verify that apartment share amount is valid (or empty) */
    share_min = gnc_numeric_zero ();
    share_max = gnc_numeric_create (1000, 1);
    if (check_edit_amount (
        ow->coowner_entry_apt_share, &share_min, &share_max,
        _("Apartment share value must be set inside the valid range [0-1000], "
          "where 1000 represents 100% of the property shares.")))
        return;

    /* Verify that apartment unit name is valid */
    if (check_entry_nonempty (
        ow->coowner_entry_apt_unit,
        _("The name referencing the Apartment Unit cannot be left blank, "
          "please enter a valid idenification string.")))
        return;

    /* Verify that distribution key is valid */
    if (check_entry_nonempty (
        ow->coowner_entry_distribution_key,
        _("The Distribution Key cannot be left blank, please "
         "enter a valid identification string.")))
         return;

    /*
     * Billing Frame
     */

    /* Verify that currency/commodity is valid (or empty) */
    currency = gnc_currency_edit_get_currency (
        GNC_CURRENCY_EDIT(ow->coowner_entry_currency));
    print_info = gnc_commodity_print_info (currency, FALSE);

    /* Verify that credit amount is valid (or empty) */
    min = gnc_numeric_zero ();
    gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (
        ow->coowner_entry_credit), print_info);
    gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (
        ow->coowner_entry_credit),
        gnc_commodity_get_fraction (currency));
    if (check_edit_amount (
        ow->coowner_entry_credit, &min, NULL,
        _("Credit must be a positive amount or "
          "you just leave it blank.")))
        return;

    /* Verify that discount amount is valid (or empty) */
    max = gnc_numeric_create (100, 1);
    if (check_edit_amount (
        ow->coowner_entry_discount, &min, &max,
        _("Discount percentage must be between 0-100 "
          "or just leave it blank.")))
        return;

    /*
     * Tenant Frame
     */

    /* Set the tenant id
     * tenant name was specified, but no tenant id has been chosen
     */
    /* if ((g_strcmp0 (gtk_entry_get_text */
    /*                 (GTK_ENTRY (ow->coowner_entry_tenant_id)), "") == 0) */
    /*   && (g_strcmp0 (gtk_entry_get_text */
    /*                  (GTK_ENTRY (ow->coowner_entry_tenant_name)), "") > 0)) */
    /* { */
    /*     // well, the tenant isn't worth its own counter or module name */
    /*     // therefore we preset the active coowner_id */
    /*     gtk_entry_set_text ( */
    /*         GTK_ENTRY (ow->coowner_entry_tenant_id), */
    /*         gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_id))); */
    /* } */

    /* /\* tenant name optional optional *\/ */
    /* if (check_entry_nonempty ( */
    /*      ow->coowner_entry_tenant_name, */
    /*      _("The Tenant name field cannot be left blank, please " */
    /*        "enter a tenant name associated to the property."))) */
    /*     return; */

    /* /\* tenant address name is optional *\/ */
    /* if (check_entry_nonempty ( */
    /*     ow->coowner_entry_tenant_name, */
    /*     _("The tenant name field inside the address section " */
    /*       "cannot be left blank, please " */
    /*       "enter a tenant name associated as contact for the apartment."))) */
    /*     return; */

    /*
     * UI actions done -> save data structure persitant via backend
     */
    {
        /* Update coowner structure from GUI fields */
        GncCoOwner *coowner = ow_get_coowner (ow);

        if (coowner)
        {
            /* Now save it off */
            gnc_ui_to_coowner (ow, coowner);
        }

        ow->created_coowner = coowner;
        ow->coowner_guid = *guid_null ();
    }

    /* Now save it off */
    {
        GncCoOwner *coowner = ow_get_coowner (ow);
        if (coowner)
        {
            gnc_ui_to_coowner (ow, coowner);
        }
        ow->created_coowner = coowner;
        ow->coowner_guid = *guid_null ();
    }

    gnc_close_gui_component (ow->component_id);
}

/* CoOwner UI specific functions */

static CoOwnerWindow
*gnc_coowner_window_new (
    GtkWindow *parent,
    QofBook *bookp,
    GncCoOwner *coowner)
{
    CoOwnerWindow *ow;
    GtkBuilder *builder;
    GtkWidget *hbox, *edit;
    gnc_commodity *currency;
    GNCPrintAmountInfo print_info;
    GList *acct_types;
    Account *ccard_acct;

    /*
     * Find an existing window for this coowner.  If found, bring it to
     * the front.
     */
    if (coowner)
    {
        // Found existing coowner -> preset window
        GncGUID coowner_guid;

        coowner_guid = *gncCoOwnerGetGUID (coowner);
        ow = gnc_find_first_gui_component (
            DIALOG_EDIT_COOWNER_CM_CLASS, find_handler, &coowner_guid);
        if (ow)
        {
            gtk_window_set_transient_for (GTK_WINDOW(ow->dialog), parent);
            gtk_window_present (GTK_WINDOW(ow->dialog));
            return(ow);
        }
    }

    /* Find the default currency */
    if (coowner)
        currency = gncCoOwnerGetCurrency (coowner);
    else
        currency = gnc_default_currency ();

    /*
     * No existing coowner window found.  Build a new one.
     */
    ow = g_new0 (CoOwnerWindow, 1);
    ow->book = bookp;

    /*
     * Create the Co-Owner UI
     */

    builder = gtk_builder_new();
    // TODO: create language dialog
    //gnc_builder_add_from_file (builder, "dialog-coowner.glade",
    //    "coowner_liststore_languages");
    gnc_builder_add_from_file (builder, "dialog-coowner.glade",
        "coowner_liststore_taxincluded");
    gnc_builder_add_from_file (builder, "dialog-coowner.glade",
        "coowner_liststore_taxtable");
    gnc_builder_add_from_file (builder, "dialog-coowner.glade",
        "coowner_liststore_terms");
    gnc_builder_add_from_file (builder, "dialog-coowner.glade",
        "coowner_dialog");
    ow->dialog = GTK_WIDGET(gtk_builder_get_object (builder,
        "coowner_dialog"));
    gtk_window_set_transient_for (GTK_WINDOW(ow->dialog), parent);

    /* Set the name for this dialog so it can be easily manipulated with css */
    gtk_widget_set_name (GTK_WIDGET(ow->dialog), "gnc-id-coowner");
    gnc_widget_style_context_add_class (GTK_WIDGET(ow->dialog),
        "gnc-class-coowners");

    g_object_set_data (G_OBJECT (ow->dialog), "dialog_info", ow);

    /*
     * Co-Owner Frame
     */

    /* Get coowner identifiers (id, name) */
    ow->coowner_entry_coowner_id = GTK_WIDGET(gtk_builder_get_object (
      builder,
      "coowner_entry_coowner_id"));
    ow->coowner_entry_coowner_name = GTK_WIDGET(gtk_builder_get_object (
      builder,
      "coowner_entry_coowner_name"));

    /* Activate/Deactivate the coowner entity */
    ow->coowner_checkbutton_active = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_checkbutton_active"));

    /* Address fields */
    ow->coowner_entry_coowner_addrname = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_addrname"));
    ow->coowner_entry_coowner_addr1 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_addr1"));
    ow->coowner_entry_coowner_addr2 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_addr2"));
    ow->coowner_entry_coowner_addr3 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_addr3"));
    ow->coowner_entry_coowner_addr4 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_addr4"));
    ow->coowner_entry_coowner_mobile = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_mobile"));
    ow->coowner_entry_coowner_phone = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_phone"));
    ow->coowner_entry_coowner_fax = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_fax"));
    ow->coowner_entry_coowner_email = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_email"));

    /* Apartment share: Percentage value */
    edit = gnc_amount_edit_new();
    gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (edit), TRUE);
    print_info = gnc_integral_print_info ();
    print_info.max_decimal_places = 4;
    gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (edit), print_info);
    gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (edit), 1000);
    ow->coowner_entry_apt_share = edit;
    gtk_widget_show (edit);

    hbox = GTK_WIDGET (gtk_builder_get_object (builder, "coowner_hbox_apt_share"));
    gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

    /* Apartment unit value */
    ow->coowner_entry_apt_unit = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_apt_unit"));

    /* Distribution key value */
    ow->coowner_entry_distribution_key = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_distribution_key"));

    /* Language */
    ow->coowner_entry_language = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_language"));

    /* Notes */
    ow->coowner_entry_coowner_text = GTK_WIDGET (gtk_builder_get_object (
        builder,
        "coowner_entry_coowner_text"));

    /*
     * Billing Frame
     */

    /* Credit: Monetary Value */
    edit = gnc_amount_edit_new();
    print_info = gnc_commodity_print_info (currency, FALSE);
    gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (edit), TRUE);
    gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (edit), print_info);
    gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (edit),
                                  gnc_commodity_get_fraction (currency));
    ow->coowner_entry_credit = edit;
    gtk_widget_show (edit);

    hbox = GTK_WIDGET (gtk_builder_get_object (builder, "coowner_hbox_credit"));
    gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

    /* CCard Account Selection */
    ow->ccard_acct_check = GTK_WIDGET(gtk_builder_get_object (builder, "ccard_check"));

    edit = gnc_account_sel_new();
    acct_types = g_list_prepend(NULL, (gpointer)ACCT_TYPE_CREDIT);
    gnc_account_sel_set_acct_filters (GNC_ACCOUNT_SEL(edit), acct_types, NULL);
    g_list_free (acct_types);

    ow->ccard_acct_sel = edit;
    gtk_widget_show (edit);

    hbox = GTK_WIDGET(gtk_builder_get_object (builder, "ccard_acct_hbox"));
    gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

    /* Currency */
    edit = gnc_currency_edit_new();
    gnc_currency_edit_set_currency (GNC_CURRENCY_EDIT(edit), currency);
    ow->coowner_entry_currency = edit;

    hbox = GTK_WIDGET(gtk_builder_get_object (builder, "coowner_hbox_currency"));
    gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

    /* Discount: Percentage Value */
    edit = gnc_amount_edit_new();
    gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (edit), TRUE);
    print_info = gnc_integral_print_info ();
    print_info.max_decimal_places = 5;
    gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (edit), print_info);
    gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (edit), 100000);
    ow->coowner_entry_discount = edit;
    gtk_widget_show (edit);

    hbox = GTK_WIDGET (gtk_builder_get_object (builder, "coowner_hbox_discount"));
    gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

    /* Tax table */
    ow->coowner_combobox_taxincluded = GTK_WIDGET (
        gtk_builder_get_object (builder, "coowner_combobox_taxincluded"));
    ow->coowner_button_taxtable = GTK_WIDGET (
        gtk_builder_get_object (builder, "coowner_button_taxtable"));
    ow->coowner_combobox_taxtable = GTK_WIDGET (
        gtk_builder_get_object (builder, "coowner_combobox_taxtable"));

    /* Terms  */
    ow->coowner_combobox_terms = GTK_WIDGET (
        gtk_builder_get_object (builder, "coowner_combobox_terms"));

    /*
     * Tenant Frame
     */

    /* Get tenant identifiers (id, name) */
    ow->coowner_entry_tenant_id = GTK_WIDGET(gtk_builder_get_object (
      builder,
      "coowner_entry_tenant_id"));
    ow->coowner_entry_tenant_name= GTK_WIDGET(gtk_builder_get_object (
      builder,
      "coowner_entry_tenant_name"));

    /* Activate/Deactivate the coowner entity */
    ow->coowner_checkbutton_tenant_active = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_checkbutton_tenant_active"));

    /* Address fields */
    ow->coowner_entry_tenant_addrname = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_addrname"));
    ow->coowner_entry_tenant_addr1 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_addr1"));
    ow->coowner_entry_tenant_addr2 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_addr2"));
    ow->coowner_entry_tenant_addr3 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_addr3"));
    ow->coowner_entry_tenant_addr4 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_addr4"));
    ow->coowner_entry_tenant_mobile = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_mobile"));
    ow->coowner_entry_tenant_phone = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_phone"));
    ow->coowner_entry_tenant_fax = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_fax"));
    ow->coowner_entry_tenant_email = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_email"));

    /* Notes */
    ow->coowner_entry_tenant_text = GTK_WIDGET (gtk_builder_get_object (
        builder,
        "coowner_entry_tenant_text"));

    /*
     * Shipping Frame
     */

    /* Shiping fields */
    ow->coowner_entry_ship_addrname = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_addrname"));
    ow->coowner_entry_ship_addr1 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_addr1"));
    ow->coowner_entry_ship_addr2 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_addr2"));
    ow->coowner_entry_ship_addr3 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_addr3"));
    ow->coowner_entry_ship_addr4 = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_addr4"));
    ow->coowner_entry_ship_mobile = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_mobile"));
    ow->coowner_entry_ship_phone = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_phone"));
    ow->coowner_entry_ship_fax = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_fax"));
    ow->coowner_entry_ship_email = GTK_WIDGET(gtk_builder_get_object (
        builder,
        "coowner_entry_ship_email"));

    /* Setup signals */
    gtk_builder_connect_signals_full (builder, gnc_builder_connect_full_func, ow);

    /*
     * Read existing values for the given coowner entity
     */
    if (coowner != NULL)
    {
        GtkTextBuffer *text_buffer;
        GncAddress *coowner_addr;
        GncAddress *ship_addr;
        GncAddress *tenant_addr;
        const char *string;

        ow->dialog_type = EDIT_COOWNER;
        ow->coowner_guid = *gncCoOwnerGetGUID (coowner);

        /*
         * Co-Owner Frame
         */

        /* Active check toggle button */
        gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_active),
            gncCoOwnerGetActive (coowner));

        /* Co-owner ID */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_id),
            gncCoOwnerGetID (coowner));

        /* Co-owner name */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_name),
            gncCoOwnerGetName (coowner));

        /* Assign Address */
        coowner_addr = gncCoOwnerGetAddr (coowner);

        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_addrname),
            gncAddressGetName (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_addr1),
            gncAddressGetAddr1 (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_addr2),
            gncAddressGetAddr2 (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_addr3),
            gncAddressGetAddr3 (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_addr4),
            gncAddressGetAddr4 (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_mobile),
            gncAddressGetMobile (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_phone),
            gncAddressGetPhone (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_fax),
            gncAddressGetFax (coowner_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_coowner_email),
            gncAddressGetEmail (coowner_addr));

        /* Distribution Key */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_distribution_key),
            gncCoOwnerGetDistributionKey (coowner));

        /* Set Language */
        /* TODO: improve that to be a selection box */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_language),
            gncCoOwnerGetLanguage (coowner));

        /* Notes */
        string = gncCoOwnerGetNotes (coowner);
        text_buffer = gtk_text_view_get_buffer
            (GTK_TEXT_VIEW(ow->coowner_entry_coowner_text));
        gtk_text_buffer_set_text (text_buffer, string, -1);

        /*
         * Billing Frame
         */

        /* Set toggle buttons */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
            (ow->coowner_button_taxtable),
            gncCoOwnerGetTaxTableOverride (coowner));

        ow->coowner_terms = gncCoOwnerGetTerms (coowner);

        /*
         * Tenant Frame
         */
        /* Tenant active state */
        gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_tenant_active),
            gncCoOwnerGetActive (coowner));

        /* Tenant ID */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_id),
            gncCoOwnerGetTenantID (coowner));

        /* Tenant name */
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_name),
            gncCoOwnerGetTenantName (coowner));

        /* Tenant address fields */
        tenant_addr = gncCoOwnerGetTenantAddr (coowner);

        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_addrname),
            gncAddressGetName (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_addr1),
            gncAddressGetAddr1 (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_addr2),
            gncAddressGetAddr2 (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_addr3),
            gncAddressGetAddr3 (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_addr4),
            gncAddressGetAddr4 (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_phone),
            gncAddressGetMobile (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_mobile),
            gncAddressGetPhone (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_fax),
            gncAddressGetFax (tenant_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_tenant_email),
            gncAddressGetEmail (tenant_addr));

        /* Set tenant notes */
        string = gncCoOwnerGetTenantNotes (coowner);
        text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(ow->coowner_entry_tenant_text));
        gtk_text_buffer_set_text (text_buffer, string, -1);

        /*
         * Shipping Frame
         */

        /* Setup Shiping Address */
        ship_addr = gncCoOwnerGetShipAddr (coowner);

        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_addrname),
            gncAddressGetName (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_addr1),
            gncAddressGetAddr1 (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_addr2),
            gncAddressGetAddr2 (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_addr3),
            gncAddressGetAddr3 (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_addr4),
            gncAddressGetAddr4 (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_phone),
            gncAddressGetPhone (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_mobile),
            gncAddressGetMobile (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_fax),
            gncAddressGetFax (ship_addr));
        gtk_entry_set_text (GTK_ENTRY (ow->coowner_entry_ship_email),
            gncAddressGetEmail (ship_addr));

        /* Registrer the handler */
        ow->component_id =
            gnc_register_gui_component
                (DIALOG_EDIT_COOWNER_CM_CLASS,
                 gnc_coowner_window_refresh_handler,
                 gnc_coowner_window_close_handler,
                 ow);
    }
    else
    {
        /*
         * Setup new coowner entity with initial values
         */
        coowner = gncCoOwnerCreate (bookp);
        ow->coowner_guid = *gncCoOwnerGetGUID (coowner);

        ow->dialog_type = NEW_COOWNER;
        ow->component_id =
            gnc_register_gui_component
                (DIALOG_NEW_COOWNER_CM_CLASS,
                 gnc_coowner_window_refresh_handler,
                 gnc_coowner_window_close_handler,
                 ow);

        /* TODO: assign global-default language */
        //ow->languages = NULL;

        /* TODO: assign global-default terms */
        ow->coowner_terms = NULL;
    }

    /*
     * From here on coowner exists
     * either passed in or newly created
     */

    /* Set the coowner Id */
    gnc_gui_component_watch_entity_type (ow->component_id,
        GNC_COOWNER_MODULE_NAME,
        QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    /* Set the coowner name value */
    gtk_entry_set_text (
         GTK_ENTRY (ow->coowner_entry_coowner_name),
         gncCoOwnerGetName (coowner));

    /* Set coowner active state */
    gtk_toggle_button_set_active (
        GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_active),
        gncCoOwnerGetActive (coowner));

    /* Set the apartment share value */
    gnc_amount_edit_set_amount (
         GNC_AMOUNT_EDIT (ow->coowner_entry_apt_share),
         gncCoOwnerGetAptShare (coowner));

    /* Set the apartment unit value */
    gtk_entry_set_text
        (GTK_ENTRY (ow->coowner_entry_apt_unit),
         gncCoOwnerGetAptUnit (coowner));

    /* Set Credit-Card entities */
    ccard_acct = gncCoOwnerGetCCard (coowner);
    if (ccard_acct == NULL)
    {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (ow->ccard_acct_check), FALSE);
        gtk_widget_set_sensitive (ow->ccard_acct_sel, FALSE);
        gtk_widget_hide (ow->ccard_acct_sel);
    }
    else
    {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (ow->ccard_acct_check), TRUE);
        gnc_account_sel_set_account (GNC_ACCOUNT_SEL
            (ow->ccard_acct_sel), ccard_acct, FALSE);
    }

    /* Set billing terms */
    gnc_billterms_combo (GTK_COMBO_BOX(ow->coowner_combobox_terms),
        bookp, TRUE, ow->coowner_terms);

    /* Setup tax related entities */
    ow->taxincluded = gncCoOwnerGetTaxIncluded (coowner);
    gnc_taxincluded_combo
        (GTK_COMBO_BOX(ow->coowner_combobox_taxincluded),
         ow->taxincluded);

    ow->coowner_entry_taxtable = gncCoOwnerGetTaxTable (coowner);
    gnc_taxtables_combo (GTK_COMBO_BOX(ow->coowner_combobox_taxtable),
        bookp, TRUE, ow->coowner_entry_taxtable);

    /* Set tenant active state */
    gtk_toggle_button_set_active (
        GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_tenant_active),
        gncCoOwnerGetActive (coowner));

    /* Set tenant Id */
    gtk_entry_set_text (
         GTK_ENTRY (ow->coowner_entry_tenant_id),
         gncCoOwnerGetTenantID (coowner));

    /* Set the tenant name value */
    gtk_entry_set_text (
         GTK_ENTRY (ow->coowner_entry_tenant_name),
         gncCoOwnerGetTenantName (coowner));

    /* Set up the addr line quickfill */
    ow->coowner_addr2_quickfill = gnc_get_shared_address_addr2_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->coowner_addr3_quickfill = gnc_get_shared_address_addr3_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->coowner_addr4_quickfill = gnc_get_shared_address_addr4_quickfill(ow->book,
        ADDR_QUICKFILL);

    /* Set up the tenant_addr line quickfill */
    ow->tenant_addr2_quickfill = gnc_get_shared_address_addr2_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->tenant_addr3_quickfill = gnc_get_shared_address_addr3_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->tenant_addr4_quickfill = gnc_get_shared_address_addr4_quickfill(ow->book,
        ADDR_QUICKFILL);

    /* Set up the tenant_addr line quickfill */
    ow->tenant_addr2_quickfill = gnc_get_shared_address_addr2_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->tenant_addr3_quickfill = gnc_get_shared_address_addr3_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->tenant_addr4_quickfill = gnc_get_shared_address_addr4_quickfill(ow->book,
        ADDR_QUICKFILL);

    /* Set up the shipaddr line quickfill */
    ow->ship_addr2_quickfill = gnc_get_shared_address_addr2_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->ship_addr3_quickfill = gnc_get_shared_address_addr3_quickfill(ow->book,
        ADDR_QUICKFILL);
    ow->ship_addr4_quickfill = gnc_get_shared_address_addr4_quickfill(ow->book,
        ADDR_QUICKFILL);

    /* Set Discount amounts */
    gnc_amount_edit_set_amount (GNC_AMOUNT_EDIT (ow->coowner_entry_discount),
        gncCoOwnerGetDiscount (coowner));

    /* Set Discount amounts */
    gnc_amount_edit_set_amount (GNC_AMOUNT_EDIT (ow->coowner_entry_credit),
        gncCoOwnerGetCredit (coowner));

    /* Handle the coowner widget */
    gtk_widget_show_all (ow->dialog);
    g_object_unref(G_OBJECT(builder));

    return ow;
}

static void
gnc_coowner_window_refresh_handler (
    GHashTable *changes,
    gpointer user_data)
{
    CoOwnerWindow *ow = user_data;
    const EventInfo *info;
    GncCoOwner *coowner = ow_get_coowner (ow);

    /* If there isn't a coowner behind us, close down */
    if (!coowner)
    {
        gnc_close_gui_component (ow->component_id);
        return;
    }

    /* Next, close if this is a destroy event */
    if (changes)
    {
        info = gnc_gui_get_entity_events (changes, &ow->coowner_guid);
        if (info && (info->event_mask & QOF_EVENT_DESTROY))
        {
            gnc_close_gui_component (ow->component_id);
            return;
        }
    }
}

CoOwnerWindow *
gnc_ui_coowner_edit (GtkWindow *parent, GncCoOwner *coowner)
{
    CoOwnerWindow *ow;

    if (!coowner) return NULL;

    ow = gnc_coowner_window_new (parent, gncCoOwnerGetBook(coowner), coowner);

    return ow;
}

CoOwnerWindow *
gnc_ui_coowner_new (GtkWindow *parent, QofBook *bookp)
{
    CoOwnerWindow *ow;

    /* Make sure required options exist */
    if (!bookp) return NULL;

    ow = gnc_coowner_window_new (parent, bookp, NULL);

    return ow;
}

static void
gnc_ui_to_coowner (CoOwnerWindow *ow, GncCoOwner *coowner)
{
    GtkTextBuffer* text_buffer;
    GtkTextIter start, end;
    gchar *text;
    GncAddress *coowner_addr;
    GncAddress *ship_addr;
    GncAddress *tenant_addr;

    /* lock gui changes */
    gnc_suspend_gui_refresh ();

    gncCoOwnerBeginEdit (coowner);

    if (ow->dialog_type == NEW_COOWNER)
        qof_event_gen(QOF_INSTANCE(coowner), QOF_EVENT_ADD, NULL);

    /*
     * Co-Owner Frame attributes
     */

    /* Set coowner active/inactive */
    gncCoOwnerSetActive (coowner, gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_active)));

    /* Fill in the apartment share */
    gncCoOwnerSetAptShare (coowner, gnc_amount_edit_get_amount
        (GNC_AMOUNT_EDIT (ow->coowner_entry_apt_share)));

    /* Parse and set the apartment unit value */
    gncCoOwnerSetAptUnit (coowner, gtk_editable_get_chars
        (GTK_EDITABLE (ow->coowner_entry_apt_unit), 0, -1));

    /* Fill in the address values */
    coowner_addr = gncCoOwnerGetAddr (coowner);
    gncAddressSetAddr1 (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_addr1)));
    gncAddressSetAddr2 (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_addr2)));
    gncAddressSetAddr3 (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_addr3)));
    gncAddressSetAddr4 (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_addr4)));
    gncAddressSetMobile (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_mobile)));
    gncAddressSetPhone (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_phone)));
    gncAddressSetFax (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_fax)));
    gncAddressSetEmail (coowner_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_email)));

    /* Fill in the distribution key */
    // FIXME: switch to distriblist reference once its finished
    gncCoOwnerSetDistributionKey (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_distribution_key)));

    /* Fill in the coowner identification key */
    gncCoOwnerSetID (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_id)));

    /* Fill in the coowner identification name */
    gncCoOwnerSetName (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_coowner_name)));

    /* Fill in the language */
    gncCoOwnerSetLanguage (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_language)));

    /* Fill in extra coowner notes */
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(ow->coowner_entry_coowner_text));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncCoOwnerSetNotes (coowner, text);

    /*
     * Billing Frame attributes
     */

    /* Fill in the Credit-Card Account */
    gncCoOwnerSetCCard (coowner, (gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (ow->ccard_acct_check)) ?
            gnc_account_sel_get_account
            (GNC_ACCOUNT_SEL (ow->ccard_acct_sel)) : NULL));
    gncCoOwnerSetCredit (coowner, gnc_amount_edit_get_amount
        (GNC_AMOUNT_EDIT (ow->coowner_entry_credit)));

    /* Parse and set the currency */
    gncCoOwnerSetCurrency (coowner,
        gnc_currency_edit_get_currency (GNC_CURRENCY_EDIT
        (ow->coowner_entry_currency)));

    /* Fill in the Discount Account */
    gncCoOwnerSetDiscount (coowner, gnc_amount_edit_get_amount
        (GNC_AMOUNT_EDIT (ow->coowner_entry_discount)));

    /* Parse and set the coowner terms */
    gncCoOwnerSetTerms (coowner, ow->coowner_terms);

    /* Parse and set the tax units */
    gncCoOwnerSetTaxIncluded (coowner, ow->taxincluded);
    gncCoOwnerSetTaxTableOverride (coowner,
       gtk_toggle_button_get_active (
           GTK_TOGGLE_BUTTON (ow->coowner_button_taxtable)));
    gncCoOwnerSetTaxTable (coowner, ow->coowner_entry_taxtable);

    /*
     * Tenant Frame attributes
     */

    /* Set tenant active/inactive */
    gncCoOwnerSetTenantActive (coowner, gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (ow->coowner_checkbutton_tenant_active)));

    /* Fill in the tenant identification key */
    gncCoOwnerSetTenantID (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_id)));

    /* Fill in the tenant name */
    gncCoOwnerSetTenantName (coowner, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_name)));

    /* Fill in the tenant address */
    tenant_addr = gncCoOwnerGetTenantAddr (coowner);
    gncAddressSetName (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_addrname)));
    gncAddressSetAddr1 (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_addr1)));
    gncAddressSetAddr2 (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_addr2)));
    gncAddressSetAddr3 (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_addr3)));
    gncAddressSetAddr4 (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_addr4)));
    gncAddressSetMobile (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_mobile)));
    gncAddressSetPhone (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_phone)));
    gncAddressSetFax (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_fax)));
    gncAddressSetEmail (tenant_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_tenant_email)));

    /* Fill in tenant extra notes */
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(ow->coowner_entry_tenant_text));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncCoOwnerSetTenantNotes (coowner, text);

    /*
     * Shipping Frame attributes
     */

    /* Fill in the shipping address */
    ship_addr = gncCoOwnerGetShipAddr (coowner);
    gncAddressSetName (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_addrname)));
    gncAddressSetAddr1 (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_addr1)));
    gncAddressSetAddr2 (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_addr2)));
    gncAddressSetAddr3 (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_addr3)));
    gncAddressSetAddr4 (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_addr4)));
    gncAddressSetMobile (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_mobile)));
    gncAddressSetPhone (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_phone)));
    gncAddressSetFax (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_fax)));
    gncAddressSetEmail (ship_addr, gtk_entry_get_text (GTK_ENTRY (ow->coowner_entry_ship_email)));

    /*
     * Save to persistent store
     */

    /* commit the data-structure */
    gncCoOwnerCommitEdit (coowner);

    /* unlock gui changes */
    gnc_resume_gui_refresh ();
}

/* CoOwner helper and selection functions */

static gboolean
check_edit_amount (
    GtkWidget *amount,
    gnc_numeric *min, gnc_numeric *max,
    const char *error_message)
{
    GError *error = NULL;
    if (!gnc_amount_edit_evaluate (GNC_AMOUNT_EDIT(amount), &error))
    {
        gnc_error_dialog (gnc_ui_get_gtk_window (amount), "%s", error->message);
        g_error_free (error);
        return TRUE;
    }
    return FALSE;
}

static gboolean
check_entry_nonempty (GtkWidget *entry,
    const char *error_message)
{
    const char *res = gtk_entry_get_text (GTK_ENTRY (entry));
    if (g_strcmp0 (res, "") == 0)
    {
        if (error_message)
            gnc_error_dialog (gnc_ui_get_gtk_window(entry), "%s", error_message);
        return TRUE;
    }
    return FALSE;
}

GNCSearchWindow *
gnc_coowner_search (GtkWindow *parent, GncCoOwner *start, QofBook *book)
{
    QofIdType type = GNC_COOWNER_MODULE_NAME;
    QofQuery *q, *q2 = NULL;
    struct _coowner_select_window *sw;
    static GList *params = NULL;
    static GList *columns = NULL;
    static GNCSearchCallbackButton buttons[] =
    {
        { N_("View/Edit Co-Owner"), edit_coowner_cb, NULL, TRUE},
        { N_("Co-Owner's Jobs"), jobs_coowner_cb, NULL, TRUE},
        { N_("Settlement"), invoice_coowner_cb, NULL, TRUE},
        { N_("Process Payment"), payment_coowner_cb, NULL, FALSE},
        { NULL },
    };
    (void)order_coowner_cb;

    g_return_val_if_fail (book, NULL);

    /* Build parameter list in reverse order */
    if (params == NULL)
    {
        params = gnc_search_param_prepend (
            params, _("Co-Owner ID"), NULL,
            type, COOWNER_ID, NULL);
        params = gnc_search_param_prepend (
            params, _("Co-Owner Name"), NULL,
            type, COOWNER_NAME, NULL);
        params = gnc_search_param_prepend (
            params, _("Apartment Unit"), NULL,
            type, COOWNER_APT_UNIT, NULL);
        params = gnc_search_param_prepend (
            params, _("Settlement Contact"), NULL,
            type, COOWNER_ADDR,
            ADDRESS_NAME, NULL);
        params = gnc_search_param_prepend (
            params, _("Shipping Contact"), NULL,
            type, COOWNER_SHIP_ADDRESS,
            ADDRESS_NAME, NULL);
        params = gnc_search_param_prepend (
            params, _("Tenant ID"), NULL,
            type, COOWNER_TENANT_ID, NULL);
        params = gnc_search_param_prepend (
            params, _("Tenant Name"), NULL,
            type, COOWNER_TENANT_NAME, NULL);
    }

    /* Build the column list in reverse order */
    if (columns == NULL)
    {
        columns = gnc_search_param_prepend (
            columns, _("Tenant Name"), NULL,
            type,
            COOWNER_TENANT_NAME, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Tenant ID"), NULL,
            type,
            COOWNER_TENANT_ID, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Shipping Contact"),
            NULL, type,
            COOWNER_SHIP_ADDRESS, ADDRESS_NAME,
            NULL);
        columns = gnc_search_param_prepend (
            columns, _("Settlement Contact"),
            NULL,
            type, COOWNER_ADDR, ADDRESS_NAME,
            NULL);
        columns = gnc_search_param_prepend (
            columns, _("Apartment Unit"),
            NULL, type,
            COOWNER_APT_UNIT, NULL);
        columns = gnc_search_param_prepend (
            columns, _("Name"), NULL, type,
            COOWNER_ADDR, ADDRESS_NAME, NULL);
        columns = gnc_search_param_prepend (
            columns, _("ID #"), NULL, type,
            COOWNER_ID, NULL);
    }

    /* Build the queries */
    q = qof_query_create_for (type);
    qof_query_set_book (q, book);

#if 0
    if (start)
    {
        q2 = qof_query_copy (q);
        qof_query_add_guid_match (
            q2, g_slist_prepend (NULL, QOF_PARAM_GUID),
            gncCoOwnerGetGUID (start), QOF_QUERY_AND);
    }
#endif

    /* launch select dialog and return the result */
    sw = g_new0 (struct _coowner_select_window, 1);
    sw->book = book;
    sw->q = q;

    return gnc_search_dialog_create (
        parent, type, _("Find Co-Owner"),
        params, columns, q, q2,
        buttons, NULL, new_coowner_cb,
        sw, free_coowner_cb,
        GNC_PREFS_GROUP_SEARCH, NULL,
        "gnc-class-coowners");
}

GNCSearchWindow *
gnc_coowner_search_select (GtkWindow *parent, gpointer start, gpointer book)
{
    if (!book) return NULL;

    return gnc_coowner_search (parent, start, book);
}

GNCSearchWindow *
gnc_coowner_search_edit (GtkWindow *parent, gpointer start, gpointer book)
{
    if (start)
        gnc_ui_coowner_edit (parent, start);

    return NULL;
}

static void
edit_coowner_cb (GtkWindow *dialog, gpointer *coowner_p, gpointer user_data)
{
    GncCoOwner *coowner;

    g_return_if_fail (coowner_p);
    coowner = *coowner_p;

    if (!coowner)
        return;

    gnc_ui_coowner_edit (dialog, coowner);

    return;
}

static gboolean
find_handler (gpointer find_data, gpointer user_data)
{
    const GncGUID *coowner_guid = find_data;
    CoOwnerWindow *ow = user_data;

    return(ow && guid_equal(&ow->coowner_guid, coowner_guid));
}

static void
free_coowner_cb (gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;

    g_return_if_fail (sw);

    qof_query_destroy (sw->q);
    g_free (sw);
}

static void
invoice_coowner_cb (GtkWindow *dialog, gpointer *coowner_p, gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;
    GncOwner owner;
    GncCoOwner *coowner;

    g_return_if_fail (coowner_p && user_data);

    coowner = *coowner_p;

    if (!coowner)
        return;

    gncOwnerInitCoOwner (&owner, coowner);
    gnc_invoice_search (dialog, NULL, &owner, sw->book);
    return;
}

static gpointer
new_coowner_cb (GtkWindow *dialog, gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;
    CoOwnerWindow *ow;

    g_return_val_if_fail (user_data, NULL);

    ow = gnc_ui_coowner_new (dialog, sw->book);
    return ow_get_coowner (ow);
}

static void
order_coowner_cb (GtkWindow *dialog, gpointer *coowner_p, gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;
    GncOwner owner;
    GncCoOwner *coowner;

    g_return_if_fail (coowner_p && user_data);

    coowner = *coowner_p;

    if (!coowner)
        return;

    gncOwnerInitCoOwner (&owner, coowner);
    gnc_order_search (dialog, NULL, &owner, sw->book);
    return;
}

static void
jobs_coowner_cb (GtkWindow *dialog, gpointer *coowner_p, gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;
    GncOwner owner;
    GncCoOwner *coowner;

    g_return_if_fail (coowner_p && user_data);

    coowner = *coowner_p;

    if (!coowner)
        return;

    gncOwnerInitCoOwner (&owner, coowner);
    gnc_job_search (dialog, NULL, &owner, sw->book);
    return;
}

static void
payment_coowner_cb (GtkWindow *dialog, gpointer *coowner_p, gpointer user_data)
{
    struct _coowner_select_window *sw = user_data;
    GncOwner owner;
    GncCoOwner *coowner;

    g_return_if_fail (coowner_p && user_data);

    coowner = *coowner_p;

    if (!coowner)
        return;

    gncOwnerInitCoOwner (&owner, coowner);
    gnc_ui_payment_new (dialog, &owner, sw->book);
    return;
}

static gboolean
idle_select_region_billaddr2(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_coowner_addr2),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_billaddr3(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_coowner_addr3),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_billaddr4(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_coowner_addr4),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);
    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_shipaddr2(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_ship_addr2),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_shipaddr3(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_ship_addr3),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_shipaddr4(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_ship_addr4),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_tenantaddr2(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_tenant_addr2),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_tenantaddr3(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_tenant_addr3),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

static gboolean
idle_select_region_tenantaddr4(gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    g_return_val_if_fail(user_data, FALSE);

    gtk_editable_select_region(GTK_EDITABLE(wdata->coowner_entry_tenant_addr4),
                               wdata->addrX_start_selection,
                               wdata->addrX_end_selection);

    wdata->addrX_selection_source_id = 0;
    return FALSE;
}

/* Implementation of the steps common to all address lines.
 * Return values:
 * TRUE  -> if anything was inserted by quickfill
 * FALSE -> nothing changed
 */
static gboolean
gnc_coowner_addr_common_insert_cb(GtkEditable *editable,
                                   gchar *new_text, gint new_text_length,
                                   gint *position, gpointer user_data, QuickFill *qf)
{
    CoOwnerWindow *wdata = user_data;
    gchar *concatenated_text;
    QuickFill *match;
    gint prefix_len, concatenated_text_len;

    if (new_text_length <= 0)
        return FALSE;

    g_warning("gnc_coowner_addr_common_insert_cb: handle address entities");

    {
        gchar *suffix = gtk_editable_get_chars(editable, *position, -1);
        /* If we are inserting in the middle, do nothing */
        if (*suffix)
        {
            g_free(suffix);
            return FALSE;
        }
        g_free(suffix);
    }

    {
        gchar *prefix = gtk_editable_get_chars(editable, 0, *position);
        prefix_len = strlen(prefix);
        concatenated_text = g_strconcat(prefix, new_text, (gchar*) NULL);
        concatenated_text_len = prefix_len + new_text_length;
        g_free(prefix);
    }

    match = gnc_quickfill_get_string_match(qf, concatenated_text);
    g_free(concatenated_text);
    if (match)
    {
        const char* match_str = gnc_quickfill_string(match);
        g_info("gnc_coowner_addr_common_insert_cb: match_str='%s'", match_str);
        if (match_str)
        {
            gint match_str_len = strlen(match_str);
            if (match_str_len > concatenated_text_len)
            {
                g_signal_handlers_block_matched (G_OBJECT (editable),
                                                 G_SIGNAL_MATCH_DATA, 0, 0,
                                                 NULL, NULL, user_data);

                gtk_editable_insert_text(editable,
                                         match_str + prefix_len,
                                         match_str_len - prefix_len,
                                         position);

                g_signal_handlers_unblock_matched (G_OBJECT (editable),
                                                   G_SIGNAL_MATCH_DATA, 0, 0,
                                                   NULL, NULL, user_data);

                /* stop the current insert */
                g_signal_stop_emission_by_name (G_OBJECT (editable), "insert_text");

                /* set the position */
                *position = concatenated_text_len;

                /* select region on idle, because it would be reset once this function
                   finishes */
                wdata->addrX_start_selection = *position;
                wdata->addrX_end_selection = -1;

                return TRUE;
            }
        }
    }
    return FALSE;
}

void
gnc_coowner_billaddr2_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
        position, user_data, wdata->coowner_addr2_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        /* select region on idle, because it would be reset once this function
           finishes */
        wdata->addrX_selection_source_id = g_idle_add(idle_select_region_billaddr2,
                                           user_data);
    }
}

void
gnc_coowner_billaddr3_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->coowner_addr3_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(idle_select_region_billaddr3,
            user_data);
    }
}

void
gnc_coowner_billaddr4_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->coowner_addr4_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(idle_select_region_billaddr4,
            user_data);
    }
}

static gboolean
gnc_coowner_common_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data, GtkWidget* editable )
{
    gboolean done_with_input = FALSE;

    /* Most "special" keys are allowed to be handled directly by
     * the entry's key press handler, but in some cases that doesn't
     * seem to work right, so handle them here.
     */
    switch ( event->keyval )
    {
    case GDK_KEY_Tab:
    case GDK_KEY_ISO_Left_Tab:
        /* Complete on Tab, but not Shift-Tab */
        if ( !( event->state & GDK_SHIFT_MASK) )
        {
            /* NOT done with input, though, since we need to focus to the next
             * field.  Unselect the current field, though.
             */
            gtk_editable_select_region( GTK_EDITABLE(editable),
                                        0, 0 );
        }
        break;
    }

    return( done_with_input );
}

gboolean
gnc_coowner_billaddr2_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
         wdata->coowner_entry_coowner_addr2);
}

gboolean
gnc_coowner_billaddr3_key_press_cb( GtkEntry *entry,
                                GdkEventKey *event,
                                gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_coowner_addr3);
}

gboolean
gnc_coowner_billaddr4_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_coowner_addr4);
}

void
gnc_coowner_shipaddr2_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->ship_addr2_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_shipaddr2,
            user_data);
    }
}

void
gnc_coowner_shipaddr3_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->ship_addr3_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_shipaddr3,
            user_data);
    }
}

void
gnc_coowner_shipaddr4_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->ship_addr4_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_shipaddr4,
            user_data);
    }
}

void
gnc_coowner_tenantaddr2_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->tenant_addr2_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_tenantaddr2,
            user_data);
    }
}

void
gnc_coowner_tenantaddr3_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->tenant_addr3_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_tenantaddr3,
            user_data);
    }
}

void
gnc_coowner_tenantaddr4_insert_cb(
    GtkEditable *editable,
    gchar *new_text, gint new_text_length,
    gint *position, gpointer user_data)
{
    CoOwnerWindow *wdata = user_data;
    gboolean r;

    /* The handling common to all address lines is done in this other
     * function. */
    r = gnc_coowner_addr_common_insert_cb(editable, new_text, new_text_length,
            position, user_data, wdata->tenant_addr4_quickfill);

    /* Did we insert something? Then set up the correct idle handler */
    if (r)
    {
        wdata->addrX_selection_source_id = g_idle_add(
            idle_select_region_tenantaddr4,
            user_data);
    }
}

gboolean
gnc_coowner_shipaddr2_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
         wdata->coowner_entry_ship_addr2);
}

gboolean
gnc_coowner_shipaddr3_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_ship_addr3);
}

gboolean
gnc_coowner_shipaddr4_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_ship_addr4);
}

gboolean
gnc_coowner_tenantaddr2_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
         wdata->coowner_entry_tenant_addr2);
}

gboolean
gnc_coowner_tenantaddr3_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_tenant_addr3);
}

gboolean
gnc_coowner_tenantaddr4_key_press_cb(
    GtkEntry *entry,
    GdkEventKey *event,
    gpointer user_data )
{
    CoOwnerWindow *wdata = user_data;
    return gnc_coowner_common_key_press_cb(entry, event, user_data,
        wdata->coowner_entry_tenant_addr4);
}
