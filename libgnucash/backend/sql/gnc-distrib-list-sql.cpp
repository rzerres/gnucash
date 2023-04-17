/********************************************************************\
 * gnc-distrib-list-sql.c -- distribution list sql backend          *
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

/** @file gnc-distrib-list-sql.c
 *  @brief load and save distribution list data to SQL
 *  @author Copyright (c) 2022 Ralf Zerres <ralf.zerres@mail.de>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring distribution list data to/from an SQL database
 */
#include <guid.hpp>
extern "C"
{
#include <config.h>

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "gncDistributionListP.h"
#include "qof.h"
}

#include <string>
#include <vector>
#include <algorithm>

#include "gnc-sql-connection.hpp"
#include "gnc-sql-backend.hpp"
#include "gnc-sql-object-backend.hpp"
#include "gnc-sql-column-table-entry.hpp"
#include "gnc-slots-sql.h"
#include "gnc-distrib-list-sql.h"

#define _GNC_MOD_NAME   GNC_ID_DISTRIBLIST

static QofLogModule log_module = G_LOG_DOMAIN;

#define MAX_NAME_LEN 2048
#define MAX_DESCRIPTION_LEN 2048
#define MAX_TYPE_LEN 2048
#define MAX_LABEL_SETTLEMENT_LEN 2048

static void set_invisible (gpointer data, gboolean value);
static gpointer distriblist_get_parent (gpointer data);
static void distriblist_set_parent (gpointer data, gpointer value);
static void distriblist_set_parent_guid (gpointer data, gpointer value);

#define TABLE_NAME "distriblists"
#define TABLE_VERSION 2

static EntryVec col_table
{
    gnc_sql_make_table_entry<CT_GUID>("guid", 0, COL_NNUL | COL_PKEY, "guid"),
    gnc_sql_make_table_entry<CT_BOOLEAN>("active", 0, COL_NNUL, "active"),
    gnc_sql_make_table_entry<CT_STRING>("name", MAX_NAME_LEN, COL_NNUL, "name"),
    gnc_sql_make_table_entry<CT_STRING>(
        "description",
        MAX_DESCRIPTION_LEN,
        COL_NNUL,
        GNC_DISTRIBLIST_DESCRIPTION,
        true),
    gnc_sql_make_table_entry<CT_INT>(
        "refcount",
        0,
        COL_NNUL,
        (QofAccessFunc)gncDistribListGetRefcount,
        (QofSetterFunc)gncDistribListSetRefcount),
    gnc_sql_make_table_entry<CT_BOOLEAN>(
        "invisible",
        0,
        COL_NNUL,
        (QofAccessFunc)gncDistribListGetInvisible,
        (QofSetterFunc)set_invisible),
    // gnc_sql_make_table_entry<CT_OWNERREF>(
    //     "owner",
    //     0,
    //     0,
    //     (QofAccessFunc)gncDistribListGetOwner,
    //     (QofSetterFunc)gncDistribListSetOwner),
    gnc_sql_make_table_entry<CT_STRING>(
        "owner_typename",
        0,
        0,
        (QofAccessFunc)gncDistribListGetOwnerTypeName,
        (QofSetterFunc)gncDistribListSetOwnerTypeName),
    gnc_sql_make_table_entry<CT_GUID>(
        "parent",
        0,
        0,
        (QofAccessFunc)distriblist_get_parent,
        (QofSetterFunc)distriblist_set_parent),
    gnc_sql_make_table_entry<CT_STRING>(
        "percentage_label_settlement",
        MAX_LABEL_SETTLEMENT_LEN,
        COL_NNUL,
        GNC_DISTRIBLIST_PERCENTAGE_LABEL_SETTLEMENT,
        true),
    gnc_sql_make_table_entry<CT_INT>(
        "percentage_total",
        0,
        0,
        GNC_DISTRIBLIST_PERCENTAGE_TOTAL,
        true),
    gnc_sql_make_table_entry<CT_STRING>(
        "shares_label_settlement",
        MAX_LABEL_SETTLEMENT_LEN,
        COL_NNUL,
        GNC_DISTRIBLIST_SHARES_LABEL_SETTLEMENT,
        true),
    gnc_sql_make_table_entry<CT_INT>(
        "shares_total",
        0,
        0,
        GNC_DISTRIBLIST_SHARES_TOTAL,
        true),
    gnc_sql_make_table_entry<CT_STRING>(
        "type",
        MAX_TYPE_LEN,
        COL_NNUL,
        GNC_DISTRIBLIST_TYPE,
        true),
};

static EntryVec distriblist_parent_col_table
{
    gnc_sql_make_table_entry<CT_GUID>(
        "parent",
        0,
        0,
        nullptr,
        distriblist_set_parent_guid),
};

GncSqlDistribListBackend::GncSqlDistribListBackend() :
        GncSqlObjectBackend(TABLE_VERSION, GNC_ID_DISTRIBLIST,
                            TABLE_NAME, col_table) {}

struct DistribListParentGuid
{
    GncDistributionList* distriblist;
    GncGUID guid;
    bool have_guid;
};

using DistribListParentGuidPtr = DistribListParentGuid*;
using DistribListParentGuidVec = std::vector<DistribListParentGuidPtr>;

static void
set_invisible (gpointer data, gboolean value)
{
    GncDistributionList *distriblist = GNC_DISTRIBLIST (data);

    g_return_if_fail (distriblist != NULL);

    if (value)
    {
        gncDistribListMakeInvisible (distriblist);
    }
}

static  gpointer
distriblist_get_parent (gpointer pObject)
{
    const GncDistributionList* distriblist;
    const GncDistributionList* pParent;
    const GncGUID* parent_guid;

    g_return_val_if_fail (pObject != NULL, NULL);
    g_return_val_if_fail (GNC_IS_DISTRIBLIST (pObject), NULL);

    distriblist = GNC_DISTRIBLIST (pObject);
    pParent = gncDistribListGetParent (distriblist);
    if (pParent == NULL)
    {
        parent_guid = NULL;
    }
    else
    {
        parent_guid = qof_instance_get_guid (QOF_INSTANCE (pParent));
    }

    return (gpointer)parent_guid;
}

static void
distriblist_set_parent (gpointer data, gpointer value)
{
    GncDistributionList* distriblist;
    GncDistributionList* parent;
    QofBook* pBook;
    GncGUID* guid = (GncGUID*)value;

    g_return_if_fail (data != NULL);
    g_return_if_fail (GNC_IS_DISTRIBLIST (data));

    distriblist = GNC_DISTRIBLIST (data);
    pBook = qof_instance_get_book (QOF_INSTANCE (distriblist));
    if (guid != NULL)
    {
        parent = gncDistribListLookup (pBook, guid);
        if (parent != NULL)
        {
            gncDistribListSetParent (distriblist, parent);
            gncDistribListSetChild (parent, distriblist);
        }
    }
}

static void
distriblist_set_parent_guid (gpointer pObject,  gpointer pValue)
{
    g_return_if_fail (pObject != NULL);
    g_return_if_fail (pValue != NULL);

    auto s = static_cast<DistribListParentGuidPtr>(pObject);
    s->guid = *static_cast<GncGUID*>(pValue);
    s->have_guid = true;
}

static GncDistributionList*
load_single_distriblist (
    GncSqlBackend* sql_be,
    GncSqlRow& row,
    DistribListParentGuidVec& l_distriblists_needing_parents)
{
    g_return_val_if_fail (sql_be != NULL, NULL);

    auto guid = gnc_sql_load_guid (sql_be, row);
    auto pDistribList = gncDistribListLookup (sql_be->book(), guid);
    if (pDistribList == nullptr)
    {
        pDistribList = gncDistribListCreate (sql_be->book());
    }
    gnc_sql_load_object (
        sql_be, row, GNC_ID_DISTRIBLIST, pDistribList, col_table);

    /* If the distriblist doesn't have a parent, it might be because
       it hasn't been loaded yet.  If so, add this distriblist to the
       list of distriblists with no parent, along with the parent
       GncGUID so that after they are all loaded, the parents can be
       fixed up. */
    if (gncDistribListGetParent (pDistribList) == NULL)
    {
        DistribListParentGuid s;

        s.distriblist = pDistribList;
        s.have_guid = false;
        gnc_sql_load_object (sql_be, row, GNC_ID_DISTRIBLIST, &s,
                             distriblist_parent_col_table);
        if (s.have_guid)
            l_distriblists_needing_parents.push_back(new DistribListParentGuid(s));

    }

    qof_instance_mark_clean (QOF_INSTANCE (pDistribList));

    return pDistribList;
}

/* Because gncDistribListLookup has the arguments backwards: */
static inline GncDistributionList*
gnc_distriblist_lookup (const GncGUID *guid, const QofBook *book)
{
     QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_DISTRIBLIST, GncDistributionList);
}

void
GncSqlDistribListBackend::load_all (GncSqlBackend* sql_be)
{

    g_return_if_fail (sql_be != NULL);

    std::string sql("SELECT * FROM " TABLE_NAME);
    auto stmt = sql_be->create_statement_from_sql(sql);
    auto result = sql_be->execute_select_statement(stmt);
    DistribListParentGuidVec l_distriblists_needing_parents;

    for (auto row : *result)
    {
        load_single_distriblist (sql_be, row, l_distriblists_needing_parents);
    }
    delete result;
    std::string pkey(col_table[0]->name());
    sql = "SELECT DISTINCT ";
    sql += pkey + " FROM " TABLE_NAME;
    gnc_sql_slots_load_for_sql_subquery (
        sql_be,
        sql,
        (BookLookupFn)gnc_distriblist_lookup);

    /* While there are items on the list of distriblists needing
       parents, try to see if the parent has now been loaded.  Theory
       says that if items are removed from the front and added to the
       back if the parent is still not available, then eventually, the
       list will shrink to size 0. */
    if (!l_distriblists_needing_parents.empty())
    {
        bool progress_made = true;
        std::reverse(l_distriblists_needing_parents.begin(),
                     l_distriblists_needing_parents.end());
        auto end = l_distriblists_needing_parents.end();
        while (progress_made)
        {
            progress_made = false;
            end = std::remove_if(
                l_distriblists_needing_parents.begin(),
                end,
                [&](DistribListParentGuidPtr s)
                {
                  auto pBook = qof_instance_get_book (
                      QOF_INSTANCE (s->distriblist));
                  auto parent = gncDistribListLookup (
                      pBook, &s->guid);
                  if (parent != nullptr)
                    {
                      gncDistribListSetParent (s->distriblist, parent);
                      gncDistribListSetChild (parent, s->distriblist);
                      progress_made = true;
                      delete s;
                      return true;
                    }
                  return false;
                });
        }
    }
}

/* ================================================================= */

static void
do_save_distriblist (QofInstance* inst, void* p2)
{
    auto data = static_cast<write_objects_t*>(p2);
    data->commit(inst);
}

bool
GncSqlDistribListBackend::write (GncSqlBackend* sql_be)
{
    g_return_val_if_fail (sql_be != NULL, FALSE);

    write_objects_t data {sql_be, true, this};
    qof_object_foreach (GNC_ID_DISTRIBLIST, sql_be->book(), do_save_distriblist, &data);
    return data.is_ok;
}

/* ================================================================= */
void
GncSqlDistribListBackend::create_tables (GncSqlBackend* sql_be)
{
    gint version;

    g_return_if_fail (sql_be != NULL);

    version = sql_be->get_table_version( TABLE_NAME);
    if (version == 0)
    {
        sql_be->create_table(TABLE_NAME, TABLE_VERSION, col_table);
    }
    else if (version < m_version)
    {
        /* Upgrade 64 bit int handling */
        sql_be->upgrade_table(TABLE_NAME, col_table);
        sql_be->set_table_version (TABLE_NAME, TABLE_VERSION);

        PINFO ("Distributon lists table upgraded from version 1 to version %d\n",
               TABLE_VERSION);
    }
}

/* ================================================================= */

template<> void
GncSqlColumnTableEntryImpl<CT_DISTRIBLISTREF>::load (
    const GncSqlBackend* sql_be,
    GncSqlRow& row,
    QofIdTypeConst obj_name,
    gpointer pObject) const noexcept
{
    load_from_guid_ref(
        row, obj_name, pObject,
        [sql_be](GncGUID* g){
          return gncDistribListLookup(sql_be->book(), g);
        });
}

template<> void
GncSqlColumnTableEntryImpl<CT_DISTRIBLISTREF>::add_to_table(
    ColVec& vec) const noexcept
{
    add_objectref_guid_to_table(vec);
}

template<> void
GncSqlColumnTableEntryImpl<CT_DISTRIBLISTREF>::add_to_query(
    QofIdTypeConst obj_name,
    const gpointer pObject,
    PairVec& vec) const noexcept
{
    add_objectref_guid_to_query(obj_name, pObject, vec);
}

/* ========================== END OF FILE ===================== */
