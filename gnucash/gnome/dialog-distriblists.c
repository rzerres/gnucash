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

#include "dialog-utils.h"
#include "gnc-component-manager.h"
#include "gnc-session.h"
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

typedef struct _distribution_list_notebook
{
    GtkWidget *notebook;

    // "Co-Ownership "Shares" widgets
    GtkWidget *label_settlement;
    GtkWidget *shares_total;

    // "Distribution lists" widgets
    GncDistributionListType type;
} DistributionListNotebook;

struct _distribution_lists_window
{
    GtkWidget *window;
    GtkWidget *label_description;
    GtkWidget *entry_description;
    GtkWidget *label_type;
    GtkWidget *entry_type;
    GtkWidget *vbox;
    GtkWidget *view_lists;
    GtkWidget *vbox_definitions;
    DistributionListNotebook notebook;

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
    DistributionListNotebook notebook;

    DistributionListsWindow *distriblists_window;
    GncDistributionList *this_distriblist;
} NewDistributionList;

/*************************************************\
 * Function prototypes declaration
 * (assume C11 semantics, where order matters)
\*************************************************/

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
    DistributionListNotebook *notebook);
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

DistributionListsWindow *gnc_ui_distriblists_window_new (
    GtkWindow *parent,
    QofBook *book);
// GncDistributionListType *gnc_ui_distriblists_new_from_name (
//    GtkWindow *parent,
//    QofBook *book,
//    const char *name);
static void init_notebook_widgets (
    DistributionListNotebook *notebook,
    gboolean read_only,
    gpointer user_data);
static void maybe_set_type (
    NewDistributionList *new_distriblist,
    GncDistributionListType type);
static GncDistributionList *new_distriblist_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name);
static gboolean new_distriblist_ok_cb (
    NewDistributionList *new_distriblist);
static GtkWidget *read_widget (GtkBuilder *builder, char *name, gboolean read_only);
static void
distriblist_selection_activated (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window);
static void
distriblist_selection_changed (
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
static void show_notebook (
    DistributionListNotebook *notebook);
static gboolean ui_to_distriblist (NewDistributionList *new_distriblist);
static gboolean verify_distriblist_ok (
    NewDistributionList *new_distriblist);

// callback function prototypes
static void
distriblists_combobox_type_changed_cb (
    GtkComboBox *combobox,
    gpointer data);
/* callback prototypes */
/* static void distriblists_combobox_type_changed_cb ( */
/*     GtkTreeSelection *selection, */
/*     DistributionListsWindow  *distriblists_window); */
void distriblists_delete_list_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void distriblists_edit_list_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
void distriblists_new_list_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window);
static void distriblist_selection_activated_cb (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window);
void distriblists_type_combobox_changed_cb (
    GtkComboBox *cb, gpointer data);
static void  distriblists_window_close_handler (gpointer data);
void distriblists_window_close (
    GtkWidget *widget,
    gpointer data);
void distriblists_window_destroy_cb (
    GtkWidget *widget,
    gpointer data);
static gboolean distriblists_window_key_press_cb (
    GtkWidget *widget, GdkEventKey *event,
    gpointer data);

void distriblists_delete_distriblist_cb (
    GtkButton *button, DistributionListsWindow *distriblist_window);
void distriblists_edit_distriblist_cb (
    GtkButton *button, DistributionListsWindow *distriblist_window);
void distriblists_new_distriblist_cb (
    GtkButton *button, DistributionListsWindow *distriblist_window);

/*************************************************\
 * Functions
\*************************************************/

/* helper functions */
static void
distriblists_list_refresh (
    DistributionListsWindow *distriblists_window)
{
    char *label_type;

    g_return_if_fail (distriblists_window);

    if (!distriblists_window->current_list)
    {
        gtk_widget_hide (distriblists_window->vbox_definitions);
        return;
    }

    gtk_widget_show_all (distriblists_window->vbox);

    // propagate structure values to ui
    distriblist_to_ui (
        distriblists_window->current_list,
        distriblists_window->entry_description,
        &distriblists_window->notebook);

    switch (gncDistribListGetType (
        distriblists_window->current_list))
    {
    case GNC_DISTRIBLIST_TYPE_SHARES:
        label_type = _("Shares");
        break;
    default:
        label_type = _("Unknown");
        break;
    }

    // show the notebook frame
    show_notebook (&distriblists_window->notebook);
    gtk_label_set_text (
        GTK_LABEL(distriblists_window->label_type), label_type);
}

static void
distriblist_selection_activated (
    GtkTreeView *tree_view,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    DistributionListsWindow *distriblists_window)
{
    new_distriblist_dialog (
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

    // If there are no distriblists, clear the distriblist display
    if (list == NULL)
    {
        distriblists_window->current_list = NULL;
        distriblists_list_refresh (distriblists_window);
    }
    else
    {
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
    GtkWidget *desc,
    DistributionListNotebook *notebook)
{
    gtk_entry_set_text (
        GTK_ENTRY(desc), gncDistribListGetDescription (distriblist));

// FIXME
    // Label Settlement
    /* gtk_entry_set_text ( */
    /*    GTK_ENTRY(entry_settlement), gncDistribListGetLabelSettlement (distriblist)); */

    // Distribution list type
    notebook->type = gncDistribListGetType (distriblist);

    switch (notebook->type)
    {
    case GNC_DISTRIBLIST_TYPE_SHARES:
        get_int (
            notebook->shares_total,
            distriblist,
            gncDistribListGetSharesTotal);
        //gtk_entry_set_text (GTK_ENTRY(distrblist->label_settlement), notebook->label_settlement);
        /* get_string ( */
        /*     notebook->label_settlement, */
        /*     distriblist, */
        /*     gncDistribListGetLabelSettlement); */
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

    // Find an existing distriblist window. If it already
    // exist, bring it to the front. If we have an actual owner, then
    // set it in the window.
    distriblists_window = gnc_find_first_gui_component (
      DIALOG_DISTRIBLISTS_CM_CLASS, find_handler, book);
    if (distriblists_window)
    {
        gtk_window_present (GTK_WINDOW(distriblists_window->window));
        return distriblists_window;
    }

    // Didn't find one -- create a new window
    distriblists_window = g_new0 (DistributionListsWindow, 1);
    distriblists_window->book = book;
    distriblists_window->session = gnc_get_current_session ();

    // Open and read the Glade File
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", "distriblists_window");

    // Assign the gtk window
    distriblists_window->window = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_window"));

    // Assign the gtk box (the primary dialog)
    distriblists_window->vbox = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox"));

    // Assign available distribution lists (as a treeview)
    distriblists_window->view_lists = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_treeview_lists"));

    // Assign distribuion list definition attributes
    distriblists_window->label_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_label_description"));
    distriblists_window->entry_description = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_entry_description"));
    distriblists_window->label_type = GTK_WIDGET(
        gtk_builder_get_object (builder, "label_type"));
    distriblists_window->vbox_definitions = GTK_WIDGET(
        gtk_builder_get_object (builder, "distriblists_vbox_definition"));

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
        gtk_builder_get_object (builder, "distriblists_notebook"));
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
    gnc_builder_add_from_file (builder, "dialog-distriblists.glade", "adjust_shares_total");
    gnc_builder_add_from_file (builder, "dialog-distriblists.glade", "distriblists_notebook_window");
    notebook->notebook = GTK_WIDGET(gtk_builder_get_object (builder, "distriblists_notebook"));
    parent = GTK_WIDGET(gtk_builder_get_object (builder, "distriblists_notebook_window"));

    // Set the name for this dialog so it can be easily manipulated with css
    gtk_widget_set_name (GTK_WIDGET(notebook->notebook), "gnc-id-distribution-list");
    gnc_widget_style_context_add_class (GTK_WIDGET(notebook->notebook), "gnc-class-distribution-lists");

    // load the "shares-total" widgets
    notebook->shares_total = read_widget (builder, "shares:shares_total", read_only);

    // Disconnect the notebook from the window
    g_object_ref (notebook->notebook);
    gtk_container_remove (GTK_CONTAINER(parent), notebook->notebook);
    g_object_unref (G_OBJECT(builder));
    gtk_widget_destroy (parent);

    // NOTE: The caller needs to unref once they attach
}

static void
maybe_set_type (
    NewDistributionList *new_distriblist,
    GncDistributionListType type)
{
    // See if anything to do?
    if (type == new_distriblist->notebook.type)
        return;

    /* Let's refresh */
    new_distriblist->notebook.type = type;
    show_notebook (&new_distriblist->notebook);
}

static GncDistributionList
*new_distriblist_dialog (
    DistributionListsWindow *distriblists_window,
    GncDistributionList *distriblist,
    const char *name)
{
    GncDistributionList *created_distriblist = NULL;
    NewDistributionList *new_distriblist;
    GtkBuilder *builder;
    GtkWidget *box;
    GtkWidget *combobox_type;
    gint response;
    gboolean done;
    const gchar *dialog_name;
    const gchar *dialog_description;
    const gchar *dialog_type;
    const gchar *dialog_notebook;

    if (!distriblists_window) return NULL;

    // Create a new distribution list
    new_distriblist = g_new0 (NewDistributionList, 1);

    // Assign the dialog window
    new_distriblist->distriblists_window = distriblists_window;

    // Assign a local copy of the given distribution list
    new_distriblist->this_distriblist = distriblist;

    // assign needed Glade dialog entities
    if (distriblist == NULL)
    {
        //dialog_label_ = "new_distriblist_dialog";
        dialog_name = "new_distriblist_dialog";
        dialog_description = "new_distriblists_entry_description";
        dialog_notebook = "new_distriblists_notebook_hbox";
        dialog_type = "new_distriblists_combobox_type";
    }
    else
    {
        dialog_name = "edit_distriblist_dialog";
        dialog_description = "edit_distriblists_entry description";
        dialog_notebook = "edit_distriblists_notebook_hbox";
        dialog_type = "edit_distriblists_combobox_type";
    }

    // Open and read the Glade file
    builder = gtk_builder_new ();
    gnc_builder_add_from_file (
        builder,
        "dialog-distriblists.glade",
        "distriblists_liststore_type");
    gnc_builder_add_from_file (
        builder, "dialog-distriblists.glade", dialog_name);
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

    // Initialize the notebook widgets
    init_notebook_widgets (&new_distriblist->notebook, FALSE, new_distriblist);

    // Attach the notebook
    box = GTK_WIDGET(gtk_builder_get_object (builder, dialog_notebook));
    gtk_box_pack_start (
        GTK_BOX(box), new_distriblist->notebook.notebook, TRUE, TRUE, 0);
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

    // Show the right notebook page
    show_notebook (&new_distriblist->notebook);

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
        gtk_widget_grab_focus (new_distriblist->entry_description);
    }
    else
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
        // Update an existing distribution list -> update entity
        gncDistribListBeginEdit (distriblists_window->current_list);

    // Update the other table entries
    if (ui_to_distriblist (new_distriblist))
        gncDistribListChanged (distriblists_window->current_list);

    // Mark the table as changed and commit it
    gncDistribListCommitEdit (distriblists_window->current_list);

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

static void
show_notebook (
    DistributionListNotebook *notebook)
{
    g_return_if_fail (notebook->type > 0);
    gtk_notebook_set_current_page (
        GTK_NOTEBOOK(notebook->notebook), notebook->type - 1);
}

// return TRUE if persitent storage of UI changes was successfully
static gboolean
ui_to_distriblist (NewDistributionList *new_distriblist)
{
    DistributionListNotebook *notebook;
    GncDistributionList *distriblist;
    const char *text;

    distriblist = new_distriblist->this_distriblist;
    notebook = &new_distriblist->notebook;

    text = gtk_entry_get_text (GTK_ENTRY(new_distriblist->entry_description));
    if (text)
        gncDistribListSetDescription (distriblist, text);

    gncDistribListSetType (
        new_distriblist->this_distriblist,
        new_distriblist->notebook.type);

    switch (new_distriblist->notebook.type)
    {
    case GNC_DISTRIBLIST_TYPE_SHARES:
        set_int (
            notebook->shares_total,
            distriblist,
            gncDistribListSetSharesTotal);
        /* set_string ( */
        /*     notebook->label_settlement, */
        /*     distriblist, */
        /*     gncDistribListSetLabelSettlement); */
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
    gint shares_total;
    char *msg_label = _("Preset a default distribution key label.");
    char *msg_shares = _("You must set max value of available property shares.");

    notebook = &new_distriblist->notebook;
    result = TRUE;

    shares_total = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(notebook->shares_total));

    switch (new_distriblist->notebook.type)
    {
    case GNC_DISTRIBLIST_TYPE_SHARES:
        if (gtk_entry_get_text_length(GTK_ENTRY(notebook->label_settlement)) == 0)
        {
            // Translators: This is the label that should be presented settlements.
            gtk_label_set_text(GTK_LABEL(notebook->label_settlement),  _("Shares"));
            gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_label);
        }
        if ((shares_total)==0)
        {
              gnc_error_dialog (GTK_WINDOW(new_distriblist->dialog), "%s", msg_shares);
              result = FALSE;
        }
        break;
    }

    return result;
}

// Callback functions
static void
distriblists_combobox_type_changed_cb (
    GtkComboBox *combobox,
    gpointer data)
{
    NewDistributionList *new_distriblist = data;
    gint value;

    value = gtk_combo_box_get_active (combobox);
    maybe_set_type (new_distriblist, value + 1);
}

void
distriblists_delete_list_cb (
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
distriblists_edit_list_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    if (!distriblists_window->current_list)
        return;
    new_distriblist_dialog (
        distriblists_window, distriblists_window->current_list, NULL);
}

void
distriblists_new_list_cb (
    GtkButton *button,
    DistributionListsWindow *distriblists_window)
{
    g_return_if_fail (distriblists_window);
    new_distriblist_dialog (distriblists_window, NULL, NULL);
}

void
distriblists_type_combobox_changed_cb (GtkComboBox *cb, gpointer data)
{
    NewDistributionList *new_distriblist = data;
    gint value;

    value = gtk_combo_box_get_active (cb);
    maybe_set_type (new_distriblist, value + 1);
}

static void
distriblists_window_close_handler (gpointer data)
{
    DistributionListsWindow *distriblists_window = data;
    g_return_if_fail (distriblists_window);

    gtk_widget_destroy (distriblists_window->window);
}

void
distriblists_window_close (
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

static gboolean
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
