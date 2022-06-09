/********************************************************************\
 * gncCoOwner.c -- the Core CoOwner Interface                     *
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

/*
 * Copyright (C) 2022 Ralf Zerres
 * Author: Ralf Zerres <ralf.zerres@mail.de>
 */

#include <config.h>

#include <glib.h>
#include <string.h>
#include <qofinstance-p.h>

#include "gncAddressP.h"
#include "gncBillTermP.h"
#include "gncInvoice.h"
#include "gncBusiness.h"

#include "gncBusiness.h"
#include "gncCoOwner.h"
#include "gncCoOwnerP.h"
#include "gncJobP.h"
#include "gncTaxTableP.h"

static gint coowner_qof_event_handler_id = 0;
static void coowner_handle_qof_events (QofInstance *entity, QofEventId event_type,
    gpointer user_data, gpointer event_data);

//void qofCoOwnerSetAddr (GncCoOwner *coowner, QofInstance *addr_ent);
//static const char* qofCoOwnerGetTaxIncluded(const GncCoOwner *coowner);
//void qofCoOwnerSetTaxIncluded(GncCoOwner *coowner, const char* type_string);

struct _gncCoOwner
{
    QofInstance inst;

    /* Generic properties: all business entities */
    const char *acl;
    gboolean active;
    GncAddress *addr;
    gnc_numeric *balance;
    Account *ccard_acc;
    gnc_commodity *currency;
    gnc_numeric credit;
    gnc_numeric discount;
    GList *jobs;
    const char *id;
    const char *language;
    const char *notes;
    const char *name;
    GncBillTerm *terms;
    GncTaxIncluded tax_included;
    gboolean tax_table_override;
    GncTaxTable *tax_table;

    /* Specific entity properties: 'coowner' */
    gnc_numeric apt_share;
    const char *distribution_key;
    GncAddress *shipaddr;
    const char *tenant;
};

struct _gncCoOwnerClass
{
    QofInstanceClass parent_class;
};

static QofLogModule log_module = GNC_MOD_BUSINESS;

#define _GNC_MOD_NAME        GNC_ID_COOWNER


/* ============================================================== */
/* misc inline funcs */

static inline void mark_coowner (GncCoOwner *coowner);
void mark_coowner (GncCoOwner *coowner)
{
    qof_instance_set_dirty(&coowner->inst);
    qof_event_gen (&coowner->inst, QOF_EVENT_MODIFY, NULL);
}

/* ============================================================== */

enum
{
    PROP_0,
    PROP_ACL,                   /* Table */
    PROP_ACTIVE,                /* Table */
    PROP_ADDRESS,               /* Table, 8 fields */
    PROP_APT_SHARE,             /* Table (numeric) */
    PROP_CURRENCY,              /* Table */
    PROP_CCARD,                 /* Table */
    PROP_DISTRIBUTION_KEY,      /* Table */
    PROP_ID,                    /* Table */
    PROP_LANGUAGE,              /* Table */
    PROP_LAST_POSTED,           /* KVP */
    PROP_NOTES,                 /* Table */
    PROP_NAME,                  /* Table */
    PROP_PAYMENT_LAST_ACCT,     /* KVP */
    PROP_PDF_DIRNAME,           /* KVP */
    PROP_PROPERTY_UNIT,         /* Table */
    PROP_TAXTABLE,              /* Table */
    PROP_TAXTABLE_OVERRIDE,     /* Table */
    PROP_TAX_INCLUDED,          /* Table */
    PROP_TAX_INCLUDED_STR,      /* Alternate setter for PROP_TAX_INCLUDED */
    PROP_TENANT,                /* Table */
};

/* GObject Initialization */
G_DEFINE_TYPE(GncCoOwner, gnc_coowner, QOF_TYPE_INSTANCE);

static void
gnc_coowner_dispose(GObject *coownerp)
{
    G_OBJECT_CLASS(gnc_coowner_parent_class)->dispose(coownerp);
}

static void
gnc_coowner_finalize(GObject* coownerp)
{
    G_OBJECT_CLASS(gnc_coowner_parent_class)->finalize(coownerp);
}

/* Note that g_value_set_object() refs the object, as does
 * g_object_get(). But g_object_get() only unrefs once when it disgorges
 * the object, leaving an unbalanced ref, which leaks. So instead of
 * using g_value_set_object(), use g_value_take_object() which doesn't
 * ref the object when used in get_property().
 */
static void
gnc_coowner_get_property (
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
    GncCoOwner *coowner;
    g_return_if_fail(GNC_IS_COOWNER(object));
    coowner = GNC_COOWNER(object);
    switch (prop_id)
    {
    case PROP_ACTIVE:
        g_value_set_boolean(value, coowner->active);
        break;
    case PROP_ACL:
        g_value_set_string(value, coowner->acl);
        break;
    case PROP_ADDRESS:
        g_value_take_object(value, coowner->addr);
        break;
    case PROP_APT_SHARE:
        g_value_set_boxed(value, &coowner->apt_share);
        break;
    case PROP_CCARD:
        g_value_take_object(value, coowner->ccard_acc);
        break;
    case PROP_CURRENCY:
        g_value_take_object(value, coowner->currency);
        break;
    case PROP_DISTRIBUTION_KEY:
        g_value_set_string(value, coowner->distribution_key);
        break;
    case PROP_LANGUAGE:
        g_value_set_string(value, coowner->language);
        break;
    case PROP_ID:
        g_value_set_string(value, coowner->id);
        break;
    case PROP_NAME:
        g_value_set_string(value, coowner->id);
        break;
    case PROP_NOTES:
        g_value_set_string(value, coowner->notes);
        break;
    case PROP_LAST_POSTED:
        qof_instance_get_kvp (QOF_INSTANCE (coowner), value, 1, LAST_POSTED_TO_ACCT);
        break;
    case PROP_PAYMENT_LAST_ACCT:
        qof_instance_get_kvp (QOF_INSTANCE (coowner), value, 2, GNC_PAYMENT, GNC_LAST_ACCOUNT);
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_get_kvp (QOF_INSTANCE (coowner), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    case PROP_PROPERTY_UNIT:
        g_value_set_string(value, coowner->property_unit);
        break;
    case PROP_TAXTABLE:
        g_value_take_object(value, coowner->tax_table);
        break;
    case PROP_TAX_INCLUDED:
        g_value_set_int(value, coowner->tax_included);
        break;
    case PROP_TENANT:
        g_value_set_string(value, coowner->tenant);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gnc_coowner_init(GncCoOwner* coowner)
{
}

static void
gnc_coowner_set_property (
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
    GncCoOwner *coowner;
    g_return_if_fail(GNC_IS_COOWNER(object));
    coowner = GNC_COOWNER(object);
    g_assert (qof_instance_get_editlevel(coowner));

    switch (prop_id)
    {
    case PROP_ACL:
        gncCoOwnerSetAcl(coowner, g_value_get_string(value));
        break;
    case PROP_ACTIVE:
        gncCoOwnerSetActive(coowner, g_value_get_boolean(value));
        break;
    case PROP_ADDRESS:
        qofCoOwnerSetAddr(coowner, g_value_get_object(value));
        break;
    case PROP_APT_SHARE:
        gncCoOwnerSetAptShare(coowner, *(gnc_numeric*)g_value_get_boxed(value));
        break;
    case PROP_CCARD:
        gncCoOwnerSetCCard(coowner, g_value_get_object(value));
        break;
    case PROP_CURRENCY:
        gncCoOwnerSetCurrency(coowner, g_value_get_object(value));
        break;
    case PROP_DISTRIBUTION_KEY:
        gncCoOwnerSetDistributionKey(coowner, g_value_get_string(value));
        break;
    case PROP_ID:
        gncCoOwnerSetID(coowner, g_value_get_string(value));
        break;
    case PROP_LANGUAGE:
        gncCoOwnerSetLanguage(coowner, g_value_get_string(value));
        break;
    case PROP_LAST_POSTED:
        qof_instance_set_kvp (QOF_INSTANCE (coowner), value, 1, LAST_POSTED_TO_ACCT);
        break;
    case PROP_NOTES:
        gncCoOwnerSetNotes(coowner, g_value_get_string(value));
        break;
    case PROP_PAYMENT_LAST_ACCT:
        qof_instance_set_kvp (QOF_INSTANCE (coowner), value, 2, GNC_PAYMENT, GNC_LAST_ACCOUNT);
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_set_kvp (QOF_INSTANCE (coowner), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    case PROP_PROPERTY_UNIT:
        gncCoOwnerSetPropertyUnit(coowner, g_value_get_string(value));
        break;
    case PROP_TAX_INCLUDED:
        gncCoOwnerSetTaxIncluded(coowner, (GncTaxIncluded)g_value_get_int(value));
        break;
    case PROP_TENANT:
        gncCoOwnerSetTenant(coowner, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/** Return display name for this object */
static gchar*
impl_get_display_name(const QofInstance* inst)
{
    GncCoOwner* coowner;

    g_return_val_if_fail(inst != NULL, FALSE);
    g_return_val_if_fail(GNC_IS_COOWNER(inst), FALSE);

    coowner = GNC_COOWNER(inst);
    /* XXX internationalization of "Co-Owner" */
    return g_strdup_printf("Co-Owner %s", coowner->name);
}

/** Does this object refer to a specific object */
static gboolean
impl_refers_to_object(const QofInstance* inst, const QofInstance* ref)
{
    GncCoOwner* coowner;

    g_return_val_if_fail(inst != NULL, FALSE);
    g_return_val_if_fail(GNC_IS_COOWNER(inst), FALSE);

    coowner = GNC_COOWNER(inst);

    if (GNC_IS_BILLTERM(ref))
    {
        return (coowner->terms == GNC_BILLTERM(ref));
    }
    else if (GNC_IS_COMMODITY(ref))
    {
        return (coowner->currency == GNC_COMMODITY(ref));
    }
    else if (GNC_IS_ACCOUNT(ref))
    {
        return (coowner->ccard_acc == GNC_ACCOUNT(ref));
    }
    else if (GNC_IS_TAXTABLE(ref))
    {
        return (coowner->tax_table == GNC_TAXTABLE(ref));
    }

    return FALSE;
}

/** Returns a list of my type of object which refers to an object.
        For example, when called as
        qof_instance_get_typed_referring_object_list(tax_table,
        account); it will return the list of tax_tables which refer to
        a specific account.  The result should be the same regardless
        of which tax_table ojbect is used.  The list must be freed by
        the caller but the objects on the list must not.
 */
static GList*
impl_get_typed_referring_object_list(const QofInstance* inst, const QofInstance* ref)
{
    if (!GNC_IS_ACCOUNT(ref) && !GNC_IS_BILLTERM(ref)
        && !GNC_IS_TAXTABLE(ref) && !GNC_IS_COMMODITY(ref))
    {
        return NULL;
    }

    /*
    if (!GNC_IS_BILLTERM(ref) && !GNC_IS_TAXTABLE(ref))
    {
        return NULL;
    }
    */

    return qof_instance_get_referring_object_list_from_collection
      (qof_instance_get_collection(inst), ref);
}

static void
gnc_coowner_class_init (GncCoOwnerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    QofInstanceClass* qof_class = QOF_INSTANCE_CLASS(klass);

    gobject_class->dispose = gnc_coowner_dispose;
    gobject_class->finalize = gnc_coowner_finalize;
    gobject_class->get_property = gnc_coowner_get_property;
    gobject_class->set_property = gnc_coowner_set_property;

    qof_class->get_display_name = NULL;
    qof_class->get_typed_referring_object_list = impl_get_typed_referring_object_list;
    qof_class->refers_to_object = impl_refers_to_object;

    g_object_class_install_property(
      gobject_class,
      PROP_ACL,
      g_param_spec_string ("acl",
                           "CoOwner ACL",
                           "The acl is an arbitrary string "
                           "assigned by the user which provides ??? "
                           " for the coowner.",
                           NULL,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_ACTIVE,
      g_param_spec_boolean ("active",
                            "Active",
                            "TRUE if the Co-Owner is active.  FALSE if inactive.",
                            FALSE,
                            G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_ADDRESS,
      g_param_spec_object ("address",
                           "Address",
                           "The address property contains the address information for this Co-Owner.",
                           GNC_TYPE_ADDRESS,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_APT_SHARE,
      g_param_spec_boxed("apt_share",
                         "Apartment share",
                         "Apartment share of the Co-Owner in the property.",
                         GNC_TYPE_NUMERIC,
                         G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_CCARD,
      g_param_spec_object ("credit-card-account",
                           "Credit card account",
                           "The credit card account for this Co-Owner.",
                           GNC_TYPE_ACCOUNT,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_CURRENCY,
      g_param_spec_object ("currency",
                           "Currency",
                           "The currency property denotes the currency used by this Co-Owner.",
                           GNC_TYPE_COMMODITY,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
       gobject_class,
       PROP_DISTRIBUTION_KEY,
       g_param_spec_string ("distribution-key",
                            "Distribution Key",
                            "Distribution key to be applied for the Co-Owner share.",
                            NULL,
                            G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_LANGUAGE,
      g_param_spec_string ("language",
                           "Co-Owners Language",
                           "The language is an arbitrary string "
                           "assigned by the user which provides the language spoken "
                           " by the CO-Owner.",
                           NULL,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_ID,
      g_param_spec_string ("id",
                           "Co-Owner ID",
                           "The Co-Owner ID is an arbitrary string "
                           "assigned by the user which provides the Co-Owner "
                           "ID.",
                           NULL,
                           G_PARAM_READWRITE));

    g_object_class_install_property(
       gobject_class,
       PROP_LAST_POSTED,
       g_param_spec_boxed("invoice-last-posted-account",
                          "Invoice Last Posted Account",
                          "The last account to which an invoice belonging to "
                          "this Co-Owner was posted.",
                          GNC_TYPE_GUID,
                          G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_LANGUAGE,
      g_param_spec_string ("language",
                           "Co-Owners language",
                           "The language is an arbitrary string "
                           "assigned by the user which provides the language spoken "
                           " by the Co-Owner.",
                           NULL,
                           G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_NAME,
     g_param_spec_string ("name",
                          "Co-Owners name",
                          "The Co-Owner is an arbitrary string "
                          "assigned by the user which provides the "
                          "co-owner name.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_NOTES,
     g_param_spec_string ("notes",
                          "Co-Owner notes",
                          "The Co-Owner notes is an arbitrary string "
                          "assigned by the user to add extra information about the Co-Owner.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property(
       gobject_class,
       PROP_PAYMENT_LAST_ACCT,
       g_param_spec_boxed("payment-last-account",
                          "Payment Last Account",
                          "The last account to which an payment belonging to "
                          "this Co-Owner was posted.",
                          GNC_TYPE_GUID,
                          G_PARAM_READWRITE));

    g_object_class_install_property(
      gobject_class,
      PROP_ADDRESS,
      g_param_spec_object ("shipaddress",
                           "Shipping Address",
                           "The shipaddress property contains the divergent address "
                           "information, that is assigned to this Co-Owner when "
                           "delivering documents.",
                           GNC_TYPE_ADDRESS,
                           G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_TAX_INCLUDED,
     g_param_spec_int  ("tax_included",
                        "Tax included",
                        "The tax-included property contains the information about "
                        "tax calculation this vendor.",
                        GNC_TAXINCLUDED_YES,       /* min */
                        GNC_TAXINCLUDED_USEGLOBAL, /* max */
                        GNC_TAXINCLUDED_USEGLOBAL, /* default */
                        G_PARAM_READWRITE));

    g_object_class_install_property(
     gobject_class,
     PROP_TENANT,
     g_param_spec_string ("tenant",
                          "Tenant Name",
                          "The active apartment unit tenant name.",
                          NULL,
                          G_PARAM_READWRITE));
}

/* Create/Destroy Functions */
GncCoOwner *gncCoOwnerCreate (QofBook *book)
{
    GncCoOwner *coowner;

    if (!book) return NULL;

    coowner = g_object_new (GNC_TYPE_COOWNER, NULL);
    qof_instance_init_data (&coowner->inst, _GNC_MOD_NAME, book);

    coowner->active = TRUE;
    coowner->acl = CACHE_INSERT ("");
    coowner->addr = gncAddressCreate (book, &coowner->inst);
    coowner->apt_share = gnc_numeric_zero();
    coowner->balance = NULL;
    coowner->credit = gnc_numeric_zero();
    coowner->discount = gnc_numeric_zero();
    coowner->id = CACHE_INSERT ("");
    coowner->jobs = NULL;
    coowner->language = CACHE_INSERT ("");
    coowner->name = CACHE_INSERT ("");
    coowner->notes= CACHE_INSERT ("");
    coowner->property_unit = CACHE_INSERT ("");
    coowner->tax_included= GNC_TAXINCLUDED_USEGLOBAL;
    coowner->tenant = CACHE_INSERT ("");

    if (coowner_qof_event_handler_id == 0)
        coowner_qof_event_handler_id = qof_event_register_handler (coowner_handle_qof_events, NULL);

    qof_event_gen (&coowner->inst, QOF_EVENT_CREATE, NULL);

    return coowner;
}

void gncCoOwnerDestroy (GncCoOwner *coowner)
{
    if (!coowner) return;
    qof_instance_set_destroying(coowner, TRUE);
    gncCoOwnerCommitEdit(coowner);
}

static void gncCoOwnerFree (GncCoOwner *coowner)
{
    if (!coowner) return;

    qof_event_gen (&coowner->inst, QOF_EVENT_DESTROY, NULL);

    CACHE_REMOVE (coowner->acl);
    gncAddressBeginEdit (coowner->addr);
    gncAddressDestroy (coowner->addr);
    g_free (coowner->balance);
    /* TODO: free boxed value of coowner->apt_share */
    CACHE_REMOVE (coowner->distribution_key);
    g_list_free (coowner->jobs);
    CACHE_REMOVE (coowner->language);
    CACHE_REMOVE (coowner->name);
    CACHE_REMOVE (coowner->notes);
    gncAddressBeginEdit (coowner->shipaddr);
    gncAddressDestroy (coowner->shipaddr);

    if (coowner->tax_table)
    {
        gncTaxTableDecRef (coowner->tax_table);
    }
    CACHE_REMOVE (coowner->tenant);


    /* qof_instance_release (&coowner->inst); */
    g_object_unref (coowner);
}

/* Get Functions */
const char *gncCoOwnerGetAcl (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->acl;
}

gboolean gncCoOwnerGetActive (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->active;
}

GncAddress *gncCoOwnerGetAddr (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->addr;
}

const char *gncCoOwnerGetAddrName (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return gncAddressGetName ( gncCoOwnerGetAddr (coowner));
}

gnc_numeric gncCoOwnerGetAptShare (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->apt_share;
}

const char *gncCoOwnerGetAptUnit (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->apt_unit;
}

Account *gncCoOwnerGetCCard (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->ccard_acc;
}

gnc_numeric gncCoOwnerGetCredit (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->credit;
}

gnc_commodity *gncCoOwnerGetCurrency (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->currency;
}

gnc_numeric gncCoOwnerGetDiscount (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->discount;
}

const char *gncCoOwnerGetDistributionKey (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->distribution_key;
}

const char *gncCoOwnerGetID (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->id;
}

GList *gncCoOwnerGetJoblist (const GncCoOwner *coowner, gboolean show_all)
{
    if (!coowner) return NULL;

    if (show_all)
    {
        return (g_list_copy (coowner->jobs));
    }
    else
    {
        GList *list = NULL, *iterator;
        for (iterator = coowner->jobs; iterator; iterator = iterator->next)
        {
            GncJob *j = iterator->data;
            if (gncJobGetActive (j))
                list = g_list_prepend (list, j);
        }
        return g_list_reverse (list);
    }
}

const char *gncCoOwnerGetLanguage (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->language;
    // if (!coowner) return FALSE;
    // return coowner->language_table;
}

const char *gncCoOwnerGetName (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->name;
}

const char *gncCoOwnerGetNotes (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->notes;
}

GncAddress *gncCoOwnerGetShipAddr (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->shipaddr;
}

GncTaxIncluded gncCoOwnerGetTaxIncluded (const GncCoOwner *coowner)
{
    if (!coowner) return GNC_TAXINCLUDED_USEGLOBAL;
    return coowner->tax_included;
}

gboolean gncCoOwnerGetTaxTableOverride  (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->tax_table_override;
}

GncTaxTable *gncCoOwnerGetTaxTable  (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->tax_table;
}

GncBillTerm *gncCoOwnerGetTerms (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->terms;
}

/* GncTaxIncluded gncCoOwnerGetTaxIncluded (const GncCoOwner *coowner) */
/* { */
/*     if (!coowner) return GNC_TAXINCLUDED_USEGLOBAL; */
/*     return coowner->tax_included; */
/* } */

const char *gncCoOwnerGetTenant (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->tenant;
}

/* ============================================================== */
/* Set Functions */

#define SET_STR(obj, member, str) { \
        if (!g_strcmp0 (member, str)) return; \
        gncCoOwnerBeginEdit (obj); \
        CACHE_REPLACE (member, str); \
        }

void gncCoOwnerSetAcl (GncCoOwner *coowner, const char *acl)
{
    if (!coowner) return;
    if (!acl) return;
    SET_STR(coowner, coowner->acl, acl);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetActive (GncCoOwner *coowner, gboolean active)
{
    if (!coowner) return;
    if (active == coowner->active) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->active = active;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void
qofCoOwnerSetAddr (GncCoOwner *coowner, QofInstance *addr_ent)
{
    GncAddress *addr;

    if (!coowner || !addr_ent)
    {
        return;
    }
    addr = (GncAddress*)addr_ent;
    if (addr == coowner->addr)
    {
        return;
    }
    if (coowner->addr != NULL)
    {
        gncAddressBeginEdit(coowner->addr);
        gncAddressDestroy(coowner->addr);
    }
    gncCoOwnerBeginEdit(coowner);
    coowner->addr = addr;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit(coowner);
}

void gncCoOwnerSetAptShare (GncCoOwner *coowner, gnc_numeric apt_share)
{
    if (!coowner) return;
    if (gnc_numeric_equal (apt_share, coowner->apt_share)) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->apt_share = apt_share;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetCCard (GncCoOwner *coowner, Account* ccard_acc)
{
    if (!coowner) return;
    if (ccard_acc == coowner->ccard_acc) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->ccard_acc = ccard_acc;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetCredit (GncCoOwner *coowner, gnc_numeric credit)
{
    if (!coowner) return;
    if (gnc_numeric_equal (credit, coowner->credit)) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->credit = credit;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetCurrency (GncCoOwner *coowner, gnc_commodity *currency)
{
    if (!coowner || !currency) return;
    if (coowner->currency &&
            gnc_commodity_equal (coowner->currency, currency))
        return;
    gncCoOwnerBeginEdit (coowner);
    coowner->currency = currency;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetDiscount (GncCoOwner *coowner, gnc_numeric discount)
{
    if (!coowner) return;
    if (gnc_numeric_equal (discount, coowner->discount)) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->discount = discount;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetDistributionKey (GncCoOwner *coowner, const char *distribution_key)
{
    if (!coowner) return;
    if (!distribution_key) return;
    SET_STR(coowner, coowner->distribution_key, distribution_key);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetID (GncCoOwner *coowner, const char *id)
{
    if (!coowner) return;
    if (!id) return;
    SET_STR(coowner, coowner->id, id);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetLanguage (GncCoOwner *coowner, const char *language)
{
    if (!coowner) return;
    if (!language) return;
    SET_STR(coowner, coowner->language, language);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

/* CoOwners don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by setting the name property of
 * the coowner's address.
 */
void gncCoOwnerSetName (GncCoOwner *coowner, const char *name)
{
    if (!coowner) return;
    if (!name) return;
    //gncAddressSetName (gncCoOwnerGetAddr (coowner), name);
    SET_STR(coowner, coowner->name, name);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetNotes (GncCoOwner *coowner, const char *notes)
{
    if (!coowner) return;
    if (!notes) return;
    SET_STR(coowner, coowner->notes, notes);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetPropertyUnit (GncCoOwner *coowner, const char *property_unit)
{
    if (!coowner) return;
    if (!property_unit) return;
    SET_STR(coowner, coowner->property_unit, property_unit);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetTaxIncluded (GncCoOwner *coowner, GncTaxIncluded tax_included)
{
    if (!coowner) return;
    if (tax_included == coowner->tax_included) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->tax_included = tax_included;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetTaxTableOverride (GncCoOwner *coowner, gboolean override)
{
    if (!coowner) return;
    if (coowner->tax_table_override == override) return;
    gncCoOwnerBeginEdit (coowner);
    coowner->tax_table_override = override;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetTaxTable (GncCoOwner *coowner, GncTaxTable *table)
{
    if (!coowner) return;
    if (coowner->tax_table == table) return;

    gncCoOwnerBeginEdit (coowner);
    if (coowner->tax_table)
        gncTaxTableDecRef (coowner->tax_table);
    if (table)
        gncTaxTableIncRef (table);
    coowner->tax_table = table;
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetTerms (GncCoOwner *coowner, GncBillTerm *terms)
{
    if (coowner->terms == terms) return;

    gncCoOwnerBeginEdit (coowner);
    if (coowner->terms)
        gncBillTermDecRef (coowner->terms);
    coowner->terms = terms;
    if (coowner->terms)
        gncBillTermIncRef (coowner->terms);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

void gncCoOwnerSetTenant (GncCoOwner *coowner, const char *tenant)
{
    if (!coowner) return;
    if (!tenant) return;
    SET_STR(coowner, coowner->tenant, tenant);
    mark_coowner (coowner);
    gncCoOwnerCommitEdit (coowner);
}

/* ============================================================== */
/* Get Functions */
const char * gncCoOwnerGetAcl (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->acl;
}

gboolean gncCoOwnerGetActive (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->active;
}

GncAddress * gncCoOwnerGetAddr (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->addr;
}

gnc_numeric gncCoOwnerGetAptShare (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->apt_share;
}

Account * gncCoOwnerGetCCard (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->ccard_acc;
}

gnc_numeric gncCoOwnerGetCredit (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->credit;
}

gnc_commodity * gncCoOwnerGetCurrency (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->currency;
}

gnc_numeric gncCoOwnerGetDiscount (const GncCoOwner *coowner)
{
    if (!coowner) return gnc_numeric_zero();
    return coowner->discount;
}

const char * gncCoOwnerGetDistributionKey (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->distribution_key;
}

const char * gncCoOwnerGetID (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->id;
}

GList * gncCoOwnerGetJoblist (const GncCoOwner *coowner, gboolean show_all)
{
    if (!coowner) return NULL;

    if (show_all)
    {
        return (g_list_copy (coowner->jobs));
    }
    else
    {
        GList *list = NULL, *iterator;
        for (iterator = coowner->jobs; iterator; iterator = iterator->next)
        {
            GncJob *j = iterator->data;
            if (gncJobGetActive (j))
                list = g_list_prepend (list, j);
        }
        return g_list_reverse (list);
    }
}

const char * gncCoOwnerGetLanguage (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->language;
}

/* CoOwners don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by returning the name property of
 * the coowner's address.
 */
const char * gncCoOwnerGetName (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return gncAddressGetName ( gncCoOwnerGetAddr (coowner));
}

const char * gncCoOwnerGetNotes (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->notes;
}

const char * gncCoOwnerGetPropertyUnit (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->property_unit;
}

GncTaxIncluded gncCoOwnerGetTaxIncluded (const GncCoOwner *coowner)
{
    if (!coowner) return GNC_TAXINCLUDED_USEGLOBAL;
    return coowner->tax_included;
}

gboolean gncCoOwnerGetTaxTableOverride  (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->tax_table_override;
}

GncTaxTable * gncCoOwnerGetTaxTable  (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return coowner->tax_table;
}

GncBillTerm * gncCoOwnerGetTerms (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->terms;
}

/* GncTaxIncluded gncCoOwnerGetTaxIncluded (const GncCoOwner *coowner) */
/* { */
/*     if (!coowner) return GNC_TAXINCLUDED_USEGLOBAL; */
/*     return coowner->tax_included; */
/* } */

const char * gncCoOwnerGetUsername (const GncCoOwner *coowner)
{
    if (!coowner) return NULL;
    return coowner->username;
}

/* ============================================================== */
/* Helper Functions */
static void coowner_free (QofInstance *inst)
{
    GncCoOwner *coowner = (GncCoOwner *) inst;
    gncCoOwnerFree (coowner);
}

/* Note that JobList changes do not affect the "dirtiness" of the Co-Owner */
void gncCoOwnerAddJob (GncCoOwner *coowner, GncJob *job)
{
    if (!coowner) return;
    if (!job) return;

    if (g_list_index(coowner->jobs, job) == -1)
        coowner->jobs = g_list_insert_sorted (coowner->jobs, job,
            (GCompareFunc)gncJobCompare);

    qof_event_gen (&coowner->inst, QOF_EVENT_MODIFY, NULL);
}

void gncCoOwnerBeginEdit (GncCoOwner *coowner)
{
    qof_begin_edit(&coowner->inst);
}

static void gncCoOwnerOnDone (QofInstance *inst)
{
    GncCoOwner *coowner = (GncCoOwner *) inst;
    gncAddressClearDirty (coowner->addr);
}

static void gncCoOwnerOnError (QofInstance *coowner, QofBackendError errcode)
{
    PERR("CoOwner QofBackend Failure: %d", errcode);
    gnc_engine_signal_commit_error( errcode );
}

void gncCoOwnerCommitEdit (GncCoOwner *coowner)
{
    if (!qof_commit_edit (QOF_INSTANCE(coowner))) return;
    qof_commit_edit_part2 (&coowner->inst, gncCoOwnerOnError,
       gncCoOwnerOnDone, coowner_free);
}

gboolean gncCoOwnerIsDirty (const GncCoOwner *coowner)
{
    if (!coowner) return FALSE;
    return (qof_instance_get_dirty_flag(coowner)
            || gncAddressIsDirty (coowner->addr));
}

void gncCoOwnerRemoveJob (GncCoOwner *coowner, GncJob *job)
{
    GList *node;

    if (!coowner) return;
    if (!job) return;

    node = g_list_find (coowner->jobs, job);
    if (!node)
    {
        /*    PERR ("split not in account"); */
    }
    else
    {
        coowner->jobs = g_list_remove_link (coowner->jobs, node);
        g_list_free_1 (node);
    }

    qof_event_gen (&coowner->inst, QOF_EVENT_MODIFY, NULL);
}

/* ============================================================== */
/* Other functions */

int gncCoOwnerCompare (const GncCoOwner *a, const GncCoOwner *b)
{
    if (!a && !b) return 0;
    if (!a && b) return 1;
    if (a && !b) return -1;

    return(strcmp(a->name, b->name));
}

gboolean gncCoOwnerEqual(const GncCoOwner* a, const GncCoOwner* b)
{
    if (a == NULL && b == NULL) return TRUE;
    if (a == NULL || b == NULL ) return FALSE;

    g_return_val_if_fail(GNC_IS_COOWNER(a), FALSE);
    g_return_val_if_fail(GNC_IS_COOWNER(b), FALSE);

    if (a->active != b->active)
    {
        PWARN("Active flags differ");
        return FALSE;
    }

    if (g_strcmp0(a->acl, b->acl) != 0)
    {
        PWARN("ACLs differ: %s vs %s", a->acl, b->acl);
        return FALSE;
    }

    if (!gncAddressEqual(a->addr, b->addr))
    {
        PWARN("Addresses differ");
        return FALSE;
    }

    if (!xaccAccountEqual(a->ccard_acc, b->ccard_acc, TRUE))
    {
        PWARN("Accounts differ");
        return FALSE;
    }

    if (!gncAddressEqual(a->addr, b->addr))
    {
        PWARN("Addresses differ");
        return FALSE;
    }

    if (!gnc_numeric_equal(a->apt_share, b->apt_share))
    {
        PWARN("Apartment shares differ");
        return FALSE;
    }

    if (!gnc_numeric_equal(a->credit, b->credit))
    {
        PWARN("Credit amounts differ");
        return FALSE;
    }

    if (!gnc_commodity_equal(a->currency, b->currency))
    {
        PWARN("Currencies differ");
        return FALSE;
    }

    if (!gnc_numeric_equal(a->discount, b->discount))
    {
        PWARN("Discount amounts differ");
        return FALSE;
    }

    if (g_strcmp0(a->id, b->id) != 0)
    {
        PWARN("IDs differ: %s vs %s", a->id, b->id);
        return FALSE;
    }

    if (g_strcmp0(a->language, b->language) != 0)
    {
        PWARN("Languages differ: %s vs %s", a->language, b->language);
        return FALSE;
    }

    if (g_strcmp0(a->notes, b->notes) != 0)
    {
        PWARN("Notes differ: %s vs %s", a->notes, b->notes);
        return FALSE;
    }

    if (g_strcmp0(a->property_unit, b->property_unit) != 0)
    {
        PWARN("Property units differ: %s vs %s", a->property_unit, b->property_unit);
        return FALSE;
    }

    if (!gncTaxTableEqual(a->tax_table, b->tax_table))
    {
        PWARN("tax tables differ");
        return FALSE;
    }

    if (a->tax_table_override != b->tax_table_override)
    {
        PWARN("Tax table override flags differ");
        return FALSE;
    }

    if (a->tax_included != b->tax_included)
    {
        PWARN("Tax included flags differ");
        return FALSE;
    }

    if (g_strcmp0(a->tenant, b->tenant) != 0)
    {
        PWARN("Tenant names differ: %s vs %s", a->tenant, b->tenant);
        return FALSE;
    }

    return TRUE;
}

/* Package-Private functions */

static const char * _gncCoOwnerPrintable (gpointer item)
{
    GncCoOwner *v = item;
    if (!item) return NULL;
    return gncAddressGetName(v->addr);
}

/**
 * Listen for qof events.
 *
 * - If the address of a coowner has changed, mark the coowner as dirty.
 * - If a lot related to a Co-Owner has changed, clear the Co-Owner's
 *   cached balance as it likely has become invalid.
 *
 * @param entity Entity for the event
 * @param event_type Event type
 * @param user_data User data registered with the handler
 * @param event_data Event data passed with the event.
 */
static void
coowner_handle_qof_events (QofInstance *entity, QofEventId event_type,
    gpointer user_data, gpointer event_data)
{

    /* Handle address change events */
    if ((GNC_IS_ADDRESS (entity) &&
        (event_type & QOF_EVENT_MODIFY) != 0))
    {
        if (GNC_IS_COOWNER (event_data))
        {
            GncCoOwner* coowner = GNC_COOWNER (event_data);
            gncCoOwnerBeginEdit (coowner);
            mark_coowner (coowner);
            gncCoOwnerCommitEdit (coowner);
        }
        return;
    }

    /* Handle lot change events */
    if (GNC_IS_LOT (entity))
    {
        GNCLot *lot = GNC_LOT (entity);
        GncOwner lot_owner;
        const GncOwner *end_owner = NULL;
        GncInvoice *invoice = gncInvoiceGetInvoiceFromLot (lot);

        /* Determine the owner associated with the lot */
        if (invoice)
            /* Invoice lots */
            end_owner = gncOwnerGetEndOwner (gncInvoiceGetOwner (invoice));
        else if (gncOwnerGetOwnerFromLot (lot, &lot_owner))
            /* Pre-payment lots */
            end_owner = gncOwnerGetEndOwner (&lot_owner);

        if (gncOwnerGetType (end_owner) == GNC_OWNER_COOWNER)
        {
            /* Clear the cached balance */
            GncCoOwner* coowner = gncOwnerGetCoOwner (end_owner);
            g_free (coowner->balance);
            coowner->balance = NULL;
        }
        return;
    }
}

static void
destroy_coowner_on_book_close(QofInstance *ent, gpointer data)
{
    GncCoOwner* e = GNC_COOWNER(ent);

    gncCoOwnerBeginEdit(e);
    gncCoOwnerDestroy(e);
}

/** Handles book end - frees all coowners from the book
 *
 * @param book Book being closed
 */
static void
gnc_coowner_book_end(QofBook* book)
{
    QofCollection *col;

    col = qof_book_get_collection(book, GNC_ID_COOWNER);
    qof_collection_foreach(col, destroy_coowner_on_book_close, NULL);
}

static QofObject gncCoOwnerDesc =
{
    DI(.interface_version = ) QOF_OBJECT_VERSION,
    DI(.e_type            = ) _GNC_MOD_NAME,
    DI(.type_label        = ) "CoOwner",
    DI(.create            = ) (gpointer)gncCoOwnerCreate,
    DI(.book_begin        = ) NULL,
    DI(.book_end          = ) gnc_coowner_book_end,
    DI(.is_dirty          = ) qof_collection_is_dirty,
    DI(.mark_clean        = ) qof_collection_mark_clean,
    DI(.foreach           = ) qof_collection_foreach,
    DI(.printable         = ) _gncCoOwnerPrintable,
    DI(.version_cmp       = ) (int (*)(gpointer, gpointer)) qof_instance_version_cmp,
};

gboolean gncCoOwnerRegister (void)
{
    static QofParam params[] =
    {
        { QOF_PARAM_ACTIVE, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncCoOwnerGetActive, (QofSetterFunc)gncCoOwnerSetActive },
        {
            COOWNER_ACL, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetAcl,
            (QofSetterFunc)gncCoOwnerSetAcl
        },
        {
            COOWNER_ADDR, GNC_ID_ADDRESS, (QofAccessFunc)gncCoOwnerGetAddr,
            (QofSetterFunc)qofCoOwnerSetAddr },
        {
            COOWNER_APT_SHARE, QOF_TYPE_NUMERIC, (QofAccessFunc)gncCoOwnerGetAptShare,
            (QofSetterFunc)gncCoOwnerSetAptShare
        },
        { QOF_PARAM_BOOK, QOF_ID_BOOK, (QofAccessFunc)qof_instance_get_book, NULL },
        {
            COOWNER_CC, GNC_ID_ACCOUNT, (QofAccessFunc)gncCoOwnerGetCCard,
            (QofSetterFunc)gncCoOwnerSetCCard },
        {
            COOWNER_CREDIT, QOF_TYPE_NUMERIC, (QofAccessFunc)gncCoOwnerGetCredit,
            (QofSetterFunc)gncCoOwnerSetCredit
        },
        {
            COOWNER_DISCOUNT, QOF_TYPE_NUMERIC, (QofAccessFunc)gncCoOwnerGetDiscount,
            (QofSetterFunc)gncCoOwnerSetDiscount
        },
        {
            COOWNER_DISTRIBUTION_KEY, QOF_TYPE_STRING,
            (QofAccessFunc)gncCoOwnerGetDistributionKey,
            (QofSetterFunc)gncCoOwnerSetAcl
        },
        {
            QOF_PARAM_GUID, QOF_TYPE_GUID,
            (QofAccessFunc)qof_instance_get_guid, NULL
        },
        {
            COOWNER_ID, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetID,
            (QofSetterFunc)gncCoOwnerSetID
        },
        {
            COOWNER_LANGUAGE, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetLanguage,
            (QofSetterFunc)gncCoOwnerSetLanguage
        },
        {
            COOWNER_NAME, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetName,
            (QofSetterFunc)gncCoOwnerSetName
        },
        {
            COOWNER_NOTES, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetNotes,
            (QofSetterFunc)gncCoOwnerSetNotes
        },
        {
            COOWNER_PROPERTY_UNIT, QOF_TYPE_STRING, (QofAccessFunc)gncCoOwnerGetPropertyUnit,
            (QofSetterFunc)gncCoOwnerSetPropertyUnit
        },
        /* { */
        /*     COOWNER_TAX_INCLUDED, QOF_TYPE_STRING, (QofAccessFunc)qofCoOwnerGetTaxIncluded, */
        /*     (QofSetterFunc)gncCoOwnerSetTaxIncluded */
        /* }, */
        {
            COOWNER_TAXTABLE_OVERRIDE, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncCoOwnerGetTaxTableOverride,
            (QofSetterFunc)gncCoOwnerSetTaxTableOverride
        },
        {
            COOWNER_TAXTABLE, GNC_ID_TAXTABLE, (QofAccessFunc)gncCoOwnerGetTaxTable,
            (QofSetterFunc)gncCoOwnerSetTaxTable
        },
        {
            COOWNER_TERMS, GNC_ID_BILLTERM, (QofAccessFunc)gncCoOwnerGetTerms,
            (QofSetterFunc)gncCoOwnerSetTerms
        },
        {
            COOWNER_TENANT, QOF_TYPE_STRING,
            (QofAccessFunc)gncCoOwnerGetTenant,
            (QofSetterFunc)gncCoOwnerSetTenant
        },
        { NULL },
    };

    if (!qof_choice_add_class(GNC_ID_INVOICE, GNC_ID_COOWNER, INVOICE_OWNER))
    {
        return FALSE;
    }
    if (!qof_choice_add_class(GNC_ID_JOB, GNC_ID_COOWNER, JOB_OWNER))
    {
        return FALSE;
    }

    qof_class_register (_GNC_MOD_NAME, (QofSortFunc)gncCoOwnerCompare, params);
    if (!qof_choice_create(GNC_ID_COOWNER))
    {
        return FALSE;
    }
    /* temp */
    _gncCoOwnerPrintable(NULL);
    return qof_object_register (&gncCoOwnerDesc);
}

gchar *gncCoOwnerNextID (QofBook *book)
{
    return qof_book_increment_and_format_counter (book, _GNC_MOD_NAME);
}

const gnc_numeric*
gncCoOwnerGetCachedBalance (GncCoOwner *coowner)
{
    return coowner->balance;
}

void gncCoOwnerSetCachedBalance (GncCoOwner *coowner, const gnc_numeric *new_bal)
{
    if (!new_bal)
    {
        if (coowner->balance)
        {
            g_free (coowner->balance);
            coowner->balance = NULL;
        }
        return;
    }

    if (!coowner->balance)
        coowner->balance = g_new0 (gnc_numeric, 1);

    *coowner->balance = *new_bal;
}
