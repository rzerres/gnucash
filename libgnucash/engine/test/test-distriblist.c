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

static void
test_numeric_fcn (
    QofBook *book, const char *message,
    void (*set) (GncDistributionList *, gnc_numeric),
    gnc_numeric (*get)(const GncDistributionList *));

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

        do_test (gncDistribListGetBook (distriblist) == book, "Get book");

        gncDistribListBeginEdit (distriblist);
        gncDistribListDestroy (distriblist);
        success ("create/destroy");
    }

    // Test set and get functions
    {
        GncGUID guid;
        GncDistributionListType type = GNC_DISTRIBLIST_TYPE_SHARES;

        guid_replace (&guid);
        distriblist = gncDistribListCreate (book);
        count++;
        gncDistribListSetGUID (distriblist, &guid);
        do_test (guid_equal (
            &guid, gncDistribListGetGUID (distriblist)), "Compare guid");

        gncDistribListSetType (distriblist, type);
        do_test (gncDistribListGetType (distriblist) == type, "Get type");

        test_string_fcn (book, "Handle Description",
            gncDistribListSetDescription, gncDistribListGetDescription);
        test_string_fcn (book, "Handle Name",
            gncDistribListSetName, gncDistribListGetName);
        test_string_fcn (book, "Handle Label Settlement",
            gncDistribListSetLabelSettlement, gncDistribListGetLabelSettlement);
        test_int_fcn (book, "Handle Shares Total",
            gncDistribListSetSharesTotal, gncDistribListGetSharesTotal);
        test_int_fcn (book, "Handle Percentage Total",
            gncDistribListSetPercentageTotal, gncDistribListGetPercentageTotal);

    }

    // Test name and length of lists
    {
        GList *list;
        const char *name = "Test-DistributionList";
        const char *res = NULL;

        //list = gncDistribListGetLists (book);
        // gncDistribListCreate() and each test-function call do
        // increment the counter. Result: count => should be 6.
        list = gncBusinessGetList (book, GNC_ID_DISTRIBLIST, TRUE);
        do_test (list != NULL, "Get lists: all");
        do_test (g_list_length (list) == count, "Compare list length: all");
        g_list_free (list);

        gncDistribListSetName (distriblist, name);
        res = gncDistribListGetName (distriblist);
        list = gncDistribListGetLists (book);
        distriblist = gncDistribListLookupByName (book, name);
        //printf ("name: '%s'\n", name);
        //printf ("res: '%s'\n", res);
        do_test (g_strcmp0 (name, res) == 0, "Lookup list: by name");
        g_list_free (list);
    }
    {
        //  Match random string after committing
        const char *str = get_random_string();
        const char *res;

        //printf ("Random string: '%s'\n", str);
        res = NULL;
        gncDistribListBeginEdit(distriblist);
        gncDistribListSetName (distriblist, str);
        gncDistribListCommitEdit(distriblist);
        res = qof_object_printable (GNC_ID_DISTRIBLIST, distriblist);
        //printf ("Test string: '%s'\n", str);
        //printf ("distriblist string: '%s'\n", res);
        //FIXME: do_test (res != NULL, "Assert Printable 'NULL' string match ");
        //FIXME: do_test (g_strcmp0 (str, res) == 0, "Assert Printable string match");
    }
    {
        // Compare explicit Distribution List string
        char str[16] = "Co-Owner shares";
        const char *DistriblistTestString = str;
        const char *res;

        res = NULL;
        gncDistribListBeginEdit (distriblist);
        //printf ("DistriblistTestString: '%s'\n", DistriblistTestString);
        gncDistribListSetName (distriblist, DistriblistTestString);
        gncDistribListCommitEdit (distriblist);

        res = qof_object_printable (GNC_ID_DISTRIBLIST, distriblist);
        //printf ("res from distriblist: '%s'\n", res);
        //FIXME: do_test (res != NULL, "Test String non NULL");
        //FIXME: do_test (g_strcmp0 (DistriblistTestString, res) == 0, "Test string match");
    }

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
    int num = 42;

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

static void
test_numeric_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncDistributionList *, gnc_numeric),
    gnc_numeric (*get)(const GncDistributionList *))
{
    GncDistributionList *distriblist = gncDistribListCreate (book);
    gnc_numeric num = gnc_numeric_create (17, 1);

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
    do_test (gnc_numeric_equal (get (distriblist), num), message);
    count++;
}

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
    // FIXME: cashobjects
    do_test (cashobjects_register(), "Register cash objects");
    /* Registration is done during cashobjects_register,
       so trying to register again naturally fails. */
#if 0
    do_test (gncDistribListRegister(), "Cannot register GncDistributionList");
#endif
    test_distriblist();
    print_test_results();
    qof_close ();
    return get_rv();
}
