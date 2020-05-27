/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "waylandnativeeventfilter.h"

#define private public
#include "qxcbconnection.h"
#undef private

#include "dxcbxsettings.h"

DPP_BEGIN_NAMESPACE

WaylandNativeEventFilter::WaylandNativeEventFilter(QXcbConnection *connection)
    : m_connection(connection)
{

}

bool WaylandNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
    uint response_type = event->response_type & ~0x80;

    switch (response_type) {
        case XCB_PROPERTY_NOTIFY: {
            xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;

            DXcbXSettings::handlePropertyNotifyEvent(pn);
            break;
        }

        case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t *ev = reinterpret_cast<xcb_client_message_event_t*>(event);

            if (DXcbXSettings::handleClientMessageEvent(ev)) {
                return true;
            }
            break;
        }
    }

    return false;
}

DPP_END_NAMESPACE
