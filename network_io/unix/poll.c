/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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
#include "fileio.h"

#ifdef HAVE_POLL    /* We can just use poll to do our socket polling. */

apr_status_t apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->pollset = (struct pollfd *)apr_pcalloc(cont, 
                                         sizeof(struct pollfd) * num);

    if ((*new)->pollset == NULL) {
        return APR_ENOMEM;
    }
    (*new)->num = num;
    (*new)->cntxt = cont;
    (*new)->curpos = 0;
    return APR_SUCCESS;
}

static apr_int16_t get_event(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & APR_POLLIN)
        rv |= POLLIN;        
    if (event & APR_POLLPRI)
        rv |= POLLPRI;        
    if (event & APR_POLLOUT)
        rv |= POLLOUT;       
    if (event & APR_POLLERR)
        rv |= POLLERR;        
    if (event & APR_POLLHUP)
        rv |= POLLHUP;        
    if (event & APR_POLLNVAL)
        rv |= POLLNVAL;        

    return rv;
}

static apr_int16_t get_revent(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & POLLIN)
        rv |= APR_POLLIN;
    if (event & POLLPRI)
        rv |= APR_POLLPRI;
    if (event & POLLOUT)
        rv |= APR_POLLOUT;
    if (event & POLLERR)
        rv |= APR_POLLERR;
    if (event & POLLHUP)
        rv |= APR_POLLHUP;
    if (event & POLLNVAL)
        rv |= APR_POLLNVAL;

    return rv;
}        

apr_status_t apr_poll_socket_add(apr_pollfd_t *aprset, 
			       apr_socket_t *sock, apr_int16_t event)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset->pollset[i].fd != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        if(aprset->curpos == aprset->num) {
            return APR_ENOMEM;
        }
        aprset->curpos++;
    } 
    aprset->pollset[i].fd = sock->socketdes;
    aprset->pollset[i].events = get_event(event);

    return APR_SUCCESS;
}

apr_status_t apr_poll(apr_pollfd_t *aprset, apr_int32_t *nsds, 
		    apr_interval_time_t timeout)
{
    int rv;

    if (timeout > 0) {
        timeout /= 1000; /* convert microseconds to milliseconds */
    }

    rv = poll(aprset->pollset, aprset->curpos, timeout);
    (*nsds) = rv;
    
    if ((*nsds) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

apr_status_t apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset->pollset[i].fd != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        return APR_EINVALSOCK;
    } 
    (*event) = get_revent(aprset->pollset[i].revents);
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                  apr_socket_t *sock, apr_int16_t events)
{
    apr_int16_t newevents;
    int i = 0;
    
    while (i < aprset->curpos && aprset->pollset[i].fd != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        return APR_NOTFOUND;
    } 
    newevents = get_event(events);
    if (aprset->pollset[i].events & newevents) {
        aprset->pollset[i].events ^= newevents;
    }

    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    int i = 0;
    while(i < aprset->curpos && aprset->pollset[i].fd != sock->socketdes) {
	i++;
    }
    if(i >= aprset->curpos) {
	return APR_NOTFOUND;
    }
    while(++i < aprset->curpos) {
	aprset->pollset[i-1].fd = aprset->pollset[i].fd;
	aprset->pollset[i-1].events = aprset->pollset[i].events;
    }
    --aprset->curpos;
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t events)
{
    int i = 0;
    apr_int16_t newevents;

    newevents = get_event(events);

    while (i < aprset->curpos) {
        if (aprset->pollset[i].events & newevents) {
            aprset->pollset[i].events &= ~newevents;
        }
        i++;
    }
    return APR_SUCCESS;
}

#else    /* Use select to mimic poll */

apr_status_t apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t) * num);
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->read       = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->write      = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->except     = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->read_set   = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->write_set  = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->except_set = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    FD_ZERO((*new)->read);
    FD_ZERO((*new)->write);
    FD_ZERO((*new)->except);
    FD_ZERO((*new)->read_set);
    FD_ZERO((*new)->write_set);
    FD_ZERO((*new)->except_set);
    (*new)->highsock = -1;
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_add(apr_pollfd_t *aprset,
                               apr_socket_t *sock, apr_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_SET(sock->socketdes, aprset->read_set);
    }
    if (event & APR_POLLPRI) {
        FD_SET(sock->socketdes, aprset->except_set);
    }
    if (event & APR_POLLOUT) {
        FD_SET(sock->socketdes, aprset->write_set);
    }
    if (sock->socketdes > aprset->highsock) {
        aprset->highsock = sock->socketdes;
    }
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                  apr_socket_t *sock, 
                                  apr_int16_t events)
{
    if (events & APR_POLLIN) {
        FD_CLR(sock->socketdes, aprset->read_set);
    }
    if (events & APR_POLLPRI) {
        FD_CLR(sock->socketdes, aprset->except_set);
    }
    if (events & APR_POLLOUT) {
        FD_CLR(sock->socketdes, aprset->write_set);
    }
    return APR_SUCCESS;
}

apr_status_t apr_poll(apr_pollfd_t *aprset, apr_int32_t *nsds, 
		    apr_interval_time_t timeout)
{
    int rv;
    struct timeval tv, *tvptr;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = apr_time_sec(timeout);
        tv.tv_usec = apr_time_usec(timeout);
        tvptr = &tv;
    }

    memcpy(aprset->read, aprset->read_set, sizeof(fd_set));
    memcpy(aprset->write, aprset->write_set, sizeof(fd_set));
    memcpy(aprset->except, aprset->except_set, sizeof(fd_set));

    rv = select(aprset->highsock + 1, aprset->read, aprset->write, 
                aprset->except, tvptr);
    
    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

apr_status_t apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    apr_int16_t revents = 0;
    char data[1];
    int flags = MSG_PEEK;
  
    /* We just want to PEEK at the data, so I am setting up a dummy WSABUF
     * variable here.
     */
    if (FD_ISSET(sock->socketdes, aprset->read)) {
        revents |= APR_POLLIN;
        if (sock->connected
	    && recv(sock->socketdes, data, sizeof data, flags) == -1) {
            switch (errno) {
                case ECONNRESET:
                case ECONNABORTED:
                case ESHUTDOWN:
                case ENETRESET:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLHUP;
                    break;
                case ENOTSOCK:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLNVAL;
                    break;
                default:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLERR;
                    break;
            }
        }
    }
    if (FD_ISSET(sock->socketdes, aprset->write)) {
        revents |= APR_POLLOUT;
    }
    /* I am assuming that the except is for out of band data, not a failed
     * connection on a non-blocking socket.  Might be a bad assumption, but
     * it works for now. rbb.
     */
    if (FD_ISSET(sock->socketdes, aprset->except)) {
        revents |= APR_POLLPRI;
    }

    (*event) = revents;
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    FD_CLR(sock->socketdes, aprset->read_set);
    FD_CLR(sock->socketdes, aprset->except_set);
    FD_CLR(sock->socketdes, aprset->write_set);
    return APR_SUCCESS;
}

apr_status_t apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_ZERO(aprset->read_set);
    }
    if (event & APR_POLLPRI) {
        FD_ZERO(aprset->except_set);
    }
    if (event & APR_POLLOUT) {
        FD_ZERO(aprset->write_set);
    }
    aprset->highsock = 0;
    return APR_SUCCESS;
}

#endif 

apr_status_t apr_poll_data_get(apr_pollfd_t *pollfd, const char *key, void *data)
{
    return apr_pool_userdata_get(data, key, pollfd->cntxt);
}

apr_status_t apr_poll_data_set(apr_pollfd_t *pollfd, void *data, const char *key,
                            apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, pollfd->cntxt);
}

#if APR_FILES_AS_SOCKETS
/* I'm not sure if this needs to return an apr_status_t or not, but
 * for right now, we'll leave it this way, and change it later if
 * necessary.
 */
apr_status_t apr_socket_from_file(apr_socket_t **newsock, apr_file_t *file)
{
    (*newsock) = apr_pcalloc(file->pool, sizeof(**newsock));
    (*newsock)->socketdes = file->filedes;
    (*newsock)->cntxt = file->pool;
    (*newsock)->timeout = file->timeout;
    return APR_SUCCESS;
}
#endif
