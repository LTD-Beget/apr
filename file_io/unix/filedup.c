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

#include "fileio.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "inherit.h"

apr_status_t apr_file_dup(apr_file_t **new_file, apr_file_t *old_file, apr_pool_t *p)
{
    int have_file = 0;

    if ((*new_file) == NULL) {
        (*new_file) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
        if ((*new_file) == NULL) {
            return APR_ENOMEM;
        }
    } else {
        have_file = 1;
    }
    
    (*new_file)->cntxt = p; 
    if (have_file) {
        dup2(old_file->filedes, (*new_file)->filedes);
    }
    else {
        (*new_file)->filedes = dup(old_file->filedes); 
    }
    (*new_file)->fname = apr_pstrdup(p, old_file->fname);
    (*new_file)->buffered = old_file->buffered;
    if ((*new_file)->buffered) {
#if APR_HAS_THREADS
        apr_lock_create(&((*new_file)->thlock), APR_MUTEX, APR_INTRAPROCESS, NULL, 
                       p);
#endif
        (*new_file)->buffer = apr_palloc(p, APR_FILE_BUFSIZE);
    }
    /* this is the way dup() works */
    (*new_file)->blocking = old_file->blocking; 
    /* apr_file_dup() clears the inherit attribute, user must call 
     * apr_file_set_inherit() again on the dupped handle, as necessary.
     */
    (*new_file)->flags = old_file->flags & ~APR_INHERIT;
    apr_pool_cleanup_register((*new_file)->cntxt, (void *)(*new_file), 
                              apr_unix_file_cleanup, apr_unix_file_cleanup);
    return APR_SUCCESS;
}

