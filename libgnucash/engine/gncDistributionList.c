/*********************************************************************\
 * gncDistributionList.c -- the Gnucash Distirbuton List interface   *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License as    *
 * published by the Free Software Foundation; either version 2 of    *
 * the License, or (at your option) any later version.               *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License for more details.                      *
 *                                                                   *
 * You should have received a copy of the GNU General Public License *
 * along with this program; if not, contact:                         *
 *                                                                   *
 * Free Software Foundation           Voice:  +1-617-542-5942        *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652        *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                    *
 *                                                                   *
\*********************************************************************/

/*
 * Copyright (C) 2020 Ralf Zerres
 * Author: Ralf Zerres <ralf.zerres@mail.de>
 */

#include <config.h>

#include <glib.h>
#include <qofinstance-p.h>

#include "gnc-engine.h"
#include "gncDistributionListP.h"

struct _gncDistributionList
{
    QofInstance inst;

    // 'visible' data fields directly manipulated by user
    const char *name;
    const char *description;
    GncDistributionListType type;
    const char *percentage_label_settlement;
    gint percentage_total;
    const char *shares_label_settlement;
    gint shares_total;
    //GList *owner_tree;

    // Internal management fields
    // See src/doc/business.txt for an explanation of the following
    // Code that handles this is *identical* to that in gncTaxTable
    gint64 refcount;
    GncDistributionList *parent;      // if non-null, we are an immutable child
    GncDistributionList *child;       // if non-null, we have not changed
    gboolean invisible;
    GList *children;                  // list of children for disconnection
};

struct _gncDistributionListClass
{
    QofInstanceClass parent_class;
};

struct _book_info
{
    GList *lists;                    // visible distribution lists
};

static QofLogModule log_module = GNC_MOD_BUSINESS;

#define _GNC_MOD_NAME GNC_ID_DISTRIBLIST

#define SET_STR(obj, member, str) { \
        if (!g_strcmp0 (member, str)) return; \
        gncDistribListBeginEdit (obj); \
        CACHE_REPLACE(member, str); \
        }

AS_STRING_DEC(GncDistributionListType, ENUM_DISTRIBLISTS_TYPE)
FROM_STRING_DEC(GncDistributionListType, ENUM_DISTRIBLISTS_TYPE)

/***********************************************************\
 * Declaration: Private function prototypes
 * We assume C11 semantics, where the definitions must
 * preceed its usage. Once declared, the function ordering
 * is independend from its actual function call.
\***********************************************************/

static void gncDistribListFree (GncDistributionList *distriblist);

static void gnc_distriblist_dispose(GObject *distriblist_parent);
static void gnc_distriblist_finalize(GObject* distriblist_parent);
static void gnc_distriblist_get_property (
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec);
static void gnc_distriblist_set_property (
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec);
static GList*
impl_get_typed_referring_object_list(const QofInstance* inst, const QofInstance* ref);

/*************************************************\
 * Private Functions
\*************************************************/

// Misc inline utilities */

static inline void
mark_distriblist (GncDistributionList *distriblist)
{
    qof_instance_set_dirty(&distriblist->inst);
    qof_event_gen (&distriblist->inst, QOF_EVENT_MODIFY, NULL);
}

static inline void maybe_resort_list (GncDistributionList *distriblist)
{
    struct _book_info *book_info;

    if (distriblist->parent || distriblist->invisible) return;
    book_info = qof_book_get_data (
        qof_instance_get_book(distriblist), _GNC_MOD_NAME);
    book_info->lists = g_list_sort (
        book_info->lists, (GCompareFunc)gncDistribListCompare);
}

static inline void addObj (GncDistributionList *distriblist)
{
    //FIXME: pointer book_info has invalid address after assignment. Why?
    struct _book_info *book_info;
    //FIXME: why is book_info <null>?
    book_info = qof_book_get_data (qof_instance_get_book(distriblist), _GNC_MOD_NAME);
    if (book_info != NULL)
    {
        book_info->lists = g_list_insert_sorted (
           book_info->lists,
           distriblist,
           (GCompareFunc)gncDistribListCompare);
    }
    else
    {
        PERR("Book info is NULL!\n");
    }
}

static inline void
remObj (GncDistributionList *distriblist)
{
    struct _book_info *book_info;
    book_info = qof_book_get_data (
        qof_instance_get_book(distriblist), _GNC_MOD_NAME);
    if (book_info != NULL)
    {
        book_info->lists = g_list_remove (
            book_info->lists, distriblist);

    }
    else
    {
        PERR("Book info is NULL!\n");
    }
}

static inline void
gncDistribListAddChild (GncDistributionList *table, GncDistributionList *child)
{
    g_return_if_fail(qof_instance_get_destroying(table) == FALSE);
    table->children = g_list_prepend(table->children, child);
}

static inline void
gncDistribListRemoveChild (
    GncDistributionList *table,
    GncDistributionList *child)
{
    if (qof_instance_get_destroying(table)) return;
    table->children = g_list_remove(table->children, child);
}

/* ============================================================== */

enum
{
    PROP_0,
    PROP_NAME
};

/* GObject Initialization */
G_DEFINE_TYPE(GncDistributionList, gnc_distriblist, QOF_TYPE_INSTANCE);

static void
distriblist_free (QofInstance *inst)
{
    GncDistributionList *distriblist = (GncDistributionList *) inst;
    gncDistribListFree(distriblist);
}

/* Create/Destroy Functions */
static void
gncDistribListFree (
    GncDistributionList *distriblist)
{
    GncDistributionList *child;
    GList *list;

    if (!distriblist) return;

    qof_event_gen (&distriblist->inst,  QOF_EVENT_DESTROY, NULL);
    CACHE_REMOVE (distriblist->name);
    CACHE_REMOVE (distriblist->description);
    CACHE_REMOVE (distriblist->percentage_label_settlement);
    CACHE_REMOVE (distriblist->shares_label_settlement);
    remObj (distriblist);

    if (!qof_instance_get_destroying(distriblist))
        PERR("free a distribution list without do_free set!");

    /* disconnect from parent */
    if (distriblist->parent)
        gncDistribListRemoveChild(distriblist->parent, distriblist);

    /* disconnect from the children */
    for (list = distriblist->children; list; list = list->next)
    {
        child = list->data;
        gncDistribListSetParent(child, NULL);
    }
    g_list_free(distriblist->children);

    /* qof_instance_release(&distriblist->inst); */
    g_object_unref (distriblist);
}

static void
gnc_distriblist_class_init (GncDistributionListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    QofInstanceClass* qof_class = QOF_INSTANCE_CLASS(klass);

    gobject_class->dispose = gnc_distriblist_dispose;
    gobject_class->finalize = gnc_distriblist_finalize;
    gobject_class->set_property = gnc_distriblist_set_property;
    gobject_class->get_property = gnc_distriblist_get_property;

    qof_class->get_display_name = NULL;
    qof_class->refers_to_object = NULL;
    qof_class->get_typed_referring_object_list = impl_get_typed_referring_object_list;

    g_object_class_install_property
    (gobject_class,
     PROP_NAME,
     g_param_spec_string (
         "name",
         "DistributionList Name",
         "The distribution list name is an arbitrary string "
         "assigned by the user. It is intended to "
         "be short string (10 to 30 character) "
         "that is displayed by the GUI identified with the "
         "distriblist mnemonic.",
         NULL,
         G_PARAM_READWRITE));
}

static void
gnc_distriblist_dispose(GObject *distriblist_parent)
{
    G_OBJECT_CLASS(
        gnc_distriblist_parent_class)->dispose(distriblist_parent);
}

static void
gnc_distriblist_finalize(GObject* distriblist_parent)
{
    G_OBJECT_CLASS(
        gnc_distriblist_parent_class)->finalize(distriblist_parent);
}

static void
gnc_distriblist_get_property (
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
    GncDistributionList *distriblist;

    g_return_if_fail(GNC_IS_DISTRIBLIST(object));

    distriblist = GNC_DISTRIBLIST(object);
    switch (prop_id)
    {
    case PROP_NAME:
        g_value_set_string(value, distriblist->name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gnc_distriblist_init(
    GncDistributionList *distriblist)
{
}

static void
gnc_distriblist_set_property (
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
    GncDistributionList *distriblist;

    g_return_if_fail(GNC_IS_DISTRIBLIST(object));

    distriblist = GNC_DISTRIBLIST(object);
    g_assert (qof_instance_get_editlevel(distriblist));

    switch (prop_id)
    {
    case PROP_NAME:
        gncDistribListSetName(
            distriblist, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/** Returns a list of given object type which refers to an object.
    For example, when called as

        qof_instance_get_typed_referring_object_list(taxtable, account);

    it will return the list of taxtables which refer to
    a specific account. The result should be the same regardless
    of which taxtable object is used. The list must be freed by
    the caller but the objects on the list must not.
 */
static GList*
impl_get_typed_referring_object_list(const QofInstance* inst, const QofInstance* ref)
{
    // Distribuion list doesn't refer to anything except other distribution lists
    return NULL;
}


/* ============================================================== */
void
gncDistribListMakeInvisible (GncDistributionList *distriblist)
{
    if (!distriblist) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->invisible = TRUE;
    remObj (distriblist);
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetChild (GncDistributionList *distriblist, GncDistributionList *child)
{
    if (!distriblist) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->child = child;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

/** \brief Convert distribution list types from text. */
FROM_STRING_FUNC(GncDistributionListType, ENUM_DISTRIBLIST_TYPE)

// TODO: Is the parent/child relationship a double-linked list?
//       Do we still need an explicite set-parent/set-child method?
//       Any misuse could goof up -> can't we asure to make it atomic?
void
gncDistribListSetParent (
    GncDistributionList *distriblist,
    GncDistributionList *parent)
{
    if (!distriblist) return;
    gncDistribListBeginEdit (distriblist);
    if (distriblist->parent)
        gncDistribListRemoveChild(distriblist->parent, distriblist);
    distriblist->parent = parent;
    if (parent)
        gncDistribListAddChild(parent, distriblist);
    distriblist->refcount = 0;
    if ( parent != NULL )
    {
        gncDistribListMakeInvisible (distriblist);
    }
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetRefcount (GncDistributionList *distriblist, gint64 refcount)
{
    if (!distriblist) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->refcount = refcount;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

static void
gncDistribListOnError (QofInstance *inst, QofBackendError errcode)
{
    PERR("DistributionList QofBackend Failure: %d", errcode);
    gnc_engine_signal_commit_error( errcode );
}

static void
on_done (QofInstance *inst) {}

static
void qofDistributionListSetType (
    GncDistributionList *distriblist,
    const char *type_label)
{
    GncDistributionListType type;

    type = GncDistributionListTypefromString(type_label);
    gncDistribListSetType(distriblist, type);
}

/* Get Functions */

static GncDistributionList
*gncDistribListCopy (
    const GncDistributionList *distriblist)
{
    GncDistributionList *new_distriblist;

    if (!distriblist) return NULL;
    new_distriblist = gncDistribListCreate (qof_instance_get_book(distriblist));

    gncDistribListBeginEdit(new_distriblist);

    gncDistribListSetName (new_distriblist, distriblist->name);
    gncDistribListSetDescription (new_distriblist, distriblist->description);

    new_distriblist->type = distriblist->type;
    new_distriblist->percentage_label_settlement = distriblist->percentage_label_settlement;
    new_distriblist->percentage_total = distriblist->percentage_total;
    new_distriblist->shares_label_settlement = distriblist->shares_label_settlement;
    new_distriblist->shares_total = distriblist->shares_total;

    mark_distriblist (new_distriblist);
    gncDistribListCommitEdit(new_distriblist);

    return new_distriblist;
}


/** \brief Convert distribution list types to text. */
AS_STRING_FUNC(GncDistributionListType, ENUM_DISTRIBLIST_TYPE)

static const char
*qofDistributionListGetType (
    const GncDistributionList *distriblist)
{
    if (!distriblist)
    {
        return NULL;
    }
    return GncDistributionListTypeasString(distriblist->type);
}

gboolean
gncDistribListGetInvisible (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return FALSE;
    return distriblist->invisible;
}

// Private functions

static void
_gncDistribListCreate (QofBook *book)
{
    struct _book_info *book_info;

    if (!book) return;

    // allocate a book structure
    book_info = g_new0 (struct _book_info, 1);
    qof_book_set_data (book, _GNC_MOD_NAME, book_info);
}

static void
_gncDistribListDestroy (QofBook *book)
{
    struct _book_info *book_info;

    if (!book) return;

    book_info = qof_book_get_data (book, _GNC_MOD_NAME);

    g_list_free (book_info->lists);
    g_free (book_info);
}

static QofObject
gncDistribListDescription =
{
    DI(.interface_version = ) QOF_OBJECT_VERSION,
    DI(.e_type            = ) _GNC_MOD_NAME,
    DI(.type_label        = ) "Distribution List",
    DI(.create            = ) (gpointer)gncDistribListCreate,
    DI(.book_begin        = ) _gncDistribListCreate,
    DI(.book_end          = ) _gncDistribListDestroy,
    DI(.is_dirty          = ) qof_collection_is_dirty,
    DI(.mark_clean        = ) qof_collection_mark_clean,
    DI(.foreach           = ) qof_collection_foreach,
    DI(.printable         = ) NULL,
    DI(.version_cmp       = ) (int (*)(gpointer, gpointer)) qof_instance_version_cmp,
};

gboolean
gncDistribListRegister (void)
{
    static QofParam params[] =
    {
        {
            QOF_PARAM_BOOK,
            QOF_ID_BOOK,
            (QofAccessFunc)qof_instance_get_book,
            NULL
        },
        {
            QOF_PARAM_GUID,
            QOF_TYPE_GUID,
            (QofAccessFunc)qof_instance_get_guid,
            NULL
        },
        {
            GNC_DISTRIBLIST_DESCRIPTION,
            QOF_TYPE_STRING,
            (QofAccessFunc)gncDistribListGetDescription,
            (QofSetterFunc)gncDistribListSetDescription
        },
        {
            GNC_DISTRIBLIST_NAME,
            QOF_TYPE_STRING,
            (QofAccessFunc)gncDistribListGetName,
            (QofSetterFunc)gncDistribListSetName
        },
        {
            GNC_DISTRIBLIST_PERCENTAGE_LABEL_SETTLEMENT,
            QOF_TYPE_STRING,
            (QofAccessFunc)gncDistribListGetPercentageLabelSettlement,
            (QofSetterFunc)gncDistribListSetPercentageLabelSettlement
        },
        {
            GNC_DISTRIBLIST_PERCENTAGE_TOTAL,
            QOF_TYPE_INT32,
            (QofAccessFunc)gncDistribListGetPercentageTotal,
            (QofSetterFunc)gncDistribListSetPercentageTotal
        },
        {
            GNC_DISTRIBLIST_REFCOUNT,
            QOF_TYPE_INT64,
            (QofAccessFunc)gncDistribListGetRefcount,
            NULL
        },
        {
            GNC_DISTRIBLIST_SHARES_LABEL_SETTLEMENT,
            QOF_TYPE_STRING,
            (QofAccessFunc)gncDistribListGetSharesLabelSettlement,
            (QofSetterFunc)gncDistribListSetSharesLabelSettlement
        },
        {
            GNC_DISTRIBLIST_SHARES_TOTAL,
            QOF_TYPE_INT32,
            (QofAccessFunc)gncDistribListGetSharesTotal,
            (QofSetterFunc)gncDistribListSetSharesTotal
        },
        {
            GNC_DISTRIBLIST_TYPE,
            QOF_TYPE_STRING,
            (QofAccessFunc)qofDistributionListGetType,
            (QofSetterFunc)qofDistributionListSetType
        },
        { NULL },
    };

    qof_class_register (
        _GNC_MOD_NAME, (QofSortFunc)gncDistribListCompare, params);

    return qof_object_register (&gncDistribListDescription);
}


/***********************************************************\
 * Public functions
\***********************************************************/

// Create/Destroy Functions

void
gncDistribListBeginEdit (GncDistributionList *distriblist)
{
    qof_begin_edit(&distriblist->inst);
}

void
gncDistribListChanged (GncDistributionList *distriblist)
{
    if (!distriblist) return;
    distriblist->child = NULL;
}

void
gncDistribListCommitEdit (
    GncDistributionList *distriblist)
{
    if (!qof_commit_edit (QOF_INSTANCE(distriblist))) return;
    qof_commit_edit_part2 (&distriblist->inst, gncDistribListOnError,
                           on_done, distriblist_free);
}

GncDistributionList
*gncDistribListCreate (QofBook *book)
{
    GncDistributionList *distriblist;
    if (!book) return NULL;

    distriblist = g_object_new (GNC_TYPE_DISTRIBLIST, NULL);
    qof_instance_init_data(&distriblist->inst, _GNC_MOD_NAME, book);
    distriblist->name = CACHE_INSERT ("");
    distriblist->description = CACHE_INSERT ("");
    distriblist->type = GNC_DISTRIBLIST_TYPE_SHARES;
    distriblist->percentage_label_settlement = CACHE_INSERT ("");
    distriblist->percentage_total = 0;
    distriblist->shares_label_settlement = CACHE_INSERT ("");
    distriblist->shares_total = 0;

    addObj (distriblist);
    qof_event_gen (&distriblist->inst,  QOF_EVENT_CREATE, NULL);
    return distriblist;
}

void
gncDistribListDecRef (GncDistributionList *distriblist)
{
    if (!distriblist) return;
    if (distriblist->parent || distriblist->invisible) return;        /* children dont need refcounts */
    g_return_if_fail (distriblist->refcount >= 1);
    gncDistribListBeginEdit (distriblist);
    distriblist->refcount--;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListDestroy (GncDistributionList *distriblist)
{
    gchar guidstr[GUID_ENCODING_LENGTH+1];
    if (!distriblist) return;
    guid_to_string_buff(qof_instance_get_guid(&distriblist->inst),guidstr);
    DEBUG("destroying  distriblist %s (%p)", guidstr, distriblist);
    qof_instance_set_destroying(distriblist, TRUE);
    qof_instance_set_dirty (&distriblist->inst);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListIncRef (GncDistributionList *distriblist)
{
    if (!distriblist) return;
    if (distriblist->parent || distriblist->invisible) return;        /* children dont need refcounts */
    gncDistribListBeginEdit (distriblist);
    distriblist->refcount++;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

// Set Functions

void
gncDistribListSetDescription (
    GncDistributionList *distriblist,
    const char *description)
{
    if (!distriblist || !description) return;
    SET_STR (distriblist, distriblist->description, description);
    mark_distriblist (distriblist);
    maybe_resort_list (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetName (GncDistributionList *distriblist, const char *name)
{
    if (!distriblist || !name) return;
    SET_STR (distriblist, distriblist->name, name);
    mark_distriblist (distriblist);
    maybe_resort_list (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetPercentageLabelSettlement (
    GncDistributionList *distriblist,
    const char *percentage_label_settlement)
{
    if (!distriblist) return;
    SET_STR (distriblist, distriblist->percentage_label_settlement, percentage_label_settlement);
    mark_distriblist (distriblist);
    maybe_resort_list (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void gncDistribListSetPercentageTotal (
    GncDistributionList *distriblist,
    gint percentage_total)
{
    if (!distriblist) return;
    if (distriblist->percentage_total == percentage_total) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->percentage_total = percentage_total;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetSharesLabelSettlement (
    GncDistributionList *distriblist,
    const char *shares_label_settlement)
{
    if (!distriblist) return;
    SET_STR (distriblist, distriblist->shares_label_settlement, shares_label_settlement);
    mark_distriblist (distriblist);
    maybe_resort_list (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetSharesTotal (
    GncDistributionList *distriblist,
    gint shares_total)
{
    if (!distriblist) return;
    if (distriblist->shares_total == shares_total) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->shares_total = shares_total;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

void
gncDistribListSetType (
    GncDistributionList *distriblist,
    GncDistributionListType type)
{
    if (!distriblist) return;
    if (distriblist->type == type) return;
    gncDistribListBeginEdit (distriblist);
    distriblist->type = type;
    mark_distriblist (distriblist);
    gncDistribListCommitEdit (distriblist);
}

// Get Functions

const char
*gncDistribListGetDescription (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return NULL;
    return distriblist->description;
}

GList
*gncDistribListGetLists (
    QofBook *book)
{
    struct _book_info *book_info;
    if (!book) return NULL;

    book_info = qof_book_get_data (book, _GNC_MOD_NAME);
    return book_info->lists;
}

const char
*gncDistribListGetName (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return NULL;
    return distriblist->name;
}

/* GList */
/* *gncDistribListGetOwners ( */
/*     GncOwner *owner) */
/* { */
/*     struct _owner_info *owner_info; */
/*     if (!owner) return NULL; */

/*     owner_info = qof_owner_get_data (owner, _GNC_MOD_NAME); */
/*     return owner_info->lists; */
/* } */

GncDistributionList
*gncDistribListGetParent (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return NULL;
    return distriblist->parent;
}

const char
*gncDistribListGetPercentageLabelSettlement (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->percentage_label_settlement;
}

gint
gncDistribListGetPercentageTotal (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->percentage_total;
}

gint64
gncDistribListGetRefcount (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->refcount;
}

const char
*gncDistribListGetSharesLabelSettlement (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->shares_label_settlement;
}

gint
gncDistribListGetSharesTotal (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->shares_total;
}

GncDistributionListType
gncDistribListGetType (
    const GncDistributionList *distriblist)
{
    if (!distriblist) return 0;
    return distriblist->type;
}

GncDistributionList
*gncDistribListReturnChild (
    GncDistributionList *distriblist,
    gboolean make_new)
{
    GncDistributionList *child = NULL;

    if (!distriblist) return NULL;
    if (distriblist->child) return distriblist->child;
    if (distriblist->parent || distriblist->invisible) return distriblist;
    if (make_new)
    {
        child = gncDistribListCopy (distriblist);
        gncDistribListSetChild (distriblist, child);
        gncDistribListSetParent (child, distriblist);
    }
    return child;
}

//Helper Functions

GncDistributionList
*gncDistribListLookupByName (
    QofBook *book,
    const char *name)
{
    GList *list = gncDistribListGetLists (book);

    for ( ; list; list = list->next)
    {
        GncDistributionList *distriblist = list->data;
        if (!g_strcmp0 (distriblist->name, name))
            return list->data;
    }
    return NULL;
}

// Comparison Functions

int
gncDistribListCompare (
    const GncDistributionList *a,
    const GncDistributionList *b)
{
    int ret;

    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    ret = g_strcmp0 (a->name, b->name);
    if (ret) return ret;

    return g_strcmp0 (a->description, b->description);
}

gboolean
gncDistribListIsDirty (const GncDistributionList *distriblist)
{
    if (!distriblist) return FALSE;
    return qof_instance_get_dirty_flag(distriblist);
}

gboolean
gncDistribListEqual(
    const GncDistributionList *a,
    const GncDistributionList *b)
{
    if (a == NULL && b == NULL) return TRUE;
    if (a == NULL || b == NULL) return FALSE;

    g_return_val_if_fail(GNC_IS_DISTRIBLIST(a), FALSE);
    g_return_val_if_fail(GNC_IS_DISTRIBLIST(b), FALSE);

    if (g_strcmp0(a->name, b->name) != 0)
    {
        PWARN("Names differ: %s vs %s", a->name, b->name);
        return FALSE;
    }

    if (g_strcmp0(a->description, b->description) != 0)
    {
        PWARN("Descriptions differ: %s vs %s", a->description, b->description);
        return FALSE;
    }

    if (a->invisible != b->invisible)
    {
        PWARN("Invisible flags differ");
        return FALSE;
    }

    if (a->percentage_label_settlement != b->percentage_label_settlement)
    {
        PWARN("Percentage label settlement differ: %s vs %s", a->percentage_label_settlement, b->percentage_label_settlement);
        return FALSE;
    }

    if (a->percentage_total != b->percentage_total)
    {
        PWARN("Percentage total differ: %d vs %d", a->percentage_total, b->percentage_total);
        return FALSE;
    }

    if (a->shares_label_settlement != b->shares_label_settlement)
    {
        PWARN("Shares label settlement differ: %s vs %s", a->shares_label_settlement, b->shares_label_settlement);
        return FALSE;
    }

    if (a->shares_total != b->shares_total)
    {
        PWARN("Shares total differ: %d vs %d", a->shares_total, b->shares_total);
        return FALSE;
    }

    if (a->type != b->type)
    {
        PWARN("Types differ");
        return FALSE;
    }

    return TRUE;
}

gboolean
gncDistribListIsFamily (
    const GncDistributionList *a,
    const GncDistributionList *b)
{
    if (!gncDistribListCompare (a, b))
        return TRUE;
    else
        return FALSE;
}

// deprecated
