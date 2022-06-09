/********************************************************************\
 * gncJob.c -- the Core Job Interface                               *
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
 * Copyright (C) 2001, 2002 Derek Atkins
 * Copyright (C) 2003 Linas Vepstas <linas@linas.org>
 * Author: Derek Atkins <warlord@MIT.EDU>
 */

#include <config.h>

#include <glib.h>
#include <string.h>
#include <qofinstance-p.h>

#include "gnc-features.h"
#include "gncInvoice.h"
#include "gncJob.h"
#include "gncJobP.h"
#include "gncOwnerP.h"

struct _gncJob
{
    QofInstance inst;

    gboolean active;
    const char *billing_id;
    GList *entries;
    const char *id;
    const char *name;
    const char *desc;
    GncOwner owner;
};

struct _gncJobClass
{
    QofInstanceClass parent_class;
};

static QofLogModule log_module = GNC_MOD_BUSINESS;

#define _GNC_MOD_NAME           GNC_ID_JOB
#define GNC_JOB_RATE            "job-rate"
#define GNC_JOB_TYPE_IS_COOWNER "type_coowner"

/* ================================================================== */
/* misc inline functions */

static inline void mark_job (GncJob *job);
void mark_job (GncJob *job)
{
    qof_instance_set_dirty(&job->inst);
    qof_event_gen (&job->inst, QOF_EVENT_MODIFY, NULL);
}

/* ================================================================== */

enum
{
    PROP_0,
    PROP_ID,            /* Table */
    PROP_NAME,          /* Table */
//  PROP_REFERENCE,     /* Table */
//  PROP_ACTIVE,        /* Table */
//  PROP_OWNER_TYPE,    /* Table */
//  PROP_OWNER,         /* Table */
    PROP_PDF_DIRNAME,   /* KVP */
};

/* GObject Initialization */
G_DEFINE_TYPE(GncJob, gnc_job, QOF_TYPE_INSTANCE)

static void
gnc_job_init(GncJob* job)
{
}

static void
gnc_job_dispose(GObject *jobp)
{
    G_OBJECT_CLASS(gnc_job_parent_class)->dispose(jobp);
}

static void
gnc_job_finalize(GObject* jobp)
{
    G_OBJECT_CLASS(gnc_job_parent_class)->finalize(jobp);
}

static void
gnc_job_get_property (GObject         *object,
                      guint            prop_id,
                      GValue          *value,
                      GParamSpec      *pspec)
{
    GncJob *job;

    g_return_if_fail(GNC_IS_JOB(object));

    job = GNC_JOB(object);
    switch (prop_id)
    {
    case PROP_ID:
        g_value_set_string(value, job->id);
        break;
    case PROP_NAME:
        g_value_set_string(value, job->name);
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_get_kvp (QOF_INSTANCE (job), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gnc_job_set_property (GObject         *object,
                      guint            prop_id,
                      const GValue          *value,
                      GParamSpec      *pspec)
{
    GncJob *job;

    g_return_if_fail(GNC_IS_JOB(object));

    job = GNC_JOB(object);
    g_assert (qof_instance_get_editlevel(job));

    switch (prop_id)
    {
    case PROP_NAME:
        gncJobSetName(job, g_value_get_string(value));
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_set_kvp (QOF_INSTANCE (job), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/** Returns a list of my type of object which refers to an object.  For example, when called as
        qof_instance_get_typed_referring_object_list(taxtable, account);
    it will return the list of taxtables which refer to a specific account.  The result should be the
    same regardless of which taxtable object is used.  The list must be freed by the caller but the
    objects on the list must not.
 */
static GList*
impl_get_typed_referring_object_list(const QofInstance* inst, const QofInstance* ref)
{
    /* Refers to nothing */
    return NULL;
}

static void
gnc_job_class_init (GncJobClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    QofInstanceClass* qof_class = QOF_INSTANCE_CLASS(klass);

    gobject_class->dispose = gnc_job_dispose;
    gobject_class->finalize = gnc_job_finalize;
    gobject_class->set_property = gnc_job_set_property;
    gobject_class->get_property = gnc_job_get_property;

    qof_class->get_display_name = NULL;
    qof_class->refers_to_object = NULL;
    qof_class->get_typed_referring_object_list = impl_get_typed_referring_object_list;

    g_object_class_install_property
    (gobject_class,
     PROP_ID,
     g_param_spec_string ("ID",
                          "Job ID",
                          "The job ID is an arbitrary string "
                          "assigned by the user.  It is intended to "
                          "be displayed inside the GUI qualifieng the "
                          "jobs identifier.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_NAME,
     g_param_spec_string ("name",
                          "Job Name",
                          "The job name is an arbitrary string "
                          "assigned by the user.  It is intended to "
                          "a short character string that is displayed "
                          "by the GUI as the job mnemonic.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_PDF_DIRNAME,
     g_param_spec_string ("export-pdf-dir",
                          "Export PDF Directory Name",
                          "A subdirectory for exporting PDF reports which is "
                          "appended to the target directory when writing them "
                          "out. It is retrieved from preferences and stored on "
                          "each 'Owner' object which prints items after "
                          "printing.",
                          NULL,
                          G_PARAM_READWRITE));
}

/* Create/Destroy Functions */
GncJob *gncJobCopy (const GncJob *from)
{
    GncJob *job;
    QofBook* book;
    GValue v = G_VALUE_INIT;

    g_assert (from);
    book = qof_instance_get_book (from);
    g_assert (book);

    job = g_object_new (GNC_TYPE_JOB, NULL);
    qof_instance_init_data (&job->inst, _GNC_MOD_NAME, book);

    gncJobBeginEdit (job);

    job->active = from->active;
    job->id = CACHE_INSERT (from->id);
    job->billing_id = CACHE_INSERT (from->billing_id);

    /* Dialog marked this type as Co-Owner */
    qof_instance_get_kvp (QOF_INSTANCE (from), &v, 1, GNC_JOB_TYPE_IS_COOWNER);
    if (G_VALUE_HOLDS_INT64 (&v))
         qof_instance_set_kvp (QOF_INSTANCE (job), &v, 1, GNC_JOB_TYPE_IS_COOWNER);
    g_value_unset (&v);

    //gncOwnerCopy (&from->billto, &job->billto);
    gncOwnerCopy (&from->owner, &job->owner);

    mark_job (job);
    gncJobCommitEdit (job);

    return job;
}

GncJob *gncJobCreate (QofBook *book)
{
    GncJob *job;

    if (!book) return NULL;

    job = g_object_new (GNC_TYPE_JOB, NULL);
    qof_instance_init_data (&job->inst, _GNC_MOD_NAME, book);

    job->id = CACHE_INSERT ("");
    job->name = CACHE_INSERT ("");
    job->desc = CACHE_INSERT ("");
    job->active = TRUE;

    /* GncOwner not initialized */
    qof_event_gen (&job->inst, QOF_EVENT_CREATE, NULL);

    return job;
}

static void free_job_list (GncJob *job)
{
    gncJobBeginEdit (job);
    gncJobDestroy (job);
}

void gncJobFreeList (GList *jobs)
{
    GList *job_list = g_list_copy (jobs);
    g_list_free_full (job_list, (GDestroyNotify)free_job_list);
}

void gncJobDestroy (GncJob *job)
{
    if (!job) return;
    qof_instance_set_destroying(job, TRUE);
    gncJobCommitEdit (job);
}

JobEntryList * gncJobGetEntries (GncJob *job)
{
    if (!job) return NULL;
    return job->entries;
}

static void gncJobFree (GncJob *job)
{
    if (!job) return;

    qof_event_gen (&job->inst, QOF_EVENT_DESTROY, NULL);

    CACHE_REMOVE (job->id);
    CACHE_REMOVE (job->name);
    CACHE_REMOVE (job->desc);

    switch (gncOwnerGetType (&(job->owner)))
    {
    case GNC_OWNER_COOWNER:
        gncCoOwnerRemoveJob (gncOwnerGetCoOwner(&job->owner), job);
        break;
    case GNC_OWNER_CUSTOMER:
        gncCustomerRemoveJob (gncOwnerGetCustomer(&job->owner), job);
        break;
    case GNC_OWNER_VENDOR:
        gncVendorRemoveJob (gncOwnerGetVendor(&job->owner), job);
        break;
    default:
        break;
    }

    /* qof_instance_release (&job->inst); */
    g_object_unref (job);
}


/* ================================================================== */
/* Set Functions */

#define SET_STR(obj, member, str) { \
        if (!g_strcmp0 (member, str)) return; \
        gncJobBeginEdit (obj); \
        CACHE_REPLACE (member, str); \
        }

void gncJobSetID (GncJob *job, const char *id)
{
    if (!job) return;
    if (!id) return;
    SET_STR(job, job->id, id);
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetName (GncJob *job, const char *name)
{
    if (!job) return;
    if (!name) return;
    SET_STR(job, job->name, name);
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetReference (GncJob *job, const char *desc)
{
    if (!job) return;
    if (!desc) return;
    SET_STR(job, job->desc, desc);
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetRate (GncJob *job, gnc_numeric rate)
{
    if (!job) return;
    if (gnc_numeric_equal (gncJobGetRate(job), rate)) return;

    gncJobBeginEdit (job);
    if (!gnc_numeric_zero_p(rate))
    {
        GValue v = G_VALUE_INIT;
        g_value_init (&v, GNC_TYPE_NUMERIC);
        g_value_set_boxed (&v, &rate);
        qof_instance_set_kvp (QOF_INSTANCE (job), &v, 1, GNC_JOB_RATE);
        g_value_unset (&v);
    }
    else
    {
        qof_instance_set_kvp (QOF_INSTANCE (job), NULL, 1, GNC_JOB_RATE);
    }
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetOwner (GncJob *job, GncOwner *owner)
{
    if (!job) return;
    if (!owner) return;
    if (gncOwnerEqual (owner, &(job->owner))) return;

    switch (gncOwnerGetType (owner))
    {
    case GNC_OWNER_COOWNER:
    case GNC_OWNER_CUSTOMER:
    case GNC_OWNER_VENDOR:
        break;
    default:
        PERR("Unsupported Owner type: %d", gncOwnerGetType(owner));
        return;
    }

    gncJobBeginEdit (job);

    switch (gncOwnerGetType (&(job->owner)))
    {
    case GNC_OWNER_COOWNER:
        gncCoOwnerRemoveJob (gncOwnerGetCoOwner(&job->owner), job);
        break;
    case GNC_OWNER_CUSTOMER:
        gncCustomerRemoveJob (gncOwnerGetCustomer(&job->owner), job);
        break;
    case GNC_OWNER_VENDOR:
        gncVendorRemoveJob (gncOwnerGetVendor(&job->owner), job);
        break;
    default:
        break;
    }

    gncOwnerCopy (owner, &(job->owner));

    switch (gncOwnerGetType (&(job->owner)))
    {
    case GNC_OWNER_COOWNER:
        gncCoOwnerAddJob (gncOwnerGetCoOwner(&job->owner), job);
        break;
    case GNC_OWNER_CUSTOMER:
        gncCustomerAddJob (gncOwnerGetCustomer(&job->owner), job);
        break;
    case GNC_OWNER_VENDOR:
        gncVendorAddJob (gncOwnerGetVendor(&job->owner), job);
        break;
    default:
        break;
    }

    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetActive (GncJob *job, gboolean active)
{
    if (!job) return;
    if (active == job->active) return;
    gncJobBeginEdit (job);
    job->active = active;
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobSetTypeIsCoOwner (GncJob *job, gboolean type_coowner)
{
     GValue v = G_VALUE_INIT;
     if (!job) return;
     gncJobBeginEdit (job);
     g_value_init (&v, G_TYPE_INT64);
     g_value_set_int64 (&v, type_coowner ? 1 : 0);
     qof_instance_set_kvp (QOF_INSTANCE (job), &v, 1, GNC_JOB_TYPE_IS_COOWNER);
     g_value_unset (&v);
     mark_job (job);
     gncJobCommitEdit (job);
}

static void
qofJobSetOwner (GncJob *job, QofInstance *ent)
{
    if (!job || !ent)
    {
        return;
    }

    gncJobBeginEdit (job);
    qofOwnerSetEntity(&job->owner, ent);
    mark_job (job);
    gncJobCommitEdit (job);
}

void gncJobBeginEdit (GncJob *job)
{
    qof_begin_edit(&job->inst);
}

static void gncJobOnError (QofInstance *inst, QofBackendError errcode)
{
    PERR("Job QofBackend Failure: %d", errcode);
    gnc_engine_signal_commit_error( errcode );
}

static void job_free (QofInstance *inst)
{
    GncJob *job = (GncJob *)inst;
    gncJobFree (job);
}

static void gncJobOnDone (QofInstance *qof) { }

void gncJobCommitEdit (GncJob *job)
{
    /* GnuCash 2.6.3 and earlier didn't handle job kvp's... */
    if (qof_instance_has_kvp (QOF_INSTANCE (job)))
        gnc_features_set_used (qof_instance_get_book (QOF_INSTANCE (job)), GNC_FEATURE_KVP_EXTRA_DATA);

    if (!qof_commit_edit (QOF_INSTANCE(job))) return;
    qof_commit_edit_part2 (&job->inst, gncJobOnError,
                           gncJobOnDone, job_free);
}

/* ================================================================== */
/* Get Functions */

gboolean gncJobGetActive (const GncJob *job)
{
    if (!job) return FALSE;
    return job->active;
}

const char *gncJobGetName (const GncJob *job)
{
    if (!job) return NULL;
    return job->name;
}

const GncOwner *gncJobGetOwner (const GncJob *job)
{
    if (!job) return NULL;
    return &job->owner;
}

const char *gncJobGetID (const GncJob *job)
{
    if (!job) return NULL;
    return job->id;
}

GncOwnerType gncJobGetOwnerType (const GncJob *job)
{
    const GncOwner *owner;
    g_return_val_if_fail (job, GNC_OWNER_NONE);

    owner = gncOwnerGetEndOwner (gncJobGetOwner (job));
    return (gncOwnerGetType (owner));
}

const char *gncJobGetReference (const GncJob *job)
{
    if (!job) return NULL;
    return job->desc;
}

gnc_numeric gncJobGetRate (const GncJob *job)
{
    GValue v = G_VALUE_INIT;
    gnc_numeric *rate = NULL;
    gnc_numeric retval;
    if (!job) return gnc_numeric_zero ();
    qof_instance_get_kvp (QOF_INSTANCE (job), &v, 1, GNC_JOB_RATE);
    if (G_VALUE_HOLDS_BOXED (&v))
        rate = (gnc_numeric*)g_value_get_boxed (&v);
    retval = rate ? *rate : gnc_numeric_zero ();
    g_value_unset (&v);
    return retval;
}

GncJobType gncJobGetType (const GncJob *job)
{
    const GncOwner *owner;
    g_return_val_if_fail (job, GNC_OWNER_NONE);

    owner = gncOwnerGetEndOwner (gncJobGetOwner (job));
    return (gncOwnerGetType (owner));
}

gboolean gncJobGetTypeIsCoOwner (const GncJob *job)
{
    GValue v = G_VALUE_INIT;
    gboolean retval;
    if (!job) return FALSE;
    qof_instance_get_kvp (QOF_INSTANCE(job), &v, 1, GNC_JOB_TYPE_IS_COOWNER);
    retval = G_VALUE_HOLDS_INT64(&v) && g_value_get_int64 (&v);
    g_value_unset (&v);
    return retval;
}

static QofInstance*
qofJobGetOwner (GncJob *job)
{
    if (!job)
    {
        return NULL;
    }
    return QOF_INSTANCE(qofOwnerGetOwner(&job->owner));
}

/* Other functions */

int gncJobCompare (const GncJob * a, const GncJob *b)
{
    if (!a && !b) return 0;
    if (!a && b) return 1;
    if (a && !b) return -1;

    return (g_strcmp0(a->id, b->id));
}

gboolean gncJobEqual(const GncJob * a, const GncJob *b)
{
    if (a == NULL && b == NULL) return TRUE;
    if (a == NULL || b == NULL) return FALSE;

    g_return_val_if_fail(GNC_IS_JOB(a), FALSE);
    g_return_val_if_fail(GNC_IS_JOB(b), FALSE);

    if (g_strcmp0(a->id, b->id) != 0)
    {
        PWARN("IDs differ: %s vs %s", a->id, b->id);
        return FALSE;
    }

    if (g_strcmp0(a->name, b->name) != 0)
    {
        PWARN("Names differ: %s vs %s", a->name, b->name);
        return FALSE;
    }

    if (g_strcmp0(a->desc, b->desc) != 0)
    {
        PWARN("Descriptions differ: %s vs %s", a->desc, b->desc);
        return FALSE;
    }

    if (!gnc_numeric_equal(gncJobGetRate(a), gncJobGetRate(b)))
    {
        PWARN("Rates differ");
        return FALSE;
    }

    if (a->active != b->active)
    {
        PWARN("Active flags differ");
        return FALSE;
    }

    /* FIXME: Need real tests */
#if 0
    GncOwner      owner;
#endif

    return TRUE;
}

/* ================================================================== */
/* Package-Private functions */

static const char *_gncJobPrintable (gpointer item)
{
    GncJob *c;
    if (!item) return NULL;
    c = item;
    return c->name;
}

static QofObject gncJobDesc =
{
    DI(.interface_version = ) QOF_OBJECT_VERSION,
    DI(.e_type            = ) _GNC_MOD_NAME,
    DI(.type_label        = ) "Job",
    DI(.create            = ) (gpointer)gncJobCreate,
    DI(.book_begin        = ) NULL,
    DI(.book_end          = ) NULL,
    DI(.is_dirty          = ) qof_collection_is_dirty,
    DI(.mark_clean        = ) qof_collection_mark_clean,
    DI(.foreach           = ) qof_collection_foreach,
    DI(.printable         = ) _gncJobPrintable,
    DI(.version_cmp       = ) (int (*)(gpointer, gpointer)) qof_instance_version_cmp,
};

gboolean gncJobRegister (void)
{
    static QofParam params[] =
    {
        { JOB_ACTIVE, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncJobGetActive, (QofSetterFunc)gncJobSetActive },
        { JOB_ID, QOF_TYPE_STRING, (QofAccessFunc)gncJobGetID, (QofSetterFunc)gncJobSetID },
        { JOB_NAME, QOF_TYPE_STRING, (QofAccessFunc)gncJobGetName, (QofSetterFunc)gncJobSetName },
        { JOB_OWNER, GNC_ID_OWNER, (QofAccessFunc)gncJobGetOwner, NULL },
        { JOB_REFERENCE, QOF_TYPE_STRING, (QofAccessFunc)gncJobGetReference, (QofSetterFunc)gncJobSetReference },
        { JOB_RATE, QOF_TYPE_NUMERIC, (QofAccessFunc)gncJobGetRate, (QofSetterFunc)gncJobSetRate },
        { JOB_TYPE_IS_COOWNER, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncJobGetTypeIsCoOwner, (QofSetterFunc)gncJobSetTypeIsCoOwner },
        { QOF_PARAM_ACTIVE, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncJobGetActive, NULL },
        { QOF_PARAM_BOOK, QOF_ID_BOOK, (QofAccessFunc)qof_instance_get_book, NULL },
        { QOF_PARAM_GUID, QOF_TYPE_GUID, (QofAccessFunc)qof_instance_get_guid, NULL },
        { NULL },
    };

    if (!qof_choice_create(GNC_ID_JOB))
    {
        return FALSE;
    }
    if (!qof_choice_add_class(GNC_ID_INVOICE, GNC_ID_JOB, INVOICE_OWNER))
    {
        return FALSE;
    }

    qof_class_register (_GNC_MOD_NAME, (QofSortFunc)gncJobCompare, params);
    qofJobGetOwner(NULL);
    qofJobSetOwner(NULL, NULL);
    return qof_object_register (&gncJobDesc);
}

gchar *gncJobNextID (QofBook *book, const GncOwner *owner)
{
    return qof_book_increment_and_format_counter (book, _GNC_MOD_NAME);

    /*
    gchar *nextID;
    switch (gncOwnerGetType (gncOwnerGetEndOwner (owner)))
    {
    case GNC_OWNER_COOWNER:
        nextID = qof_book_increment_and_format_counter (book, "gncJob");
        break;
    case GNC_OWNER_CUSTOMER:
        nextID = qof_book_increment_and_format_counter (book, "gncJob");
        break;
    case GNC_OWNER_VENDOR:
        nextID = qof_book_increment_and_format_counter (book, "gncBill");
        break;
    case GNC_OWNER_EMPLOYEE:
        nextID = qof_book_increment_and_format_counter (book, "gncExpVoucher");
        break;
    default:
        nextID = qof_book_increment_and_format_counter (book, _GNC_MOD_NAME);
        break;
    }
    return nextID;
    */
}
