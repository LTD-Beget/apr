/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "dso.h"
#include "apr_strings.h"
#include "apr_private.h"
#include "fileio.h"

#if APR_HAS_DSO

static apr_status_t dso_cleanup(void *thedso)
{
    apr_dso_handle_t *dso = thedso;

    if (dso->handle != NULL && !FreeLibrary(dso->handle)) {
        return apr_get_os_error();
    }
    dso->handle = NULL;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_dso_load(struct apr_dso_handle_t **res_handle, 
                                       const char *path, apr_pool_t *ctx)
{
    HINSTANCE os_handle;
    UINT em;

#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(ctx, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                              / sizeof(apr_wchar_t), path)) {
            return rv;
        }
        /* Prevent ugly popups from killing our app */
        em = SetErrorMode(SEM_FAILCRITICALERRORS);
        os_handle = LoadLibraryExW(wpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!os_handle)
            os_handle = LoadLibraryExW(wpath, NULL, 0);
        SetErrorMode(em);
    }
    else
#endif
    {
        char fspec[APR_PATH_MAX], *p = fspec;
        /* Must convert path from / to \ notation.
         * Per PR2555, the LoadLibraryEx function is very picky about slashes.
         * Debugging on NT 4 SP 6a reveals First Chance Exception within NTDLL.
         * LoadLibrary in the MS PSDK also reveals that it -explicitly- states
         * that backslashes must be used for the LoadLibrary family of calls.
         */
        apr_cpystrn(fspec, path, sizeof(fspec));
        while (p = strchr(p, '/'))
            *p = '\\';
        
        /* Prevent ugly popups from killing our app */
        em = SetErrorMode(SEM_FAILCRITICALERRORS);
        os_handle = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!os_handle)
            os_handle = LoadLibraryEx(path, NULL, 0);
        SetErrorMode(em);
    }
    *res_handle = apr_pcalloc(ctx, sizeof(*res_handle));

    if(os_handle == NULL) {
        (*res_handle)->load_error = apr_get_os_error();
        return (*res_handle)->load_error;
    }

    (*res_handle)->handle = (void*)os_handle;
    (*res_handle)->cont = ctx;
    (*res_handle)->load_error = APR_SUCCESS;

    apr_pool_cleanup_register(ctx, *res_handle, dso_cleanup, apr_pool_cleanup_null);

    return APR_SUCCESS;
}
    
APR_DECLARE(apr_status_t) apr_dso_unload(struct apr_dso_handle_t *handle)
{
    return apr_pool_cleanup_run(handle->cont, handle, dso_cleanup);
}

APR_DECLARE(apr_status_t) apr_dso_sym(apr_dso_handle_sym_t *ressym, 
                         struct apr_dso_handle_t *handle, 
                         const char *symname)
{
    FARPROC retval = GetProcAddress(handle->handle, symname);
    if (!retval) {
        return apr_get_os_error();
    }
    
    *ressym = retval;
    
    return APR_SUCCESS;
}

APR_DECLARE(const char *) apr_dso_error(apr_dso_handle_t *dso, char *buf, apr_size_t bufsize)
{
    return apr_strerror(dso->load_error, buf, bufsize);
}

#endif
