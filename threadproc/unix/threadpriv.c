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

#include "apr.h"
#include "apr_portable.h"
#include "threadproc.h"

#if APR_HAS_THREADS

#if APR_HAVE_PTHREAD_H
ap_status_t ap_create_thread_private(ap_threadkey_t **key, 
                                     void (*dest)(void *), ap_pool_t *cont)
{
    ap_status_t stat;
    (*key) = (ap_threadkey_t *)ap_pcalloc(cont, sizeof(ap_threadkey_t));

    if ((*key) == NULL) {
        return APR_ENOMEM;
    }

    (*key)->cntxt = cont;

    if ((stat = pthread_key_create(&(*key)->key, dest)) == 0) {
        return stat;
    }
    return stat;
}

ap_status_t ap_get_thread_private(void **new, ap_threadkey_t *key)
{
#ifdef PTHREAD_GETSPECIFIC_TAKES_TWO_ARGS
    if (pthread_getspecific(key->key,new))
       *new = NULL;
#else  
    (*new) = pthread_getspecific(key->key);
#endif
    return APR_SUCCESS;
}

ap_status_t ap_set_thread_private(void *priv, ap_threadkey_t *key)
{
    ap_status_t stat;
    if ((stat = pthread_setspecific(key->key, priv)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

ap_status_t ap_delete_thread_private(ap_threadkey_t *key)
{
    ap_status_t stat;
    if ((stat = pthread_key_delete(key->key)) == 0) {
        return APR_SUCCESS; 
    }
    return stat;
}

ap_status_t ap_get_threadkeydata(void **data, const char *key,
                                 ap_threadkey_t *threadkey)
{
    if (threadkey != NULL) {
        return ap_get_userdata(data, key, threadkey->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHDKEY;
    }
}

ap_status_t ap_set_threadkeydata(void *data, const char *key,
                                 ap_status_t (*cleanup) (void *),
                                 ap_threadkey_t *threadkey)
{
    if (threadkey != NULL) {
        return ap_set_userdata(data, key, cleanup, threadkey->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHDKEY;
    }
}

ap_status_t ap_get_os_threadkey(ap_os_threadkey_t *thekey, ap_threadkey_t *key)
{
    if (key == NULL) {
        return APR_ENOFILE;
    }
    thekey = &(key->key);
    return APR_SUCCESS;
}

ap_status_t ap_put_os_threadkey(ap_threadkey_t **key, 
                                ap_os_threadkey_t *thekey, ap_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*key) == NULL) {
        (*key) = (ap_threadkey_t *)ap_pcalloc(cont, sizeof(ap_threadkey_t));
        (*key)->cntxt = cont;
    }
    (*key)->key = *thekey;
    return APR_SUCCESS;
}           
#endif
#endif
