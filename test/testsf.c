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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"

#if !APR_HAS_SENDFILE
int main(void)
{
    fprintf(stderr, 
            "This program won't work on this platform because there is no "
            "support for sendfile().\n");
    return 0;
}
#else /* !APR_HAS_SENDFILE */

#define FILE_LENGTH    200000

#define FILE_DATA_CHAR '0'

#define HDR1           "1234567890ABCD\n"
#define HDR2           "EFGH\n"
#define HDR3_LEN       80000
#define HDR3_CHAR      '^'
#define TRL1           "IJKLMNOPQRSTUVWXYZ\n"
#define TRL2           "!@#$%&*()\n"
#define TRL3_LEN       90000
#define TRL3_CHAR      '@'

#define TESTSF_PORT    8021

#define TESTFILE       "testsf.dat"

typedef enum {BLK, NONBLK, TIMEOUT} client_socket_mode_t;

static void apr_setup(apr_pool_t **p, apr_socket_t **sock, int *family)
{
    char buf[120];
    apr_status_t rv;

    rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_initialize()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    atexit(apr_terminate);

    rv = apr_create_pool(p, NULL);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_create_pool()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    *sock = NULL;
    rv = apr_create_socket(sock, *family, SOCK_STREAM, *p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_create_socket()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    if (*family == APR_UNSPEC) {
        apr_sockaddr_t *localsa;

        rv = apr_get_sockaddr(&localsa, APR_LOCAL, *sock);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_get_sockaddr()->%d/%s\n",
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        *family = localsa->sa.sin.sin_family;
    }
}

static void create_testfile(apr_pool_t *p, const char *fname)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[120];
    int i;
    apr_finfo_t finfo;

    printf("Creating a test file...\n");
    rv = apr_open(&f, fname, 
                 APR_CREATE | APR_WRITE | APR_TRUNCATE | APR_BUFFERED,
                 APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    
    buf[0] = FILE_DATA_CHAR;
    buf[1] = '\0';
    for (i = 0; i < FILE_LENGTH; i++) {
        /* exercise apr_putc() and apr_puts() on buffered files */
        if ((i % 2) == 0) {
            rv = apr_putc(buf[0], f);
            if (rv) {
                fprintf(stderr, "apr_putc()->%d/%s\n",
                        rv, apr_strerror(rv, buf, sizeof buf));
                exit(1);
            }
        }
        else {
            rv = apr_puts(buf, f);
            if (rv) {
                fprintf(stderr, "apr_puts()->%d/%s\n",
                        rv, apr_strerror(rv, buf, sizeof buf));
                exit(1);
            }
        }
    }

    rv = apr_close(f);
    if (rv) {
        fprintf(stderr, "apr_close()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_stat(&finfo, fname, p);
    if (rv) {
        fprintf(stderr, "apr_close()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    if (finfo.size != FILE_LENGTH) {
        fprintf(stderr, 
                "test file %s should be %ld-bytes long\n"
                "instead it is %ld-bytes long\n",
                fname,
                (long int)FILE_LENGTH,
                (long int)finfo.size);
        exit(1);
    }
}

static int client(client_socket_mode_t socket_mode)
{
    apr_status_t rv, tmprv;
    apr_socket_t *sock;
    apr_pool_t *p;
    char buf[120];
    apr_file_t *f = NULL;
    apr_ssize_t len;
    apr_size_t expected_len;
    apr_off_t current_file_offset;
    apr_hdtr_t hdtr;
    struct iovec headers[3];
    struct iovec trailers[3];
    apr_ssize_t bytes_read;
    apr_pollfd_t *pfd;
    apr_int32_t nsocks;
    int i;
    int family;
    apr_sockaddr_t *destsa;

    family = APR_INET;
    apr_setup(&p, &sock, &family);
    create_testfile(p, TESTFILE);

    rv = apr_open(&f, TESTFILE, APR_READ, 0, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_open()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_getaddrinfo(&destsa, "127.0.0.1", family, TESTSF_PORT, 0, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_getaddrinfo()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_connect(sock, destsa);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_connect()->%d/%s\n", 
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    switch(socket_mode) {
    case BLK:
        /* leave it blocking */
        break;
    case NONBLK:
        /* set it non-blocking */
        rv = apr_setsocketopt(sock, APR_SO_NONBLOCK, 1);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_setsocketopt(APR_SO_NONBLOCK)->%d/%s\n", 
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        break;
    case TIMEOUT:
        /* set a timeout */
        rv = apr_setsocketopt(sock, APR_SO_TIMEOUT, 
                             100 * APR_USEC_PER_SEC);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_setsocketopt(APR_SO_NONBLOCK)->%d/%s\n", 
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        break;
    default:
        assert(1 != 1);
    }

    printf("Sending the file...\n");

    hdtr.headers = headers;
    hdtr.numheaders = 3;
    hdtr.headers[0].iov_base = HDR1;
    hdtr.headers[0].iov_len  = strlen(hdtr.headers[0].iov_base);
    hdtr.headers[1].iov_base = HDR2;
    hdtr.headers[1].iov_len  = strlen(hdtr.headers[1].iov_base);
    hdtr.headers[2].iov_base = malloc(HDR3_LEN);
    assert(hdtr.headers[2].iov_base);
    memset(hdtr.headers[2].iov_base, HDR3_CHAR, HDR3_LEN);
    hdtr.headers[2].iov_len  = HDR3_LEN;

    hdtr.trailers = trailers;
    hdtr.numtrailers = 3;
    hdtr.trailers[0].iov_base = TRL1;
    hdtr.trailers[0].iov_len  = strlen(hdtr.trailers[0].iov_base);
    hdtr.trailers[1].iov_base = TRL2;
    hdtr.trailers[1].iov_len  = strlen(hdtr.trailers[1].iov_base);
    hdtr.trailers[2].iov_base = malloc(TRL3_LEN);
    memset(hdtr.trailers[2].iov_base, TRL3_CHAR, TRL3_LEN);
    assert(hdtr.trailers[2].iov_base);
    hdtr.trailers[2].iov_len  = TRL3_LEN;

    expected_len = 
        strlen(HDR1) + strlen(HDR2) + HDR3_LEN +
        strlen(TRL1) + strlen(TRL2) + TRL3_LEN +
        FILE_LENGTH;
    
    if (socket_mode == BLK || socket_mode == TIMEOUT) {
        current_file_offset = 0;
        len = FILE_LENGTH;
        rv = apr_sendfile(sock, f, &hdtr, &current_file_offset, &len, 0);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_sendfile()->%d/%s\n",
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        
        printf("apr_sendfile() updated offset with %ld\n",
               (long int)current_file_offset);
        
        printf("apr_sendfile() updated len with %ld\n",
               (long int)len);
        
        printf("bytes really sent: %d\n",
               expected_len);

        if (len != expected_len) {
            fprintf(stderr, "apr_sendfile() didn't report the correct "
                    "number of bytes sent!\n");
            exit(1);
        }
    }
    else {
        /* non-blocking... wooooooo */
        apr_size_t total_bytes_sent;

        pfd = NULL;
        rv = apr_setup_poll(&pfd, 1, p);
        assert(!rv);
        rv = apr_add_poll_socket(pfd, sock, APR_POLLOUT);
        assert(!rv);

        total_bytes_sent = 0;
        current_file_offset = 0;
        len = FILE_LENGTH;
        do {
            apr_ssize_t tmplen;

            tmplen = len; /* bytes remaining to send from the file */
            printf("Calling apr_sendfile()...\n");
            printf("Headers:\n");
            for (i = 0; i < hdtr.numheaders; i++) {
                printf("\t%d bytes\n",
                       hdtr.headers[i].iov_len);
            }
            printf("File: %ld bytes from offset %ld\n",
                   (long)tmplen, (long)current_file_offset);
            printf("Trailers:\n");
            for (i = 0; i < hdtr.numtrailers; i++) {
                printf("\t%d bytes\n",
                       hdtr.trailers[i].iov_len);
            }

            rv = apr_sendfile(sock, f, &hdtr, &current_file_offset, &tmplen, 0);
            printf("apr_sendfile()->%d, sent %ld bytes\n", rv, (long)tmplen);
            if (rv) {
                if (APR_STATUS_IS_EAGAIN(rv)) {
                    nsocks = 1;
                    tmprv = apr_poll(pfd, &nsocks, -1);
                    assert(!tmprv);
                    assert(nsocks == 1);
                    /* continue; */
                }
            }

            total_bytes_sent += tmplen;

            /* Adjust hdtr to compensate for partially-written
             * data.
             */

            /* First, skip over any header data which might have
             * been written.
             */
            while (tmplen && hdtr.numheaders) {
                if (tmplen >= hdtr.headers[0].iov_len) {
                    tmplen -= hdtr.headers[0].iov_len;
                    --hdtr.numheaders;
                    ++hdtr.headers;
                }
                else {
                    hdtr.headers[0].iov_len -= tmplen;
                    hdtr.headers[0].iov_base = 
			(char*) hdtr.headers[0].iov_base + tmplen;
                    tmplen = 0;
                }
            }

            /* Now, skip over any file data which might have been
             * written.
             */

            if (tmplen <= len) {
                current_file_offset += tmplen;
                len -= tmplen;
                tmplen = 0;
            }
            else {
                tmplen -= len;
                len = 0;
                current_file_offset = 0;
            }

            /* Last, skip over any trailer data which might have
             * been written.
             */

            while (tmplen && hdtr.numtrailers) {
                if (tmplen >= hdtr.trailers[0].iov_len) {
                    tmplen -= hdtr.trailers[0].iov_len;
                    --hdtr.numtrailers;
                    ++hdtr.trailers;
                }
                else {
                    hdtr.trailers[0].iov_len -= tmplen;
                    hdtr.trailers[0].iov_base = 
			(char *)hdtr.trailers[0].iov_base + tmplen;
                    tmplen = 0;
                }
            }

        } while (total_bytes_sent < expected_len &&
                 (rv == APR_SUCCESS || 
                  APR_STATUS_IS_EAGAIN(rv)));
        if (total_bytes_sent != expected_len) {
            fprintf(stderr,
                    "client problem: sent %ld of %ld bytes\n",
                    (long)total_bytes_sent, (long)expected_len);
            exit(1);
        }

        if (rv) {
            fprintf(stderr,
                    "client problem: rv %d\n",
                    rv);
            exit(1);
        }
    }
    
    current_file_offset = 0;
    rv = apr_seek(f, APR_CUR, &current_file_offset);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_seek()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("After apr_sendfile(), the kernel file pointer is "
           "at offset %ld.\n",
           (long int)current_file_offset);

    rv = apr_shutdown(sock, APR_SHUTDOWN_WRITE);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_shutdown()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    bytes_read = 1;
    rv = apr_recv(sock, buf, &bytes_read);
    if (rv != APR_EOF) {
        fprintf(stderr, "apr_recv()->%d/%s (expected APR_EOF)\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != 0) {
        fprintf(stderr, "We expected to get 0 bytes read with APR_EOF\n"
                "but instead we read %ld bytes.\n",
                (long int)bytes_read);
        exit(1);
    }

    printf("client: apr_sendfile() worked as expected!\n");

    rv = apr_remove_file(TESTFILE, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_remove_file()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    return 0;
}

static int server(void)
{
    apr_status_t rv;
    apr_socket_t *sock;
    apr_pool_t *p;
    char buf[120];
    int i;
    apr_socket_t *newsock = NULL;
    apr_ssize_t bytes_read;
    apr_sockaddr_t *localsa;
    int family;

    family = APR_UNSPEC;
    apr_setup(&p, &sock, &family);

    rv = apr_setsocketopt(sock, APR_SO_REUSEADDR, 1);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_setsocketopt()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_getaddrinfo(&localsa, NULL, family, TESTSF_PORT, 0, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_getaddrinfo()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_bind(sock, localsa);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_bind()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = apr_listen(sock, 5);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_listen()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("Waiting for a client to connect...\n");

    rv = apr_accept(&newsock, sock, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_accept()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("Processing a client...\n");

    assert(sizeof buf > strlen(HDR1));
    bytes_read = strlen(HDR1);
    rv = apr_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_recv()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(HDR1)) {
        fprintf(stderr, "wrong data read (1)\n");
        exit(1);
    }
    if (memcmp(buf, HDR1, strlen(HDR1))) {
        fprintf(stderr, "wrong data read (2)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, HDR1);
        exit(1);
    }
        
    assert(sizeof buf > strlen(HDR2));
    bytes_read = strlen(HDR2);
    rv = apr_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_recv()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(HDR2)) {
        fprintf(stderr, "wrong data read (3)\n");
        exit(1);
    }
    if (memcmp(buf, HDR2, strlen(HDR2))) {
        fprintf(stderr, "wrong data read (4)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, HDR2);
        exit(1);
    }

    for (i = 0; i < HDR3_LEN; i++) {
        bytes_read = 1;
        rv = apr_recv(newsock, buf, &bytes_read);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_recv()->%d/%s\n",
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        if (bytes_read != 1) {
            fprintf(stderr, "apr_recv()->%ld bytes instead of 1\n",
                    (long int)bytes_read);
            exit(1);
        }
        if (buf[0] != HDR3_CHAR) {
            fprintf(stderr,
                    "problem with data read (byte %d of hdr 3):\n",
                    i);
            fprintf(stderr, "read `%c' (0x%x) from client; expected "
                    "`%c'\n",
                    buf[0], buf[0], HDR3_CHAR);
            exit(1);
        }
    }
        
    for (i = 0; i < FILE_LENGTH; i++) {
        bytes_read = 1;
        rv = apr_recv(newsock, buf, &bytes_read);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_recv()->%d/%s\n",
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        if (bytes_read != 1) {
            fprintf(stderr, "apr_recv()->%ld bytes instead of 1\n",
                    (long int)bytes_read);
            exit(1);
        }
        if (buf[0] != FILE_DATA_CHAR) {
            fprintf(stderr,
                    "problem with data read (byte %d of file):\n",
                    i);
            fprintf(stderr, "read `%c' (0x%x) from client; expected "
                    "`%c'\n",
                    buf[0], buf[0], FILE_DATA_CHAR);
            exit(1);
        }
    }
        
    assert(sizeof buf > strlen(TRL1));
    bytes_read = strlen(TRL1);
    rv = apr_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_recv()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(TRL1)) {
        fprintf(stderr, "wrong data read (5)\n");
        exit(1);
    }
    if (memcmp(buf, TRL1, strlen(TRL1))) {
        fprintf(stderr, "wrong data read (6)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, TRL1);
        exit(1);
    }
        
    assert(sizeof buf > strlen(TRL2));
    bytes_read = strlen(TRL2);
    rv = apr_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_recv()->%d/%s\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(TRL2)) {
        fprintf(stderr, "wrong data read (7)\n");
        exit(1);
    }
    if (memcmp(buf, TRL2, strlen(TRL2))) {
        fprintf(stderr, "wrong data read (8)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, TRL2);
        exit(1);
    }

    for (i = 0; i < TRL3_LEN; i++) {
        bytes_read = 1;
        rv = apr_recv(newsock, buf, &bytes_read);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "apr_recv()->%d/%s\n",
                    rv,
                    apr_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        if (bytes_read != 1) {
            fprintf(stderr, "apr_recv()->%ld bytes instead of 1\n",
                    (long int)bytes_read);
            exit(1);
        }
        if (buf[0] != TRL3_CHAR) {
            fprintf(stderr,
                    "problem with data read (byte %d of trl 3):\n",
                    i);
            fprintf(stderr, "read `%c' (0x%x) from client; expected "
                    "`%c'\n",
                    buf[0], buf[0], TRL3_CHAR);
            exit(1);
        }
    }
        
    bytes_read = 1;
    rv = apr_recv(newsock, buf, &bytes_read);
    if (rv != APR_EOF) {
        fprintf(stderr, "apr_recv()->%d/%s (expected APR_EOF)\n",
                rv,
		apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != 0) {
        fprintf(stderr, "We expected to get 0 bytes read with APR_EOF\n"
                "but instead we read %ld bytes (%c).\n",
                (long int)bytes_read, buf[0]);
        exit(1);
    }

    printf("server: apr_sendfile() worked as expected!\n");

    return 0;
}

int main(int argc, char *argv[])
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Gee whiz this is goofy logic but I wanna drive sendfile right now, 
     * not dork around with the command line!
     */
    if (argc == 3 && !strcmp(argv[1], "client")) {
        if (!strcmp(argv[2], "blocking")) {
            return client(BLK);
        }
        else if (!strcmp(argv[2], "timeout")) {
            return client(TIMEOUT);
        }
        else if (!strcmp(argv[2], "nonblocking")) {
            return client(NONBLK);
        }
    }
    else if (argc == 2 && !strcmp(argv[1], "server")) {
        return server();
    }

    fprintf(stderr, 
            "Usage: %s client {blocking|nonblocking|timeout}\n"
            "       %s server\n",
            argv[0], argv[0]);
    return -1;
}

#endif /* !APR_HAS_SENDFILE */
