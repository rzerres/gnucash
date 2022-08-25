/********************************************************************\
 * gncDistributionList.h -- the Gnucash distribution list interface *
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
/** @addtogroup Business
    @{ */
/** @addtogroup DistributionList

This file implements the engine methods needed to manage
::Distribution Lists objects.

A distributon list is used to save attributes that are needed, when
account balances are to be split proportionally into co-owner
settlements.

Each list defines the 100% share value (the numerator) as the
calculation base. You assign the appropriate unit share value inside
the co-owners objects, which in term is referenced to calculate the
costs to be settled to this apartment unit (the devisor).

    @{ */
/** @file gncDistributionList.h
    @brief Distribution list interface
    @author Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef GNC_DISTRIBLIST_H_
#define GNC_DISTRIBLIST_H_

/** @struct GncDistributionList

@param  QofInstance inst - The instance entity.
@param  const char *name - Pointer to the name of the distribution list.
@param  const char *descrition - Pointer to the description of the distribution list.
@param  GncDistributionListType type - The type of the distribution list.
@param  GncOwnerType owner-type - The owner type that is assigned to selectable list members.
@param  const char *percentage_label_stettlement - Label of the distribution list.
@param  gint percentage-total - Total percentage per property unit (the numerator).
@param  const char *shares_label_stettlement - Label of the distribution list.
@param  gint shares-total - Total shares per property unit (the numerator).
//@param  Glist owners-assigned - List of assigned owner entities considerd when calculationg the costs to be settled.
*/
typedef struct _gncDistributionList GncDistributionList;

/** @struct GncDistributionListClass

@param  QofInstanceClass parent_class - The parent entity class.
*/
typedef struct _gncDistributionListClass GncDistributionListClass;

#include "qof.h"
#include "gncOwner.h"
#include "gncBusiness.h"

#define GNC_ID_DISTRIBLIST "gncDistribList"

/* --- type macros --- */
#define GNC_TYPE_DISTRIBLIST (gnc_distriblist_get_type ())
#define GNC_DISTRIBLIST(o) \
     (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_DISTRIBLIST, GncDistributionList))
#define GNC_DISTRIBLIST_CLASS(k) \
     (G_TYPE_CHECK_CLASS_CAST((k), GNC_TYPE_DISTRIBLIST, GncDistributionListClass))
#define GNC_IS_DISTRIBLIST(o) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_DISTRIBLIST))
#define GNC_IS_DISTRIBLIST_CLASS(k) \
     (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_DISTRIBLIST))
#define GNC_DISTRIBLIST_GET_CLASS(o) \
     (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_DISTRIBLIST, GncDistributionListClass))
GType gnc_distriblist_get_type(void);

/** @name DistributonList parameter names
 @{ */
#define GNC_DISTRIBLIST_DESCRIPTION "description"
#define GNC_DISTRIBLIST_NAME "name"
#define GNC_DISTRIBLIST_OWNER "owner"
#define GNC_DISTRIBLIST_PERCENTAGE_LABEL_SETTLEMENT "percentage label settlement"
#define GNC_DISTRIBLIST_PERCENTAGE_TOTAL "percentage total"
#define GNC_DISTRIBLIST_REFCOUNT "reference counter"
#define GNC_DISTRIBLIST_SHARES_LABEL_SETTLEMENT "shares label settlement"
#define GNC_DISTRIBLIST_SHARES_TOTAL "shares total"
#define GNC_DISTRIBLIST_TYPE "distribution list type"
#define GNC_DISTRIBLIST_OWNERS "distribution list owner"
/** @} */

/**
 * The type is to be interpreted as a VALUE.
 * NOTE: This enum /depends/ on starting at value 1
 */
#ifndef SWIG
#define ENUM_DISTRIBLIST_TYPE(_)  \
 _(GNC_DISTRIBLIST_TYPE_SHARES,=1) \
 _(GNC_DISTRIBLIST_TYPE_PERCENTAGE,)

DEFINE_ENUM(GncDistributionListType, ENUM_DISTRIBLIST_TYPE)
#else
typedef enum
{
    GNC_DISTRIBLIST_TYPE_SHARES = 1,
    GNC_DISTRIBLIST_TYPE_PERCENTAGE,
} GncDistributionListType;
#endif

/** @name Create/Destroy Functions
 @{ */
GncDistributionList *gncDistribListCreate (QofBook *book);
void gncDistribListDecRef (GncDistributionList *distriblist);
void gncDistribListDestroy (GncDistributionList *distriblist);
void gncDistribListIncRef (GncDistributionList *distriblist);

void gncDistribListBeginEdit (GncDistributionList *distriblist);
void gncDistribListChanged (GncDistributionList *distriblist);
void gncDistribListCommitEdit (GncDistributionList *distriblist);
/** @} */

/** @name Set Functions
@{
*/
void gncDistribListSetDescription (GncDistributionList *distriblist, const char *name);
void gncDistribListSetName (GncDistributionList *distriblist, const char *name);
void gncDistribListSetOwner (GncDistributionList *distriblist, GncOwner *owner);
//void gncDistribListSetOwners (GncDistributionList *distriblist, GList owners);
void gncDistribListSetPercentageLabelSettlement (GncDistributionList *distriblist, const char *percentage_label_settlement);
void gncDistribListSetPercentageTotal (GncDistributionList *distriblist, gint percentage_total);
void gncDistribListSetSharesLabelSettlement (GncDistributionList *distriblist, const char *shares_label_settlement);
void gncDistribListSetSharesTotal (GncDistributionList *distriblist, gint shares_total);
void gncDistribListSetType (GncDistributionList *distriblist, GncDistributionListType type);

/** @} */

/** @name Get Functions
 @{ */
#define gncDistribListGetChild(t) gncDistribListReturnChild((t),FALSE)
const char *gncDistribListGetDescription (const GncDistributionList *distriblist);
GList * gncDistribListGetLists (QofBook *book);
const char *gncDistribListGetName (const GncDistributionList *distriblist);
GncOwner *gncDistribListGetOwner (GncDistributionList *distriblist);
//GList *gncDistribListGetOwners (GncOwner *owner);
GncDistributionList *gncDistribListGetParent (const GncDistributionList *distriblist);
const char *gncDistribListGetPercentageLabelSettlement (const GncDistributionList *distriblist);
gint gncDistribListGetPercentageTotal (const GncDistributionList *distriblist);
gint64 gncDistribListGetRefcount (const GncDistributionList *distriblist);
const char *gncDistribListGetSharesLabelSettlement (const GncDistributionList *distriblist);
gint gncDistribListGetSharesTotal (const GncDistributionList *distriblist);
GncDistributionListType gncDistribListGetType (const GncDistributionList *distriblist);

GncDistributionList *gncDistribListReturnChild (GncDistributionList *distriblist, gboolean make_new);
/** @} */

/** @name Helper Functions
 @{ */
/** Return a pointer to the instance `gncDistributionList` that is identified
 *  by the guid, and is residing in the book. Returns NULL if the
 *  instance can't be found.
 *  Equivalent function prototype is
 *  GncDistributionList *gncDistribListLookup (QofBook *book, const GncGUID *guid);
 */
static inline GncDistributionList *gncDistribListLookup (const QofBook *book, const GncGUID *guid)
{
    QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_DISTRIBLIST, GncDistributionList);
}

GncDistributionList *gncDistribListLookupByName (QofBook *book, const char *name);

/** @name Comparison Functions
 @{ */
/** Compare distributon lists on their name for sorting. */
int gncDistribListCompare (const GncDistributionList *a, const GncDistributionList *b);

/** Check if dirty, which states that attributes have changed. */
gboolean gncDistribListIsDirty (const GncDistributionList *distriblist);

/** Check if all internal fields of a and b match. */
gboolean gncDistribListEqual(const GncDistributionList *a, const GncDistributionList *b);
/** Check only if the distribution list are "family". This is the case if
 *  - a and b are the same distribution list
 *  - a is b's parent or vice versa
 *  - a and be are children of the same parent
 *
 *  In practice, this check if performed by comparing the distribution list's names.
 *  This is required to be unique per parent/children group.
 */
gboolean gncDistribListIsFamily (const GncDistributionList *a, const GncDistributionList *b);
/** @} */

/* deprecated */
#define gncDistribListGetGUID(x) qof_instance_get_guid (QOF_INSTANCE(x))
/** @deprecated functions, should be removed */
#define gncDistribListRetGUID(x) (x ? *(qof_instance_get_guid(QOF_INSTANCE(x))) : *(guid_null()))
#define gncDistribListGetBook(x) qof_instance_get_book(QOF_INSTANCE(x))
#define gncDistribListLookupDirect(g,b) gncCoOwnerLookup((b), &(g))

#endif /* GNC_DISTRIBLIST_H_ */
/** @} */
/** @} */
