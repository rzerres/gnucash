/*
 * dialog-job.c -- Dialog for Job entry
 * Copyright (C) 2001, 2002 Derek Atkins
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "dialog-utils.h"
#include "gnc-amount-edit.h"
#include "gnc-component-manager.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "qof.h"
#include "dialog-search.h"
#include "search-param.h"

#include "gncBusiness.h"
#include "gncJob.h"
#include "gncJobP.h"

#include "business-gnome-utils.h"
#include "dialog-invoice.h"
#include "dialog-job.h"
#include "dialog-payment.h"

#define DIALOG_NEW_JOB_CM_CLASS "dialog-new-job"
#define DIALOG_EDIT_JOB_CM_CLASS "dialog-edit-job"

#define GNC_PREFS_GROUP_SEARCH "dialogs.business.job-search"

void gnc_job_window_ok_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_cancel_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_help_cb (GtkWidget *widget, gpointer data);
void gnc_job_window_destroy_cb (GtkWidget *widget, gpointer data);
void gnc_job_name_changed_cb (GtkWidget *widget, gpointer data);
void gnc_job_type_toggled_cb (GtkWidget *widget, gpointer data);

typedef enum
{
    DUP_JOB,
    EDIT_JOB,
    MOD_JOB,
    NEW_JOB,
    VIEW_JOB,
} JobDialogType;

struct _job_select_window
{
    QofBook  *book;
    GncOwner *owner;
    QofQuery *q;
    GncOwner  owner_def;
};

struct _job_window
{
    GtkWidget *radiobutton_active;
    GtkWidget *desc_entry;
    GtkWidget *dialog;
    GtkWidget *entry_billing_id;
    GtkWidget *entry_id;
    GtkWidget *entry_name;
    GtkWidget *entry_owner;
    GtkWidget *entry_rate;
    GtkWidget *owner_choice;

    QofBook *book;
    gint component_id;
    JobDialogType dialog_type;
    GncGUID job_guid;
    gboolean job_type_is_coowner;
    GncOwner owner;
    GncJob *created_job;


};

/*******************************************************************************/
/* JOB WINDOW */

static GncJob *
jw_get_job (JobWindow *jw)
{
    if (!jw)
        return NULL;

    return gncJobLookup (jw->book, &jw->job_guid);
}

static void gnc_ui_to_job (JobWindow *jw, GncJob *job)
{
    gnc_suspend_gui_refresh ();
    gncJobBeginEdit (job);

    qof_event_gen(QOF_INSTANCE(job), QOF_EVENT_ADD, NULL);

    if (jw->radiobutton_active)
        gncJobSetActive (job, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
            (jw->radiobutton_active)));

    /* Only set these values for NEW/MOD job types */
    if (jw->dialog_type != EDIT_JOB)
    {
        const GncOwner *old = gncJobGetOwner (job);
        gnc_owner_get_owner (jw->owner_choice, &(jw->owner));
        if (! gncOwnerEqual (old, &(jw->owner)))
            gncJobSetOwner (job, &(jw->owner));
        gncJobSetID (job, gtk_editable_get_chars (GTK_EDITABLE (jw->entry_id),
                                                  0, -1));
        gncJobSetName (job, gtk_editable_get_chars (GTK_EDITABLE (jw->entry_name),
                                                    0, -1));
        gncJobSetReference (job, gtk_editable_get_chars
                            (GTK_EDITABLE (jw->desc_entry), 0, -1));
        gncJobSetRate (job, gnc_amount_edit_get_amount
                       (GNC_AMOUNT_EDIT (jw->entry_rate)));
    }

    gncJobCommitEdit (job);
    gnc_resume_gui_refresh ();
}

static gboolean
gnc_job_verify_ok (JobWindow *jw)
{
    const char *res;
    gchar *string;

    /* Set a valid id if one was not created */
    res = gtk_entry_get_text (GTK_ENTRY (jw->entry_id));
    if (g_strcmp0 (res, "") == 0)
    {
        string = gncJobNextID(jw->book, &(jw->owner));
        gtk_entry_set_text (GTK_ENTRY (jw->entry_id), string);
        g_free(string);
    }

    /* Check for valid name */
    res = gtk_entry_get_text (GTK_ENTRY (jw->entry_name));
    if (g_strcmp0 (res, "") == 0)
    {
        const char *message = _("The Job must be given a name.");
        gnc_error_dialog (GTK_WINDOW (jw->dialog), "%s", message);
        return FALSE;
    }

    /* Check for dialog selected owner type  */
    /* if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) */
    /* { */
    /*     GSLIST *group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget)); */
    /*     g_print ("Index = %i%\n", g_slist_index (group, widget)); */
    /* } */

    gnc_owner_get_owner (jw->owner_choice, &(jw->owner));
    res = gncOwnerGetName (&(jw->owner));
    if (res == NULL || g_strcmp0 (res, "") == 0)
    {
        const char *message = _("You must choose an owner name for this job.");
        gnc_error_dialog (GTK_WINDOW (jw->dialog), "%s", message);
        return FALSE;
    }

    /* Check for valid rate */
    if (!gnc_amount_edit_evaluate (GNC_AMOUNT_EDIT(jw->entry_rate), NULL))
    {
        const char *message = _("The rate amount must be valid. You may leave it blank.");
        gnc_error_dialog (GTK_WINDOW (jw->dialog), "%s", message);
        return FALSE;
    }

    /* Set a valid id if one was not created */
    res = gtk_entry_get_text (GTK_ENTRY (jw->entry_id));
    if (g_strcmp0 (res, "") == 0)
    {
        string = gncJobNextID(jw->book, &(jw->owner));
        gtk_entry_set_text (GTK_ENTRY (jw->entry_id), string);
        g_free(string);
    }

    /* Now save it off */
    {
        GncJob *job = jw_get_job (jw);
        if (job)
        {
            gnc_ui_to_job (jw, job);
        }
    }

    /* Ok, it's been saved... Change to an editor.. */
    jw->dialog_type = EDIT_JOB;

    return TRUE;
}

void
gnc_job_window_ok_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;

    /* Make sure this is ok */
    if (!gnc_job_verify_ok (jw))
        return;

    /* Now save off the job so we can return it */
    jw->created_job = jw_get_job (jw);
    jw->job_guid = *guid_null ();

    gnc_close_gui_component (jw->component_id);
}

void
gnc_job_window_cancel_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;

    gnc_close_gui_component (jw->component_id);
}

void
gnc_job_window_help_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;
    gnc_gnome_help (GTK_WINDOW(jw->dialog), HF_HELP, HL_USAGE_JOB);
}


void
gnc_job_window_destroy_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;
    GncJob *job = jw_get_job (jw);

    gnc_suspend_gui_refresh ();

    if (jw->dialog_type == NEW_JOB && job != NULL)
    {
        gncJobBeginEdit (job);
        gncJobDestroy (job);
        jw->job_guid = *guid_null ();
    }

    gnc_unregister_gui_component (jw->component_id);
    gnc_resume_gui_refresh ();

    g_free (jw);
}

void
gnc_job_name_changed_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;
    char *fullname, *title;
    const char *name, *id;

    if (!jw)
        return;

    name = gtk_entry_get_text (GTK_ENTRY (jw->entry_name));
    if (!name || *name == '\0')
        name = _("<No name>");

    id = gtk_entry_get_text (GTK_ENTRY (jw->entry_id));

    fullname = g_strconcat (name, " (", id, ")", (char *)NULL);

    if (jw->dialog_type == EDIT_JOB)
        title = g_strconcat (_("Edit Job"), " - ", fullname, (char *)NULL);
    else
        title = g_strconcat (_("New Job"), " - ", fullname, (char *)NULL);

    gtk_window_set_title (GTK_WINDOW (jw->dialog), title);

    g_free (fullname);
    g_free (title);
}

void
gnc_job_type_toggled_cb (GtkWidget *widget, gpointer data)
{
    JobWindow *jw = data;

    if (!jw) return;
    jw->job_type_is_coowner = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
}

static void
gnc_job_window_close_handler (gpointer user_data)
{
    JobWindow *jw = user_data;

    gtk_widget_destroy (jw->dialog);
    /* jw is already freed at this point
    jw->dialog = NULL; */
}

static void
gnc_job_window_refresh_handler (GHashTable *changes, gpointer user_data)
{
    JobWindow *jw = user_data;
    const EventInfo *info;
    GncJob *job = jw_get_job (jw);

    /* If there isn't a job behind us, close down */
    if (!job)
    {
        gnc_close_gui_component (jw->component_id);
        return;
    }

    /* Next, close if this is a destroy event */
    if (changes)
    {
        info = gnc_gui_get_entity_events (changes, &jw->job_guid);
        if (info && (info->event_mask & QOF_EVENT_DESTROY))
        {
            gnc_close_gui_component (jw->component_id);
            return;
        }
   }
}

static gboolean
find_handler (gpointer find_data, gpointer user_data)
{
    const GncGUID *job_guid = find_data;
    JobWindow *jw = user_data;

    return(jw && guid_equal(&jw->job_guid, job_guid));
}

static JobWindow *
gnc_job_window_new (GtkWindow *parent, JobDialogType dialog_type, QofBook *bookp,
                    GncOwner *owner, GncJob *job)
{
    JobWindow *jw = NULL;
    GtkBuilder *builder = NULL;
    GtkWidget *amount_rate, *entry_billing_id, *entry_owner, *entry_rate, *label_owner,
      *radiobutton_active, *radiobutton_owner;

    //g_assert (dialog_type == NEW_JOB || dialog_type == MOD_JOB || dialog_type == DUP_JOB);

    /*
     * Find an existing window for this job.  If found, bring it to
     * the front.
     */
    if (job)
    {
        GncGUID job_guid;

        job_guid = *gncJobGetGUID (job);
        jw = gnc_find_first_gui_component (DIALOG_EDIT_JOB_CM_CLASS,
                                           find_handler, &job_guid);
        if (jw)
        {
            gtk_window_set_transient_for (GTK_WINDOW(jw->dialog), parent);
            gtk_window_present (GTK_WINDOW(jw->dialog));
            return(jw);
        }
    }

    /* No existing job window found.  Build a new one. */
    jw = g_new0 (JobWindow, 1);
    jw->book = bookp;
    jw->dialog_type = NEW_JOB;
    jw->job_guid = *gncJobGetGUID (job);
    jw->job_type_is_coowner = gncJobGetTypeIsCoOwner (job);

    /* The default GUI activates radiobutton "Customer", change if needed */
    if (dialog_type == DUP_JOB)
    {
        GtkWidget *radiobutton_owner = GTK_WIDGET (gtk_builder_get_object (builder, "radiobutton_coowner"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radiobutton_owner), gncJobGetTypeIsCoOwner (job));
    }

    /* Save for later usage */
    gncOwnerCopy (owner, &(jw->owner));

    /* Load the Glade File */
    builder = gtk_builder_new();
    gnc_builder_add_from_file (builder, "dialog-job.glade", "job_dialog");

    /* Find the dialog */
    jw->dialog = GTK_WIDGET(gtk_builder_get_object (builder, "job_dialog"));
    gtk_window_set_transient_for (GTK_WINDOW(jw->dialog), parent);

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(jw->dialog), "gnc-id-job");
    gnc_widget_style_context_add_class (GTK_WIDGET(jw->dialog), "gnc-class-jobs");

    /* Get entry points */
    jw->radiobutton_active = GTK_WIDGET(gtk_builder_get_object (builder, "radiobutton_active"));
    jw->entry_id  = GTK_WIDGET(gtk_builder_get_object (builder, "entry_job_id"));
    jw->entry_name = GTK_WIDGET(gtk_builder_get_object (builder, "entry_job_name"));
    jw->entry_billing_id = GTK_WIDGET(gtk_builder_get_object (builder, "entry_billing_id"));

    label_owner = GTK_WIDGET(gtk_builder_get_object (builder, "label_owner"));
    entry_owner = GTK_WIDGET(gtk_builder_get_object (builder, "entry_owner"));
    radiobutton_owner = GTK_WIDGET(gtk_builder_get_object (builder, "radiobutton_customer"));

    amount_rate = gnc_amount_edit_new();
    gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (amount_rate), TRUE);

    jw->entry_rate = amount_rate;
    gtk_widget_show (amount_rate);

    entry_rate = GTK_WIDGET(gtk_builder_get_object (builder, "entry_rate"));
    gtk_box_pack_start (GTK_BOX (entry_rate), entry_rate, TRUE, TRUE, 0);

    jw->entry_billing_id = GTK_WIDGET(gtk_builder_get_object (builder, "entry_billing_id"));

    /* Setup signals */
    gtk_builder_connect_signals_full (builder, gnc_builder_connect_full_func, jw);

    /* Set initial entries */
    if (job != NULL)
    {
        jw->job_guid = *gncJobGetGUID (job);

        jw->dialog_type = NEW_JOB;
        jw->entry_owner = gnc_owner_edit_create (label_owner, entry_owner,
                                                 bookp, owner);

        gtk_entry_set_text (GTK_ENTRY (jw->entry_id), gncJobGetID (job));
        gtk_entry_set_text (GTK_ENTRY (jw->entry_name), gncJobGetName (job));
        gtk_entry_set_text (GTK_ENTRY (jw->entry_billing_id), gncJobGetReference (job));
        gnc_amount_edit_set_amount (GNC_AMOUNT_EDIT (jw->entry_rate),
                                      gncJobGetRate (job));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (jw->radiobutton_active),
                                      gncJobGetActive (job));

        jw->component_id = gnc_register_gui_component (DIALOG_EDIT_JOB_CM_CLASS,
                           gnc_job_window_refresh_handler,
                           gnc_job_window_close_handler,
                           jw);
    }
    else
    {
        job = gncJobCreate (bookp);
        jw->job_guid = *gncJobGetGUID (job);

        jw->dialog_type = NEW_JOB;

        /* If we are passed a real owner, don't allow the user to change it */
        if (owner->owner.undefined)
        {
            jw->entry_owner = gnc_owner_edit_create (label_owner, entry_owner,
                                                     bookp, owner);
        }
        else
        {
            jw->entry_owner = gnc_owner_select_create (label_owner, entry_owner,
                                                       bookp, owner);
        }

        jw->component_id = gnc_register_gui_component (DIALOG_NEW_JOB_CM_CLASS,
                           gnc_job_window_refresh_handler,
                           gnc_job_window_close_handler,
                           jw);
    }

    gnc_job_name_changed_cb (NULL, jw);
    gnc_gui_component_watch_entity_type (jw->component_id,
                                         GNC_JOB_MODULE_NAME,
                                         QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    gtk_widget_show_all (jw->dialog);

    // The job name should have keyboard focus
    gtk_widget_grab_focus(jw->entry_name);
    // Or should the owner field have focus?
    //    if (GNC_IS_GENERAL_SEARCH(jw->entry_owner))
    //    {
    //        gnc_general_search_grab_focus(GNC_GENERAL_SEARCH(jw->entry_owner));
    //    }

    g_object_unref(G_OBJECT(builder));

    return jw;
}


JobWindow *
gnc_ui_job_edit (GtkWindow *parent, GncOwner *ownerp, GncJob *job)
{
    JobDialogType dialog_type;
    JobWindow *jw;
    GncOwner owner;

    if (!job) return NULL;
    if (!ownerp) return NULL;

    dialog_type = EDIT_JOB;
    gncOwnerCopy (ownerp, &owner);

    jw = gnc_job_window_new (parent, dialog_type, gncJobGetBook(job),
                             &owner, job);
    return jw;
}

JobWindow *
gnc_ui_job_duplicate (GtkWindow *parent, GncJob *old_job, gboolean open_properties, const GDate *new_date)
{
    JobWindow *jw = NULL;
    GncJob *new_job = NULL;
    GncOwner *owner = NULL;
    time64 entry_date;

    g_assert(old_job);

    // Create a deep copy of the old job
    new_job = gncJobCopy(old_job);

    // The new job is set active
    gncJobSetActive(new_job, TRUE);

    // Unset the job ID, let it get allocated later
    gncJobSetID(new_job, "");

    if (open_properties)
    {
        // Open the "properties" pop-up for the job...
        jw = gnc_job_window_new (parent, DUP_JOB, NULL, NULL, new_job);
    }
    else
    {
        // Open the newly created job in the "edit" window
        jw = gnc_ui_job_edit (parent, owner, new_job);
        // Check the ID; set one if necessary
        if (g_strcmp0 (gtk_entry_get_text (GTK_ENTRY (jw->entry_id)), "") == 0)
        {
            gncJobSetID (new_job, gncJobNextID(jw->book, &(jw->owner)));
        }
    }
    return jw;
}

JobWindow *
gnc_ui_job_new (GtkWindow *parent, GncOwner *ownerp, QofBook *bookp)
{
    JobDialogType dialog_type;
    JobWindow *jw;
    GncOwner owner;

    dialog_type = EDIT_JOB;

    /* Make sure required options exist */
    if (!bookp) return NULL;

    if (ownerp)
    {
        g_return_val_if_fail ((gncOwnerGetType (ownerp) == GNC_OWNER_COOWNER) ||
                              (gncOwnerGetType (ownerp) == GNC_OWNER_CUSTOMER) ||
                              (gncOwnerGetType (ownerp) == GNC_OWNER_VENDOR),
                              NULL);
        gncOwnerCopy (ownerp, &owner);
    }
    else
    {
      /* Customer or CoOwner */
      if (gncOwnerGetType (ownerp) == GNC_OWNER_COOWNER)
          gncOwnerInitCoOwner (&owner, NULL);
      else
          gncOwnerInitCustomer (&owner, NULL);
      }

    jw = gnc_job_window_new (parent, dialog_type, bookp, &owner, NULL);
    return jw;
}

static JobWindow *
gnc_ui_job_modify (GtkWindow *parent, GncJob *job)
{
    JobWindow *jw;
    if (!job) return NULL;

    jw = gnc_job_window_new (parent, MOD_JOB, NULL, NULL, job);
    return jw;
}

/* Search functionality */

static void
edit_job_cb (GtkWindow *dialog, gpointer *job_p, gpointer user_data)
{
    GncOwner owner;
    GncJob *job;

    g_return_if_fail (job_p && user_data);

    job = *job_p;

    if (!job)
        return;

    gnc_ui_job_edit (dialog, &owner, job);
}

static void
invoice_job_cb (GtkWindow *dialog, gpointer *job_p, gpointer user_data)
{
    struct _job_select_window * sw = user_data;
    GncJob *job;
    GncOwner owner;

    g_return_if_fail (job_p && user_data);

    job = *job_p;
    if (!job)
        return;

    gncOwnerInitJob (&owner, job);
    gnc_invoice_search (dialog, NULL, &owner, sw->book);
}

static void
payment_job_cb (GtkWindow *dialog, gpointer *job_p, gpointer user_data)
{
    struct _job_select_window *sw = user_data;
    GncOwner owner;
    GncJob *job;

    g_return_if_fail (job_p && user_data);

    job = *job_p;

    if (!job)
        return;

    gncOwnerInitJob (&owner, job);
    gnc_ui_payment_new (dialog, &owner, sw->book);
    return;
}

static gpointer
new_job_cb (GtkWindow *dialog, gpointer user_data)
{
    struct _job_select_window *sw = user_data;
    JobWindow *jw;

    g_return_val_if_fail (user_data, NULL);

    jw = gnc_ui_job_new (dialog, sw->owner, sw->book);
    return jw_get_job (jw);
}

static void
free_userdata_cb (gpointer user_data)
{
    struct _job_select_window *sw = user_data;

    g_return_if_fail (sw);

    qof_query_destroy (sw->q);
    g_free (sw);
}

GNCSearchWindow *
gnc_job_search (GtkWindow *parent, GncJob *start, GncOwner *owner, QofBook *book)
{
    QofQuery *q, *q2 = NULL;
    QofIdType type = GNC_JOB_MODULE_NAME;
    struct _job_select_window *sw;
    static GList *params = NULL;
    static GList *columns = NULL;
    static GNCSearchCallbackButton buttons[] =
    {
        { N_("View/Edit Job"), edit_job_cb, NULL, TRUE},
        { N_("View Invoices"), invoice_job_cb, NULL, TRUE},
        { N_("Process Payment"), payment_job_cb, NULL, FALSE},
        { NULL },
    };

    g_return_val_if_fail (book, NULL);

    /* Build parameter list in reverse order */
    if (params == NULL)
    {
        params = gnc_search_param_prepend (params, _("Owner's Name"), NULL, type,
                                           JOB_OWNER, OWNER_NAME, NULL);
        params = gnc_search_param_prepend (params, _("Only Active?"), NULL, type,
                                           JOB_ACTIVE, NULL);
        params = gnc_search_param_prepend (params, _("Billing ID"), NULL, type,
                                           JOB_REFERENCE, NULL);
        params = gnc_search_param_prepend (params, _("Rate"), NULL, type,
                                           JOB_RATE, NULL);
        params = gnc_search_param_prepend (params, _("Job Number"), NULL, type,
                                           JOB_ID, NULL);
        params = gnc_search_param_prepend (params, _("Job Name"), NULL, type,
                                           JOB_NAME, NULL);
    }

    /* Build the column list in reverse order */
    if (columns == NULL)
    {
        columns = gnc_search_param_prepend (columns, _("Billing ID"), NULL, type,
                                            JOB_REFERENCE, NULL);
        columns = gnc_search_param_prepend (columns, _("Rate"), NULL, type,
                                            JOB_RATE, NULL);
        columns = gnc_search_param_prepend (columns, _("Company"), NULL, type,
                                            JOB_OWNER, OWNER_NAME, NULL);
        columns = gnc_search_param_prepend (columns, _("Job Name"), NULL, type,
                                            JOB_NAME, NULL);
        columns = gnc_search_param_prepend (columns, _("ID #"), NULL, type,
                                            JOB_ID, NULL);
    }

    /* Build the queries */
    q = qof_query_create_for (type);
    qof_query_set_book (q, book);

    /* If we have a start job but, for some reason, not an owner -- grab
     * the owner from the starting job.
     */
    /* if ((!owner || !gncOwnerGetGUID (owner)) && start) */
    /*     owner = gncJobGetOwner (start); */

    /* If owner is supplied, limit all searches to invoices who's owner
     * is the supplied owner!  Show all invoices by this owner.
     */
    if (owner && gncOwnerGetGUID (owner))
    {
        qof_query_add_guid_match (q, g_slist_prepend
                                  (g_slist_prepend (NULL, QOF_PARAM_GUID),
                                   JOB_OWNER),
                                  gncOwnerGetGUID (owner), QOF_QUERY_AND);

        q2 = qof_query_copy (q);
    }

#if 0
    if (start)
    {
        if (q2 == NULL)
            q2 = qof_query_copy (q);

        qof_query_add_guid_match (q2, g_slist_prepend (NULL, QOF_PARAM_GUID),
                                  gncJobGetGUID (start), QOF_QUERY_AND);
    }
#endif

    /* launch select dialog and return the result */
    sw = g_new0 (struct _job_select_window, 1);

    if (owner)
    {
        gncOwnerCopy (owner, &(sw->owner_def));
        sw->owner = &(sw->owner_def);
    }
    sw->book = book;
    sw->q = q;

    return gnc_search_dialog_create (parent, type, _("Find Job"),
                                     params, columns, q, q2, buttons, NULL,
                                     new_job_cb, sw, free_userdata_cb,
                                     GNC_PREFS_GROUP_SEARCH, NULL,
                                     "gnc-class-jobs");
}

/* Functions for widgets for job selection */

GNCSearchWindow *
gnc_job_search_select (GtkWindow *parent, gpointer start, gpointer book)
{
    GncJob *job = start;
    GncOwner owner;
    const GncOwner *ownerp = NULL;

    if (!book) return NULL;

    if (job)
    {
        ownerp = gncJobGetOwner (job);
        gncOwnerCopy (ownerp, &owner);
    }
    else
    {
      /* Customer or CoOwner */
      if (gncOwnerGetType (ownerp) == GNC_OWNER_COOWNER)
          gncOwnerInitCoOwner (&owner, NULL);
      else
          gncOwnerInitCustomer (&owner, NULL);
    }

    return gnc_job_search (parent, start, &owner, book);
}

GNCSearchWindow *
gnc_job_search_edit (GtkWindow *parent, gpointer start, gpointer book)
{
    GncOwner owner;

    if (start)
      gnc_ui_job_edit (parent, &owner, start);

    return NULL;
}
