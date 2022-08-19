/*********************************************************************\
 * gncCoOwner.h -- the Core CoOwner Interface                        *
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
/** @addtogroup Business
    @{ */
/** @addtogroup CoOwner
Implements CoOwner objects.

You may create new entities, or edit exiting ones.
Deletion isn't allowed, as soon as transactions are assined to a
given entity (e.g. invoices, settlements, credit notes or jobs ).


    @{ */
/** @file gncCoOwner.h
    @brief Core CoOwner Object
    @author Copyright (C) 2022 Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef GNC_COOWNER_H_
#define GNC_COOWNER_H_

/** @struct GncCoOwner

Generic params (identical to all business objects, e.g. Customers, Employees, Vendors):

@param  QofInstance inst;
@param  char* id;

@param  gboolean active;
@param  GncAddress* addr;
@param  gnc_numeric balance;
@param  Account* ccard_acc;
@param  gnc_commodity *currency;
@param  GList *jobs;
@param  char *language;
@param  char *notes;
@param  char *name;
@param  GncBillTerm *coowner_terms;
@param  GncTaxTable *taxtable;
@param  gboolean taxtable_override;
@param  GncTaxIncluded tax_included;

CoOwner specific params:

@param  gnc_numeric apt_share;
@param  char *apt_unit;
@param  gnc_numeric credit;
@param  gnc_numeric discount;
@param  char *distibution_key;
@param  char *property_unit;
@param  GncAddress *ship_addr;
@param  gboolean tenant_active;
@param  GncAddress *tenant_addr;
@param  char *tenant_id;
@param  char *tenant_name;
@param  char *tenant_notes;
*/

typedef struct _gncCoOwner GncCoOwner;
typedef struct _gncCoOwnerClass GncCoOwnerClass;

#include "gncAddress.h"
#include "gncBillTerm.h"
#include "gncTaxTable.h"
#include "gncJob.h"

#define GNC_ID_COOWNER "gncCoOwner"

/* --- type macros --- */
#define GNC_TYPE_COOWNER            (gnc_coowner_get_type ())
#define GNC_COOWNER(o)              \
     (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_COOWNER, GncCoOwner))
#define GNC_COOWNER_CLASS(k)        \
     (G_TYPE_CHECK_CLASS_CAST((k), GNC_TYPE_COOWNER, GncCoOwnerClass))
#define GNC_IS_COOWNER(o)           \
     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_COOWNER))
#define GNC_IS_COOWNER_CLASS(k)     \
     (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_COOWNER))
#define GNC_COOWNER_GET_CLASS(o)    \
     (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_COOWNER, GncCoOwnerClass))
GType gnc_coowner_get_type(void);

/** @name Create/Destroy Functions
 @{ */
GncCoOwner *gncCoOwnerCreate (QofBook *book);
void gncCoOwnerDestroy (GncCoOwner *coowner);
//** @} */

/** @name Get Functions
 @{ */
QofBook * gncCoOwnerGetBook (GncCoOwner *coowner);
const char * gncCoOwnerGetID (const GncCoOwner *coowner);
const char * gncCoOwnerGetAcl (const GncCoOwner *coowner);
gboolean gncCoOwnerGetActive (const GncCoOwner *coowner);
const char * gncCoOwnerGetAddrName (const GncCoOwner *coowner);
GncAddress * gncCoOwnerGetAddr (const GncCoOwner *coowner);
gnc_numeric gncCoOwnerGetAptShare (const GncCoOwner *coowner);
const char * gncCoOwnerGetAptUnit (const GncCoOwner *coowner);
Account * gncCoOwnerGetCCard (const GncCoOwner *coowner);
gnc_numeric gncCoOwnerGetCredit (const GncCoOwner *coowner);
gnc_commodity * gncCoOwnerGetCurrency (const GncCoOwner *coowner);
gnc_numeric gncCoOwnerGetDiscount (const GncCoOwner *coowner);
const char * gncCoOwnerGetDistributionKey (const GncCoOwner *coowner);
const char * gncCoOwnerGetLanguage (const GncCoOwner *coowner);
const char * gncCoOwnerGetName (const GncCoOwner *coowner);
const char * gncCoOwnerGetNotes (const GncCoOwner *coowner);
GncTaxIncluded gncCoOwnerGetTaxIncluded (const GncCoOwner *coowner);
GncAddress * gncCoOwnerGetShipAddr (const GncCoOwner *coowner);
GncBillTerm * gncCoOwnerGetTerms (const GncCoOwner *coowner);
gboolean gncCoOwnerGetTaxTableOverride (const GncCoOwner *coowner);
GncTaxTable* gncCoOwnerGetTaxTable (const GncCoOwner *coowner);
gboolean gncCoOwnerGetTenantActive (const GncCoOwner *coowner);
GncAddress * gncCoOwnerGetTenantAddr (const GncCoOwner *coowner);
const char * gncCoOwnerGetTenantID (const GncCoOwner *coowner);
const char * gncCoOwnerGetTenantName (const GncCoOwner *coowner);
const char * gncCoOwnerGetTenantNotes (const GncCoOwner *coowner);
//** @} */

/** @name Set Functions
 @{ */
void gncCoOwnerSetID (GncCoOwner *coowner, const char *id);
void gncCoOwnerSetAcl (GncCoOwner *coowner, const char *acl);
void gncCoOwnerSetActive (GncCoOwner *coowner, gboolean active);
void qofCoOwnerSetAddr (GncCoOwner *coowner, QofInstance *addr_ent);
void gncCoOwnerSetAptShare (GncCoOwner *coowner, gnc_numeric apt_share);
void gncCoOwnerSetAptUnit (GncCoOwner *coowner, const char *apt_unit);
void gncCoOwnerSetCCard (GncCoOwner *coowner, Account* ccard_acc);
void gncCoOwnerSetCredit (GncCoOwner *coowner, gnc_numeric credit);
void gncCoOwnerSetCurrency (GncCoOwner *coowner, gnc_commodity * currency);
void gncCoOwnerSetDiscount (GncCoOwner *coowner, gnc_numeric credit);
void gncCoOwnerSetDistributionKey (GncCoOwner *coowner, const char *distribution_key);
void gncCoOwnerSetLanguage (GncCoOwner *coowner, const char *language);
void gncCoOwnerSetName (GncCoOwner *coowner, const char *name);
void gncCoOwnerSetNotes (GncCoOwner *coowner, const char *notes);
void qofCoOwnerSetShipAddr (GncCoOwner *coowner, QofInstance *shipaddr_ent);
void gncCoOwnerSetTerms (GncCoOwner *coowner, GncBillTerm *coowner_terms);
void gncCoOwnerSetTaxIncluded (GncCoOwner *coowner, GncTaxIncluded tax_included);
void gncCoOwnerSetTaxTableOverride (GncCoOwner *coowner, gboolean override);
void gncCoOwnerSetTaxTable (GncCoOwner *coowner, GncTaxTable *table);
void gncCoOwnerSetTenantActive (GncCoOwner *coowner, gboolean tenant_active);
void qofCoOwnerSetTenantAddr (GncCoOwner *coowner, QofInstance *addraddr_ent);
void gncCoOwnerSetTenantID (GncCoOwner *coowner, const char *tenant_id);
void gncCoOwnerSetTenantName (GncCoOwner *coowner, const char *tenant_name);
void gncCoOwnerSetTenantNotes (GncCoOwner *coowner, const char *tenant_notes);

void gncCoOwnerAddJob (GncCoOwner *coowner, GncJob *job);
void gncCoOwnerBeginEdit (GncCoOwner *coowner);
void gncCoOwnerCommitEdit (GncCoOwner *coowner);
void gncCoOwnerRemoveJob (GncCoOwner *coowner, GncJob *job);
//** @} */

/** Return values:
 *   <0   => a is less then b
 *   >0   => b is less then a
 *   NULL => a equals b
 */
int gncCoOwnerCompare (const GncCoOwner *a, const GncCoOwner *b);

/** Return a pointer to the instance gncCoOwner that is identified
 *  by the guid, and is residing in the book. Returns NULL if the
 *  instance can not be found.
 *  Equivalent function prototype is
 *  GncCoOwner * gncCoOwnerLookup (QofBook *book, const GncGUID *guid);
 */
static inline GncCoOwner * gncCoOwnerLookup (const QofBook *book, const GncGUID *guid)
{
    QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_COOWNER, GncCoOwner);
}

/** Constants used as identifier keys */
#define COOWNER_ID                "id"
#define COOWNER_ACL               "acl"
#define COOWNER_ACTIVE            "active"
#define COOWNER_ADDR              "addr"
#define COOWNER_APT_SHARE         "apt_share"
#define COOWNER_APT_UNIT          "apt_unit"
#define COOWNER_CC                "credit_card_account"
#define COOWNER_CREDIT            "credit"
#define COOWNER_CURRENCY          "currency"
#define COOWNER_DISCOUNT          "discount"
#define COOWNER_DISTRIBUTION_KEY  "distribution_key"
#define COOWNER_LANGUAGE          "language"
#define COOWNER_NAME              "name"
#define COOWNER_NOTES             "notes"
#define COOWNER_SHIP_ADDRESS      "ship_addr"
#define COOWNER_SLOTS             "values"
#define COOWNER_TAX_INCLUDED      "tax_included"
#define COOWNER_TAXTABLE_OVERRIDE "tax_table_override"
#define COOWNER_TAXTABLE          "tax_table"
#define COOWNER_TENANT_ACTIVE     "tenant_active"
#define COOWNER_TENANT_ADDRESS    "tenant_addr"
#define COOWNER_TENANT_ID         "tenant_id"
#define COOWNER_TENANT_NAME       "tenant_name"
#define COOWNER_TENANT_NOTES      "tenant_notes"
#define COOWNER_TERMS             "coowner_terms"

/** @deprecated functions, should be removed */
#define gncCoOwnerGetGUID(x) qof_instance_get_guid(QOF_INSTANCE(x))
#define gncCoOwnerRetGUID(x) (x ? *(qof_instance_get_guid(QOF_INSTANCE(x))) : *(guid_null()))
#define gncCoOwnerGetBook(x) qof_instance_get_book(QOF_INSTANCE(x))
#define gncCoOwnerLookupDirect(g,b) gncCoOwnerLookup((b), &(g))


/** Test support function, used by test-dbi-business-stuff.c */
gboolean gncCoOwnerEqual(const GncCoOwner* e1, const GncCoOwner* e2);
GList * gncCoOwnerGetJoblist (const GncCoOwner *coowner, gboolean show_all);
gboolean gncCoOwnerIsDirty (const GncCoOwner *coowner);

#endif /* GNC_COOWNER_H_ */
/** @} */
/** @} */
