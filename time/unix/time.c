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

#include "apr_portable.h"
#include "apr_time.h"
#include "apr_lib.h"
#include "apr_private.h"
#include "apr_strings.h"
/* System Headers required for time library */
#if APR_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
/* End System Headers */


apr_status_t apr_ansi_time_to_apr_time(apr_time_t *result, time_t input)
{
    *result = (apr_time_t)input * APR_USEC_PER_SEC;
    return APR_SUCCESS;
}


apr_time_t apr_time_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * APR_USEC_PER_SEC + tv.tv_usec;
}

static void set_xt_gmtoff_from_tm(apr_exploded_time_t *xt, struct tm *tm,
                                  time_t *tt)
{
#ifdef HAVE_GMTOFF
    xt->tm_gmtoff = tm->tm_gmtoff;
#elif defined(HAVE___OFFSET)
    xt->tm_gmtoff = tm->__tm_gmtoff;
#else
    {
        struct tm t;
        int days = 0, hours = 0, minutes = 0;
#ifdef HAVE_GMTIME_R
        gmtime_r(tt, &t);
#else
        t = *gmtime(tt);
#endif
        days = xt->tm_yday - t.tm_yday;
        hours = ((days < -1 ? 24 : 1 < days ? -24 : days * 24) +
                 xt->tm_hour - t.tm_hour);
        minutes = hours * 60 + xt->tm_min - t.tm_min;
        xt->tm_gmtoff = minutes * 60;
    }
#endif
}

static void tm_to_exp(apr_exploded_time_t *xt, struct tm *tm, time_t *tt)
{
    xt->tm_sec  = tm->tm_sec;
    xt->tm_min  = tm->tm_min;
    xt->tm_hour = tm->tm_hour;
    xt->tm_mday = tm->tm_mday;
    xt->tm_mon  = tm->tm_mon;
    xt->tm_year = tm->tm_year;
    xt->tm_wday = tm->tm_wday;
    xt->tm_yday = tm->tm_yday;
    xt->tm_isdst = tm->tm_isdst;
    set_xt_gmtoff_from_tm(xt,tm,tt);
}

apr_status_t apr_explode_time(apr_exploded_time_t *result, apr_time_t input,
                              apr_int32_t offs)
{
    time_t t = (input / APR_USEC_PER_SEC) + offs;
    result->tm_usec = input % APR_USEC_PER_SEC;

#if APR_HAS_THREADS && defined (_POSIX_THREAD_SAFE_FUNCTIONS)
    {
        struct tm apple;
        gmtime_r(&t, &apple);
        tm_to_exp(result, &apple, &t);
    }
#else
    tm_to_exp(result, gmtime(&t), &t);
#endif
    result->tm_gmtoff = offs;
    return APR_SUCCESS;
}

apr_status_t apr_explode_gmt(apr_exploded_time_t *result, apr_time_t input)
{
    return apr_explode_time(result, input, 0);
}

apr_status_t apr_explode_localtime(apr_exploded_time_t *result, apr_time_t input)
{
#if defined(__EMX__)
    /* EMX gcc (OS/2) has a timezone global we can use */
    return apr_explode_time(result, input, -timezone);
#else
    time_t mango = input / APR_USEC_PER_SEC;
    apr_int32_t offs = 0;

#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    struct tm mangotm;
    localtime_r(&mango, &mangotm);
/* XXX - Add support for Solaris */
#ifdef HAVE_GMTOFF
    offs = mangotm.tm_gmtoff;
#elif defined(HAVE___OFFSET)
    offs = mangotm.__tm_gmtoff;
#endif
#else /* !APR_HAS_THREADS */
    struct tm *mangotm;
    mangotm=localtime(&mango);
#ifdef HAVE_GMTOFF
    offs = mangotm->tm_gmtoff;
#endif    
#endif
    return apr_explode_time(result, input, offs);
#endif /* __EMX__ */
}

apr_status_t apr_implode_time(apr_time_t *t, apr_exploded_time_t *xt)
{
    int year;
    time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    year = xt->tm_year;
    if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138))) {
        return APR_EBADDATE;
    }

    /* shift new year to 1st March in order to make leap year calc easy */

    if (xt->tm_mon < 2)
        year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[xt->tm_mon] + xt->tm_mday - 1;
    days -= 25508;              /* 1 jan 1970 is 25508 days since 1 mar 1900 */
    days = ((days * 24 + xt->tm_hour) * 60 + xt->tm_min) * 60 + xt->tm_sec;

    if (days < 0) {
        return APR_EBADDATE;
    }
    *t = days * APR_USEC_PER_SEC + xt->tm_usec;
    return APR_SUCCESS;
}

apr_status_t apr_os_imp_time_get(apr_os_imp_time_t **ostime, apr_time_t *aprtime)
{
    (*ostime)->tv_usec = *aprtime % APR_USEC_PER_SEC;
    (*ostime)->tv_sec = *aprtime / APR_USEC_PER_SEC;
    return APR_SUCCESS;
}

apr_status_t apr_os_exp_time_get(apr_os_exp_time_t **ostime, 
                                 apr_exploded_time_t *aprtime)
{
    (*ostime)->tm_sec  = aprtime->tm_sec;
    (*ostime)->tm_min  = aprtime->tm_min;
    (*ostime)->tm_hour = aprtime->tm_hour;
    (*ostime)->tm_mday = aprtime->tm_mday;
    (*ostime)->tm_mon  = aprtime->tm_mon;
    (*ostime)->tm_year = aprtime->tm_year;
    (*ostime)->tm_wday = aprtime->tm_wday;
    (*ostime)->tm_yday = aprtime->tm_yday;
    (*ostime)->tm_isdst = aprtime->tm_isdst;
    /* XXX - Need to handle gmt_offset's here ! */ 
    return APR_SUCCESS;
}

apr_status_t apr_os_imp_time_put(apr_time_t *aprtime, apr_os_imp_time_t **ostime,
                               apr_pool_t *cont)
{
    *aprtime = (*ostime)->tv_sec * APR_USEC_PER_SEC + (*ostime)->tv_usec;
    return APR_SUCCESS;
}

apr_status_t apr_os_exp_time_put(apr_exploded_time_t *aprtime,
                                 apr_os_exp_time_t **ostime, apr_pool_t *cont)
{
    aprtime->tm_sec = (*ostime)->tm_sec;
    aprtime->tm_min = (*ostime)->tm_min;
    aprtime->tm_hour = (*ostime)->tm_hour;
    aprtime->tm_mday = (*ostime)->tm_mday;
    aprtime->tm_mon = (*ostime)->tm_mon;
    aprtime->tm_year = (*ostime)->tm_year;
    aprtime->tm_wday = (*ostime)->tm_wday;
    aprtime->tm_yday = (*ostime)->tm_yday;
    aprtime->tm_isdst = (*ostime)->tm_isdst;
    /* XXX - Need to handle gmt_offsets here */
    return APR_SUCCESS;
}

void apr_sleep(apr_interval_time_t t)
{
#ifdef OS2
    DosSleep(t/1000);
#elif defined(BEOS)
    snooze(t);
#else
    struct timeval tv;
    tv.tv_usec = t % APR_USEC_PER_SEC;
    tv.tv_sec = t / APR_USEC_PER_SEC;
    select(0, NULL, NULL, NULL, &tv);
#endif
}

#ifdef OS2
apr_status_t apr_os2_time_to_apr_time(apr_time_t *result, FDATE os2date, 
                                      FTIME os2time)
{
  struct tm tmpdate;

  memset(&tmpdate, 0, sizeof(tmpdate));
  tmpdate.tm_hour  = os2time.hours;
  tmpdate.tm_min   = os2time.minutes;
  tmpdate.tm_sec   = os2time.twosecs * 2;

  tmpdate.tm_mday  = os2date.day;
  tmpdate.tm_mon   = os2date.month - 1;
  tmpdate.tm_year  = os2date.year + 80;
  tmpdate.tm_isdst = -1;

  *result = mktime(&tmpdate) * APR_USEC_PER_SEC;
  return APR_SUCCESS;
}
#endif
