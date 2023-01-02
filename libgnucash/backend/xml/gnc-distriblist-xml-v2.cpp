/********************************************************************\
 * gnc-distlist-xml-v2.c -- distibution list xml i/o implementation *
 *                                                                  *
 * Copyright (C) 2002 Derek Atkins <warlord@MIT.EDU>                *
 * Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>             *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/
extern "C"
{
#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "gncDistributionListP.h"
#include "gncDistributionListP.h"
// #include "gncOwner.h"
#include "qof.h"
}

#include "gnc-xml-helper.h"

#include "sixtp.h"
#include "sixtp-utils.h"
#include "sixtp-parsers.h"
#include "sixtp-utils.h"
#include "sixtp-dom-parsers.h"
#include "sixtp-dom-generators.h"

#include "gnc-xml.h"
#include "io-gncxml-gen.h"
#include "io-gncxml-v2.h"
// #include "gnc-owner-xml-v2.h"
#include "gnc-distriblist-xml-v2.h"

#include "xml-helpers.h"

#define _GNC_MOD_NAME   GNC_ID_DISTRIBLIST

static QofLogModule log_module = GNC_MOD_IO;

const gchar* distriblist_version_string = "2.0.0";

/*************************************************\
 * constants
\*************************************************/
#define distriblist_guid_string "distriblist:guid"
#define distriblist_name_string "distriblist:name"
#define distriblist_desc_string "distriblist:description"
#define distriblist_refcount_string "distriblist:refcount"
#define distriblist_invisible_string "distriblist:invisible"
#define distriblist_parent_string "distriblist:parent"
#define distriblist_child_string "distriblist:child"
#define distriblist_slots_string "distriblist:slots"
// #define distriblist_owner_string "distriblist:owner"
#define distriblist_owner_typename_string "distriblist:owner_typename"

#define percentage_labelsettlement_string "dl-percentage:label_settlement"
#define percentage_owner_string "distriblist:owner"
#define percentage_percentagetotal_string "dl-percentage:percentage_total"
#define shares_labelsettlement_string "dl-shares:label_settlement"
#define shares_owner_string "distriblist:owner"
#define shares_sharestotal_string "dl-shares:shares_total"

#define gnc_distriblist_string "gnc:GncDistribList"
#define gnc_percentagetype_string "distriblist:percentage"
#define gnc_sharestype_string "distriblist:shares"

/*************************************************\
 * Function prototypes declaration
 * (assume C11 semantics, where order matters)
\*************************************************/
static void
distriblist_add_xml (QofInstance* distriblist_p, gpointer out_p);
static gboolean
distriblist_child_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_desc_handler (xmlNodePtr node, gpointer distriblist_pdata);
static int distriblist_get_count (QofBook *book);
static xmlNodePtr
distriblist_dom_tree_create (GncDistributionList* distriblist);
static gboolean
distriblist_end_handler (
    gpointer data_for_children,
    GSList *data_from_children, GSList* sibling_data,
    gpointer parent_data, gpointer global_data,
    gpointer *result, const gchar* tag);
static GncDistributionList
*distriblist_find_senior (GncDistributionList *distriblist);
static gboolean
distriblist_guid_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_invisible_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_is_grandchild (GncDistributionList* distriblist);
static gboolean
distriblist_name_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean distriblist_ns (FILE *out);
static gboolean
distriblist_parent_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_percentage_data_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_refcount_handler (xmlNodePtr node, gpointer distriblist_pdata);
static void
distriblist_reset_refcount (gpointer key, gpointer value, gpointer notused);
static void
distriblist_scrub_cb (QofInstance *distriblist_p, gpointer list_p);
static void distriblist_scrub (QofBook *book);
static gboolean
distriblist_shares_data_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_slots_handler (xmlNodePtr node, gpointer distriblist_pdata);
static sixtp* distriblist_sixtp_parser_create (void);
static gboolean distriblist_write (FILE *out, QofBook *book);
// static gboolean
// owner_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
distriblist_owner_typename_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
percentage_labelsettlement_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
percentage_percentagetotal_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
shares_labelsettlement_handler (xmlNodePtr node, gpointer distriblist_pdata);
static gboolean
shares_sharestotal_handler (xmlNodePtr node, gpointer distriblist_pdata);

/*************************************************      \
 * Private Functions
\*************************************************/
/* helper functions */
struct distriblist_pdata
{
    GncDistributionList* distriblist;
    QofBook* book;
};

static void
do_count (QofInstance* distriblist_p, gpointer count_p)
{
    int *count = static_cast<decltype (count)> (count_p);
    (*count)++;
}

static gboolean
set_int (
    xmlNodePtr node,
    GncDistributionList *distriblist,
    void (*func) (GncDistributionList*, gint))
{
    gint64 val;
    dom_tree_to_integer (node, &val);
    func (distriblist, val);
    return TRUE;
}

static gboolean
set_parent_child (
    xmlNodePtr node,
    struct distriblist_pdata *pdata,
    void (*func) (GncDistributionList*, GncDistributionList*))
{
    GncGUID *guid;
    GncDistributionList *distriblist;

    guid = dom_tree_to_guid (node);
    g_return_val_if_fail (guid, FALSE);
    distriblist = gncDistribListLookup (pdata->book, guid);
    if (!distriblist)
    {
        distriblist = gncDistribListCreate (pdata->book);
        gncDistribListBeginEdit (distriblist);
        gncDistribListSetGUID (distriblist, guid);
        gncDistribListCommitEdit (distriblist);
    }
    guid_free (guid);
    g_return_val_if_fail (distriblist, FALSE);
    func (pdata->distriblist, distriblist);

    return TRUE;
}

static gboolean
set_string (
    xmlNodePtr node,
    GncDistributionList* distriblist,
    void (*func) (GncDistributionList*, const char*))
{
    char *txt = dom_tree_to_text (node);
    g_return_val_if_fail (txt, FALSE);
    func (distriblist, txt);
    g_free (txt);
    return TRUE;
}

static gboolean
set_numeric (
    xmlNodePtr node,
    GncDistributionList* distriblist,
    void (*func) (GncDistributionList*, gnc_numeric))
{
    gnc_numeric *num = dom_tree_to_gnc_numeric (node);
    g_return_val_if_fail (num, FALSE);

    func (distriblist, *num);
    g_free (num);
    return TRUE;
}

/* private dom tree functions */
static xmlNodePtr
distriblist_dom_tree_create (GncDistributionList* distriblist)
{
    xmlNodePtr ret;
    xmlNodePtr data;
    // GncOwner *owner;

    ret = xmlNewNode (NULL, BAD_CAST gnc_distriblist_string);
    xmlSetProp (ret, BAD_CAST "version", BAD_CAST distriblist_version_string);

    maybe_add_guid (ret, distriblist_guid_string, QOF_INSTANCE (distriblist));
    xmlAddChild (ret, text_to_dom_tree (
        distriblist_name_string,
        gncDistribListGetName (distriblist)));
    xmlAddChild (ret, text_to_dom_tree (
        distriblist_desc_string,
        gncDistribListGetDescription (distriblist)));

    xmlAddChild (ret, int_to_dom_tree (
        distriblist_refcount_string,
        gncDistribListGetRefcount (distriblist)));
    xmlAddChild (ret, int_to_dom_tree (
        distriblist_invisible_string,
        gncDistribListGetInvisible (distriblist)));

    // write back the string value of the assigned owner type
    // (GNC_ID_[OWNER] -> e.g "gncCoOwner")
    // Hint: we do not reference to an owner guid , so id is empty
    xmlAddChild ( ret, text_to_dom_tree (
        distriblist_owner_typename_string,
        gncDistribListGetOwnerTypeName (distriblist)));

    // owner = gncDistribListGetOwner (distriblist);
    // DEBUG ("Write string value of owner '%s' ('%i')\n",
    //        gncOwnerTypeToQofIdType(owner->type),
    //        owner->type);
    // //if (owner && owner->owner.undefined != NULL)
    // if (owner)
    //     xmlAddChild (ret, gnc_owner_to_dom_tree (percentage_owner_string, owner));

    // xmlAddChild won't do anything with a NULL, so tests are superfluous.
    // xmlAddChild (ret, qof_instance_slots_to_dom_tree (
    //    distriblist_slots_string,
    //    QOF_INSTANCE (distriblist)));

    /* We should not be our own child */
    if (gncDistribListGetChild (distriblist) != distriblist)
    {
        maybe_add_guid (
            ret,
            distriblist_child_string,
            QOF_INSTANCE (gncDistribListGetChild (distriblist)));
        maybe_add_guid (
            ret,
            distriblist_parent_string,
            QOF_INSTANCE (gncDistribListGetParent (distriblist)));
    }

    switch (gncDistribListGetType (distriblist))
    {
    case GNC_DISTRIBLIST_TYPE_PERCENTAGE:
    {
        data = xmlNewChild (
            ret, NULL, BAD_CAST gnc_percentagetype_string, NULL);

        // write back the label to be used in settlements
        maybe_add_string (
            data,
            percentage_labelsettlement_string,
            gncDistribListGetPercentageLabelSettlement (distriblist));

        // write back the percentage total value
        maybe_add_int (
            data,
            percentage_percentagetotal_string,
            gncDistribListGetPercentageTotal (distriblist));
        break;
    }
    case GNC_DISTRIBLIST_TYPE_SHARES:
    {
        data = xmlNewChild (
            ret, NULL, BAD_CAST gnc_sharestype_string, NULL);

        // write back the label to be used in settlements
        maybe_add_string (
            data,
            shares_labelsettlement_string,
            gncDistribListGetSharesLabelSettlement (distriblist));

        // write back the shares total value
        maybe_add_int (
            data,
            shares_sharestotal_string,
            gncDistribListGetSharesTotal (distriblist));
        break;
    }
    }

    return ret;
}

static struct
dom_tree_handler percentage_data_handlers_v2[] =
{
    {
        percentage_labelsettlement_string,
        percentage_labelsettlement_handler,
        0,
        0
    },
    {
        percentage_percentagetotal_string,
        percentage_percentagetotal_handler,
        0,
        0
    },
    { NULL, 0, 0, 0 }
};

static struct
dom_tree_handler shares_data_handlers_v2[] =
{
    {
        shares_labelsettlement_string,
        shares_labelsettlement_handler,
        0,
        0
    },
    {
        shares_sharestotal_string,
        shares_sharestotal_handler,
        0,
        0
    },
    { NULL, 0, 0, 0 }
};

static struct
dom_tree_handler distriblist_handlers_v2[] =
{
    {
        distriblist_child_string,
        distriblist_child_handler,
        0,
        0
    },
    {
        distriblist_desc_string,
        distriblist_desc_handler,
        1,
        0
    },
    {
        distriblist_guid_string,
        distriblist_guid_handler,
        1,
        0
    },
    {
        distriblist_invisible_string,
        distriblist_invisible_handler,
        1,
        0
    },
    {
        distriblist_name_string,
        distriblist_name_handler,
        1,
        0
    },
    // {
    //     distriblist_owner_string,
    //     owner_handler,
    //     0,
    //     0
    // },
    {
        distriblist_owner_typename_string,
        distriblist_owner_typename_handler,
        0,
        0
    },
    {
        distriblist_parent_string,
        distriblist_parent_handler,
        0,
        0
    },
    {
        gnc_percentagetype_string,
        distriblist_percentage_data_handler,
        0,
        0
    },
    {
        distriblist_refcount_string,
        distriblist_refcount_handler,
        1,
        0
    },
    {
        gnc_sharestype_string,
        distriblist_shares_data_handler,
        0,
        0
    },
    {
        distriblist_slots_string,
        distriblist_slots_handler,
        0,
        0
    },
    { NULL, 0, 0, 0 }
};

/**
 * Read in a dom tree to a distribution list structure.
 *
 * @param node - pointer to the node
 * @param book - pointer to the book that is assigned to this distribution list.
 */
static GncDistributionList*
dom_tree_to_distriblist (xmlNodePtr node, QofBook *book)
{
    struct distriblist_pdata distriblist_pdata;
    gboolean successful;

    distriblist_pdata.distriblist = gncDistribListCreate (book);
    distriblist_pdata.book = book;
    gncDistribListBeginEdit (distriblist_pdata.distriblist);

    successful = dom_tree_generic_parse (
        node, distriblist_handlers_v2, &distriblist_pdata);

    if (successful)
    {
        gncDistribListCommitEdit (distriblist_pdata.distriblist);
    }
    else
    {
        PERR ("failed to parse distriblist tree");
        gncDistribListDestroy (distriblist_pdata.distriblist);
        distriblist_pdata.distriblist = NULL;
    }

    return distriblist_pdata.distriblist;
}

static gboolean
dom_tree_to_percentage_data (xmlNodePtr node, struct distriblist_pdata *pdata)
{
    gboolean successful;

    successful = dom_tree_generic_parse (
        node, percentage_data_handlers_v2, pdata);

    if (!successful)
        PERR ("failed to parse distribution list percentage data");

    return successful;
}

static gboolean
dom_tree_to_shares_data (xmlNodePtr node, struct distriblist_pdata *pdata)
{
    gboolean successful;

    successful = dom_tree_generic_parse (
        node, shares_data_handlers_v2, pdata);

    if (!successful)
        PERR ("failed to parse distribution list shares data");

    return successful;
}

/* distriblist handler functions */
/**
 * Add an xml representation of a distribution list to a given file.
 *
 * @param distriblist_p - pointer to the distribution list struct.
 * @param out_p - pointer to the filename that is used to store the data.
 */
static void
distriblist_add_xml (QofInstance *distriblist_p, gpointer out_p)
{
    xmlNodePtr node;
    GncDistributionList *distriblist = (GncDistributionList*) distriblist_p;
    FILE *out = static_cast<decltype (out)> (out_p);

    if (ferror (out))
        return;

    node = distriblist_dom_tree_create (distriblist);
    xmlElemDump (out, NULL, node);
    xmlFreeNode (node);
    if (ferror (out) || fprintf (out, "\n") < 0)
return;
}

static gboolean
distriblist_child_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_parent_child (node, pdata, gncDistribListSetChild);
}

static gboolean
distriblist_desc_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_string (node, pdata->distriblist, gncDistribListSetDescription);
}

static gboolean
distriblist_end_handler (
    gpointer data_for_children,
    GSList *data_from_children, GSList *sibling_data,
    gpointer parent_data, gpointer global_data,
    gpointer *result, const gchar *tag)
{
    GncDistributionList *distriblist;
    xmlNodePtr tree = (xmlNodePtr)data_for_children;
    gxpf_data *gdata = (gxpf_data*)global_data;
    QofBook *book = static_cast<decltype (book)> (gdata->bookdata);


    if (parent_data)
    {
        return TRUE;
    }

    // OK. For some messed up reason this is getting called again
    // with a NULL tag. So we ignore those cases
    if (!tag)
    {
        return TRUE;
    }

    g_return_val_if_fail (tree, FALSE);

    distriblist = dom_tree_to_distriblist (tree, book);
    if (distriblist != NULL)
    {
        gdata->cb (tag, gdata->parsedata, distriblist);
    }

    xmlFreeNode (tree);

    return distriblist != NULL;
}

static GncDistributionList
*distriblist_find_senior (GncDistributionList *distriblist)
{
    GncDistributionList *temp, *parent, *gp = NULL;

    temp = distriblist;
    do
    {
        // See if "temp" is a grandchild
        parent = gncDistribListGetParent (temp);
        if (!parent)
            break;
        gp = gncDistribListGetParent (parent);
        if (!gp)
            break;

        // Yep, this is a grandchild. Move up one generation and try again
        temp = parent;
    }
    while (TRUE);

    /* Ok, at this point temp points to the most senior child and parent
     * should point to the top distriblist (and gp should be NULL).  If
     * parent is NULL then we are the most senior child (and have no
     * children), so do nothing.  If temp == distriblist then there is no
     * grandparent, so do nothing.
     *
     * Do something if parent != NULL && temp != distriblist
     */
    g_assert (gp == NULL);

    // return the most senior distriblist
    return temp;
}

static int
distriblist_get_count (QofBook* book)
{
    int count = 0;
    qof_object_foreach (_GNC_MOD_NAME, book, do_count, (gpointer) &count);
    return count;
}

static gboolean
distriblist_guid_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata* pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    GncGUID* guid;
    GncDistributionList* distriblist;

    guid = dom_tree_to_guid (node);
    g_return_val_if_fail (guid, FALSE);
    distriblist = gncDistribListLookup (pdata->book, guid);
    if (distriblist)
    {
        DEBUG ("Assigned distribution list: %p", distriblist);
        gncDistribListDestroy (pdata->distriblist);
        pdata->distriblist = distriblist;
        gncDistribListBeginEdit (distriblist);
    }
    else
    {
        gncDistribListSetGUID (pdata->distriblist, guid);
    }

    guid_free (guid);

    return TRUE;
}

static gboolean
distriblist_invisible_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata* pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    guint val;

    dom_tree_to_guint (node, &val);
    if (val)
        gncDistribListMakeInvisible (pdata->distriblist);
    return TRUE;
}

static gboolean
distriblist_is_grandchild (GncDistributionList* distriblist)
{
    return (gncDistribListGetParent (
        gncDistribListGetParent (distriblist)) != NULL);
}

static gboolean
distriblist_name_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata* pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_string (node, pdata->distriblist, gncDistribListSetName);
}

static gboolean
distriblist_ns (FILE *out)
{
    g_return_val_if_fail (out, FALSE);
    return

    gnc_xml2_write_namespace_decl (out, "distriblist")
        && gnc_xml2_write_namespace_decl (out, "dl-percentage")
        && gnc_xml2_write_namespace_decl (out, "dl-shares");

}

static gboolean
distriblist_parent_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_parent_child (node, pdata, gncDistribListSetParent);
}

static gboolean
distriblist_percentage_data_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
  struct distriblist_pdata* pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);

    g_return_val_if_fail (node, FALSE);
//FIXME: assertion 'gncDistribListGetType (pdata->distriblist) == 0' fails
    //g_return_val_if_fail (
    //    gncDistribListGetType (pdata->distriblist) == 0, FALSE);

    gncDistribListSetType (pdata->distriblist, GNC_DISTRIBLIST_TYPE_PERCENTAGE);
    return dom_tree_to_percentage_data (node, pdata);
}

// static gboolean
// distriblist_owner_data_handler (xmlNodePtr node, gpointer distriblist_pdata)
// {
//     struct distriblist_pdata* pdata =
//         static_cast<decltype (pdata)> (distriblist_pdata);
//     GncOwner owner;
//     gboolean ret;

//     ret = gnc_dom_tree_to_owner (node, &owner, pdata->book);
//     if (ret)
//         gncDistribListSetOwner (pdata->distriblist, &owner);

//     return ret;

//     // g_return_val_if_fail (node, FALSE);

//     // gncDistribListSetOwner (pdata->distriblist, GNC_DISTRIBLIST_TYPE_PERCENTAGE);
//     // return dom_tree_to_owner_data (node, pdata);
// }

static gboolean
distriblist_refcount_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata* pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    gint64 val;

    dom_tree_to_integer (node, &val);
    gncDistribListSetRefcount (pdata->distriblist, val);
    return TRUE;
}

static void
distriblist_reset_refcount (gpointer key, gpointer value, gpointer notused)
{
    GncDistributionList *distriblist = static_cast<decltype (distriblist)> (key);
    gint32 count = GPOINTER_TO_INT (value);

    if (count != gncDistribListGetRefcount (distriblist)
        && !gncDistribListGetInvisible (distriblist))
    {
        gchar distribliststr[GUID_ENCODING_LENGTH + 1];
        guid_to_string_buff (qof_instance_get_guid (
            QOF_INSTANCE (distriblist)), distribliststr);
        PWARN ("Fixing refcount on distriblist %s (%" G_GINT64_FORMAT " -> %d)\n",
               distribliststr, gncDistribListGetRefcount (distriblist), count);
        gncDistribListSetRefcount (distriblist, count);
    }
}

// build a list of bill distriblists that are grandchildren or bogus
// (empty entry list).
static void
distriblist_scrub_cb (QofInstance* distriblist_p, gpointer list_p)
{
    GncDistributionList *distriblist = GNC_DISTRIBLIST (distriblist_p);
    GList **list = static_cast<decltype (list)> (list_p);

    if (distriblist_is_grandchild (distriblist))
    {
        *list = g_list_prepend (*list, distriblist);

    }
    else if (!gncDistribListGetType (distriblist))
    {
        GncDistributionList *t = gncDistribListGetParent (distriblist);
        if (t)
        {
            // Fix up the broken "copy" function
            gchar guidstr[GUID_ENCODING_LENGTH + 1];
            guid_to_string_buff (
                               qof_instance_get_guid (QOF_INSTANCE (distriblist)), guidstr);
            PWARN ("Fixing broken child distriblist: %s", guidstr);

            gncDistribListBeginEdit (distriblist);
            gncDistribListSetType (distriblist, gncDistribListGetType (t));
            gncDistribListSetName (distriblist, gncDistribListGetName (t));
            gncDistribListSetDescription (
                distriblist, gncDistribListGetDescription (t));
            gncDistribListSetPercentageLabelSettlement (
                distriblist, gncDistribListGetPercentageLabelSettlement (t));
            gncDistribListSetPercentageTotal (
                distriblist, gncDistribListGetPercentageTotal (t));
            gncDistribListSetSharesLabelSettlement (
                distriblist, gncDistribListGetSharesLabelSettlement (t));
            gncDistribListSetSharesTotal (
                distriblist, gncDistribListGetSharesTotal (t));
            // gncDistribListSetOwner (
            //     distriblist, gncDistribListGetOwner (t));
            gncDistribListSetOwnerTypeName (
                distriblist, gncDistribListGetOwnerTypeName (t));
            gncDistribListCommitEdit (distriblist);
        }
        else
        {
            // No parent?  Must be a standalone
            *list = g_list_prepend (*list, distriblist);
        }
    }
}

static void
distriblist_scrub (QofBook* book)
{
    GList *list = NULL;
    GList *node;
    GncDistributionList *parent;
    GncDistributionList *distriblist;
    GHashTable *ht = g_hash_table_new (g_direct_hash, g_direct_equal);

    DEBUG ("scrubbing distriblists...");
    qof_object_foreach (GNC_ID_DISTRIBLIST, book, distriblist_scrub_cb, &list);
    // TODO: once we integrate the distriblist into coowner
    //qof_object_foreach (GNC_ID_COOWNER, book, distriblist_scrub_coowner, ht);

    // destroy the list of "grandchildren" distribution lists
    for (node = list; node; node = node->next)
    {
        gchar distribliststr[GUID_ENCODING_LENGTH + 1];
        distriblist = static_cast<decltype (distriblist)> (node->data);

        guid_to_string_buff (qof_instance_get_guid (
            QOF_INSTANCE (distriblist)), distribliststr);
        PWARN ("deleting grandchild distriblist: %s\n", distribliststr);

        // Make sure the parent has no children
        parent = gncDistribListGetParent (distriblist);
        gncDistribListSetChild (parent, NULL);

        // Destroy this distribution list
        gncDistribListBeginEdit (distriblist);
        gncDistribListDestroy (distriblist);
    }

    // reset the refcounts as necessary
    g_hash_table_foreach (ht, distriblist_reset_refcount, NULL);

    g_list_free (list);
    g_hash_table_destroy (ht);
}

static gboolean
distriblist_shares_data_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    GncDistributionListType type;
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);

    type = gncDistribListGetType (pdata->distriblist);

    g_return_val_if_fail (node, FALSE);
//FIXME: assertion 'gncDistribListGetType (pdata->distriblist) == 0' fails
    //g_return_val_if_fail (
    //    gncDistribListGetType (pdata->distriblist) == 0, FALSE);

//FIXME: ignoring the test and just set the tye works out as expected.
    gncDistribListSetType (pdata->distriblist, GNC_DISTRIBLIST_TYPE_SHARES);
    return dom_tree_to_shares_data (node, pdata);
}

static gboolean
distriblist_slots_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return dom_tree_create_instance_slots (
        node, QOF_INSTANCE (pdata->distriblist));
}

static sixtp
*distriblist_sixtp_parser_create (void)
{
    return sixtp_dom_parser_new (distriblist_end_handler, NULL, NULL);
}

/**
 * Write an xml representation of a distribution list to filesystem.
 *
 * @param out - pointer to the filename that is used to store the data.
 * @param book - pointer to the book that is assigned to this distribution list.
 */
static gboolean
distriblist_write (FILE *out, QofBook* book)
{
    qof_object_foreach_sorted (
        _GNC_MOD_NAME, book, distriblist_add_xml, (gpointer) out);
    return ferror (out) == 0;
}

// static gboolean
// owner_handler (xmlNodePtr node, gpointer distriblist_pdata)
// {
//     struct distriblist_pdata* pdata =
//         static_cast<decltype (pdata)> (distriblist_pdata);
//     GncOwner owner;
//     gboolean ret;

//     ret = gnc_dom_tree_to_owner (node, &owner, pdata->book);
//     if (ret)
//       gncDistribListSetOwner ( pdata->distriblist, &owner);

//     return ret;

//     // // Get the string representation of the owner type
//     // QofIdTypeConst ownertype_name = gncOwnerGetTypeString (
//     //     gncDistribListGetOwner (pdata->distriblist));
// }

static gboolean
distriblist_owner_typename_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_string (
        node, pdata->distriblist, gncDistribListSetOwnerTypeName);
}

static gboolean
percentage_labelsettlement_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_string (
        node, pdata->distriblist, gncDistribListSetPercentageLabelSettlement);
}

static gboolean
percentage_percentagetotal_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_int (
        node, pdata->distriblist, gncDistribListSetPercentageTotal);
}

static gboolean
shares_labelsettlement_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_string (
        node, pdata->distriblist, gncDistribListSetSharesLabelSettlement);
}

static gboolean
shares_sharestotal_handler (xmlNodePtr node, gpointer distriblist_pdata)
{
    struct distriblist_pdata *pdata =
        static_cast<decltype (pdata)> (distriblist_pdata);
    return set_int (
        node, pdata->distriblist, gncDistribListSetSharesTotal);
}

/***********************************************************************/

// TODO: once we introduce distribution lists in the Co-Owner dialog
// static void
// distriblist_scrub_coowner (QofInstance* coowner_p, gpointer ht_p)
// {
//     GHashTable* ht = static_cast<decltype (ht)> (ht_p);
//     GncCoOwner* coowner = GNC_COOWNER (coowner_p);
//     GncDistributionList* distriblist;
//     gint32 count;

//     distriblist = gncCoOwnerGetDistriblists (coowner);
//     if (distriblist)
//     {
//         count = GPOINTER_TO_INT (g_hash_table_lookup (ht, distriblist));
//         count++;
//         g_hash_table_insert (ht, distriblist, GINT_TO_POINTER (count));
//         if (distriblist_is_grandchild (distriblist))
//         {
//             gchar coownerstr[GUID_ENCODING_LENGTH + 1];
//             gchar distribliststr[GUID_ENCODING_LENGTH + 1];
//             guid_to_string_buff (qof_instance_get_guid (
//                 QOF_INSTANCE (coowner)), coownerstr);
//             guid_to_string_buff (qof_instance_get_guid (
//                 QOF_INSTANCE (distriblist)), distribliststr);
//             PWARN ("co-owner %s has grandchild distriblist %s\n",
//                 coownerstr, distribliststr);
//         }
//     }
// }

/*************************************************\
 * Public Functions
\*************************************************/
void
gnc_distriblist_xml_initialize (void)
{
    static GncXmlDataType_t be_data =
    {
        GNC_FILE_BACKEND_VERS,
        gnc_distriblist_string,
        distriblist_sixtp_parser_create,
        NULL,           /* add_item */
        distriblist_get_count,
        distriblist_write,
        distriblist_scrub,
        distriblist_ns,
    };

    gnc_xml_register_backend(be_data);
}

GncDistributionList
*gnc_distriblist_xml_find_or_create (QofBook *book, GncGUID *guid)
{
    GncDistributionList *distriblist;
    gchar guidstr[GUID_ENCODING_LENGTH + 1];

    guid_to_string_buff (guid, guidstr);
    g_return_val_if_fail (book, NULL);
    g_return_val_if_fail (guid, NULL);
    distriblist = gncDistribListLookup (book, guid);
    DEBUG ("looking for distribution list %s, found %p", guidstr, distriblist);
    if (!distriblist)
    {
        distriblist = gncDistribListCreate (book);
        gncDistribListBeginEdit (distriblist);
        gncDistribListSetGUID (distriblist, guid);
        gncDistribListCommitEdit (distriblist);
        DEBUG ("Created distribution list: %p", distriblist);
    }
    else
        gncDistribListDecRef (distriblist);

    return distriblist;
}
