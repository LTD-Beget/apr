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

#include "networkio.h"

#if APR_HAS_SENDFILE
/* This file is needed to allow us access to the apr_file_t internals. */
#include "fileio.h"

/* Glibc2.1.1 fails to define TCP_CORK.  This is a bug that will be 
 *fixed in the next release.  It should be 3
 */
#if !defined(TCP_CORK) && defined(__linux__)
#define TCP_CORK 3
#endif

#endif /* APR_HAS_SENDFILE */

static apr_status_t wait_for_io_or_timeout(apr_socket_t *sock, int for_read)
{
    struct timeval tv, *tvptr;
    fd_set fdset;
    int srv;

    do {
        FD_ZERO(&fdset);
        FD_SET(sock->socketdes, &fdset);
        if (sock->timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = sock->timeout / APR_USEC_PER_SEC;
            tv.tv_usec = sock->timeout % APR_USEC_PER_SEC;
            tvptr = &tv;
        }
        srv = select(sock->socketdes + 1,
            for_read ? &fdset : NULL,
            for_read ? NULL : &fdset,
            NULL,
            tvptr);
        /* TODO - timeout should be smaller on repeats of this loop */
    } while (srv == -1 && errno == EINTR);

    if (srv == 0) {
        return APR_TIMEUP;
    }
    else if (srv < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

apr_status_t apr_send(apr_socket_t *sock, const char *buf, apr_size_t *len)
{
    ssize_t rv;
    
    do {
        rv = write(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) 
      && sock->timeout != 0) {
        apr_status_t arv = wait_for_io_or_timeout(sock, 0);
        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
        else {
            do {
                rv = write(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
        *len = 0;
        return errno;
    }
    (*len) = rv;
    return APR_SUCCESS;
}

apr_status_t apr_recv(apr_socket_t *sock, char *buf, apr_size_t *len)
{
    ssize_t rv;
    
    do {
        rv = read(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && 
      sock->timeout != 0) {
        apr_status_t arv = wait_for_io_or_timeout(sock, 1);
        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
        else {
            do {
                rv = read(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
        (*len) = 0;
        return errno;
    }
    (*len) = rv;
    if (rv == 0) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}

#ifdef HAVE_WRITEV
apr_status_t apr_sendv(apr_socket_t * sock, const struct iovec *vec,
                     apr_int32_t nvec, apr_size_t *len)
{
    apr_ssize_t rv;

    do {
        rv = writev(sock->socketdes, vec, nvec);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && 
      sock->timeout != 0) {
        apr_status_t arv = wait_for_io_or_timeout(sock, 0);
        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
        else {
            do {
                rv = writev(sock->socketdes, vec, nvec);
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
        *len = 0;
        return errno;
    }
    (*len) = rv;
    return APR_SUCCESS;
}
#endif

/* XXX - if we start using these elsewhere in this file we'll
 * need to move these to the top...
 */

#if APR_HAVE_CORKABLE_TCP && !defined(__FreeBSD__)

/* TCP_CORK & TCP_NOPUSH keep us from sending partial frames when we
 * shouldn't. They are however, mutually exclusive with TCP_NODELAY  
 */

static int os_cork(apr_socket_t *sock)
{
    int nodelay_off = 0, corkflag = 1, rv, delayflag;
    apr_socklen_t delaylen = sizeof(delayflag);

    /* XXX it would be cheaper to use an apr_socket_t flag here */
    rv = getsockopt(sock->socketdes, IPPROTO_TCP, TCP_NODELAY,
                    (void *) &delayflag, &delaylen);
    if (rv == 0) {  
        if (delayflag != 0) {
            /* turn off nodelay temporarily to allow cork */
            rv = setsockopt(sock->socketdes, IPPROTO_TCP, TCP_NODELAY,
                            (const void *) &nodelay_off, sizeof(nodelay_off));
            /* XXX nuke the rv checking once this is proven solid */
            if (rv < 0) {
                return rv;    
            }   
        } 
    }
    rv = setsockopt(sock->socketdes, IPPROTO_TCP, APR_TCP_NOPUSH_FLAG,
                    (const void *) &corkflag, sizeof(corkflag));
    return rv == 0 ? delayflag : rv;
}   

static int os_uncork(apr_socket_t *sock, int delayflag)
{
    /* Uncork to send queued frames */
    
    int corkflag = 0, rv;
    rv = setsockopt(sock->socketdes, IPPROTO_TCP, APR_TCP_NOPUSH_FLAG,
                    (const void *) &corkflag, sizeof(corkflag));
    if (rv == 0) {
        /* restore TCP_NODELAY to its original setting */
        rv = setsockopt(sock->socketdes, IPPROTO_TCP, TCP_NODELAY,
                        (const void *) &delayflag, sizeof(delayflag));
    }
    return rv;
}
#endif /* APR_HAVE_CORKABLE_TCP */

#if APR_HAS_SENDFILE

/* TODO: Verify that all platforms handle the fd the same way,
 * i.e. that they don't move the file pointer.
 */
/* TODO: what should flags be?  int_32? */

/* Define a structure to pass in when we have a NULL header value */
static apr_hdtr_t no_hdtr;

#if defined(__linux__) && defined(HAVE_WRITEV)

apr_status_t apr_sendfile(apr_socket_t *sock, apr_file_t *file,
        		apr_hdtr_t *hdtr, apr_off_t *offset, apr_size_t *len,
        		apr_int32_t flags)
{
    off_t off = *offset;
    int rv, nbytes = 0, total_hdrbytes, i, delayflag = APR_EINIT, corked = 0;
    apr_status_t arv;

    if (!hdtr) {
        hdtr = &no_hdtr;
    }

    /* Ignore flags for now. */
    flags = 0;

    if (hdtr->numheaders > 0) {
        apr_int32_t hdrbytes;

        /* cork before writing headers */
        rv = os_cork(sock);
        if (rv < 0) {
            return errno;
        }
        delayflag = rv;
        corked = 1;

        /* Now write the headers */
        arv = apr_sendv(sock, hdtr->headers, hdtr->numheaders, &hdrbytes);
        if (arv != APR_SUCCESS) {
	    *len = 0;
            return errno;
        }
        nbytes += hdrbytes;

        /* If this was a partial write and we aren't doing timeouts, 
         * return now with the partial byte count; this is a non-blocking 
         * socket.
         */
        total_hdrbytes = 0;
        for (i = 0; i < hdtr->numheaders; i++) {
            total_hdrbytes += hdtr->headers[i].iov_len;
        }
        if (hdrbytes < total_hdrbytes) {
            *len = hdrbytes;
            return os_uncork(sock, delayflag);
        }
    }

    do {
        rv = sendfile(sock->socketdes,	/* socket */
        	      file->filedes,	/* open file descriptor of the file to be sent */
        	      &off,	/* where in the file to start */
        	      *len	/* number of bytes to send */
            );
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK) && 
        sock->timeout > 0) {
	arv = wait_for_io_or_timeout(sock, 0);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
        	rv = sendfile(sock->socketdes,	/* socket */
        		      file->filedes,	/* open file descriptor of the file to be sent */
        		      &off,	/* where in the file to start */
        		      *len);	/* number of bytes to send */
            } while (rv == -1 && errno == EINTR);
        }
    }

    if (rv == -1) {
	*len = nbytes;
        rv = errno;
        if (corked) {
            os_uncork(sock, delayflag);
        }
        return rv;
    }

    nbytes += rv;

    /* If this was a partial write, return now with the partial byte count; 
     * this is a non-blocking socket.
     */

    if (rv < *len) {
        *len = nbytes;
        if (corked) {
            os_uncork(sock, delayflag);
        }
        return APR_SUCCESS;
    }

    /* Now write the footers */
    if (hdtr->numtrailers > 0) {
        apr_int32_t trbytes;
        arv = apr_sendv(sock, hdtr->trailers, hdtr->numtrailers, &trbytes);
        nbytes += trbytes;
        if (arv != APR_SUCCESS) {
	    *len = nbytes;
            rv = errno;
            if (corked) {
                os_uncork(sock, delayflag);
            }
            return rv;
        }
    }

    if (corked) {
        /* if we corked, uncork & restore TCP_NODELAY setting */
        rv = os_uncork(sock, delayflag);
    }

    (*len) = nbytes;
    return rv < 0 ? errno : APR_SUCCESS;
}

/* These are just demos of how the code for the other OSes.
 * I haven't tested these, but they're right in terms of interface.
 * I just wanted to see what types of vars would be required from other OSes. 
 */

#elif defined(__FreeBSD__)

/* Release 3.1 or greater */
apr_status_t apr_sendfile(apr_socket_t * sock, apr_file_t * file,
        		apr_hdtr_t * hdtr, apr_off_t * offset, apr_size_t * len,
        		apr_int32_t flags)
{
    off_t nbytes = 0;
    int rv, i;
    struct sf_hdtr headerstruct;
    size_t bytes_to_send = *len;

    if (!hdtr) {
        hdtr = &no_hdtr;
    }

    /* Ignore flags for now. */
    flags = 0;

    /* On FreeBSD, the number of bytes to send must include the length of
     * the headers.  Don't look at the man page for this :(  Instead, look
     * at the the logic in src/sys/kern/uipc_syscalls::sendfile().
     */
    
    for (i = 0; i < hdtr->numheaders; i++) {
        bytes_to_send += hdtr->headers[i].iov_len;
    }

    headerstruct.headers = hdtr->headers;
    headerstruct.hdr_cnt = hdtr->numheaders;
    headerstruct.trailers = hdtr->trailers;
    headerstruct.trl_cnt = hdtr->numtrailers;

    /* FreeBSD can send the headers/footers as part of the system call */
    if (sock->timeout >= 0) {
        /* On FreeBSD, it is possible for the first call to sendfile to
         * get EAGAIN, but still send some data.  This means that we cannot
         * call sendfile and then check for EAGAIN, and then wait and call
         * sendfile again.  If we do that, then we are likely to send the
         * first chunk of data twice, once in the first call and once in the
         * second.  If we are using a timed write, then we check to make sure
         * we can send data before trying to send it.
         */
        apr_status_t arv = wait_for_io_or_timeout(sock, 0);
        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
    }

    do {
        if (bytes_to_send) {
            /* We won't dare call sendfile() if we don't have
             * header or file bytes to send because bytes_to_send == 0
             * means send the whole file.
             */
            rv = sendfile(file->filedes, /* file to be sent */
                          sock->socketdes, /* socket */
                          *offset,       /* where in the file to start */
                          bytes_to_send, /* number of bytes to send */
                          &headerstruct, /* Headers/footers */
                          &nbytes,       /* number of bytes written */
                          flags);        /* undefined, set to 0 */
        }
        else {
            /* just trailer bytes... use writev()
             */
            rv = writev(sock->socketdes,
                        hdtr->trailers,
                        hdtr->numtrailers);
            if (rv > 0) {
                nbytes = rv;
                rv = 0;
            }
            else {
                nbytes = 0;
            }
        }
    } while (rv == -1 && errno == EINTR);

    (*len) = nbytes;
    if (rv == -1 && (errno != EAGAIN || (errno == EAGAIN && sock->timeout < 0))) {
        return errno;
    }
    return APR_SUCCESS;
}

#elif defined(__hpux) || defined(__hpux__)

/* HP cc in ANSI mode defines __hpux; gcc defines __hpux__ */

/* HP-UX Version 10.30 or greater
 * (no worries, because we only get here if autoconfiguration found sendfile)
 */

/* ssize_t sendfile(int s, int fd, off_t offset, size_t nbytes,
 *                  const struct iovec *hdtrl, int flags);
 *
 * nbytes is the number of bytes to send just from the file; as with FreeBSD, 
 * if nbytes == 0, the rest of the file (from offset) is sent
 */

apr_status_t apr_sendfile(apr_socket_t *sock, apr_file_t *file,
			  apr_hdtr_t *hdtr, apr_off_t *offset, apr_size_t *len,
			  apr_int32_t flags)
{
    int i;
    ssize_t rc;
    size_t nbytes = *len, headerlen, trailerlen;
    struct iovec hdtrarray[2];
    char *headerbuf, *trailerbuf;

    if (!hdtr) {
        hdtr = &no_hdtr;
    }

    /* Ignore flags for now. */
    flags = 0;

    /* HP-UX can only send one header iovec and one footer iovec; try to
     * only allocate storage to combine input iovecs when we really have to
     */

    switch(hdtr->numheaders) {
    case 0:
        hdtrarray[0].iov_base = NULL;
        hdtrarray[0].iov_len = 0;
        break;
    case 1:
        hdtrarray[0] = hdtr->headers[0];
        break;
    default:
        headerlen = 0;
        for (i = 0; i < hdtr->numheaders; i++) {
            headerlen += hdtr->headers[i].iov_len;
        }  

        /* XXX:  BUHHH? wow, what a memory leak! */
        headerbuf = hdtrarray[0].iov_base = apr_palloc(sock->cntxt, headerlen);
        hdtrarray[0].iov_len = headerlen;

        for (i = 0; i < hdtr->numheaders; i++) {
            memcpy(headerbuf, hdtr->headers[i].iov_base,
                   hdtr->headers[i].iov_len);
            headerbuf += hdtr->headers[i].iov_len;
        }
    }

    switch(hdtr->numtrailers) {
    case 0:
        hdtrarray[1].iov_base = NULL;
        hdtrarray[1].iov_len = 0;
        break;
    case 1:
        hdtrarray[1] = hdtr->trailers[0];
        break;
    default:
        trailerlen = 0;
        for (i = 0; i < hdtr->numtrailers; i++) {
            trailerlen += hdtr->trailers[i].iov_len;
        }

        /* XXX:  BUHHH? wow, what a memory leak! */
        trailerbuf = hdtrarray[1].iov_base = apr_palloc(sock->cntxt, trailerlen);
        hdtrarray[1].iov_len = trailerlen;

        for (i = 0; i < hdtr->numtrailers; i++) {
            memcpy(trailerbuf, hdtr->trailers[i].iov_base,
                   hdtr->trailers[i].iov_len);
            trailerbuf += hdtr->trailers[i].iov_len;
        }
    }

    do {
        if (nbytes) {       /* any bytes to send from the file? */
            rc = sendfile(sock->socketdes,      /* socket  */
                          file->filedes,        /* file descriptor to send */
                          *offset,              /* where in the file to start */
                          nbytes,               /* number of bytes to send from file */
                          hdtrarray,            /* Headers/footers */
                          flags);               /* undefined, set to 0 */
        }
        else {              /* we can't call sendfile() with no bytes to send from the file */
            rc = writev(sock->socketdes, hdtrarray, 2);
        }
    } while (rc == -1 && errno == EINTR);

    if (rc == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK) && 
        sock->timeout > 0) {
        apr_status_t arv = wait_for_io_or_timeout(sock, 0);

        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
        else {
            do {
                if (nbytes) {
                    rc = sendfile(sock->socketdes,    /* socket  */
                                  file->filedes,      /* file descriptor to send */
                                  *offset,            /* where in the file to start */
                                  nbytes,             /* number of bytes to send from file */
                                  hdtrarray,          /* Headers/footers */
                                  flags);             /* undefined, set to 0 */
                }
                else {      /* we can't call sendfile() with no bytes to send from the file */
                    rc = writev(sock->socketdes, hdtrarray, 2);
                }
            } while (rc == -1 && errno == EINTR);
        }
    }

    if (rc == -1) {
	*len = 0;
        return errno;
    }

    /* Set len to the number of bytes written */
    *len = rc;
    return APR_SUCCESS;
}
#elif defined(_AIX) || defined(__MVS__)
/* AIX and OS/390 have the same send_file() interface.
 *
 * subtle differences:
 *   AIX doesn't update the file ptr but OS/390 does
 *
 * availability (correctly determined by autoconf):
 *
 * AIX -  version 4.3.2 with APAR IX85388, or version 4.3.3 and above
 * OS/390 - V2R7 and above
 */
apr_status_t apr_sendfile(apr_socket_t * sock, apr_file_t * file,
                        apr_hdtr_t * hdtr, apr_off_t * offset, apr_size_t * len,
                        apr_int32_t flags)
{
    int i, ptr, rv = 0;
    void * hbuf=NULL, * tbuf=NULL;
    apr_status_t arv;
    struct sf_parms parms;

    if (!hdtr) {
        hdtr = &no_hdtr;
    }

    /* Ignore flags for now. */
    flags = 0;

    /* AIX can also send the headers/footers as part of the system call */
    parms.header_length = 0;
    if (hdtr && hdtr->numheaders) {
        if (hdtr->numheaders == 1) {
            parms.header_data = hdtr->headers[0].iov_base;
            parms.header_length = hdtr->headers[0].iov_len;
        }
        else {
            for (i = 0; i < hdtr->numheaders; i++) {
                parms.header_length += hdtr->headers[i].iov_len;
            }
#if 0
            /* Keepalives make apr_palloc a bad idea */
            hbuf = malloc(parms.header_length);
#else
            /* but headers are small, so maybe we can hold on to the
             * memory for the life of the socket...
             */
            hbuf = apr_palloc(sock->cntxt, parms.header_length);
#endif
            ptr = 0;
            for (i = 0; i < hdtr->numheaders; i++) {
                memcpy((char *)hbuf + ptr, hdtr->headers[i].iov_base,
                       hdtr->headers[i].iov_len);
                ptr += hdtr->headers[i].iov_len;
            }
            parms.header_data = hbuf;
        }
    }
    else parms.header_data = NULL;
    parms.trailer_length = 0;
    if (hdtr && hdtr->numtrailers) {
        if (hdtr->numtrailers == 1) {
            parms.trailer_data = hdtr->trailers[0].iov_base;
            parms.trailer_length = hdtr->trailers[0].iov_len;
        }
        else {
            for (i = 0; i < hdtr->numtrailers; i++) {
                parms.trailer_length += hdtr->trailers[i].iov_len;
            }
#if 0
            /* Keepalives make apr_palloc a bad idea */
            tbuf = malloc(parms.trailer_length);
#else
            tbuf = apr_palloc(sock->cntxt, parms.trailer_length);
#endif
            ptr = 0;
            for (i = 0; i < hdtr->numtrailers; i++) {
                memcpy((char *)tbuf + ptr, hdtr->trailers[i].iov_base,
                       hdtr->trailers[i].iov_len);
                ptr += hdtr->trailers[i].iov_len;
            }
            parms.trailer_data = tbuf;
        }
    }
    else parms.trailer_data = NULL;

    /* Whew! Headers and trailers set up. Now for the file data */

    parms.file_descriptor = file->filedes;
    parms.file_offset = *offset;
    parms.file_bytes = *len;

    /* O.K. All set up now. Let's go to town */

    do {
        rv = send_file(&(sock->socketdes), /* socket */
                       &(parms),           /* all data */
                       flags);             /* flags */
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 &&
        (errno == EAGAIN || errno == EWOULDBLOCK) &&
        sock->timeout > 0) {
        arv = wait_for_io_or_timeout(sock, 0);
        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        }
        else {
            do {
                rv = send_file(&(sock->socketdes), /* socket */
                               &(parms),           /* all data */
                               flags);             /* flags */
            } while (rv == -1 && errno == EINTR);
        }
    }

    (*len) = parms.bytes_sent;

#if 0
    /* Clean up after ourselves */
    if(hbuf) free(hbuf);
    if(tbuf) free(tbuf);
#endif

    if (rv == -1) {
        return errno;
    }
    return APR_SUCCESS;
}
#elif defined(__osf__) && defined (__alpha)
/* Tru64's sendfile implementation doesn't work, and we need to make sure that
 * we don't use it until it is fixed.  If it is used as it is now, it will
 * hang the machine and the only way to fix it is a reboot.
 */
#else
#error APR has detected sendfile on your system, but nobody has written a
#error version of it for APR yet.  To get past this, either write apr_sendfile
#error or change APR_HAS_SENDFILE in apr.h to 0. 
#endif /* __linux__, __FreeBSD__, __HPUX__, _AIX, __MVS__, Tru64/OSF1 */
#endif /* APR_HAS_SENDFILE */

#if !APR_HAS_SENDFILE
/* currently, exports.c includes a reference to apr_sendfile() even if 
 * apr_sendfile() doesn't work on the platform;
 * this dummy version is just to get exports.c to compile/link
 */
apr_status_t apr_sendfile(apr_socket_t *sock, apr_file_t *file,
                          apr_hdtr_t *hdtr, apr_off_t *offset, apr_size_t *len,
                          apr_int32_t flags); /* avoid warning for no proto */

apr_status_t apr_sendfile(apr_socket_t *sock, apr_file_t *file,
                          apr_hdtr_t *hdtr, apr_off_t *offset, apr_size_t *len,
                          apr_int32_t flags)
{
    return APR_ENOTIMPL;
}
#endif
