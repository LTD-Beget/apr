/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "testutil.h"
#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_skiplist.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static apr_pool_t *ptmp = NULL;
static apr_skiplist *skiplist = NULL;

static int skiplist_get_size(abts_case *tc, apr_skiplist *sl)
{
    size_t size = 0;
    apr_skiplistnode *n;
    for (n = apr_skiplist_getlist(sl); n; apr_skiplist_next(sl, &n)) {
        ++size;
    }
    ABTS_TRUE(tc, size == apr_skiplist_size(sl));
    return size;
}

static void skiplist_init(abts_case *tc, void *data)
{
    apr_time_t now = apr_time_now();
    srand((unsigned int)(((now >> 32) ^ now) & 0xffffffff));

    ABTS_INT_EQUAL(tc, APR_SUCCESS, apr_skiplist_init(&skiplist, p));
    ABTS_PTR_NOTNULL(tc, skiplist);
    apr_skiplist_set_compare(skiplist, (void*)strcmp, (void*)strcmp);
}

static void skiplist_find(abts_case *tc, void *data)
{
    const char *val;

    apr_skiplist_insert(skiplist, "baton");
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "baton", val);
}

static void skiplist_dontfind(abts_case *tc, void *data)
{
    const char *val;

    val = apr_skiplist_find(skiplist, "keynotthere", NULL);
    ABTS_PTR_EQUAL(tc, NULL, (void *)val);
}

static void skiplist_insert(abts_case *tc, void *data)
{
    const char *val;
    int i, height = 0;

    for (i = 0; i < 10; ++i) {
        apr_skiplist_insert(skiplist, "baton");
        ABTS_TRUE(tc, 1 == skiplist_get_size(tc, skiplist));
        val = apr_skiplist_find(skiplist, "baton", NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, "baton", val);

        if (height == 0) {
            height = apr_skiplist_height(skiplist);
        }
        else {
            ABTS_INT_EQUAL(tc, height, apr_skiplist_height(skiplist));
        }
    }

    apr_skiplist_insert(skiplist, "foo");
    ABTS_TRUE(tc, 2 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "foo", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "foo", val);

    apr_skiplist_insert(skiplist, "atfirst");
    ABTS_TRUE(tc, 3 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "atfirst", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "atfirst", val);
}

static void skiplist_add(abts_case *tc, void *data)
{
    const char *val;
    size_t i, n = skiplist_get_size(tc, skiplist);

    for (i = 0; i < 100; ++i) {
        n++;
        apr_skiplist_add(skiplist, "daton");
        ABTS_TRUE(tc, n == skiplist_get_size(tc, skiplist));
        val = apr_skiplist_find(skiplist, "daton", NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, "daton", val);

        n++;
        apr_skiplist_add(skiplist, "baton");
        ABTS_TRUE(tc, n == skiplist_get_size(tc, skiplist));
        val = apr_skiplist_find(skiplist, "baton", NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, "baton", val);

        n++;
        apr_skiplist_add(skiplist, "caton");
        ABTS_TRUE(tc, n == skiplist_get_size(tc, skiplist));
        val = apr_skiplist_find(skiplist, "caton", NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, "caton", val);

        n++;
        apr_skiplist_add(skiplist, "aaton");
        ABTS_TRUE(tc, n == skiplist_get_size(tc, skiplist));
        val = apr_skiplist_find(skiplist, "aaton", NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, "aaton", val);
    }
}

static void skiplist_destroy(abts_case *tc, void *data)
{
    apr_skiplist_destroy(skiplist, NULL);
    ABTS_TRUE(tc, 0 == skiplist_get_size(tc, skiplist));
}

static void skiplist_size(abts_case *tc, void *data)
{
    const char *val;

    ABTS_TRUE(tc, 0 == skiplist_get_size(tc, skiplist));

    apr_skiplist_insert(skiplist, "abc");
    apr_skiplist_insert(skiplist, "ghi");
    apr_skiplist_insert(skiplist, "def");
    val = apr_skiplist_find(skiplist, "abc", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "abc", val);
    val = apr_skiplist_find(skiplist, "ghi", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "ghi", val);
    val = apr_skiplist_find(skiplist, "def", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "def", val);

    ABTS_TRUE(tc, 3 == skiplist_get_size(tc, skiplist));
    apr_skiplist_destroy(skiplist, NULL);
}

static void skiplist_remove(abts_case *tc, void *data)
{
    const char *val;

    ABTS_TRUE(tc, 0 == skiplist_get_size(tc, skiplist));

    apr_skiplist_add(skiplist, "baton");
    ABTS_TRUE(tc, 1 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "baton", val);

    apr_skiplist_add(skiplist, "baton");
    ABTS_TRUE(tc, 2 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "baton", val);

    apr_skiplist_remove(skiplist, "baton", NULL);
    ABTS_TRUE(tc, 1 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "baton", val);

    apr_skiplist_add(skiplist, "baton");
    ABTS_TRUE(tc, 2 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_NOTNULL(tc, val);
    ABTS_STR_EQUAL(tc, "baton", val);

    /* remove all "baton"s */
    while (apr_skiplist_remove(skiplist, "baton", NULL))
        ;
    ABTS_TRUE(tc, 0 == skiplist_get_size(tc, skiplist));
    val = apr_skiplist_find(skiplist, "baton", NULL);
    ABTS_PTR_EQUAL(tc, NULL, val);
}

#define NUM_RAND (100)
#define NUM_FIND (3 * NUM_RAND)
static void skiplist_random_loop(abts_case *tc, void *data)
{
    char **batons;
    apr_skiplist *sl;
    const char *val;
    int i;

    ABTS_INT_EQUAL(tc, APR_SUCCESS, apr_skiplist_init(&sl, ptmp));
    apr_skiplist_set_compare(sl, (void*)strcmp, (void*)strcmp);
    apr_skiplist_set_preheight(sl, 7);
    ABTS_INT_EQUAL(tc, 7, apr_skiplist_preheight(sl));

    batons = apr_palloc(ptmp, NUM_FIND * sizeof(char*));

    for (i = 0; i < NUM_FIND; ++i) {
        if (i < NUM_RAND) {
            batons[i] = apr_psprintf(ptmp, "%.6u", rand() % 1000000);
        }
        else {
            batons[i] = apr_pstrdup(ptmp, batons[i % NUM_RAND]);
        }
        apr_skiplist_add(sl, batons[i]);
        val = apr_skiplist_find(sl, batons[i], NULL);
        ABTS_PTR_NOTNULL(tc, val);
        ABTS_STR_EQUAL(tc, batons[i], val);
    }

    apr_pool_clear(ptmp);
}


static void add_int_to_skiplist(apr_skiplist *list, int n){
    int* a = apr_skiplist_alloc(list, sizeof(int));
    *a = n;
    apr_skiplist_insert(list, a);
}

static int comp(void *a, void *b){
    return *((int*) a) - *((int*) b);
}

static int compk(void *a, void *b){
    return comp(a, b);
}

static void skiplist_test(abts_case *tc, void *data) {
    int test_elems = 10;
    int i = 0, j = 0;
    int *val = NULL;
    apr_skiplist * list = NULL;

    ABTS_INT_EQUAL(tc, APR_SUCCESS, apr_skiplist_init(&list, ptmp));
    apr_skiplist_set_compare(list, comp, compk);
    
    /* insert 10 objects */
    for (i = 0; i < test_elems; ++i){
        add_int_to_skiplist(list, i);
    }

    /* remove all objects */
    while ((val = apr_skiplist_pop(list, NULL))){
        ABTS_INT_EQUAL(tc, *val, j++);
    }

    /* insert 10 objects again */
    for (i = test_elems; i < test_elems+test_elems; ++i){
        add_int_to_skiplist(list, i);
    }

    j = test_elems;
    while ((val = apr_skiplist_pop(list, NULL))){
        ABTS_INT_EQUAL(tc, *val, j++);
    }

    /* empty */
    val = apr_skiplist_pop(list, NULL);
    ABTS_PTR_EQUAL(tc, val, NULL);

    add_int_to_skiplist(list, 42);
    val = apr_skiplist_pop(list, NULL);
    ABTS_INT_EQUAL(tc, *val, 42); 

    /* empty */
    val = apr_skiplist_pop(list, NULL);
    ABTS_PTR_EQUAL(tc, val, NULL);

    apr_pool_clear(ptmp);
}


abts_suite *testskiplist(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    apr_pool_create(&ptmp, p);

    abts_run_test(suite, skiplist_init, NULL);
    abts_run_test(suite, skiplist_find, NULL);
    abts_run_test(suite, skiplist_dontfind, NULL);
    abts_run_test(suite, skiplist_insert, NULL);
    abts_run_test(suite, skiplist_add, NULL);
    abts_run_test(suite, skiplist_destroy, NULL);
    abts_run_test(suite, skiplist_size, NULL);
    abts_run_test(suite, skiplist_remove, NULL);
    abts_run_test(suite, skiplist_random_loop, NULL);

    abts_run_test(suite, skiplist_test, NULL);

    apr_pool_destroy(ptmp);

    return suite;
}

