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

#ifndef FILE_IO_H
#define FILE_IO_H

#include "apr.h"
#include "apr_private.h"
#include "apr_pools.h"
#include "apr_general.h"
#include "apr_tables.h"
#include "apr_lock.h"
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "misc.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#if APR_HAS_UNICODE_FS
#include "i18n.h"
#include <wchar.h>

typedef apr_int16_t apr_wchar_t;

apr_status_t utf8_to_unicode_path(apr_wchar_t* dststr, apr_size_t dstchars, 
                                  const char* srcstr);
apr_status_t unicode_to_utf8_path(char* dststr, apr_size_t dstchars, 
                                  const apr_wchar_t* srcstr);

#define APR_FILE_MAX MAX_PATH
#else /* !APR_HAS_UNICODE_FS */
#define APR_FILE_MAX MAX_PATH
#endif

#define APR_FILE_BUFSIZE 4096

/* obscure ommissions from msvc's sys/stat.h */
#ifdef _MSC_VER
#define S_IFIFO        _S_IFIFO /* pipe */
#define S_IFBLK        0060000  /* Block Special */
#define S_IFLNK        0120000  /* Symbolic Link */
#define S_IFSOCK       0140000  /* Socket */
#define S_IFWHT        0160000  /* Whiteout */
#endif

/* Internal Flags for apr_file_open */
#define APR_OPENLINK      8192    /* Open a link itself, if supported */
#define APR_READCONTROL   4096    /* Read the file's owner/perms */
#define APR_WRITECONTROL  2048    /* Modifythe file's owner/perms */

/* Entries missing from the MSVC 5.0 Win32 SDK:
 */
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif
#ifndef FILE_FLAG_OPEN_NO_RECALL
#define FILE_FLAG_OPEN_NO_RECALL     0x00100000
#endif
#ifndef FILE_FLAG_OPEN_REPARSE_POINT
#define FILE_FLAG_OPEN_REPARSE_POINT 0x00200000
#endif

/* Information bits available from the WIN32 FindFirstFile function */
#define APR_FINFO_WIN32_DIR (APR_FINFO_NAME  | APR_FINFO_TYPE \
                           | APR_FINFO_CTIME | APR_FINFO_ATIME \
                           | APR_FINFO_MTIME | APR_FINFO_SIZE)

/* Sneak the Readonly bit through finfo->protection for internal use _only_ */
#define APR_FREADONLY 0x10000000 

/* Private function for apr_stat/lstat/getfileinfo/dir_read */
void fillin_fileinfo(apr_finfo_t *finfo, WIN32_FILE_ATTRIBUTE_DATA *wininfo, 
                     int byhandle);

/* Private function that extends apr_stat/lstat/getfileinfo/dir_read */
apr_status_t more_finfo(apr_finfo_t *finfo, const void *ufile, 
                        apr_int32_t wanted, int whatfile, 
                        apr_oslevel_e os_level);

/* whatfile types for the ufile arg */
#define MORE_OF_HANDLE 0
#define MORE_OF_FSPEC  1
#define MORE_OF_WFSPEC 2

/* quick run-down of fields in windows' apr_file_t structure that may have 
 * obvious uses.
 * fname --  the filename as passed to the open call.
 * dwFileAttricutes -- Attributes used to open the file.
 * append -- Windows doesn't support the append concept when opening files.
 *           APR needs to keep track of this, and always make sure we append
 *           correctly when writing to a file with this flag set TRUE.
 */

struct apr_file_t {
    apr_pool_t *cntxt;
    HANDLE filehand;
    BOOLEAN pipe;              // Is this a pipe of a file?
    OVERLAPPED *pOverlapped;
    apr_interval_time_t timeout;

    /* File specific info */
    apr_finfo_t *finfo;
    char *fname;
    DWORD dwFileAttributes;
    int eof_hit;
    BOOLEAN buffered;          // Use buffered I/O?
    int ungetchar;             // Last char provided by an unget op. (-1 = no char)
    int append; 

    /* Stuff for buffered mode */
    char *buffer;
    apr_ssize_t bufpos;        // Read/Write position in buffer
    apr_ssize_t dataRead;      // amount of valid data read into buffer
    int direction;            // buffer being used for 0 = read, 1 = write
    apr_ssize_t filePtr;       // position in file of handle
    apr_lock_t *mutex;         // mutex semaphore, must be owned to access the above fields

    /* Pipe specific info */    
};

struct apr_dir_t {
    apr_pool_t *cntxt;
    HANDLE dirhand;
    apr_size_t rootlen;
    char *dirname;
    char *name;
    union {
#if APR_HAS_UNICODE_FS
        struct {
            WIN32_FIND_DATAW *entry;
        } w;
#endif
        struct {
            WIN32_FIND_DATAA *entry;
        } n;
    };
};

apr_status_t file_cleanup(void *);

/**
 * Internal function to create a Win32/NT pipe that respects some async
 * timeout options.
 */
apr_status_t apr_create_nt_pipe(apr_file_t **in, apr_file_t **out, 
                                BOOLEAN bAsyncRead, BOOLEAN bAsyncWrite, 
                                apr_pool_t *p);
#endif  /* ! FILE_IO_H */
