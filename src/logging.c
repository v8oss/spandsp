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
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
 * $Id: logging.c,v 1.2 2005/08/31 19:27:52 steveu Exp $
 */

/*! \file */

//#define _ISOC9X_SOURCE  1
//#define _ISOC99_SOURCE  1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "spandsp/telephony.h"
#include "spandsp/logging.h"

static void (*__span_error)(const char *text);
static void (*__span_message)(const char *text);

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

int span_log(logging_state_t *s, int level, const char *format, ...)
{
    char msg[1024 + 1];
    va_list arg_ptr;
    int len;
    char timestr[17 + 1];
    struct tm *tim;
    time_t now;

    if (s  &&  (s->level & SPAN_LOG_SEVERITY_MASK) >= level)
    {
        va_start(arg_ptr, format);
        len = 0;
        if ((s->level & SPAN_LOG_SHOW_DATE))
        {
            time(&now);
            tim = gmtime(&now);
            sprintf(msg + len,
                    "%04d/%02d/%02d %02d:%02d:%02d ",
                    tim->tm_year + 1900,
                    tim->tm_mon + 1,
                    tim->tm_mday,
                    tim->tm_hour,
                    tim->tm_min,
                    tim->tm_sec);
            len += strlen(msg + len);
        }
        /*endif*/
        if ((s->level & SPAN_LOG_SHOW_SEVERITY)  &&  (s->level & SPAN_LOG_SEVERITY_MASK) <= SPAN_LOG_DEBUG_3)
        {
            strcpy(msg + len, severities[s->level & SPAN_LOG_SEVERITY_MASK]);
            len += strlen(msg + len);
            msg[len++] = ' ';
        }
        /*endif*/
        if ((s->level & SPAN_LOG_SHOW_PROTOCOL)  &&  s->protocol)
        {
            strcpy(msg + len, s->protocol);
            len += strlen(msg + len);
            msg[len++] = ' ';
        }
        /*endif*/
        if ((s->level & SPAN_LOG_SHOW_TAG)  &&  s->tag)
        {
            strcpy(msg + len, s->tag);
            len += strlen(msg + len);
            msg[len++] = ' ';
        }
        /*endif*/
        len += vsnprintf(msg + len, 1024 - len, format, arg_ptr);
        if (__span_error)
            __span_error(msg);
        else
            fprintf(stderr, msg);
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
    int i;
    char msg[1024];
    char *t;
    
    if (s  &&  (s->level & SPAN_LOG_SEVERITY_MASK) >= level)
    {
        t = msg;
        strcpy(t, tag);
        t += strlen(t);
        *t++ = ' ';
        for (i = 0;  i < len;  i++)
        {
            sprintf(t, " %02x", buf[i]);
            t += 3;
        }
        *t++ = '\n';
        *t = '\0';
        return span_log(s, level, msg);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

int span_log_init(logging_state_t *s, int level, const char *tag)
{
    s->level = level;
    s->tag = tag;
    s->protocol = NULL;

    return  0;
}
/*- End of function --------------------------------------------------------*/

int span_log_set_protocol(logging_state_t *s, const char *protocol)
{
    s->protocol = protocol;

    return  0;
}
/*- End of function --------------------------------------------------------*/

void span_set_error_handler(void (*func)(const char *text))
{
    __span_error = func;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/