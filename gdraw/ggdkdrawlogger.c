/* Copyright (C) 2016 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ggdkdrawlogger.c
 * @brief Implement logging and error handling functions
 */

#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK
#include <stdio.h>
#include <stdarg.h>

static const char *unspecified_funct = "???";

/**
 * Print a message to stderr and log it via syslog. The message must be
 * less than BUFSIZ characters long, or it will be truncated.
 * @param level - Specify how severe the message is.
	If level is higher (less urgent) than the program's verbosity (see options.h) no message will be printed
 * @param funct - String indicating the function name from which this function was called.
	If this is NULL, Log will show the unspecified_funct string instead
 * @param file - Source file containing the function
 * @param line - Line in the source file at which Log is called
 * @param fmt - A format string
 * @param ... - Arguments to be printed according to the format string
 */
void LogEx(int level, const char *funct, const char *file, int line, const char *fmt, ...) {
    char buffer[BUFSIZ];
    va_list va;

    if (getenv("GGDK_QUIET")) {
        return;
    }

    va_start(va, fmt);
    vsnprintf(buffer, BUFSIZ, fmt, va);
    va_end(va);

    if (funct == NULL) {
        funct = unspecified_funct;
    }

    // Make a human readable severity string
    const char *severity;
    switch (level) {
        case LOGERR:
            severity = "ERROR";
            break;
        case LOGWARN:
            severity = "WARNING";
            break;
        case LOGNOTE:
            severity = "NOTICE";
            break;
        case LOGINFO:
            severity = "INFO";
            break;
        default:
            severity = "DEBUG";
            break;
    }

    GDateTime *now = g_date_time_new_now_local();
    fprintf(stderr, "%02d:%02d:%02.3f %s: %s (%s:%d) - %s\n",
            g_date_time_get_hour(now), g_date_time_get_minute(now),
            g_date_time_get_seconds(now),
            severity, funct, file, line, buffer);
    fflush(stderr);
    g_date_time_unref(now);
}

const char *GdkEventName(int code) {
    switch (code) {
        case GDK_NOTHING:
            return "GDK_NOTHING";
            break;
        case GDK_DELETE:
            return "GDK_DELETE";
            break;
        case GDK_DESTROY:
            return "GDK_DESTROY";
            break;
        case GDK_EXPOSE:
            return "GDK_EXPOSE";
            break;
        case GDK_MOTION_NOTIFY:
            return "GDK_MOTION_NOTIFY";
            break;
        case GDK_BUTTON_PRESS:
            return "GDK_BUTTON_PRESS";
            break;
        case GDK_2BUTTON_PRESS:
            return "GDK_2BUTTON_PRESS";
            break;
        case GDK_3BUTTON_PRESS:
            return "GDK_3BUTTON_PRESS";
            break;
        case GDK_BUTTON_RELEASE:
            return "GDK_BUTTON_RELEASE";
            break;
        case GDK_KEY_PRESS:
            return "GDK_KEY_PRESS";
            break;
        case GDK_KEY_RELEASE:
            return "GDK_KEY_RELEASE";
            break;
        case GDK_ENTER_NOTIFY:
            return "GDK_ENTER_NOTIFY";
            break;
        case GDK_LEAVE_NOTIFY:
            return "GDK_LEAVE_NOTIFY";
            break;
        case GDK_FOCUS_CHANGE:
            return "GDK_FOCUS_CHANGE";
            break;
        case GDK_CONFIGURE:
            return "GDK_CONFIGURE";
            break;
        case GDK_MAP:
            return "GDK_MAP";
            break;
        case GDK_UNMAP:
            return "GDK_UNMAP";
            break;
        case GDK_PROPERTY_NOTIFY:
            return "GDK_PROPERTY_NOTIFY";
            break;
        case GDK_SELECTION_CLEAR:
            return "GDK_SELECTION_CLEAR";
            break;
        case GDK_SELECTION_REQUEST:
            return "GDK_SELECTION_REQUEST";
            break;
        case GDK_SELECTION_NOTIFY:
            return "GDK_SELECTION_NOTIFY";
            break;
        case GDK_PROXIMITY_IN:
            return "GDK_PROXIMITY_IN";
            break;
        case GDK_PROXIMITY_OUT:
            return "GDK_PROXIMITY_OUT";
            break;
        case GDK_DRAG_ENTER:
            return "GDK_DRAG_ENTER";
            break;
        case GDK_DRAG_LEAVE:
            return "GDK_DRAG_LEAVE";
            break;
        case GDK_DRAG_MOTION:
            return "GDK_DRAG_MOTION";
            break;
        case GDK_DRAG_STATUS:
            return "GDK_DRAG_STATUS";
            break;
        case GDK_DROP_START:
            return "GDK_DROP_START";
            break;
        case GDK_DROP_FINISHED:
            return "GDK_DROP_FINISHED";
            break;
        case GDK_CLIENT_EVENT:
            return "GDK_CLIENT_EVENT";
            break;
        case GDK_VISIBILITY_NOTIFY:
            return "GDK_VISIBILITY_NOTIFY";
            break;
        case GDK_SCROLL:
            return "GDK_SCROLL";
            break;
        case GDK_WINDOW_STATE:
            return "GDK_WINDOW_STATE";
            break;
        case GDK_SETTING:
            return "GDK_SETTING";
            break;
        case GDK_OWNER_CHANGE:
            return "GDK_OWNER_CHANGE";
            break;
        case GDK_GRAB_BROKEN:
            return "GDK_GRAB_BROKEN";
            break;
        case GDK_DAMAGE:
            return "GDK_DAMAGE";
            break;
#ifndef GGDKDRAW_GDK_2
        case GDK_TOUCH_BEGIN:
            return "GDK_TOUCH_BEGIN";
            break;
        case GDK_TOUCH_UPDATE:
            return "GDK_TOUCH_UPDATE";
            break;
        case GDK_TOUCH_END:
            return "GDK_TOUCH_END";
            break;
        case GDK_TOUCH_CANCEL:
            return "GDK_TOUCH_CANCEL";
            break;
#endif
#ifdef GGDKDRAW_GDK_3_20
        case GDK_TOUCHPAD_SWIPE:
            return "GDK_TOUCHPAD_SWIPE";
            break;
        case GDK_TOUCHPAD_PINCH:
            return "GDK_TOUCHPAD_PINCH";
            break;
#endif
        case GDK_EVENT_LAST:
            return "GDK_EVENT_LAST";
            break;
        default:
            return "UNKNOWN";
    }
}

#endif // FONTFORGE_CAN_USE_GDK
