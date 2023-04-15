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
#include "gnc-session.h"
#include "gnc-tree-view-owner.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "gnc-ui-util.h"
#include "qof.h"

#include "gncDistributionList.h"
#include "dialog-distriblists.h"

#define DIALOG_DISTRIBLISTS_CM_CLASS "distribution-lists-dialog"


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
    GtkWidget *owners_typename;

    // TODO
    GtkWidget *acct_tree;

    // Distriblist "owners" entities
    gboolean owner_new;
    GncOwner *owner;
    GList *owners_list;
    const char *owner_typename;
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
    GtkWidget *label_percentage_total;
    GtkWidget *label_settlement_percentage;
    GtkWidget *label_settlement_shares;
    GtkWidget *label_shares_total;
    GtkWidget *label_owner;
    GtkWidget *owner;
    GtkWidget *owner_tree;
    GtkWidget *parent;
    GtkWidget *percentage_total;
    GtkWidget *shares_total;
    GtkWidget *percentage_total;

    // "Distribution lists" widgets
    GtkWidget *percentage_owner_typename;
    GtkWidget *percentage_total;
    GtkWidget *shares_owner_type;
    GtkWidget *shares_owner_typename;
    GtkWidget *shares_total;
    GtkWidget *view_percentage_owner;
    GtkWidget *view_shares_owner;

    GncDistributionListType type;
} DistributionListNotebook;

struct _distribution_lists_window
{
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
static GncDistributionList *new_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name);
static gboolean new_distriblist_ok_cb (NewDistributionList *new_distriblist);
void notebook_owner_edit (
    DistributionListNotebook *notebook);
static void notebook_init (
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data);
static void notebook_show (DistributionListNotebook *notebook);
static void owners_edit (
    DistributionListOwners *owners,
    DistributionListNotebook *notebook);
static void owners_init (
    DistributionListOwners *owners,
    gboolean read_only,
    gpointer user_data);
static void owners_maybe_set_type (
    DistributionListOwners *owners,
    GncOwnerType owner_type);
static void owners_remove (
    DistributionListOwners *owners);
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

// callback function prototypes
/* callback prototypes */
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
void owners_assign_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void owners_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void owners_ok_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void owners_remove_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
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
    char *label_type = _("Label type");
    char *entry_type;

    g_return_if_fail (distriblists_window);

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
    case GNC_DISTRIBLIST_TYPE_SHARES:
        entry_type = _("Shares");
        break;
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
        entry_type = _("Percentage");
        break;
    default:
        entry_type = _("Unknown");
        break;
    }

    // show the notebook widgets (active page)
    notebook_show (&distriblists_window->notebook);
    gtk_label_set_text (
        GTK_LABEL(distriblists_window->entry_type), entry_type);

    /* gtk_entry_set_text ( */
    /*     GTK_ENTRY (notebook->percentage_owner_typename), */
    /*     notebook->owners_typename); */

    /* gtk_entry_set_text ( */
    /*     GTK_ENTRY (notebook->shares_owner_typename), */
    /*     notebook->owners_typename); */

    /* gtk_entry_set_int ( */
    /*     GTK_ENTRY (notebook->owner_type), */
    /*     notebook->owner_type); */
}

static void
distriblist_selection_activated (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window)
{
    new_dialog (
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

    g_return_if_fail (distriblists_window);
    view = GTK_TREE_VIEW(distriblists_window->view_lists);
    store = GTK_LIST_STORE(gtk_tree_view_get_model (view));
    selection = gtk_tree_view_get_selection (view);

    // Clear the list
    gtk_list_store_clear (store);
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
    // Description widget
    gtk_entry_set_text (
        GTK_ENTRY(description), gncDistribListGetDescription (distriblist));

    // FIXME: hardcoded for now
    /* owners->owner = gncOwnerNew(); */
    /* owners->owner->type = GNC_OWNER_COOWNER; */
    /* gncOwnerTypeToQofIdType(owners->owner->type); */
    /* gncDistribListSetOwner (distriblist, owners->owner); */

    // Set the owner type
    owners->owner = gncDistribListGetOwner (distriblist);
    g_warning ("[distriblist_to_ui] Got owner type: '%s'\n",
               gncOwnerTypeToQofIdType(gncOwnerGetType (owners->owner)));

    /* notebook->owner->type = GNC_OWNER_COOWNER; */
    /* g_warning ("[distriblist_to_ui] Hardcode owner type: '%s'\n", */
    /*            gncOwnerTypeToQofIdType(notebook->owner->type)); */

    // Set the owner typename (e.g GNC_OWNER_COOWNER -> N_"Co-Owner")
    owners->owner_typename = gncOwnerGetTypeString(
         gncDistribListGetOwner (distriblist));
    g_warning ("[distriblist_to_ui] Got owner typename: '%s'\n", owners->owner_typename);

    gtk_entry_set_text (
        GTK_ENTRY (notebook->percentage_owner_typename),
        owners->owner_typename);

    gtk_entry_set_text (
        GTK_ENTRY (notebook->shares_owner_typename),
        owners->owner_typename);

    // Distribution list type
    notebook->type = gncDistribListGetType (distriblist);

    switch (notebook->type)
    {
    case GNC_DISTRIBLIST_TYPE_SHARES:
        get_int (
            notebook->shares_total,
            distriblist,
            gncDistribListGetSharesTotal);
        break;
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
        get_int (
            notebook->percentage_total,
            distriblist,
            gncDistribListGetPercentageTotal);
        break;
    }
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

// Create a distriblists window
DistributionListsWindow *
gnc_ui_distriblists_window_new (
    GtkWindow *parent,
    QofBook *book)
{
    GtkBuilder *builder;
    GtkTreeViewColumn *column;
    DistributionListsWindow *distriblists_window;
    GtkCellRenderer *renderer;
    GtkListStore *store;
    GtkTreeSelection *selection;
    GtkTreeView *view;
    GtkWidget *widget;

    if (!book) return NULL;

    // Find an existing distriblist window.  If it already
    // exist, bring it to the front. If we have an actual owner, then
    // set it in the window.
    distriblists_window = gnc_find_first_gui_component (
      DIALOG_DISTRIBLISTS_CM_CLASS, find_handler, book);
    if (distriblists_window)
    {
        // got one -- present the given window
        gtk_window_present (GTK_WINDOW(distriblists_window->window));
        return distriblists_window;
    }

    // Didn't find one -- create a new window
    distriblists_window = g_new0 (DistributionListsWindow, 1);
    distriblists_window->book = book;
    distriblists_window->session = gnc_get_current_session ();

    // Open and read the Glade file
    g_warning ("[gnc_ui_distriblists_window_new] read in distribution list glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_window");

    // Assign the gtk window
    g_warning ("[gnc_ui_distriblists_window_new] assign distribution list ui widgets\n");
    distriblists_window->window = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_window"));

    // Assign the gtk box (the primary dialog)
    distriblists_window->vbox = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox"));

    // Assign distribution lists frame (as a treeview)
    distriblists_window->view_lists = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_treeview_lists"));

    // Assign distribution list definition frame
    distriblists_window->vbox_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_definition"));

    // Assign distribution list type
    distriblists_window->label_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_type"));
    distriblists_window->entry_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_type"));

    // Assign distribution list attributes
    distriblists_window->label_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_definition_distribution_list"));
    distriblists_window->label_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_description"));
    distriblists_window->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_description"));
    distriblists_window->entry_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_type"));

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        distriblists_window->window), "gnc-id-distribution-lists");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        distriblists_window->window), "gnc-class-distribution-lists");

    g_signal_connect (
        distriblists_window->window, "key_press_event",
        G_CALLBACK (
            distriblists_window_key_press_cb), distriblists_window);

    // Initialize the view
    view = GTK_TREE_VIEW(distriblists_window->view_lists);
    store = gtk_list_store_new (NUM_DISTRIBUTION_LIST_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
    g_object_unref (store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
             "",
             renderer,
             "text",
             DISTRIBUTION_LIST_COLUMN_NAME,
             NULL);
    gtk_tree_view_append_column (view, column);

    g_signal_connect (
        view, "row-activated",
        G_CALLBACK(distriblist_selection_activated),
        distriblists_window);
    selection = gtk_tree_view_get_selection (view);
    g_signal_connect (
        selection, "changed",
        G_CALLBACK(distriblist_selection_changed),
        distriblists_window);

    // Initialize the notebook widgets
    init_notebook_widgets (
        &distriblists_window->notebook, FALSE, distriblists_window);

    // Attach the notebook (list attributes)
    widget = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_nootbook"));
    gtk_box_pack_start (
        GTK_BOX(widget),
        distriblists_window->notebook.notebook,
        TRUE,
        TRUE,
        0);
    g_object_unref (distriblists_window->notebook.notebook);

    // Setup signals
    gtk_builder_connect_signals_full (
        builder, gnc_builder_connect_full_func, distriblists_window);

    // register with component manager
    distriblists_window->component_id =
        gnc_register_gui_component (
            DIALOG_DISTRIBLISTS_CM_CLASS,
            distriblists_window_refresh_handler,
            distriblists_window_close_handler,
            distriblists_window);

    gnc_gui_component_set_session (
        distriblists_window->component_id,
        distriblists_window->session);

    gtk_widget_show_all (distriblists_window->window);
    distriblists_window_refresh (distriblists_window);

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

    return new_distriblist_dialog (
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

// NOTE: The caller needs to unref once they attach
static void
init_notebook_widgets (
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data)
{
    GtkBuilder *builder;
    GtkWidget *parent;

    // Load the notebook from Glade file
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_shares_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_percentage_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_notebook_window");
    notebook->notebook = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook"));
    parent = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook_window"));

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        notebook->notebook), "gnc-id-distribution-list");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        notebook->notebook), "gnc-class-distribution-lists");

    // load the notebook widgets
    g_warning ("[gnc_ui_distriblists_window_new] assign notebook ui widgets\n");
    notebook->label_settlement = read_widget (
        builder, "distriblists_notebook_label_settlement", read_only);
    notebook->entry_settlement = read_widget (
        builder, "distriblists_notebook_entry_settlement", read_only);

    // load the "shares" widgets
    notebook->label_shares_total = read_widget (
        builder, "distriblists_notebook_label_shares_total", read_only);
    notebook->shares_total = read_widget (
        builder, "shares:shares_total", read_only);

    // load the "percentage" widgets
    notebook->label_percentage_total = read_widget (
        builder, "distriblists_notebook_label_percentage_total", read_only);
    notebook->percentage_total = read_widget (
        builder, "percentage:percentage_total", read_only);

    // Disconnect the notebook from the window
    g_object_ref (notebook->notebook);
    gtk_container_remove (GTK_CONTAINER(parent), notebook->notebook);
    g_object_unref (G_OBJECT(builder));
    gtk_widget_destroy (parent);

    // NOTE: The caller needs to handle unref once they attach a notebook
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

static GncDistributionList
*new_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name)
{
    GncDistributionList *created_distriblist = NULL;
    NewDistributionList *new_distriblist;
    GtkBuilder *builder;
    GtkWidget *box;
    GtkWidget *buttonbox_shares;
    GtkWidget *buttonbox_percentage;
    GtkWidget *list_type;

    GtkBuilder *builder;
    GncDistributionList *created_distriblist = NULL;
    NewDistributionList *new_distriblist;

    gint response;
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

    // assign needed Glade dialog entities
    if (distriblist == NULL)
    {
        dialog_name = "new_distriblists_dialog";
        dialog_description = "new_distriblists_entry_description";
        dialog_notebook = "new_distriblists_notebook_hbox";
        dialog_type = "new_distriblists_combobox_type";
    }
    else
    {
        dialog_name = "edit_distriblists_dialog";
        dialog_description = "edit_distriblists_entry description";
        dialog_notebook = "edit_distriblists_notebook_hbox";
        dialog_type = "edit_distriblists_combobox_type";
    }

    // Open and read the Glade file
    g_warning ("[new_dialog] read in distriblist glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder,
        "dialog-distriblists.glade",
        "liststore_type");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", dialog_name);

    // Assign the distriblists widgets
    g_warning ("[new_dialog] assign distriblists ui widgets\n");
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

    // Fill in the widgets appropriately
    if (distriblist)
    {
        // Set backend stored values
        g_warning ("[new_dialog] get backend values\n");
        distriblist_to_ui (
            distriblist,
            new_distriblist->entry_description,
            &new_distriblist->notebook,
            &new_distriblist->owners);
    } else
    {
        // Set reasonable defaults (GNC_OWNER_NONE | GNC_OWNER_COOWNER)
        g_warning ("[new_dialog] assign type defaults\n");
        new_distriblist->notebook.type = GNC_DISTRIBLIST_TYPE_SHARES;
        new_distriblist->owners.type = GNC_OWNER_NONE;
    }

    // Initialize the notebook widgets (read_write)
    notebook_init (&new_distriblist->notebook, FALSE, new_distriblist);

    // Attach the notebook (expanded and filled, no padding)
    box = GTK_WIDGET(gtk_builder_get_object (builder, dialog_notebook));
    gtk_box_pack_start (
        GTK_BOX(box), new_distriblist->notebook.notebook, TRUE, TRUE, 0);

    // Decrease the reference pointer to the notebook object
    g_object_unref (new_distriblist->notebook.notebook);

    // Fill in the widgets appropriately
    if (distriblist)
        distriblist_to_ui (
            distriblist,
            new_distriblist->entry_description,
            &new_distriblist->notebook);
    else
        new_distriblist->notebook.type = GNC_DISTRIBLIST_TYPE_SHARES;

    // Create the menu
    combobox_type = GTK_WIDGET(gtk_builder_get_object (builder, dialog_type));
    gtk_combo_box_set_active (
        GTK_COMBO_BOX(combobox_type),
        new_distriblist->notebook.type - 1);

    // Show the associated notebook widgets (active page)
    notebook_show (&new_distriblist->notebook);
    // hide bottonbox here, or when initializing?
    //gtk_widget_hide();

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
       // New list: focus the distribution list name
      gtk_widget_grab_focus (new_distriblist->entry_name);

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

void
notebook_owner_edit (
    DistributionListNotebook *notebook)
{
    // load the notebook widgets
    g_warning ("[notebook_owner_edit] handle valid owners.\n");
}

static void
notebook_owner_init (
    DistributionListNotebook *notebook,
    gboolean read_only)
{
    GtkWidget *box;
    GtkBuilder *builder;
    GtkWidget *parent;
    GtkTreeSelection *selection;
    GtkWidget *widget;

    // Load the owner notebook from Glade file
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_owner_label_owner_assigned");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_owner_treeview_list");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_owner_window");
    notebook->owner = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_owner"));
    notebook->buttonbox_shares = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_ notebook_buttonbox_shares_actions_lists"));
    notebook->buttonbox_percentage = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_ notebook_buttonbox_percentage_actions_lists"));
    notebook->button_edit = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook_button_percentage_edit"));

    parent = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_owner_window"));
}

// NOTE: The caller needs to unref once they attach
static void
notebook_init (
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data)
{
    GtkWidget *box;
    GtkBuilder *builder;
    GtkTreeViewColumn *col;
    GtkWidget *parent;
    GtkTreeSelection *selection;
    GtkWidget *scrolled_window;
    GtkTreeView *tree_view;

    // Hardcode owner type (for now)
    owner_type = GNC_OWNER_COOWNER;

    // Load the notebook from Glade file
    g_warning ("[notebook_init] read in distriblists_notebook glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_shares_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "adjust_percentage_total");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_notebook_window");

    // Assign the parent widgets
    g_warning ("[notebook_init] assign distriblists_notebook parent\n");
    parent = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook_window"));

    // Assign the notebook widgets
    g_warning ("[gnc_ui_distriblists_window_new] assign notebook ui widgets\n");
    notebook->notebook = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_notebook"));

    // Set the name for this dialog
    // (Hint: allows easy manipulation via css)
    gtk_widget_set_name (GTK_WIDGET(
        notebook->notebook), "gnc-id-distribution-list");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        notebook->notebook), "gnc-class-distribution-lists");

    // Load the "percentage" widgets
    notebook->label_settlement_percentage = GTK_WIDGET(gtk_builder_get_object (
        builder, "distriblists_notebook_label_settlement_percentage"));
    notebook->entry_settlement_percentage = GTK_WIDGET(gtk_builder_get_object (
        builder, "distriblists_notebook_entry_settlement_percentage"));
    notebook->label_percentage_total = GTK_WIDGET(gtk_builder_get_object (
        builder, "distriblists_notebook_label_percentage_total"));
    notebook->percentage_total = read_widget (builder,
        "percentage:percentage_total", read_only);
    notebook->percentage_total = GTK_WIDGET(gtk_builder_get_object (
        builder, "percentage:percentage_total"));
    notebook->percentage_owner_typename = GTK_WIDGET(gtk_builder_get_object (
        builder, "distriblists_notebook_percentage_owner_typename"));
    box = GTK_WIDGET (gtk_builder_get_object (
        builder,
        "distriblists_notebook_scrolled_window_lists_percentage"));
    notebook->owner_tree = GTK_WIDGET (
        gnc_tree_view_owner_new (owner_type));
    gtk_container_add (GTK_CONTAINER(box), notebook->owner_tree);
    gtk_tree_view_set_headers_visible (
        GTK_TREE_VIEW(notebook->owner_tree), FALSE);

    // Load notebook buttonbox widgets
    notebook->buttonbox_percentage = read_widget (
        builder, "distriblists_notebook_grid_percentage_owner", read_only);

    notebook->buttonbox_shares = read_widget (
        builder, "distriblists_notebook_grid_shares_owner", read_only);

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
owners_assign (
    DistributionListOwners *owners)
{
    // TODO: handle new owner assignment
    g_warning ("[owner_assign] assing a new owner to the list\n");
}

static void
owners_edit (
    DistributionListOwners *owners,
    DistributionListNotebook *notebook)
{
    //GtkWidget *button_text_dialog;
    GtkBuilder *builder;
    GtkWidget *dialog;

    GtkTreeSelection *selection;
    gboolean done;
    gint response;

    // load the dialog widgets
    g_warning ("[owners_edit] handle assignement and removal of owners.\n");

    /* g_warning ("[owners_edit] given owner type '%s'.\n", */
    /*            gncOwnerTypeToQofIdType(gncOwnerGetType (owners->owner))); */
    /*            //gncOwnerGetTypeString (owners->owner->type)); */
    /*            //gncOwnerTypeGetTypeString(owners->owner->type)); */

    /* switch (owners->owner->type) */
    /* { */
    /* case GNC_OWNER_COOWNER: */
    /*     owners->owner_typename = gncOwnerTypeGetTypeString( */
    /*         owners->owner->type); */
    /*     break; */
    /* case GNC_OWNER_EMPLOYEE: */
    /*     owners->owner_typename = gncOwnerTypeGetTypeString( */
    /*         owners->owner->type); */
    /*     break; */
    /* case GNC_OWNER_NONE: */
    /*     g_warning ("[owners_edit] given owner type '%s'.\n", */
    /*                gncOwnerTypeGetTypeString(owners->owner->type)); */
    /*     owners->owner_typename = gncOwnerTypeGetTypeString( */
    /*         owners->owner->type); */
    /*     break; */
    /* default: */
    /*     g_warning ("[owners_edit] given owner type '%s' is unsupported.\n", */
    /*                gncOwnerTypeGetTypeString(owners->owner->type)); */
    /*     //owner = _("Unsupported owner type"); */
    /*     owners->owner_typename = gncOwnerTypeGetTypeString( */
    /*         owners->owner->type); */
    /*     break; */
    /* } */
    /* g_warning ("[owners_edit] owner_typename done\n"); */

// FIXME: this works!
    /* create a new dialog */
    /* button_text_dialog = gtk_dialog_new_with_buttons("Edit owners dialog", */
    /*     owners->dialog, GTK_DIALOG_DESTROY_WITH_PARENT, "OK", */
    /*     GTK_RESPONSE_OK, NULL); */

    /* gtk_widget_show_all(button_text_dialog); */

// FIXME: the dialog never shows up!
    // Create a new owners object (pointer to owner struct)
    owners = g_new0 (DistributionListOwners, 1);

    // Load the owners dialog from Glade file
    g_warning ("[owners_edit] read in distriblists owner glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "owners_dialog");

    // Assign the owners widgets
    g_warning ("[owners_edit] assign owners ui widgets\n");
    owners->dialog = GTK_WIDGET(
        gtk_builder_get_object (builder, "owners_dialog"));
    g_warning ("[owners_edit] owners dialog done\n");

    // Show dialog and set focus
    // gtk_widget_show (owners->dialog);
    //gtk_widget_show_all (owners->dialog);

    // owners_combobox_type vs list_typename
    // gtk_widget_grab_focus (owners->list_typename);

    done = FALSE;
    while (!done)
    {
        response = gtk_dialog_run (GTK_DIALOG(owners->dialog));
        switch (response)
        {
        case GTK_RESPONSE_OK:
            g_warning ("[owners_edit] ended with reponse ok.\n");
            done = TRUE;
            break;
        default:
            done = TRUE;
            break;
        }
    }

    g_object_unref (G_OBJECT(builder));

    gtk_widget_destroy (owners->dialog);
    g_free (owners);

    // *owners_list and *owner needs to be saved via ui_to_distriblist
}

static void
owners_init (
    DistributionListOwners *owners,
    gboolean read_only,
    gpointer user_data)
{
    GtkBuilder *builder;
    GtkWidget *dialog;
    GtkWidget *list_type;
    GtkWidget *owners_treeview;
    GtkWidget *parent;

    GncOwnerType owner_type;
    GncOwner owner;
    GtkTreeSelection *selection;
    const gchar *dialog_type;

    // Create a new owners object (pointer to owner struct)
    owners = g_new0 (DistributionListOwners, 1);

    // Assign the dialog window
    //owners->dialog = gtk_dialog_new ();

    // Load the owners from Glade file
    g_warning ("[owners_init] read in distriblists owner glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "owners_dialog");
    /* gnc_builder_add_from_file ( */
    /*     builder, */
    /*     "dialog-distriblists.glade", */
    /*     "ownerstore_type"); */

    // Assign the parent widgets
    g_warning ("[owners_init] assign distriblists owner parent\n");
    parent = GTK_WIDGET(
        gtk_builder_get_object (builder, "edit_distriblists_dialog"));

    // Assign the owners widgets
    g_warning ("[owners_init] assign owners ui widgets\n");
    owners->dialog = GTK_WIDGET(
        gtk_builder_get_object (builder, "owners_vbox"));
    g_warning ("[owners_init] owners dialog done\n");

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        owners->dialog), "gnc-id-owners-dialog");
    gnc_widget_style_context_add_class (
        GTK_WIDGET(owners->dialog), "gnc-class-owners-dialog");
    g_warning ("[owners_init] class and name done\n");

    // Create the owner type list (menu as combobox)
    list_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "owners_combobox_type"));
    g_warning ("[owners_init] combobox_type done\n");

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
    /* owners->owner_typename = gncOwnerTypeGetTypeString( */
    /*      owners->owner->type); */
    /* g_warning ("[owners_init] owners_typename done\n"); */

    /* owners->owner_typename = gncOwnerGetTypeString( */
    /*      gncDistribListGetOwner (distriblist)); */
    /* owners->owners_typename = GTK_WIDGET( */
    /*     gtk_builder_get_object (builder, "owners_combobox_typename")); */

    // Load the view of assigned owners
    owners_treeview = GTK_WIDGET (gtk_builder_get_object (
        builder, "owners_treeview_owners"));
    g_warning ("[owners_init] owners_treeview done\n");

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

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        owners->dialog), "gnc-id-owners-dialog");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        owners->dialog), "gnc-class-owners-dialog");

    // Set widgets inactive
    //gtk_widget_set_sensitive (owners_dialog, TRUE);

    // Disconnect the owner from the window
    g_object_ref (owners->dialog);
    //gtk_container_remove (GTK_CONTAINER(parent), owners->dialog);
    g_object_unref (G_OBJECT(builder));

    // NOTE: The caller needs to handle unref once they attach a notebook
}

static void
owners_maybe_set_type (
    DistributionListOwners *owners,
    GncOwnerType owner_type)
{
    // See if anything to do?
    if (owner_type == owners->owner->type)
        return;

    /* Let's refresh */
    owners->owner->type = owner_type;
    //owners_show (owners);
}

static void
owners_remove (
    DistributionListOwners *owners)
{
    g_warning ("[owners_remove] removew existing owner from the list\n");
}

static gboolean
owners_select_ok_cb (
    DistributionListOwners *owners)
{
    const char *message;
    GncOwnerType owner_type;

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
        message = _("You must choose an Account.");
        gnc_error_dialog (GTK_WINDOW(owners->dialog), "%s", message);
        return FALSE;
    }

    gnc_suspend_gui_refresh ();

    // TODO: handle assigned owner_list
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

    // Set the type
    gncDistribListSetType (
        new_distriblist->this_distriblist,
        new_distriblist->notebook.type);

    // Set the attributes
    gncDistribListSetLabelSettlement (distriblist, gtk_editable_get_chars
        (GTK_EDITABLE (notebook->entry_settlement), 0, -1));

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
       PWARN ("Write owner ('%i') -> '%s'\n",
              owners->owner->type,
              gncOwnerTypeToQofIdType(owners->owner->type));
        gncDistribListSetOwner (
            distriblist,
            owners->owner);
        break;
    case GNC_DISTRIBLIST_TYPE_SHARES:
        set_int (
            notebook->shares_total,
            distriblist,
            gncDistribListSetSharesTotal);
       // FIXME: set a default owner-type (e.g: GNC_OWNER_NONE, GNC_OWNER_COOWNER)
        PWARN ("Write owner ('%i') -> '%s'\n",
               owners->owner->type,
               gncOwnerTypeToQofIdType(owners->owner->type));
        gncDistribListSetOwner (
            distriblist,
            owners->owner);
        break;
    }

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
        if (gtk_entry_get_text_length(GTK_ENTRY(notebook->entry_settlement)) == 0)
        {
            // Translators: This is the label that should be presented in settlements.
            gtk_entry_set_text(GTK_ENTRY(notebook->entry_settlement),  _("Percentage"));
            gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_label);
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
            gtk_entry_set_text(GTK_ENTRY(notebook->entry_settlement),  _("Shares"));
            gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_label);
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
    GtkWidget *widget;

    GtkBuilder *builder;
    GtkTreeViewColumn *column;
    DistributionListsWindow *distriblists_window;
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

    // Assign the gtk box (the primary dialog)
    distriblists_window->vbox = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox"));

    // Assign distribution lists frame
    distriblists_window->view_lists = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_treeview_lists"));

    // Assign distribution list definition frame
    distriblists_window->vbox_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_definition"));

    // Assign distribution list attributes
    distriblists_window->label_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_definition_distribution_list"));
    distriblists_window->label_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_description"));
    distriblists_window->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_description"));
    distriblists_window->label_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_type"));
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

    // Initialize the view
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

    // View signals that should be handled via callbacks
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

    // Attach the notebook (list attributes)
    widget = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_notebook"));
    gtk_box_pack_start (
        GTK_BOX(widget),
        distriblists_window->notebook.notebook,
        TRUE,
        TRUE,
        0);
    g_object_unref (distriblists_window->notebook.notebook);

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
    g_warning ("[gnc_ui_distriblists_window_new] "
               "refresh the UI\n");
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

    return new_dialog (
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

    new_dialog (
        distriblists_window, distriblists_window->current_list, NULL);
    gtk_widget_show (distriblists_window->vbox_definition);
    //gtk_widget_hide (distriblists_window->notebook.buttonbox_percentage);
    //gtk_widget_hide (distriblists_window->notebook.buttonbox_shares);
    gtk_widget_hide (distriblists_window->notebook.button_edit);
}

void
distriblists_list_new_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    new_dialog (distriblists_window, NULL, NULL);
}

void
distriblists_owner_assign_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;
}

void
distriblists_owner_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    DistributionListNotebook *notebook;
    NewDistributionList *new_distriblist = NULL;
    //gboolean done;
    //gint response;

    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    notebook = &distriblists_window->notebook;

    // Initialize the owner widgets (read_write)
    owner_init (&distriblists_window->notebook, FALSE, new_distriblist);

    // Assign or remove owners to the list
}

void
distriblists_owner_remove_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;
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
distriblists_window_close_handler (gpointer data)
{
    DistributionListsWindow *distriblists_window = data;
    g_return_if_fail (distriblists_window);

    gtk_widget_destroy (distriblists_window->window);
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
    GtkWidget *widget, GdkEventKey *event,
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

/***********************************************************\
 * Public functions
\***********************************************************/

// Create a distriblists window
DistributionListsWindow *
gnc_ui_distriblists_window_new (
    GtkWindow *parent,
    QofBook *book)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    // Assign new owner to the list
    owners_assign (&distriblists_window->owners);
}

void
owners_edit_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    DistributionListNotebook *notebook;
    NewDistributionList *new_distriblist = NULL;

    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    g_warning ("[owners_edit_cb] start owners dialog\n");
    /* owners = &distriblists_window->owners; */

    // Initialize the owner widgets (read_write)
    //owners_init (&distriblists_window->owners, FALSE, new_distriblist);

    // Assign or remove owners to the list
    owners_edit (&distriblists_window->owners, &distriblists_window->notebook);
}

void
owners_ok_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    // Checks once the selection is ok
    g_warning ("[owners_ok_cb] validate assigned owners.\n");
}

void
owners_remove_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;

    // Remove given owner from the list
    owners_remove (&distriblists_window->owners);
}

static void
owners_tree_selection_changed_cb (
    GtkTreeSelection *selection,
    DistributionListsWindow *distriblists_window)
{
    GtkActionGroup *action_group;
    GtkTreeView *view;
    GtkWidget *widget;

    if (!book) return NULL;

    // Find an existing distriblist window.  If it already
    // exist, bring it to the front. If we have an actual owner, then
    // set it in the window.
    distriblists_window = gnc_find_first_gui_component (
        DIALOG_DISTRIBLISTS_CM_CLASS, find_handler, book);
    if (distriblists_window)
    {
        // got one -- present the given window
        gtk_window_present (GTK_WINDOW(distriblists_window->window));
        return distriblists_window;
    }

    // Didn't find one -- create a new window
    distriblists_window = g_new0 (DistributionListsWindow, 1);
    distriblists_window->book = book;
    distriblists_window->session = gnc_get_current_session ();

    // Open and read the Glade file
    g_warning ("[gnc_ui_distriblists_window_new] read in distribution list glade definitions\n");
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_window");

    // Assign the gtk window
    g_warning ("[gnc_ui_distriblists_window_new] assign distribution list ui widgets\n");
    distriblists_window->window = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_window"));

    // Assign the gtk box (the primary dialog)
    distriblists_window->vbox = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox"));

    // Assign distribution lists frame (as a treeview)
    distriblists_window->view_lists = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_treeview_lists"));

    // Assign distribution list definition frame
    distriblists_window->vbox_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_definition"));

    // Assign distribution list type
    distriblists_window->label_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_type"));
    distriblists_window->entry_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_type"));

    // Assign distribution list attributes
    distriblists_window->label_definition = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_definition_distribution_list"));
    distriblists_window->label_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_description"));
    distriblists_window->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_description"));
    distriblists_window->entry_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_type"));

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(
        distriblists_window->window), "gnc-id-distribution-lists");
    gnc_widget_style_context_add_class (GTK_WIDGET(
        distriblists_window->window), "gnc-class-distribution-lists");

    g_signal_connect (
        distriblists_window->window, "key_press_event",
        G_CALLBACK (
            distriblists_window_key_press_cb), distriblists_window);

    // Initialize the view
    view = GTK_TREE_VIEW(distriblists_window->view_lists);
    store = gtk_list_store_new (NUM_DISTRIBUTION_LIST_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
    g_object_unref (store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
             "",
             renderer,
             "text",
             DISTRIBUTION_LIST_COLUMN_NAME,
             NULL);
    gtk_tree_view_append_column (view, column);

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
    notebook_init (
        &distriblists_window->notebook, TRUE, distriblists_window);

    // Attach the notebook (list attributes)
    widget = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_notebook"));
    gtk_box_pack_start (
        GTK_BOX(widget),
        distriblists_window->notebook.notebook,
        TRUE,
        TRUE,
        0);
    g_object_unref (distriblists_window->notebook.notebook);

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

    gnc_gui_component_set_session (
        distriblists_window->component_id,
        distriblists_window->session);

    gtk_widget_show_all (distriblists_window->window);

    distriblists_window_refresh (distriblists_window);

    g_object_unref (G_OBJECT(builder));

    return distriblists_window;
}

void
owners_type_changed_cb (
    GtkComboBox *combobox,
    gpointer user_data)
{
    DistributionListOwners *owners = user_data;
    gint value;

    value = gtk_combo_box_get_active (combobox);
    owners_maybe_set_type (owners, value + 1);
}
