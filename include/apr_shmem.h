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

#ifndef APR_SHMEM_H
#define APR_SHMEM_H
/** 
 * @file apr_shmem.h
 * @brief APR Shared Memory Routines
 */
/**
 * @defgroup APR_shmem Shared Memory Routines
 * @ingroup APR
 * @{
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if   APR_USES_FILEBASED_SHM
typedef   char *         apr_shm_name_t;
#elif APR_USES_KEYBASED_SHM
typedef   key_t          apr_shm_name_t;
#else
/* If APR_USES_ANONYMOUS_SHM or any other case...
 * we can't leave apr_shm_name_t entirely undefined.
 */
typedef   void           apr_shm_name_t;
#endif

typedef   struct apr_shmem_t apr_shmem_t;

/**
 * Create a pool of shared memory for use later.
 * @param m The shared memory block.
 * @param reqsize The size of the shared memory pool.
 * @param file The file to use for the shared memory on platforms 
 *        that require it.
 * @param cont The pool to use
 */
APR_DECLARE(apr_status_t) apr_shm_init(apr_shmem_t **m, apr_size_t reqsize,
                                       const char *file, apr_pool_t *cont);

/**
 * Destroy the shared memory block.
 * @param m The shared memory block to destroy. 
 */
APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shmem_t *m);

/**
 * allocate memory from the block of shared memory.
 * @param c The shared memory block to destroy. 
 * @param reqsize How much memory to allocate
 */
APR_DECLARE(void *) apr_shm_malloc(apr_shmem_t *c, apr_size_t reqsize);

/**
 * allocate memory from the block of shared memory and initialize it to zero.
 * @param shared The shared memory block to destroy. 
 * @param size How much memory to allocate
 */
APR_DECLARE(void *) apr_shm_calloc(apr_shmem_t *shared, apr_size_t size);

/**
 * free shared memory previously allocated.
 * @param shared The shared memory block to destroy. 
 * @param entity The actual data to free. 
 */
APR_DECLARE(apr_status_t) apr_shm_free(apr_shmem_t *shared, void *entity);

/**
 * Get the name of the shared memory segment if not using anonymous 
 * shared memory.
 * @param  c The shared memory block to destroy. 
 * @param  name The name of the shared memory block, NULL if anonymous 
 *              shared memory.
 * @return APR_USES_ANONYMOUS_SHM if we are using anonymous shared
 *         memory.  APR_USES_FILEBASED_SHM if our shared memory is
 *         based on file access.  APR_USES_KEYBASED_SHM if shared
 *         memory is based on a key value such as shmctl.  If the
 *         shared memory is anonymous, the name is NULL.
 */
APR_DECLARE(apr_status_t) apr_shm_name_get(apr_shmem_t *c,
                                           apr_shm_name_t **name);

/**
 * Set the name of the shared memory segment if not using anonymous 
 * shared memory.  This is to allow processes to open shared memory 
 * created by another process.
 * @param c The shared memory block to destroy. 
 * @param name The name of the shared memory block, NULL if anonymous 
 *             shared memory.
 * @return APR_USES_ANONYMOUS_SHM if we are using anonymous shared
 *         memory.  APR_SUCCESS if we are using named shared memory
 *         and we were able to assign the name correctly. 
 */
APR_DECLARE(apr_status_t) apr_shm_name_set(apr_shmem_t *c,
                                           apr_shm_name_t *name);

/**
 * Open the shared memory block in a child process.
 * @param  The shared memory block to open in the child. 
 */
APR_DECLARE(apr_status_t) apr_shm_open(apr_shmem_t *c);

/**
 * Determine how much memory is available in the specified shared memory block
 * @param c The shared memory block to open in the child. 
 * @param avail The amount of space available in the shared memory block.
 */
APR_DECLARE(apr_status_t) apr_shm_avail(apr_shmem_t *c, apr_size_t *avail);

#ifdef __cplusplus
}
#endif
/** @} */
#endif  /* ! APR_SHMEM_H */

