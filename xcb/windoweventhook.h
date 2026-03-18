// SPDX-FileCopyrightText: 2016 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef WINDOWEVENTHOOK_H
#define WINDOWEVENTHOOK_H

#include "qxcbwindow.h"

#include "global.h"

DPP_BEGIN_NAMESPACE

class WindowEventHook
{
public:
    static void init(QXcbWindow *window, bool redirectContent);

    static void handleConfigureNotifyEvent(QXcbWindowEventListener *el, const xcb_configure_notify_event_t *);
    static void handleMapNotifyEvent(QXcbWindowEventListener *el, const xcb_map_notify_event_t *);

    static void handleClientMessageEvent(QXcbWindowEventListener *el, const xcb_client_message_event_t *event);
    static void handleFocusInEvent(QXcbWindowEventListener *el, const xcb_focus_in_event_t *event);
    static void handleFocusOutEvent(QXcbWindowEventListener *el, const xcb_focus_out_event_t *event);
    static void handlePropertyNotifyEvent(QXcbWindowEventListener *el, const xcb_property_notify_event_t *event);
#ifdef XCB_USE_XINPUT22
    static void handleXIEnterLeave(QXcbWindowEventListener *el, xcb_ge_event_t *event);
#endif
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
    static void windowEvent(QPlatformWindow *window, QEvent *event);
#else
    static bool windowEvent(QXcbWindow *window, QEvent *event);
#endif
};

DPP_END_NAMESPACE

#endif // WINDOWEVENTHOOK_H
