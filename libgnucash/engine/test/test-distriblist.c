/*********************************************************************\
 * test-distriblist.c                                                *
 * Test the distribution list object                                 *
 *                                                                   *
 * Copyright (c) 2022 Ralf Zerres (ralf.zerres@mail.de)              *
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

#include <config.h>
#include <glib.h>
#include <qof.h>
#include <qofinstance-p.h>

#include "cashobjects.h"
#include "gncOwner.h"
#include "gncDistributionListP.h"
#include "test-stuff.h"

static int count = 0;

// function header definitions
static void
test_bool_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, gboolean),
    gboolean (*get) (const GncDistributionList *));

static void
test_int_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, int),
    int (*get) (const GncDistributionList *));

/* static void */
/* test_numeric_fcn ( */
/*     QofBook *book, const char *message, */
/*     void (*set) (GncDistributionList *, gnc_numeric), */
/*     gnc_numeric (*get)(const GncDistributionList *)); */

static void
test_string_fcn (
    QofBook *book, const char *message,
    void (*set) (GncDistributionList *, const char *str),
    const char * (*get)(const GncDistributionList *));

/*
 * Distribution list Tests
 */

static void
test_distriblist (void)
{
    QofBook *book;
    GncDistributionList *distriblist;

    book = qof_book_new ();

    /* Test creation/destruction */
    {
        do_test (gncDistribListCreate (NULL) == NULL, "Create - NULL");

        distriblist = gncDistribListCreate (book);
        do_test (distriblist != NULL, "Create - book");

        // Do we need this deprecated function any more?
        //do_test (qof_instance_get_book(QOF_INSTANCE(distriblist)) == book,
        //    "getbook");

        do_test (gncDistribListGetBook (distriblist) == book, "Get book - book");

        gncDistribListBeginEdit (distriblist);
        success ("Edit - distriblist");
        gncDistribListDestroy (distriblist);
        success ("Destroy - distriblist ");
    }

    // Test set and get functions
    {
        GncGUID guid;
        GncDistributionListType type_shares = GNC_DISTRIBLIST_TYPE_SHARES;
        GncDistributionListType type_percentage = GNC_DISTRIBLIST_TYPE_PERCENTAGE;
        /* GncOwner *owner_1 = NULL; */
        /* GncOwner *owner_2 = gncOwnerNew(); */
        const char *owner_typename = "Co-Owner";

        guid_replace (&guid);
        distriblist = gncDistribListCreate (book);
        count++;
        gncDistribListSetGUID (distriblist, &guid);
        do_test (guid_equal (
            &guid, gncDistribListGetGUID (distriblist)), "Compare guid - distriblist");

        gncDistribListSetType (distriblist, type_shares);
        //printf ("Testvalue  distriblist->type: '%d' (%d)'\n", type_shares, gncDistribListGetType (distriblist));
        do_test (gncDistribListGetType (distriblist) == type_shares, "Get type -  shares");
        gncDistribListSetType (distriblist, type_percentage);
        // printf ("Testvalue  distriblist->type: '%d' (%d)\n", type_percentage, gncDistribListGetType (distriblist));
        do_test (gncDistribListGetType (distriblist) == type_percentage, "Get type - percentage");

        gncDistribListSetOwnerTypeName (distriblist, owner_typename);

        // do_test (gncDistribListGetOwnerTypeName (distriblist) == owner_typename, "Get owner typename - distriblist");
        // Test setting an explicit owner_type
        /* do_test (g_strcmp0 (gncOwnerGetTypeString ( */
        /*              gncDistribListGetOwner (distriblist)), */
        /*              owner_typename) */
        /*          == 0, */
        /*          "Get owner typename"); */

        /* owner_2->type = GNC_OWNER_COOWNER; */
        /* gncOwnerTypeToQofIdType(owner_2->type); */

        /* gncDistribListSetOwner (distriblist, owner_2); */

        /* printf ("Testvalue src (owner->typename): '%s'\n", owner_typename); */
        /* printf ("Testvalue dst (owner->typename): '%s' (owner->type '%d')\n", */
        /*         gncOwnerGetTypeString ( */
        /*             gncDistribListGetOwner (distriblist)), */
        /*         gncOwnerGetType ( */
        /*             gncDistribListGetOwner (distriblist))); */

        /* do_test (g_strcmp0 (gncOwnerGetTypeString ( */
        /*              gncDistribListGetOwner (distriblist)), */
        /*              owner_typename) */
        /*          == 0, */
        /*          "Get owner typename"); */

        test_string_fcn (book, "Handle Description",
            gncDistribListSetDescription, gncDistribListGetDescription);
        test_string_fcn (book, "Handle Name",
            gncDistribListSetName, gncDistribListGetName);
        test_string_fcn (book, "Handle Percentage Label Settlement",
            gncDistribListSetPercentageLabelSettlement,
                gncDistribListGetPercentageLabelSettlement);
        test_int_fcn (book, "Handle Percentage Total",
            gncDistribListSetPercentageTotal, gncDistribListGetPercentageTotal);
        test_string_fcn (book, "Handle Share Label Settlement",
            gncDistribListSetSharesLabelSettlement,
                gncDistribListGetSharesLabelSettlement);
        test_int_fcn (book, "Handle Shares Total",
            gncDistribListSetSharesTotal, gncDistribListGetSharesTotal);

        test_bool_fcn (book, "Activate/De-Activate", gncDistribListSetActive, gncDistribListGetActive);
    }

    // Test name and length of lists
    {
        GList *list;
        const char *name = "Test-DistributionList";
        const char *res = NULL;

        // gncDistribListCreate() and each test_xxx_fcn() call
        // increment the counter. Result: count => should be 8.
        list = gncDistribListGetLists (book);
        //list = gncBusinessGetList (book, GNC_ID_DISTRIBLIST, FALSE);
        do_test (list != NULL, "Get distribution lists");
        // printf ("list entry count: '%i'\n", count);
        //printf ("g_list_length: '%i'\n", g_list_length (list));
        do_test (g_list_length (list) == count, "Number of created list elements");

        gncDistribListSetName (distriblist, name);
        res = gncDistribListGetName (distriblist);
        list = gncDistribListGetLists (book);
        distriblist = gncDistribListLookupByName (book, name);
        //printf ("name: '%s'\n", name);
        //printf ("res: '%s'\n", res);
        do_test (g_strcmp0 (name, res) == 0, "Lookup list: by name");
    }
    /* { */
    /*     //  Match random string after committing */
    /*     const char *str = get_random_string(); */
    /*     const char *res; */

    /*     printf ("Random string: '%s'\n", str); */
    /*     gncDistribListBeginEdit(distriblist); */
    /*     gncDistribListSetName (distriblist, str); */
    /*     gncDistribListCommitEdit(distriblist); */
    /*     res = qof_object_printable (GNC_ID_DISTRIBLIST, distriblist); */
    /*     // printf ("distriblist string: '%s'\n", res); */
    /*     // printf ("Test string: '%s'\n", gncDistribListGetName(distriblist)); */
    /*     // FIXME: do_test (res != NULL, "Assert Printable non 'NULL' string"); */
    /*     // FIXME: do_test (g_strcmp0 (str, res) == 0, "Assert Printable string match"); */
    /* } */

    // Test the Entity Table
    {
        const GncGUID *guid;

        guid = gncDistribListGetGUID (distriblist);
        do_test (gncDistribListLookup (book, guid) == distriblist, "Entity Table");
    }

    qof_book_destroy (book);
}

/*
 * Helper functions
 */

static void
test_bool_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, gboolean),
    gboolean (*get) (const GncDistributionList *))
{
    GncDistributionList *distriblist = gncDistribListCreate (book);
    gboolean num = get_random_boolean ();

    do_test (!gncDistribListIsDirty (distriblist), "test if start dirty");
    gncDistribListBeginEdit (distriblist);
    set (distriblist, FALSE);
    set (distriblist, TRUE);
    set (distriblist, num);
    /* Distriblist record should be dirty */
    do_test (gncDistribListIsDirty (distriblist), "test dirty later");
    gncDistribListCommitEdit (distriblist);
    /* Distriblist record should not be dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDistribListIsDirty (distriblist), "test dirty after commit");
    do_test (get (distriblist) == num, message);
    count++;
}

static void
test_int_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, int),
    int (*get)(const GncDistributionList *))
{
    GncDistributionList *distriblist = gncDistribListCreate (book);
    int num = 2;

    do_test (!gncDistribListIsDirty (distriblist), "test if start dirty");
    gncDistribListBeginEdit (distriblist);
    set (distriblist, num);
    /* Distriblist record should be dirty */
    do_test (gncDistribListIsDirty (distriblist), "test dirty later");
    gncDistribListCommitEdit (distriblist);
    /* Distriblist record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDistribListIsDirty (distriblist), "test dirty after commit");
    do_test (get (distriblist) == num, message);
    count++;
}

/* static void */
/* test_numeric_fcn ( */
/*     QofBook *book, */
/*     const char *message, */
/*     void (*set) (GncDistributionList *, gnc_numeric), */
/*     gnc_numeric (*get)(const GncDistributionList *)) */
/* { */
/*     GncDistributionList *distriblist = gncDistribListCreate (book); */
/*     gnc_numeric num = gnc_numeric_create (17, 1); */

/*     do_test (!gncDistribListIsDirty (distriblist), "test if start dirty"); */
/*     gncDistribListBeginEdit (distriblist); */
/*     set (distriblist, num); */
/*     /\* Distriblist record should be dirty *\/ */
/*     do_test (gncDistribListIsDirty (distriblist), "test dirty later"); */
/*     gncDistribListCommitEdit (distriblist); */
/*     /\* Distriblist record should be not dirty *\/ */
/*     /\* Skip, because will always fail without a backend. */
/*      * It's not possible to load a backend in the engine code */
/*      * without having circular dependencies. */
/*      *\/ */
/*     // do_test (!gncDistribListIsDirty (distriblist), "test dirty after commit"); */
/*     do_test (gnc_numeric_equal (get (distriblist), num), message); */
/*     count++; */
/* } */

static void
test_string_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, const char *str),
    const char * (*get)(const GncDistributionList *))
{
    GncDistributionList *distriblist = gncDistribListCreate (book);
    char const *str = get_random_string ();

    do_test (!gncDistribListIsDirty (distriblist), "test if start dirty");
    gncDistribListBeginEdit (distriblist);
    set (distriblist, str);
    /* Distriblist record should be dirty now */
    do_test (gncDistribListIsDirty (distriblist), "test dirty later");
    gncDistribListCommitEdit (distriblist);
    /* Distriblist record should have been updated -> dirty flag cleaned!
     * But we need to skip: Without a backend we will always fail.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDistribListIsDirty (distriblist), "test dirty after commit");
    do_test (g_strcmp0 (get (distriblist), str) == 0, message);
    count++;
}

int
main (int argc, char **argv)
{
    // Print out successfull tests
    set_success_print(TRUE);

    qof_init();
    if (cashobjects_register())
    {
      test_distriblist();
      print_test_results();
    }

    qof_close ();
    return get_rv();
}
