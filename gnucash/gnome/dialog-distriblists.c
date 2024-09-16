/*
 * dialog-distriblists.C -- Dialog to handle distribution lists
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "dialog-search.h"
#include "dialog-utils.h"
//#include "search-owner.h"
#include "search-param.h"
#include "gnc-component-manager.h"
#include "gncOwner.h"
#include "gnc-session.h"
#include "gnc-tree-view-owner.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "gnc-tree-view-account.h"
#include "gnc-ui-util.h"
#include "qof.h"

#include "gncDistributionList.h"
#include "dialog-distriblists.h"

/* This static indicates the debugging module that this .o belongs to.  */
static QofLogModule log_module = GNC_MOD_GUI;

#define DIALOG_DISTRIBLISTS_CM_CLASS "distribution-lists-dialog"

typedef struct _distribution_list_owners DistributionListOwners;
typedef struct _distribution_list_revlookup_data DistributionListRevlookupData;


/*************************************************\
 * enumerations and type definitions
\*************************************************/

enum distriblist_columns
{
    DISTRIBUTION_LIST_COLUMN_NAME = 0,
    DISTRIBUTION_LIST_COLUMN_LIST,
    NUM_DISTRIBUTION_LIST_COLUMNS
};

typedef struct _distribution_list_owners
{
    // Distriblists "owners" widgets
    GtkWidget *dialog;
    GtkWidget *owners;
    GtkWidget *parent;

    GtkWidget *label_description;
    GtkWidget *label_owners;
    GtkWidget *label_typename;
    GtkWidget *entry_typename;

    // Distriblist assigned "owners" accounts
    GtkWidget *acct_tree;
    GtkTreeView *account_view;
    GtkListStore *account_store;

    // Distriblist "owners"
    gboolean owner_new;
    //GncOwner *owner;
    GList *owner_account_types;
    GList *owner_list;
    const char *typename;
    GncDistributionListType type;
} DistributionListOwners;

typedef struct _distribution_list_notebook
{
    GtkWidget *dialog;
    GtkWidget *notebook;
    GtkWidget *parent;

    // Distriblist "notebook" widgets
    GtkWidget *button_edit;
    GtkWidget *buttonbox_percentage;
    GtkWidget *buttonbox_shares;
    GtkWidget *entry_settlement_percentage;
    GtkWidget *entry_settlement_shares;
    GtkLabel *label_owner;
    GtkLabel *label_percentage_owner_typename;
    GtkLabel *label_percentage_total;
    GtkLabel *label_settlement_percentage;
    GtkLabel *label_settlement_shares;
    GtkLabel *label_shares_owner_typename;
    GtkLabel *label_shares_total;
    //GtkWidget *owners;
    GtkLabel *owners_typename;
    GtkWidget *view_percentage_owner;
    GtkLabel *percentage_owner_typename;
    GtkWidget *percentage_total;
    GtkWidget *view_shares_owner;
    //GtkWidget *shares_owner_type;
    GtkLabel *shares_owner_typename;
    GtkWidget *shares_total;

    // Distriblist "notebook" entities
    DistributionListOwners owners;
    GncDistributionListType type;
} DistributionListNotebook;

struct _distribution_list_revlookup_data
{
    Account *gnc_accounts;
};

struct _distribution_lists_window
{
    // Distriblist "window" widgets
    GtkWidget *window;
    GtkWidget *label_definition;
    GtkWidget *label_description;
    GtkWidget *entry_description;
    GtkWidget *label_type;
    GtkWidget *entry_type;
    GtkWidget *vbox;
    GtkWidget *vbox_definition;
    GtkWidget *view_lists;

    // Distriblist "window" entities
    DistributionListNotebook notebook;
    DistributionListOwners owners;
    GncDistributionList *current_list;
    QofBook *book;
    gint component_id;
    QofSession *session;
};

typedef struct _new_distribution_list
{
    // Distriblists widgets
    GtkWidget *dialog;
    GtkWidget *entry_name;
    GtkWidget *entry_description;

    // Distriblists entities
    DistributionListsWindow *distriblists_window;
    DistributionListNotebook notebook;
    DistributionListOwners owners;
    GncDistributionList *this_distriblist;
} NewDistributionList;

/***********************************************************\
 * Declaration: Private function prototypes
 * We assume C11 semantics, where the definitions must
 * preceed its usage. Once declared, the function ordering
 * is independend from its actual function call.
\***********************************************************/

// helper function prototypes
static void distriblists_list_refresh (
    DistributionListsWindow *distriblists_window);
static void distriblists_window_refresh (
    DistributionListsWindow *distriblists_window);
static void distriblists_window_refresh_handler (
    GHashTable *changes,
    gpointer data);
static void distriblist_to_ui (
    GncDistributionList *distriblist,
    GtkWidget *desc,
    DistributionListNotebook *notebook,
    DistributionListOwners *owners);
static gboolean find_handler (
    gpointer find_data,
    gpointer data);
static void get_int (
    GtkWidget *widget, GncDistributionList *distriblist,
    gint (*func)(const GncDistributionList *));
static void get_numeric (
    GtkWidget *widget,
    GncDistributionListType *distriblist,
    gnc_numeric (*func)(const GncDistributionListType *));
static void distriblist_maybe_set_type (
    NewDistributionList *new_distriblist,
    GncDistributionListType type);
static GncDistributionList *new_notebook_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name);
static DistributionListOwners *new_owners_dialog (
    DistributionListsWindow *distriblists_window,
    DistributionListOwners *owners);
    //GncDistributionList *distriblist);
    //NewDistributionList *new_distriblist);
static void notebook_init
(
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data);
static void notebook_show (DistributionListNotebook *notebook);
static void owners_edit (DistributionListOwners *owners);
static void owners_init (
    DistributionListOwners *owners,
    gboolean read_only,
    gpointer user_data);
static void owners_maybe_set_type (
    DistributionListOwners *owners);
    //GncOwnerType owner_type);
static void owners_remove (DistributionListOwners *owners);
static GtkWidget *read_widget (GtkBuilder *builder, char *name, gboolean read_only);
static void distriblist_selection_activated (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window);
static void distriblist_selection_changed (
    GtkTreeSelection *selection,
    DistributionListsWindow *distriblists_window);
static void set_numeric (
    GtkWidget *widget,
    GncDistributionListType *distriblist,
    void (*func)(GncDistributionListType *, gnc_numeric));
static void set_int (
    GtkWidget *widget,
    GncDistributionList *distriblist,
    void (*func)(GncDistributionList *, gint));
static gboolean ui_to_distriblist (NewDistributionList *new_distriblist);
static gboolean verify_distriblist_ok (NewDistributionList *new_distriblist);


/*************************************************\
 * Public Prototypes
\*************************************************/

void distriblists_list_delete_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void distriblists_list_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void distriblists_list_new_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void distriblist_owners_selection_changed_cb(
    GtkTreeSelection *selection,
    DistributionListNotebook *notebook);
void distriblist_selection_activated_cb (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window);
void distriblists_type_changed_cb (
    GtkComboBox *combobox,
    gpointer  data);
static void distriblists_window_close_handler (gpointer data);
void distriblists_window_close_cb (
    GtkWidget *widget,
    gpointer data);
void distriblists_window_destroy_cb (
    GtkWidget *widget,
    gpointer data);
gboolean distriblists_window_key_press_cb (
    GtkWidget *widget, GdkEventKey *event,
    gpointer data);
static gboolean new_distriblist_ok_cb (NewDistributionList *new_distriblist);
void owners_assign_cb (
    GtkButton *button,
    DistributionListOwners *owners);
void owners_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window,
    NewDistributionList *new_distriblist);
void owners_ok_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void owners_remove_cb (
    GtkButton *button,
    DistributionListOwners *owners);
static void owner_tree_selection_changed_cb (
    GtkTreeSelection *selection,
    DistributionListsWindow *distriblists_window);
void owners_type_changed_cb (
    GtkComboBox *combobox,
    gpointer data);

/*************************************************\
 * Private Functions
\*************************************************/

/* helper functions */
static void
distriblists_list_refresh (
    DistributionListsWindow *distriblists_window)
{
    DistributionListNotebook *notebook;
    DistributionListOwners *owners;
    //char *label_type = _("Label type");
    char *entry_type;

    g_return_if_fail (distriblists_window);

    notebook = &distriblists_window->notebook;
    owners = &distriblists_window->owners;

    if (!distriblists_window->current_list)
    {
        gtk_widget_hide (distriblists_window->vbox_definition);
        return;
    }

    gtk_widget_show_all (distriblists_window->vbox_definition);

    // propagate structure values to ui
    distriblist_to_ui (
        distriblists_window->current_list,
        distriblists_window->entry_description,
        &distriblists_window->notebook,
        &distriblists_window->owners);

    switch (gncDistribListGetType (
        distriblists_window->current_list))
    {
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
        entry_type = _("Percentage");
        break;
    case GNC_DISTRIBLIST_TYPE_SHARES:
        entry_type = _("Shares");
        break;
    default:
        entry_type = _("Unknown");
        break;
    }

    gtk_label_set_text (
        GTK_LABEL(distriblists_window->entry_type), entry_type);

    gtk_entry_set_text (
        GTK_ENTRY (notebook->percentage_owner_typename),
        owners->typename);
    gtk_entry_set_text (
        GTK_ENTRY (notebook->shares_owner_typename),
        owners->typename);


    // show the notebook widgets (active page)
    notebook_show (&distriblists_window->notebook);

    /* gtk_entry_set_int ( */
    /*     GTK_ENTRY (notebook->owner_type), */
    /*     owners->owner_type); */
}

static void
distriblist_selection_activated (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window)
{
    new_notebook_dialog (
        distriblists_window, distriblists_window->current_list, NULL);
}

static void
distriblist_selection_changed (
    GtkTreeSelection *selection,
    DistributionListsWindow *distriblists_window)
{
    GncDistributionList *distriblist = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (distriblists_window);

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
        gtk_tree_model_get (
            model, &iter, DISTRIBUTION_LIST_COLUMN_LIST, &distriblist, -1);

    // If we've changed, then reset the distribution list
    if (GNC_IS_DISTRIBLIST(distriblist) && (distriblist != distriblists_window->current_list))
        distriblists_window->current_list = distriblist;

    // And force a refresh of the entries
    distriblists_list_refresh (distriblists_window);
}

static void
distriblists_window_refresh (
    DistributionListsWindow *distriblists_window)
{
    GList *list, *node;
    GncDistributionList *distriblist;
    GtkTreeView *view;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeSelection *selection;
    GtkTreeRowReference *reference = NULL;

    PWARN ("Refresh the window list\n");
    g_return_if_fail (distriblists_window);
    view = GTK_TREE_VIEW(distriblists_window->view_lists);
    store = GTK_LIST_STORE(gtk_tree_view_get_model (view));
    selection = gtk_tree_view_get_selection (view);

    // Clear the list
    g_return_if_fail (distriblists_window);    gtk_list_store_clear (store);
    gnc_gui_component_clear_watches (distriblists_window->component_id);

    // Add the items to the list
    list = gncDistribListGetLists (distriblists_window->book);

    // Handle the list view
    if (list == NULL)
    {
        // If there are no distribution lists, clear the view
        distriblists_window->current_list = NULL;
        distriblists_list_refresh (distriblists_window);
    }
    else
    {
        // reverse the list of given elements
        // -> the loop will prepend them as needed
        list = g_list_reverse (g_list_copy (list));
    }

    for ( node = list; node; node = node->next)
    {
        distriblist = node->data;
        gnc_gui_component_watch_entity (
            distriblists_window->component_id,
            gncDistribListGetGUID (distriblist),
            QOF_EVENT_MODIFY);

        gtk_list_store_prepend (store, &iter);
        gtk_list_store_set (
            store,
            &iter,
            DISTRIBUTION_LIST_COLUMN_NAME,
            gncDistribListGetName (distriblist),
            DISTRIBUTION_LIST_COLUMN_LIST,
            distriblist,
            -1);
        if (distriblist ==
            distriblists_window->current_list)
        {
            path = gtk_tree_model_get_path (GTK_TREE_MODEL(store), &iter);
            reference = gtk_tree_row_reference_new (
                GTK_TREE_MODEL(store), path);
            gtk_tree_path_free (path);
        }
    }

    g_list_free (list);

    gnc_gui_component_watch_entity_type (
        distriblists_window->component_id,
        GNC_DISTRIBLIST_MODULE_NAME,
        QOF_EVENT_CREATE | QOF_EVENT_DESTROY);

    if (reference)
    {
        path = gtk_tree_row_reference_get_path (reference);
        gtk_tree_row_reference_free (reference);
        if (path)
        {
            gtk_tree_selection_select_path (selection, path);
            gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 0.5, 0.0);
            gtk_tree_path_free (path);
        }
    }
    else
    {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
            gtk_tree_selection_select_iter (selection, &iter);
    }
    PWARN ("Refresh the window list done\n");
}

static void
distriblists_window_refresh_handler (
    GHashTable *changes,
    gpointer data)
{
    DistributionListsWindow *distriblists_window = data;

    g_return_if_fail (data);
    distriblists_window_refresh (distriblists_window);
}

static void
distriblist_to_ui (
    GncDistributionList *distriblist,
    GtkWidget *description,
    DistributionListNotebook *notebook,
    DistributionListOwners *owners)
{
    /*
     *  Create the notebook UI
     */
    // Get the list description
    gtk_entry_set_text (
        GTK_ENTRY(description), gncDistribListGetDescription (distriblist));

    // Get notbook attributes
    notebook->type = gncDistribListGetType (distriblist);

    // Get owners attributes
    owners->typename = gncDistribListGetOwnerTypeName (distriblist);

    // Set widgets
    switch (notebook->type)
    {
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
    {
        // Set the settlement label
        gtk_entry_set_text (
            GTK_ENTRY (notebook->entry_settlement_percentage),
            gncDistribListGetPercentageLabelSettlement (distriblist));
        PWARN("Label Settlement percentage: '%s'\n", gncDistribListGetPercentageLabelSettlement (distriblist));

        // Set the total percentage value representing available shares
        get_int (
            notebook->percentage_total,
            distriblist,
            gncDistribListGetPercentageTotal);
        //PWARN("Percentage total: '%d'\n", gtk_spin_button_get_value(notebook->percentage_total));

        // Set the owner typename
        gtk_label_set_label (
            GTK_LABEL (notebook->percentage_owner_typename),
            owners->typename);
        //PWARN("Label Owner typename percentage: '%s'\n", gncDistribListGetOwnerTypeName (distriblist));
        break;
    }
    case GNC_DISTRIBLIST_TYPE_SHARES:
    {
        // Set the settlement label
        gtk_entry_set_text (
            GTK_ENTRY (notebook->entry_settlement_shares),
            gncDistribListGetSharesLabelSettlement (distriblist));
        PWARN("Label Settlement shares: '%s'\n", gncDistribListGetSharesLabelSettlement (distriblist));

        // Set the amount of total shares
        get_int (
            notebook->shares_total,
            distriblist,
            gncDistribListGetSharesTotal);
        //PWARN("Settlement total: '%d'\n", gtk_spin_button_get_value(notebook->shares_total));

        // Set the owner typename
        gtk_label_set_label (
            GTK_LABEL (notebook->shares_owner_typename),
            owners->typename);
        PWARN("Label Owner typename shares: '%s'\n", gncDistribListGetOwnerTypeName (distriblist));
        break;
    }
    }

    // Set the owner typename (e.g GNC_OWNER_COOWNER -> N_"Co-Owner")
    /* gtk_entry_set_text ( GTK_ENTRY ( */
    /*     owners->entry_typename), */
    /*     gncDistribListGetOwnerTypeName (distriblist)); */
    //PWARN ("Set owner typename: '%s'\n", owners->typename);

    /* owners->owner = gncOwnerNew(); */
    /* owners->owner->type = GNC_OWNER_COOWNER; */
    /* gncOwnerTypeToQofIdType(owners->owner->type); */
    /* gncDistribListSetOwner (distriblist, owners->owner); */

    // Set the owner
    /* owners->owner = gncDistribListGetOwner (distriblist); */

    // Set the owner typename (e.g GNC_OWNER_COOWNER -> N_"Co-Owner")
    /* owners->owner_typename = gncOwnerGetTypeString( */
    /*      gncDistribListGetOwner (distriblist)); */

    /* g_warning ("[distriblist_to_ui] Got owner type: '%s' -> typename: '%s'\n", */
    /*            gncOwnerTypeToQofIdType(gncOwnerGetType (owners->owner)), */
    /*            gncOwnerGetTypeString(gncDistribListGetOwner (distriblist))); */

    /* notebook->owner->type = GNC_OWNER_COOWNER; */
    /* g_warning ("[distriblist_to_ui] Hardcode owner type: '%s'\n", */
    /*            gncOwnerTypeToQofIdType(notebook->owner->type)); */

    /* gtk_entry_set_text ( GTK_ENTRY ( */
    /*     notebook->shares_owner_typename), */
    /*     gncDistribListGetOwnerTypeName (distriblist)); */

    /* gtk_entry_set_text ( GTK_ENTRY ( */
    /*     notebook->percentage_owner_typename), */
    /*     gncDistribListGetOwnerTypeName (distriblist)); */


//FIXME: assign list of saved owners
//FIXME: assign correct owner_type
    /* // Load the assigned "percentage" owner widgets (read-only) */
    /* box = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, */
    /*     "distriblists_notebook_treeview_percentage_owner")); */

    /* /\* Create the treeview  *\/ */
    /* // Can't be filled, since we didn't read in from backend yet !! */
    /* tree_view = gnc_tree_view_owner_new (notebook->owner_type); */
    /* renderer = gtk_cell_renderer_text_new (); */

    /* scrolled_window = gtk_scrolled_window_new (NULL, NULL); */
    /* gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), */
    /*                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC); */
    /* gtk_widget_show (scrolled_window); */
    /* gtk_box_pack_start (GTK_BOX (notebook->notebook), scrolled_window, */
    /*                     TRUE, TRUE, 0); */

    /* col = gnc_tree_view_find_column_by_name ( */
    /*           GNC_TREE_VIEW(tree_view), GNC_OWNER_TREE_NAME_COL); */
    /* gnc_tree_view_configure_columns (GNC_TREE_VIEW(tree_view)); */

    /* gtk_container_add (GTK_CONTAINER(box), notebook->owners_tree_percentage); */
    /* gtk_tree_view_set_headers_visible ( */
    /*     GTK_TREE_VIEW (notebook->owners_tree_percentage), FALSE); */

    /* /\* TODO: Maybe we like to assign a style *\/ */
    /* switch (notebook->owner_type) */
    /* { */
    /* case GNC_OWNER_NONE : */
    /* case GNC_OWNER_UNDEFINED : */
    /*     PWARN("missing owner_type"); */
    /*     label = _("Unknown"); */
    /*     style_label = "gnc-class-unknown"; */
    /*     break; */
    /* case GNC_OWNER_COOWNER : */
    /*     label = _("Co-Owner"); */
    /*     style_label = "gnc-class-coowners"; */
    /*     break; */
    /* case GNC_OWNER_CUSTOMER : */
    /*     //PWARN("not supported"); */
    /*     label = _("Customers"); */
    /*     style_label = "gnc-class-customers"; */
    /*     break; */
    /* case GNC_OWNER_EMPLOYEE : */
    /*     label = _("Employees"); */
    /*     style_label = "gnc-class-employees"; */
    /*     break; */
    /* case GNC_OWNER_JOB : */
    /*     //PWARN("not supported"); */
    /*     label = _("Jobs"); */
    /*     style_label = "gnc-class-jobs"; */
    /*     break; */
    /* case GNC_OWNER_VENDOR : */
    /*     PWARN("not supported"); */
    /*     label = _("Vendors"); */
    /*     style_label = "gnc-class-vendors"; */
    /*     break; */
    /* } */

    /* notebook->owners_view = tree_view; */
    /* selection = gtk_tree_view_get_selection(tree_view); */
    /* gtk_tree_view_set_headers_visible(tree_view, TRUE); */

    /* gtk_widget_show (GTK_WIDGET (tree_view)); */
    /* gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET(tree_view)); */

    /* g_signal_connect (G_OBJECT (selection), "changed", */
    /*                   G_CALLBACK ( */
    /*                       gnc_plugin_page_owner_tree_selection_changed_cb), */
    /*                   notebook->notebook); */
    /* g_signal_connect (G_OBJECT (tree_view), "button-press-event", */
    /*                   G_CALLBACK ( */
    /*                       gnc_plugin_page_owner_tree_button_press_cb), */
    /*                   notebook->notebook); */
    /* g_signal_connect (G_OBJECT (tree_view), "row-activated", */
    /*                   G_CALLBACK ( */
    /*                       gnc_plugin_page_owner_tree_double_click_cb), */
    /*                   notebook->notebook); */
    /* gnc_plugin_page_owner_tree_selection_changed_cb (NULL, notebook->notebook); */

    /* gnc_tree_view_owner_set_filter ( */
    /*     GNC_TREE_VIEW_OWNER(tree_view), */
}

static gboolean
find_handler (
    gpointer find_data,
    gpointer data)
{
    DistributionListsWindow *distriblists_window = data;
    QofBook *book = find_data;

    return (distriblists_window != NULL && distriblists_window->book == book);
}

static void
get_int (
    GtkWidget *widget,
    GncDistributionList *distriblist,
    gint (*func)(const GncDistributionList *))
{
    gint val;

    val = func (distriblist);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), (gfloat)val);
}

static void
get_numeric (
    GtkWidget *widget,
    GncDistributionListType *distriblist,
    gnc_numeric (*func)(const GncDistributionListType *))
{
    gnc_numeric val;
    gdouble fl;

    val = func (distriblist);
    fl = gnc_numeric_to_double (val);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), fl);
}

static void
distriblist_maybe_set_type (
    NewDistributionList *new_distriblist,
    GncDistributionListType type)
{
    // See if anything to do?
    if (type == new_distriblist->notebook.type)
        return;

    /* Let's refresh */
    new_distriblist->notebook.type = type;
    notebook_show (&new_distriblist->notebook);
}

static gboolean
new_distriblist_ok_cb (
    NewDistributionList *new_distriblist)
{
    DistributionListsWindow *distriblists_window;
    const char *name = NULL;
    char *message;

    g_return_val_if_fail (new_distriblist, FALSE);
    distriblists_window = new_distriblist->distriblists_window;

    // Verify that we've got real, valid data

    // verify the name
    if (new_distriblist->this_distriblist == NULL)
    {
        name = gtk_entry_get_text (GTK_ENTRY(new_distriblist->entry_name));
        if (name == NULL || *name == '\0')
        {
            message = _("You must provide a name for this distribution list.");
            gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", message);
            return FALSE;
        }
        if (gncDistribListLookupByName (distriblists_window->book, name))
        {
            message = g_strdup_printf (_(
                "You must provide a unique name for this distribution list. "
                "Your choice \"%s\" is already in use."),
                name);
            gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", message);
            g_free (message);
            return FALSE;
        }
    }

    // Verify the actual data
    if (!verify_distriblist_ok (new_distriblist))
        return FALSE;

    gnc_suspend_gui_refresh ();

    // All good: handle the validated entities
    if (new_distriblist->this_distriblist == NULL)
    {
        // New distribution list -> create new table entity
        new_distriblist->this_distriblist = gncDistribListCreate (
            distriblists_window->book);
        gncDistribListBeginEdit (new_distriblist->this_distriblist);
        gncDistribListSetName (new_distriblist->this_distriblist, name);

        // Reset the current distriblist
        distriblists_window->current_list =
            new_distriblist->this_distriblist;
    }
    else
    {
        // Update an existing distribution list -> update entity
        gncDistribListBeginEdit (distriblists_window->current_list);
    }

    // Update the other table entries
    if (ui_to_distriblist (new_distriblist))
        gncDistribListChanged (distriblists_window->current_list);

    // Mark the table as changed and commit it
    gncDistribListCommitEdit (distriblists_window->current_list);

    gnc_resume_gui_refresh ();
    return TRUE;
}

static GncDistributionList
*new_notebook_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name)
{
    GtkBuilder *builder;
    GtkWidget *box;
    GtkWidget *list_type;

    GncDistributionList *created_distriblist = NULL;
    NewDistributionList *new_distriblist;

    gint response;
    //gboolean read_only;
    gboolean done;
    const gchar *dialog_name;
    const gchar *dialog_description;
    const gchar *dialog_type;
    const gchar *dialog_notebook;

    if (!distriblists_window) return NULL;

    // Create a new distribution list (pointer to distriblist struct)
    new_distriblist = g_new0 (NewDistributionList, 1);

    // Assign the dialog window
    new_distriblist->distriblists_window = distriblists_window;

    // Assign a local copy of the given distribution list
    new_distriblist->this_distriblist = distriblist;

    // Assign needed Glade dialog entities
    if (distriblist == NULL)
    {
        dialog_name = "new_distriblists_dialog";
        dialog_description = "new_distriblists_entry_description";
        dialog_notebook = "new_distriblists_notebook_hbox";
        dialog_type = "new_distriblists_combobox_type";
	//read_only = FALSE;
    }
    else
    {
        dialog_name = "edit_distriblists_dialog";
        dialog_description = "edit_distriblists_entry description";
        dialog_notebook = "edit_distriblists_notebook_hbox";
        dialog_type = "edit_distriblists_combobox_type";
	//read_only = TRUE;
    }

    // Open and read the Glade file
    g_warning ("[new_distriblist_dialog] read in distriblist glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "liststore_type");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", dialog_name);

    // Assign the distriblists widgets
    g_warning ("[new_distriblist_dialog] assign distriblists ui widgets\n");
    new_distriblist->dialog = GTK_WIDGET(
        gtk_builder_get_object (builder, dialog_name));
    new_distriblist->entry_name = GTK_WIDGET(
        gtk_builder_get_object (builder, "new_distriblists_entry_name"));
    new_distriblist->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, dialog_description));

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        new_distriblist->dialog), "gnc-id-new-distribution-lists");
    gnc_widget_style_context_add_class (
        GTK_WIDGET(new_distriblist->dialog), "gnc-class-distribution-lists");

    if (name)
        gtk_entry_set_text (GTK_ENTRY(new_distriblist->entry_name), name);

    // Attach distriblist attributes
    new_distriblist->entry_description = read_widget (
        builder, "distriblists_entry_description", FALSE);

    // Fill in the widgets appropriately
    if (distriblist)
    {
        // Set backend stored values
        g_warning ("[new_notebook_dialog] get backend values\n");
        distriblist_to_ui (
            distriblist,
            new_distriblist->entry_description,
            &new_distriblist->notebook,
            &new_distriblist->owners);
    } else
    {
        // Set reasonable defaults (GNC_OWNER_NONE | GNC_OWNER_COOWNER)
        g_warning ("[new_notebook_dialog] assign type defaults\n");
        new_distriblist->notebook.type = GNC_DISTRIBLIST_TYPE_SHARES;
        /* new_distriblist->owners.owner->type = GNC_OWNER_NONE; */
        new_distriblist->owners.typename = "Undefined";
    }

    // Initialize the notebook widgets (read_only)
    notebook_init (&new_distriblist->notebook, FALSE, new_distriblist);

    // Initialize the owners widgets (read_write)
    //owners_init (&new_distriblist->owners, TRUE, new_distriblist);

    // Attach the notebook (expanded and filled, no padding)
    box = GTK_WIDGET(gtk_builder_get_object (builder, dialog_notebook));
    gtk_box_pack_start (
        GTK_BOX(box), new_distriblist->notebook.notebook, TRUE, TRUE, 0);

    // Decrease the reference pointer to the notebook object
    g_object_unref (new_distriblist->notebook.notebook);

    // Create the active list (menu as combobox)
    list_type = GTK_WIDGET(gtk_builder_get_object (builder, dialog_type));
    gtk_combo_box_set_active (
        GTK_COMBO_BOX(list_type),
        new_distriblist->notebook.type - 1);

    // Show the associated notebook widgets (active page)
    notebook_show (&new_distriblist->notebook);

    // Setup signals
    gtk_builder_connect_signals_full (
        builder, gnc_builder_connect_full_func, new_distriblist);

    gtk_window_set_transient_for (
        GTK_WINDOW(new_distriblist->dialog),
        GTK_WINDOW(distriblists_window->window));

    // Show page with focus set to appropriate entity
    gtk_widget_show_all (new_distriblist->dialog);
    if (distriblist)
    {
        // Given list: focus the description entry
        gtk_widget_grab_focus (new_distriblist->entry_description);
    }
    else
    {
        // New list: focus the distribution list name
        gtk_widget_grab_focus (new_distriblist->entry_name);
    }

    done = FALSE;
    while (!done)
    {
        response = gtk_dialog_run (GTK_DIALOG(new_distriblist->dialog));
        switch (response)
        {
        case GTK_RESPONSE_OK:
            if (new_distriblist_ok_cb (new_distriblist))
            {
                created_distriblist =
                    new_distriblist->this_distriblist;
                done = TRUE;
            }
            break;
        default:
            done = TRUE;
            break;
        }
    }

    g_object_unref (G_OBJECT(builder));

    gtk_widget_destroy (new_distriblist->dialog);
    g_free (new_distriblist);

    return created_distriblist;
}

static DistributionListOwners
*new_owners_dialog (
    DistributionListsWindow *distriblists_window,
    DistributionListOwners *owners)
{
    GtkBuilder *builder;
    //GtkWidget *owners_typename;
    //GtkWidget *owners_treeview;
    //GtkWidget *parent;
    //GtkTreeSelection *selection;
    gboolean done;
    gint response;

    g_warning ("[new_owners_dialog] handle assignement and removal of owners.\n");

    // Initialize the owners widgets (read_write)
    //owners_init (owners, TRUE, distriblists_window);

    // Create the active list (menu as combobox)
    /* owners_typename = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, owners_combobox_type)); */
    /* gtk_combo_box_set_active ( */
    /*     GTK_COMBO_BOX(owners_typename), */
    /*     owners->entry_typename - 1); */

    // Show dialog and set focus
    gtk_widget_show_all (owners->dialog);

    // owners_combobox_type vs list_typename
    gtk_widget_grab_focus (owners->entry_typename);

    // Setup signals
    gtk_builder_connect_signals_full (
    builder, gnc_builder_connect_full_func, owners);

    gtk_window_set_transient_for (
        GTK_WINDOW(owners->dialog),
        GTK_WINDOW(distriblists_window->notebook.dialog));

    done = FALSE;
    while (!done)
    {
        response = gtk_dialog_run (GTK_DIALOG(owners->dialog));
        switch (response)
        {
        /* case GTK_RESPONSE_ASSIGN: */
        /*     g_warning ("[new_owners_dialog] apply owner assignement dialog.\n"); */
        /*     done = TRUE; */
        /*     break; */
        case GTK_RESPONSE_CLOSE:
            g_warning ("[new_owners_dialog] close selected owner from owner list.\n");
            done = TRUE;
            break;
        case GTK_RESPONSE_OK:
            g_warning ("[new_owners_dialog] ended with reponse ok.\n");
            done = TRUE;
            break;
        case GTK_RESPONSE_CANCEL:
            g_warning ("[new_owners_dialog] ended with reponse cancel.\n");
            done = TRUE;
            break;
        default:
            done = TRUE;
            break;
        }
    }

    //g_object_unref (G_OBJECT(builder));

    gtk_widget_destroy (owners->dialog);
    //g_free (owners);

    // persitent storage of the owners struct will be handled in nootbook dialog
    g_warning ("[new_owners_dialog] owners dialog done\n");

    return owners;
}

// NOTE: The caller needs to unref once they attach
static void
notebook_init (
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data)
{
    GtkBuilder *builder;
    //GtkWidget *owners_percentage;
    //GtkWidget *owners_shares;
    //GtkTreeViewColumn *col;
    GtkWidget *parent;
    //GtkCellRenderer *renderer;
    //GtkTreeSelection *selection;
    //GtkWidget *scrolled_window;
    //GtkTreeView *tree_view;

    //gchar* label = "";
    //GncOwner *owner;
    //const gchar *style_label = NULL;
    //PangoAttrList *pango_attributes_list;
    //gchar *entry_attributes = "";

    // Load the notebook from Glade file
    g_warning ("[notebook_init] read in distriblists_notebook glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_shares_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_percentage_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "ownerstore_type");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_notebook_window");

    // Assign the parent widgets
    g_warning ("[notebook_init] assign distriblists_notebook parent\n");
    parent = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook_window"));

    // Assign the notebook widgets
    g_warning ("[notebook_init] assign notebook ui widgets\n");
    notebook->notebook = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook"));

    // Set the name for this dialog
    // (Hint: allows easy manipulation via css)
    gtk_widget_set_name (GTK_WIDGET(
        notebook->notebook), "gnc-id-distribution-list");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        notebook->notebook), "gnc-class-distribution-lists");

    // Load the "percentage" widgets
    /* owners_percentage = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, */
    /*     "distriblists_notebook_scrolled_window_percentage")); */
    /* gtk_box_pack_start ( */
    /*     GTK_BOX (notebook->notebook), */
    /* 	owners_percentage, */
    /*     TRUE, */
    /* 	TRUE, */
    /* 	0); */

    notebook->label_settlement_percentage = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_label_settlement_percentage"));
    notebook->entry_settlement_percentage = read_widget (
        builder, "distriblists_notebook_entry_settlement_percentage", read_only);
    notebook->label_percentage_total = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_label_percentage_total"));
    notebook->percentage_total = GTK_WIDGET (gtk_builder_get_object (
        builder, "percentage:percentage_total"));
    notebook->percentage_total = read_widget (
	builder, "percentage:percentage_total", read_only);

    notebook->percentage_owner_typename = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_percentage_owner_typename"));
    /* notebook->label_percentage_total = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, "distriblists_notebook_label_percentage_total")); */

    /* notebook->label_percentage_owner_typename = GTK_WIDGET (gtk_builder_get_object ( */
    /*      builder, "distriblists_notebook_label_percentage_owner_typename")); */
    // Load the "shares" widgets
    /* owners_shares = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, */
    /*     "distriblists_notebook_scrolled_window_shares")); */
    /* gtk_box_pack_start ( */
    /*     GTK_BOX (notebook->notebook), */
    /* 	owners_shares, */
    /*     TRUE, */
    /* 	TRUE, */
    /* 	0); */

    notebook->label_settlement_shares = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_label_settlement_shares"));
    notebook->entry_settlement_shares = read_widget (
        builder, "distriblists_notebook_entry_settlement_shares", read_only);
    notebook->label_shares_total = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_label_shares_total"));
    notebook->shares_total = read_widget (
        builder, "shares:shares_total", read_only);

    notebook->shares_owner_typename = GTK_LABEL (gtk_builder_get_object (
        builder, "distriblists_notebook_shares_owner_typename"));
    /* notebook->shares_owner_typename = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, "distriblists_notebook_shares_owner_typename")); */
    /* notebook->label_shares_owner_typename = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, "distriblists_notebook_label_shares_owner_typename")); */

    /* notebook->view_percentage_owner = GTK_WIDGET ( */
    /*     gnc_tree_view_owner_new (notebook->owner->type)); */
    /* gtk_container_add (GTK_CONTAINER(box), notebook->view_percentage_owner); */
    /* gtk_tree_view_set_headers_visible ( */
    /*     GTK_TREE_VIEW(notebook->view_percentage_owner), FALSE); */

    /* // Load the assigned "shares" owner widgets (read-only) */
    /* box = GTK_WIDGET (gtk_builder_get_object ( */
    /*     builder, */
    /*     "distriblists_notebook_treeview_shares_owner")); */
    /* notebook->owners_tree_shares = GTK_WIDGET ( */
    /*     gnc_tree_view_owner_new (notebook->owner_type)); */
    /* gtk_container_add (GTK_CONTAINER(box), notebook->owners_tree_shares); */
    /* gtk_tree_view_set_headers_visible ( */
    /*     GTK_TREE_VIEW(notebook->owners_tree_shares), FALSE); */

    /* // Load notebook buttonbox widgets */
    /* notebook->buttonbox_percentage = read_widget ( */
    /*     builder, "distriblists_notebook_grid_percentage_owner", read_only); */

    /* notebook->buttonbox_shares = read_widget ( */
    /*     builder, "distriblists_notebook_grid_shares_owner", read_only); */

    // Set widgets inactive
    //gtk_widget_set_sensitive ( notebook->notebook, FALSE);

    // Disconnect the notebook from the window
    g_object_ref (notebook->notebook);
    gtk_container_remove (GTK_CONTAINER(parent), notebook->notebook);
    g_object_unref (G_OBJECT(builder));
    gtk_widget_destroy (parent);

    // NOTE: The caller needs to handle unref once they attach a notebook
}

static void
notebook_show (
    DistributionListNotebook *notebook)
{
    g_return_if_fail (notebook->type > 0);
    gtk_notebook_set_current_page (
        GTK_NOTEBOOK(notebook->notebook), notebook->type - 1);
}

static void
owners_assign (DistributionListOwners *owners)
{
    // TODO: handle new owner assignment
    g_warning ("[owner_assign] assing a new owner to the list\n");
}

static void
owners_edit (DistributionListOwners *owners)
{
    // TODO: handle new owner assignment
    g_warning ("[owner_assign] assing a new owner to the list\n");
}

static void
owners_init (
    DistributionListOwners *owners,
    gboolean read_only,
    gpointer user_data)
{
    //GncOwnerType owner_type;
    //GncOwner owner;
    //const gchar *dialog_type;

    GtkBuilder *builder;
    GtkWidget *list_type;
    GtkWidget *owners_combox_type;
    GtkWidget *owners_treeview;
    //GtkTreeSelection *selection;

    // Load the owners from Glade file
    g_warning ("[owners_init] read in distriblists owner glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "owners_dialog");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "ownerstore_type");

    // Assign the parent widgets
    g_warning ("[owners_init] assign distriblists owners parent\n");

    // Assign the owners widgets
    g_warning ("[owners_init] assign owners ui widgets\n");
    owners->dialog = GTK_WIDGET(
        gtk_builder_get_object (builder, "owners_dialog"));

    // Create the owner type list (menu as combobox)
    //owners->list_type = GTK_WIDGET(
    owners->entry_typename = GTK_WIDGET(
        gtk_builder_get_object (builder, "owners_combobox_type"));
    g_warning ("[owners_init] combobox_type done\n");

    // Load the view of assigned owners
    owners_treeview = GTK_WIDGET (gtk_builder_get_object (
        builder, "owners_treeview_owners"));
    g_warning ("[owners_init] owners_treeview done\n");

    /* gtk_combo_box_set_active ( */
    /*     GTK_COMBO_BOX(list_type), */
    /*     owners->owner->type - 1); */

    // Load owner typename (handled via notebook_init)
    // g_warning ("[owners_init] owner '%i'\n",
    //           owner->owner_type);

    /* g_warning ("[owners_init] owner typename '%s' ('%i')\n", */
    /*            gncOwnerTypeGetTypeString(notebook->owner_typename), */
    /*            gncOwnerGetType(notebook->owner) */
    /*            ); */

    // (Note: assigned owners must have same type: e.g Co-Owner)
    /* owners->typename = gncOwnerTypeGetTypeString( */
    /*      owners->owner->type); */
    /* g_warning ("[owners_init] typename done\n"); */

    /* owners->typename = gncOwnerGetTypeString( */
    /*      gncDistribListGetOwner (distriblist)); */
    /* owners->typename = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "owners_combobox_typename")); */

    // test: prepare an account list
    /* notebook->owners_tree_shares = GTK_WIDGET(gnc_tree_view_account_new (FALSE)); */
    /* gtk_container_add (GTK_CONTAINER(owners_view), notebook->owners_tree_shares); */

    // aim: prepare the list of owner objects
    /* notebook->owner_type = gncOwnerGetType ( */
    /*     gncDistribListGetOwner (distriblist)); */
    /* ownertype_name = gncOwnerGetTypeString ( */
    /*     gncDistribListGetOwner (distriblist)); */

    // case notebook->type
    // //owners->owner_tree_shares = GTK_WIDGET (
    //    gnc_tree_view_owner_new (owner_type));
    //gtk_container_add (GTK_CONTAINER(box), owners->owners_tree_shares);

    // Set the combobox with assigned owners (read_only)
    // gtk_tree_view_set_headers_visible (
    //     GTK_TREE_VIEW(owners->owner_tree), FALSE);

    /* g_signal_connect_swapped (owners->owners_dialog, */
    /*     "response", */
    /*     G_CALLBACK (gtk_widget_destroy), */
    /*     notebook->owners_dialog); */

    // Set the name for this dialog
    // (Hint: allows easy manipulation via css
    gtk_widget_set_name (GTK_WIDGET(
        owners->dialog), "gnc-id-owner-list-dialog");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        owners->dialog), "gnc-class-owner-list-dialog");

    // Set widgets inactive
    //gtk_widget_set_sensitive (owners_dialog, TRUE);

    // Disconnect the owner dialog from the parent window
    g_object_ref (owners->dialog);
    //gtk_container_remove (GTK_CONTAINER(parent), owners->dialog);
    g_object_unref (G_OBJECT(builder));
    //gtk_widget_destroy (parent);

    // NOTE: The caller needs to handle unref once they attach a owner dialog
}

static void
owners_maybe_set_type (
    DistributionListOwners *owners)
    //GncOwnerType owner_type)
{
    g_warning ("[owners_maybe_set_type] not implmentet yet.\n");

    // See if anything to do?
    /* if (owners->typename == owners->typename) */
    /*     return; */

    /* Let's refresh */
    /* owners->typename = typename; */
    /* owners_show (owners); */
}

static void
owners_remove (
    DistributionListOwners *owners)
{
    g_warning ("[owners_remove] remove existing owner from the list\n");
}

static gboolean
owners_select_ok_cb (
    DistributionListOwners *owners)
{
    const char *message;
    //GncOwnerType owner_type;

    // TEST: with Account
    Account *acc;

    g_return_val_if_fail (owners, FALSE);

    // load the owners widgets
    g_warning ("[owner_select_ok_cb] validate assigned owners.\n");

// FIXME
    // Test: verify the account
    acc = gnc_tree_view_account_get_selected_account (
        GNC_TREE_VIEW_ACCOUNT(owners->acct_tree));
    if (acc == NULL)
    {

        gnc_error_dialog (GTK_WINDOW(owners->dialog), "%s", message);
        return FALSE;
    }

    gnc_suspend_gui_refresh ();

    // TODO: handle assigned owners
    // All valid, now either change existing or add the new selection
    /* { */
    /*     if (notebook->owner_new) */
    /*     { */
    /*         owner = notebook->owner; */
    /*     } */
    /*     else */
    /*     { */
    /*         owner = gncDistribListCreate (); */
    /*         gncDisrtribListAddOwner (distriblist_window->current_table, owner); */
    /*         distriblist_window->current_entry = owner; */
    /*     } */

    /*     gncTaxTableEntrySetAccount (entry, acc); */
    /*     gncTaxTableEntrySetType (entry, ntt->type); */
    /*     gncTaxTableEntrySetAmount (entry, amount); */
    /* } */

    /* /\* Mark the table as changed and commit it *\/ */
    /* gncTaxTableChanged (ttw->current_table); */
    /* gncTaxTableCommitEdit (ttw->current_table); */

    gnc_resume_gui_refresh ();
    return TRUE;
}

static GtkWidget
*read_widget (GtkBuilder *builder, char *name, gboolean read_only)
{
    GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, name));
    if (read_only)
    {
        GtkAdjustment *adjust;
        gtk_editable_set_editable (GTK_EDITABLE(widget), FALSE);
        adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(widget));
        gtk_adjustment_set_step_increment (adjust, 0.0);
        gtk_adjustment_set_page_increment (adjust, 0.0);
        gtk_widget_set_can_focus (widget, FALSE);
    }

    return widget;
}

static void
set_numeric (
    GtkWidget *widget,
    GncDistributionListType *distriblist,
    void (*func)(GncDistributionListType *, gnc_numeric))
{
    gnc_numeric val;
    gdouble fl = 0.0;

    fl = gtk_spin_button_get_value (GTK_SPIN_BUTTON(widget));
    val = double_to_gnc_numeric (fl, 100000, GNC_HOW_RND_ROUND_HALF_UP);
    func (distriblist, val);
}

static void
set_int (
    GtkWidget *widget,
    GncDistributionList *distriblist,
    void (*func)(GncDistributionList *, gint))
{
    gint val;

    val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    func (distriblist, val);
}

// return TRUE if persitent storage of UI changes was successfully
static gboolean
ui_to_distriblist (NewDistributionList *new_distriblist)
{
    DistributionListNotebook *notebook;
    DistributionListOwners *owners;
    GncDistributionList *distriblist;
    const char *text;

    distriblist = new_distriblist->this_distriblist;
    notebook = &new_distriblist->notebook;
    owners = &new_distriblist->owners;

    // Set the description
    text = gtk_entry_get_text (GTK_ENTRY(new_distriblist->entry_description));
    if (text)
        gncDistribListSetDescription (distriblist, text);

    // Set the list type
    gncDistribListSetType (
        new_distriblist->this_distriblist,
        new_distriblist->notebook.type);

    // Set the notebook attributes
    switch (new_distriblist->notebook.type)
    {
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
        gncDistribListSetPercentageLabelSettlement (
            distriblist, gtk_editable_get_chars (
                GTK_EDITABLE (notebook->entry_settlement_percentage), 0, -1));
        set_int (
            notebook->percentage_total,
            distriblist,
            gncDistribListSetPercentageTotal);
        // FIXME: set a default owner-type (e.g: GNC_OWNER_NONE, GNC_OWNER_COOWNER)
        PWARN ("Set owner typename: '%s'\n", owners->typename);
        gtk_label_set_label(
            GTK_LABEL (notebook->percentage_owner_typename),
            owners->typename);
        gncDistribListSetOwnerTypeName (
            distriblist,
            owners->typename);

        /* PWARN ("Write owner: '%s' -> '%s'\n", */
        /*       owners->owner->type, */
        /*       gncOwnerTypeToQofIdType(owners->owner->type)); */
        /* gncDistribListSetOwner ( */
        /*     distriblist, */
        /*     owners->owner); */
        break;
    case GNC_DISTRIBLIST_TYPE_SHARES:
        gncDistribListSetSharesLabelSettlement (
            distriblist, gtk_editable_get_chars (
                GTK_EDITABLE (notebook->entry_settlement_shares), 0, -1));
        set_int (
            notebook->shares_total,
            distriblist,
            gncDistribListSetSharesTotal);
        gtk_label_set_label(
            GTK_LABEL (notebook->shares_owner_typename),
            owners->typename);
        break;
    }

    // Set the owner attributes
    gncDistribListSetOwnerTypeName (
        distriblist, gtk_editable_get_chars (
            GTK_EDITABLE (owners->entry_typename), 0, -1));

    return gncDistribListIsDirty (distriblist);
}

static gboolean
verify_distriblist_ok (
    NewDistributionList *new_distriblist)
{
    gboolean result;
    DistributionListNotebook *notebook;
    gint percentage_total;
    gint shares_total;
    char *msg_label = _("A default distribution key label has been preset.");
    char *msg_percentage = _("You must set the max percentage value representing all property shares.");
    char *msg_shares = _("You must set a max value representing all property shares.");

    notebook = &new_distriblist->notebook;
    result = TRUE;

    switch (new_distriblist->notebook.type)
    {
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
        // Preset a default label, if nothing has been captured
        if (gtk_entry_get_text_length(GTK_ENTRY(notebook->entry_settlement_percentage)) == 0)
        {
            // Translators: This is the label that should be presented in settlements.
            gtk_entry_set_text(GTK_ENTRY(notebook->entry_settlement_percentage),  _("Percentage"));
            gnc_warning_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_label);
        }

        // Set the "percentage_total" value
        percentage_total = gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON(notebook->percentage_total));
        if ((percentage_total)==0)
        {
              gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_percentage);
              result = FALSE;
        }

//TODO
        // Set the selected owners
        break;

    case GNC_DISTRIBLIST_TYPE_SHARES:
        // Preset a default label, if nothing has been captured
        if (gtk_entry_get_text_length(GTK_ENTRY(notebook->entry_settlement_shares)) == 0)
        {
            // Translators: This is the label that should be presented in settlements.
            gtk_entry_set_text(GTK_ENTRY(notebook->entry_settlement_shares),  _("Shares"));
            gnc_warning_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_label);
        }

        // Set the "shares_total" value
        shares_total = gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON(notebook->shares_total));

        if ((shares_total)==0)
        {
              gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_shares);
              result = FALSE;
        }

//TODO
        // Set the selected owners
        break;
    }

    return result;
}

void
distriblists_window_close_handler (gpointer data)
{
    DistributionListsWindow *distriblists_window = data;
    g_return_if_fail (distriblists_window);

    gtk_widget_destroy (distriblists_window->window);
}

/***********************************************************\
 * Public functions
\***********************************************************/

// Create a distriblists window
DistributionListsWindow *
gnc_ui_distriblists_window_new (
    GtkWindow *parent,
    QofBook *book)
{
    DistributionListsWindow *distriblists_window;
    DistributionListNotebook *notebook;

    GtkBuilder *builder;
    GtkWidget *notebook_vbox;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkListStore *store;
    GtkTreeSelection *selection;
    GtkTreeView *view;

    if (!book) return NULL;

    // Find an existing distriblist window.
    // If it already exist, bring it to the front.
    // If we have an actual owner, then set it in the window.
    distriblists_window = gnc_find_first_gui_component (
        DIALOG_DISTRIBLISTS_CM_CLASS, find_handler, book);
    if (distriblists_window)
    {
        // Given window: Present the values
        gtk_window_present (GTK_WINDOW(distriblists_window->window));
        return distriblists_window;
    }

    // No window yet: Create a new window
    distriblists_window = g_new0 (DistributionListsWindow, 1);
    distriblists_window->book = book;
    distriblists_window->session = gnc_get_current_session ();
    
    // Open and read the Glade file
    g_warning ("[gnc_ui_distriblists_window_new] "
               "read in distribution list glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_window");

    // Assign the gtk window
    g_warning ("[gnc_ui_distriblists_window_new] assign distribution list ui widgets\n");
    distriblists_window->window = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_window"));

    /* // Assign the gtk primary dialog (as vbox) */
    /* distriblists_window->vbox = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "distriblists_vbox")); */

    // Assign distribution lists tree frame
    distriblists_window->view_lists = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_treeview_lists"));

    // Assign distribution list definition frame */
    distriblists_window->vbox_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_definition"));

    // Assign distribution list attributes (read_only)
    /* distriblists_window->label_definition = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "distriblists_label_definition_distribution_list")); */
    /* distriblists_window->label_description = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "distriblists_label_description")); */
    distriblists_window->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_description"));
    gtk_editable_set_editable(
        GTK_EDITABLE(distriblists_window->entry_description), FALSE);
    gtk_widget_set_can_focus (distriblists_window->entry_description, FALSE);

    /* distriblists_window->label_type = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "distriblists_label_type")); */
    distriblists_window->entry_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_type"));

    // Set the name for this dialog
    // (Hint: allows easy manipulation via css)
    gtk_widget_set_name (GTK_WIDGET(
        distriblists_window->window), "gnc-id-distribution-lists");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        distriblists_window->window), "gnc-class-distribution-lists");

    // Dialog signals that should be handled via callbacks
    g_signal_connect (
        distriblists_window->window, "key_press_event",
        G_CALLBACK (
            distriblists_window_key_press_cb), distriblists_window);

    // Initialize the list view
    view = GTK_TREE_VIEW(distriblists_window->view_lists);
    store = gtk_list_store_new (
        NUM_DISTRIBUTION_LIST_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
    g_object_unref (store);

    // Render the list, ordered by column name
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store),
                                          DISTRIBUTION_LIST_COLUMN_NAME,
                                          GTK_SORT_ASCENDING);
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
             "",
             renderer,
             "text",
             DISTRIBUTION_LIST_COLUMN_NAME,
             NULL);
    g_object_set (G_OBJECT(column), "reorderable", TRUE, NULL);
    gtk_tree_view_append_column (view, column);
    gtk_tree_view_column_set_sort_column_id (column, DISTRIBUTION_LIST_COLUMN_NAME);

    // Define view signals that should be handled via callbacks
    g_signal_connect (
        view, "row-activated",
        G_CALLBACK(distriblist_selection_activated),
        distriblists_window);
    selection = gtk_tree_view_get_selection (view);
    g_signal_connect (
        selection, "changed",
        G_CALLBACK(distriblist_selection_changed),
        distriblists_window);

    // Initialize the notebook widgets (read_only)
    notebook_init (&distriblists_window->notebook, TRUE, distriblists_window);

    // Attach the notebook inside the distributionlist vbox
    notebook_vbox = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_notebook"));

    gtk_box_pack_start (
        GTK_BOX (notebook_vbox),
        distriblists_window->notebook.notebook,
        TRUE,
        TRUE,
        0);
    g_object_unref (distriblists_window->notebook.notebook);

    // Initialize the owners widgets (read_only)
    //owners_init (&distriblists_window->owners, TRUE, distriblists_window);

    // Setup signals
    gtk_builder_connect_signals_full (
        builder, gnc_builder_connect_full_func, distriblists_window);

    // Register with component manager
    distriblists_window->component_id =
        gnc_register_gui_component (
            DIALOG_DISTRIBLISTS_CM_CLASS,
            distriblists_window_refresh_handler,
            distriblists_window_close_handler,
            distriblists_window);

    // Connect the session
    gnc_gui_component_set_session (
        distriblists_window->component_id,
        distriblists_window->session);

    // Show the UI widgets
    gtk_widget_show_all (distriblists_window->window);

    // Refresh the UI widgets with updated entities
    distriblists_window_refresh (distriblists_window);

    // Decrement the reference counter
    g_object_unref (G_OBJECT(builder));

    return distriblists_window;
}

#if 0
// Create a new distribution list by name */
GncDistributionListType *
gnc_ui_distriblists_new_from_name (
    GtkWindow *parent,
    QofBook *book,
    const char *name)
{
    DistributionListsWindow *distriblists_window;

    if (!book) return NULL;

    distriblists_window =
        gnc_ui_distriblists_window_new (parent, book);
    if (!distriblists_window) return NULL;

    return new_notebook_dialog (
        distriblists_window, NULL, name);
}
#endif

/* Destroy a distriblists window */
/* DistribListsWindow * */
/* gnc_ui_distriblists_window_destroy ( */
/*     GtkWindow *parent, */
/*     QofBook *book) */
/* { */
/* } */

// Callback functions
void
distriblists_list_delete_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);

    if (!distriblists_window->current_list)
        return;

    if (gncDistribListGetRefcount (distriblists_window->current_list) > 0)
    {
        gnc_error_dialog (
            GTK_WINDOW(distriblists_window->window),
            _("Distriblist \"%s\" is in use. You cannot delete it."),
            gncDistribListGetName (distriblists_window->current_list));
        return;
     }

    if (gnc_verify_dialog (
            GTK_WINDOW(distriblists_window->window), FALSE,
            _("Are you sure you want to delete \"%s\"?"),
            gncDistribListGetName (distriblists_window->current_list)))
    {
        /* Ok, let's remove it */
        gnc_suspend_gui_refresh ();
        gncDistribListBeginEdit (distriblists_window->current_list);
        gncDistribListDestroy (distriblists_window->current_list);
        distriblists_window->current_list = NULL;
        gnc_resume_gui_refresh ();
    }
}

void
distriblists_list_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    new_notebook_dialog (
        distriblists_window, distriblists_window->current_list, NULL);
    gtk_widget_show (distriblists_window->vbox_definition);
}

void
distriblists_list_new_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    new_notebook_dialog (distriblists_window, NULL, NULL);
}

void
distriblists_type_changed_cb (
    GtkComboBox *combobox,
    gpointer data)
{
    NewDistributionList *new_distriblist = data;
    gint value;

    value = gtk_combo_box_get_active (combobox);
    distriblist_maybe_set_type (new_distriblist, value + 1);
}

void
distriblists_window_close_cb (
    GtkWidget *widget,
    gpointer data)
{
    DistributionListsWindow *distriblists_window = data;
    gnc_close_gui_component (distriblists_window->component_id);
}

void
distriblists_window_destroy_cb (
    GtkWidget *widget,
    gpointer data)
{
    DistributionListsWindow *distriblists_window = data;

    if (!distriblists_window) return;

    gnc_unregister_gui_component (distriblists_window->component_id);

    if (distriblists_window->window)
    {
        gtk_widget_destroy (distriblists_window->window);
    }
    g_free (distriblists_window);
}

gboolean
distriblists_window_key_press_cb (
    GtkWidget *widget,
    GdkEventKey *event,
    gpointer data)
{
    DistributionListsWindow *distriblists_window = data;

    if (event->keyval == GDK_KEY_Escape)
    {
        distriblists_window_close_handler (distriblists_window);
        return TRUE;
    }
    else
        return FALSE;
}

void
owners_assign_cb (
    GtkButton *button,
    DistributionListOwners *owners)
{
    // Assign new owner to the list
    g_warning ("[owners_assign_cb] assign new owner to owners list\n");

    g_return_if_fail (owners);

    owners_assign (owners);
}

void
owners_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window,
    NewDistributionList *new_distriblist)
{
    // only act if we can access the active new_distriblist
    g_return_if_fail (new_distriblist);
    /* if (!new_distriblist->owners) */
    /*     return; */

    // Assign or remove owners to the list
    //new_owners_dialog (distriblists_window, distriblists_window->current_list);
    g_warning ("[owners_edit_cb] edit owner assignment in new dialog\n");
    new_owners_dialog (distriblists_window, &new_distriblist->owners);
    gtk_widget_show (distriblists_window->vbox_definition);
}

void
owners_ok_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    //g_return_if_fail (owners);

    // Checks once the selection is ok
    g_warning ("[owners_ok_cb] validate assigned owners.\n");
}

void
owners_remove_cb (
    GtkButton *button,
    DistributionListOwners *owners)
{
    // Remove owners from the list
    g_warning ("[owners_remove_cb] remove owner from owners list\n");

    g_return_if_fail (owners);

    // Remove given owner from the list
    owners_remove (owners);
}

static void
owners_tree_selection_changed_cb (
    GtkTreeSelection *selection,
    DistributionListsWindow *distriblists_window)
{
    //GtkActionGroup *action_group;
    GtkTreeView *view;
    GncOwner *owner = NULL;
    gboolean sensitive;
    //gboolean is_readwrite = !qof_book_is_readonly(gnc_get_current_book());

    g_return_if_fail (distriblists_window);

    if (!selection)
    {
        sensitive = FALSE;
    }
    else
    {
        g_return_if_fail(GTK_IS_TREE_SELECTION(selection));
        view = gtk_tree_selection_get_tree_view (selection);
        owner = gnc_tree_view_owner_get_selected_owner (
            GNC_TREE_VIEW_OWNER(view));
        sensitive = (owner != NULL);
    }
}

void
owners_type_changed_cb (
    GtkComboBox *combobox,
    gpointer user_data)
{
    DistributionListOwners *owners = user_data;
    //gint value;

    //value = gtk_combo_box_get_active (combobox);
    owners_maybe_set_type (owners);
    /* owners_maybe_set_type (owners, value + 1); */
}
