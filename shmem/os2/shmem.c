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

#include "apr_general.h"
#include "apr_shmem.h"
#include "apr_errno.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include <umalloc.h>
#include <stdlib.h>

struct shmem_t {
    void *memblock;
    Heap_t heap;
};



apr_status_t apr_shm_init(struct shmem_t **m, apr_size_t reqsize, const char *file, apr_pool_t *cont)
{
    int rc;
    struct shmem_t *newm = (struct shmem_t *)apr_palloc(cont, sizeof(struct shmem_t));
    char *name = NULL;

    if (file)
        name = apr_pstrcat(cont, "\\SHAREMEM\\", file, NULL);

    rc = DosAllocSharedMem(&(newm->memblock), name, reqsize, PAG_COMMIT|OBJ_GETTABLE|PAG_READ|PAG_WRITE);

    if (rc)
        return APR_OS2_STATUS(rc);

    newm->heap = _ucreate(newm->memblock, reqsize, !_BLOCK_CLEAN, _HEAP_REGULAR|_HEAP_SHARED, NULL, NULL);
    _uopen(newm->heap);
    *m = newm;
    return APR_SUCCESS;
}



apr_status_t apr_shm_destroy(struct shmem_t *m)
{
    _uclose(m->heap);
    _udestroy(m->heap, _FORCE);
    DosFreeMem(m->memblock);
    return APR_SUCCESS;
}



void *apr_shm_malloc(struct shmem_t *m, apr_size_t reqsize)
{
    return _umalloc(m->heap, reqsize);
}



void *apr_shm_calloc(struct shmem_t *m, apr_size_t size)
{
    return _ucalloc(m->heap, size, 1);
}



apr_status_t apr_shm_free(struct shmem_t *m, void *entity)
{
    free(entity);
    return APR_SUCCESS;
}



apr_status_t apr_shm_name_get(apr_shmem_t *c, apr_shm_name_t **name)
{
    *name = NULL;
    return APR_ANONYMOUS;
}



apr_status_t apr_shm_name_set(apr_shmem_t *c, apr_shm_name_t *name)
{
    return APR_ANONYMOUS;
}



apr_status_t apr_shm_open(struct shmem_t *m)
{
    int rc;

    rc = DosGetSharedMem(m->memblock, PAG_READ|PAG_WRITE);

    if (rc)
        return APR_OS2_STATUS(rc);

    _uopen(m->heap);
    return APR_SUCCESS;
}



apr_status_t apr_shm_avail(struct shmem_t *c, apr_size_t *size)
{

    return APR_ENOTIMPL;
}
