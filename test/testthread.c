/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apr_thread_proc.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef BEOS
#include <unistd.h>
#endif

#if !APR_HAS_THREADS
int main(void)
{
    fprintf(stderr,
            "This program won't work on this platform because there is no "
            "support for threads.\n");
    return 0;
}
#else /* !APR_HAS_THREADS */

void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func2(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func3(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func4(apr_thread_t *thd, void *data);


apr_lock_t *thread_lock;
apr_pool_t *context;
int x = 0;
apr_status_t exit_ret_val = 123; /* just some made up number to check on later */

void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        apr_lock_acquire(thread_lock);
        x++;
        apr_lock_release(thread_lock);
    }
    apr_thread_exit(thd, &exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func2(apr_thread_t *thd, void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        apr_lock_acquire(thread_lock);
        x++;
        apr_lock_release(thread_lock);
    }
    apr_thread_exit(thd, &exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func3(apr_thread_t *thd, void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        apr_lock_acquire(thread_lock);
        x++;
        apr_lock_release(thread_lock);
    }
    apr_thread_exit(thd, &exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func4(apr_thread_t *thd, void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        apr_lock_acquire(thread_lock);
        x++;
        apr_lock_release(thread_lock);
    }
    apr_thread_exit(thd, &exit_ret_val);
    return NULL;
} 

int main(void)
{
    apr_thread_t *t1;
    apr_thread_t *t2;
    apr_thread_t *t3;
    apr_thread_t *t4;
    apr_status_t s1;
    apr_status_t s2;
    apr_status_t s3;
    apr_status_t s4;

    apr_initialize();

    fprintf(stdout, "Initializing the context......."); 
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "could not initialize\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Initializing the lock......."); 
    s1 = apr_lock_create(&thread_lock, APR_MUTEX, APR_INTRAPROCESS, "lock.file", context); 
    if (s1 != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Could not create lock\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Starting all the threads......."); 
    s1 = apr_thread_create(&t1, NULL, thread_func1, NULL, context);
    s2 = apr_thread_create(&t2, NULL, thread_func2, NULL, context);
    s3 = apr_thread_create(&t3, NULL, thread_func3, NULL, context);
    s4 = apr_thread_create(&t4, NULL, thread_func4, NULL, context);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Error starting thread\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Waiting for threads to exit.......");
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Checking thread's returned value.......");
    if (s1 != exit_ret_val || s2 != exit_ret_val ||
        s3 != exit_ret_val || s4 != exit_ret_val) {
        fflush(stdout);
        fprintf(stderr, 
                "Invalid return value %d/%d/%d/%d (not expected value %d)\n",
                s1, s2, s3, s4, exit_ret_val);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Checking if locks worked......."); 
    if (x != 40000) {
        fflush(stdout);
        fprintf(stderr, "The locks didn't work????  %d\n", x);
        exit(-1);
    }
    else {
        fprintf(stdout, "Everything is working!\n");
    }
    return 0;
}

#endif /* !APR_HAS_THREADS */
