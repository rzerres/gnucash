/*********************************************************************\
 * test-billterm.c                                                *
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
#include "gncBillTermP.h"
#include "test-stuff.h"

static int count = 0;

// function header definitions
/* static void */
/* test_bool_fcn ( */
/*     QofBook *book, */
/*     const char *message, */
/*     void (*set) (GncBillTerm *, gboolean), */
/*     gboolean (*get) (const GncBillTerm *)); */

static void
test_int_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncBillTerm *, int),
    int (*get) (const GncBillTerm *));

static void
test_numeric_fcn (
    QofBook *book, const char *message,
    void (*set) (GncBillTerm *, gnc_numeric),
    gnc_numeric (*get)(const GncBillTerm *));

static void
test_string_fcn (
    QofBook *book, const char *message,
    void (*set) (GncBillTerm *, const char *str),
    const char * (*get)(const GncBillTerm *));

/*
 * Distribution list Tests
 */

static void
test_billterm (void)
{
    QofBook *book;
    GncBillTerm *billterm;
    book = qof_book_new ();

    /* Test creation/destruction */
    {
        do_test (gncBillTermCreate (NULL) == NULL, "billterm create - NULL");

        billterm = gncBillTermCreate (book);
        do_test (billterm != NULL, "billterm create - book");

        // Do we need this deprecated function any more?
        // do_test (gncBillTermGetBook (billterm) == book, "getbook");

        gncBillTermBeginEdit (billterm);
        gncBillTermDestroy (billterm);
        success ("create/destroy");
    }

    // Test set and get functions
    {
        GncGUID guid;
        GncBillTermType type = GNC_TERM_TYPE_DAYS;

        // test GUID
        guid_replace (&guid);
        billterm = gncBillTermCreate (book);
        count++;
        gncBillTermSetGUID (billterm, &guid);
        do_test (guid_equal (
            &guid, gncBillTermGetGUID (billterm)), "Compare guid");

        // test Type
        gncBillTermSetType (billterm, type);
        do_test (gncBillTermGetType (billterm) == type, "Compare type");

        // test Attributes
        test_string_fcn (book, "Handle Description",
            gncBillTermSetDescription, gncBillTermGetDescription);
        test_string_fcn (book, "Handle Name",
            gncBillTermSetName, gncBillTermGetName);
        test_int_fcn (book, "Handle Due-Days",
            gncBillTermSetDueDays, gncBillTermGetDueDays);
        test_int_fcn (book, "Handle Discount-Days",
            gncBillTermSetDiscountDays, gncBillTermGetDiscountDays);
        test_numeric_fcn (book, "Handle Discount",
            gncBillTermSetDiscount, gncBillTermGetDiscount);
        test_int_fcn (book, "Handle Cutoff-Days",
            gncBillTermSetCutoff, gncBillTermGetCutoff);
        //test_bool_fcn (book, "Handle Invisibility",
        //    gncBillTermGetInvisible);

    }

    // Test name and length of lists
    {
        GList *list;
        const char *name = "Test BillTerm";
        //const char *res = NULL;

        // gncBillTermCreate() and each test-function call do
        // increment the counter. Result: count => should be 7.
        list = gncBillTermGetTerms (book);
        //printf ("list count: '%d'\n", g_list_length (list));
        do_test (list != NULL, "Get lists: all");
        do_test (g_list_length (list) == count, "Compare list length: all");

        gncBillTermSetName (billterm, name);
        //res = gncBillTermGetName (billterm);
        //printf ("name: '%s'\n", name);
        //printf ("res: '%s'\n", res);
        list = gncBillTermGetTerms (book);
        // billterm shouldn't be Null after assignment.
        billterm = gncBillTermLookupByName (book, name);
        do_test (billterm != NULL, "Lookup list: by name");
    }

    // Test the Entity Table
    {
        const GncGUID *guid;

        guid = gncBillTermGetGUID (billterm);
        do_test (gncBillTermLookup (book, guid) == billterm, "Entity Table");
    }

    qof_book_destroy (book);
}

/*
 * Helper functions
 */

/* static void */
/* test_bool_fcn ( */
/*     QofBook *book, */
/*     const char *message, */
/*     void (*set) (GncBillTerm *, gboolean), */
/*     gboolean (*get) (const GncBillTerm *)) */
/* { */
/*     GncBillTerm *billterm = gncBillTermCreate (book); */
/*     gboolean num = get_random_boolean (); */

/*     do_test (!gncBillTermIsDirty (billterm), "test if start dirty"); */
/*     gncBillTermBeginEdit (billterm); */
/*     set (billterm, FALSE); */
/*     set (billterm, TRUE); */
/*     set (billterm, num); */
/*     /\* Billterm record should be dirty *\/ */
/*     do_test (gncBillTermIsDirty (billterm), "test dirty later"); */
/*     gncBillTermCommitEdit (billterm); */
/*     /\* Billterm record should not be dirty *\/ */
/*     /\* Skip, because will always fail without a backend. */
/*      * It's not possible to load a backend in the engine code */
/*      * without having circular dependencies. */
/*      *\/ */
/*     // do_test (!gncBillTermIsDirty (billterm), "test dirty after commit"); */
/*     do_test (get (billterm) == num, message); */
/*     count++; */
/* } */

static void
test_int_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncBillTerm *, int),
    int (*get)(const GncBillTerm *))
{
    GncBillTerm *billterm = gncBillTermCreate (book);
    int num = 42;

    do_test (!gncBillTermIsDirty (billterm), "test if start dirty");
    gncBillTermBeginEdit (billterm);
    set (billterm, num);
    /* Billterm record should be dirty */
    do_test (gncBillTermIsDirty (billterm), "test dirty later");
    gncBillTermCommitEdit (billterm);
    /* Billterm record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncBillTermIsDirty (billterm), "test dirty after commit");
    do_test (get (billterm) == num, message);
    count++;
}

static void
test_numeric_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncBillTerm *, gnc_numeric),
    gnc_numeric (*get)(const GncBillTerm *))
{
    GncBillTerm *billterm = gncBillTermCreate (book);
    gnc_numeric num = gnc_numeric_create (17, 1);

    do_test (!gncBillTermIsDirty (billterm), "test if start dirty");
    gncBillTermBeginEdit (billterm);
    set (billterm, num);
    /* Billterm record should be dirty */
    do_test (gncBillTermIsDirty (billterm), "test dirty later");
    gncBillTermCommitEdit (billterm);
    /* Billterm record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncBillTermIsDirty (billterm), "test dirty after commit");
    do_test (gnc_numeric_equal (get (billterm), num), message);
    count++;
}

static void
test_string_fcn (
    QofBook *book,
    const char *message,
    void (*set) (GncBillTerm *, const char *str),
    const char * (*get)(const GncBillTerm *))
{
    GncBillTerm *billterm = gncBillTermCreate (book);
    char const *str = get_random_string ();

    do_test (!gncBillTermIsDirty (billterm), "test if start dirty");
    gncBillTermBeginEdit (billterm);
    set (billterm, str);
    /* Billterm record should be dirty now */
    do_test (gncBillTermIsDirty (billterm), "test dirty later");
    gncBillTermCommitEdit (billterm);
    /* Billterm record should have been updated -> dirty flag cleaned!
     * But we need to skip: Without a backend we will always fail.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncBillTermIsDirty (billterm), "test dirty after commit");
    do_test (g_strcmp0 (get (billterm), str) == 0, message);
    count++;
}

int
main (int argc, char **argv)
{
    // Print out successfull tests
    set_success_print(TRUE);

    qof_init();
    do_test (cashobjects_register(), "Register cash objects");
    /* Registration is done during cashobjects_register,
       so trying to register again naturally fails. */
#if 0
    do_test (gncBillTermRegister(), "Cannot register GncBillTerm");
#endif
    test_billterm();
    print_test_results();
    qof_close ();
    return get_rv();
}
