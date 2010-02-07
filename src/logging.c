/*
 * SpanDSP - a series of DSP components for telephony
 *
 * logging.c - error and debug logging.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2005 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: logging.c,v 1.21 2006/10/24 13:45:25 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "spandsp/telephony.h"
#include "spandsp/logging.h"

static void default_message_handler(int level, const char *text);

static message_handler_func_t __span_message = *default_message_handler;
static error_handler_func_t __span_error = NULL;

static const char *severities[] =
{
    "NONE",
    "ERROR",
    "WARNING",
    "PROTOCOL_ERROR",
    "PROTOCOL_WARNING",
    "FLOW",
    "FLOW 2",
    "FLOW 3",
    "DEBUG 1",
    "DEBUG 2",
    "DEBUG 3"
};

static void default_message_handler(int level, const char *text)
{
    fprintf(stderr, "%s", text);
}
/*- End of function --------------------------------------------------------*/

int span_log_test(logging_state_t *s, int level)
{
    if (s  &&  (s->level & SPAN_LOG_SEVERITY_MASK) >= (level & SPAN_LOG_SEVERITY_MASK))
        return TRUE;
    return FALSE;
}
/*- End of function --------------------------------------------------------*/

int span_log(logging_state_t *s, int level, const char *format, ...)
{
    char msg[1024 + 1];
    va_list arg_ptr;
    int len;
    struct tm *tim;
    struct timeval nowx;
    time_t now;

    if (span_log_test(s, level))
    {
        va_start(arg_ptr, format);
        len = 0;
        if ((level & SPAN_LOG_SUPPRESS_LABELLING) == 0)
        {
            if ((s->level & SPAN_LOG_SHOW_DATE))
            {
                gettimeofday(&nowx, NULL);
                tim = gmtime(&nowx.tv_sec);
                snprintf(msg + len,
                         1024 - len,
                         "%04d/%02d/%02d %02d:%02d:%02d.%03d ",
                         tim->tm_year + 1900,
                         tim->tm_mon + 1,
                         tim->tm_mday,
                         tim->tm_hour,
                         tim->tm_min,
                         tim->tm_sec,
                         (int) nowx.tv_usec/1000);
                len += (int) strlen(msg + len);
            }
            /*endif*/
            if ((s->level & SPAN_LOG_SHOW_SAMPLE_TIME))
            {
                now = s->elapsed_samples/s->samples_per_second;
                tim = gmtime(&now);
                snprintf(msg + len,
                         1024 - len,
                         "%02d:%02d:%02d.%03d ",
                         tim->tm_hour,
                         tim->tm_min,
                         tim->tm_sec,
                         (int) (s->elapsed_samples%s->samples_per_second)*1000/s->samples_per_second);
                len += (int) strlen(msg + len);
            }
            /*endif*/
            if ((s->level & SPAN_LOG_SHOW_SEVERITY)  &&  (level & SPAN_LOG_SEVERITY_MASK) <= SPAN_LOG_DEBUG_3)
                len += snprintf(msg + len, 1024 - len, "%s ", severities[level & SPAN_LOG_SEVERITY_MASK]);
            /*endif*/
            if ((s->level & SPAN_LOG_SHOW_PROTOCOL)  &&  s->protocol)
                len += snprintf(msg + len, 1024 - len, "%s ", s->protocol);
            /*endif*/
            if ((s->level & SPAN_LOG_SHOW_TAG)  &&  s->tag)
                len += snprintf(msg + len, 1024 - len, "%s ", s->tag);
            /*endif*/
        }
        /*endif*/
        len += vsnprintf(msg + len, 1024 - len, format, arg_ptr);
        if (s->span_error  &&  level == SPAN_LOG_ERROR)
            s->span_error(msg);
        else if (__span_error  &&  level == SPAN_LOG_ERROR)
            __span_error(msg);
        else if (s->span_message)
            s->span_message(level, msg);
        else if (__span_message)
            __span_message(level, msg);
        /*endif*/
        va_end(arg_ptr);
        return  1;
    }
    /*endif*/
    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_buf(logging_state_t *s, int level, const char *tag, const uint8_t *buf, int len)
{
    char msg[1024];
    int i;
    int msg_len;

    if (span_log_test(s, level))
    {
        msg_len = 0;
        if (tag)
            msg_len += snprintf(msg + msg_len, 1024 - msg_len, "%s", tag);
        for (i = 0;  i < len  &&  msg_len < 800;  i++)
            msg_len += snprintf(msg + msg_len, 1024 - msg_len, " %02x", buf[i]);
        msg_len += snprintf(msg + msg_len, 1024 - msg_len, "\n");
        return span_log(s, level, msg);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

int span_log_init(logging_state_t *s, int level, const char *tag)
{
    s->span_error = __span_error;
    s->span_message = __span_message;
    s->level = level;
    s->tag = tag;
    s->protocol = NULL;
    s->samples_per_second = SAMPLE_RATE;
    s->elapsed_samples = 0;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_set_level(logging_state_t *s, int level)
{
    s->level = level;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_set_tag(logging_state_t *s, const char *tag)
{
    s->tag = tag;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_set_protocol(logging_state_t *s, const char *protocol)
{
    s->protocol = protocol;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_set_sample_rate(logging_state_t *s, int samples_per_second)
{
    s->samples_per_second = samples_per_second;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_bump_samples(logging_state_t *s, int samples)
{
    s->elapsed_samples += samples;

    return  0;
}
/*- End of function --------------------------------------------------------*/

void span_log_set_message_handler(logging_state_t *s, message_handler_func_t func)
{
    s->span_message = func;
}
/*- End of function --------------------------------------------------------*/

void span_log_set_error_handler(logging_state_t *s, error_handler_func_t func)
{
    s->span_error = func;
}
/*- End of function --------------------------------------------------------*/

void span_set_message_handler(message_handler_func_t func)
{
    __span_message = func;
}
/*- End of function --------------------------------------------------------*/

void span_set_error_handler(error_handler_func_t func)
{
    __span_error = func;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
