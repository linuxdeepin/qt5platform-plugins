/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DXCBXSETTINGS_H
#define DXCBXSETTINGS_H

#include "qxcbscreen.h"
#include "global.h"

DPP_BEGIN_NAMESPACE

class DXcbXSettingsPrivate;

class DXcbXSettings
{
    Q_DECLARE_PRIVATE(DXcbXSettings)
public:
    DXcbXSettings(QXcbVirtualDesktop *screen, const QByteArray &property = QByteArray());
    DXcbXSettings(QXcbVirtualDesktop *screen, xcb_window_t setting_window, const QByteArray &property = QByteArray());
    DXcbXSettings(xcb_window_t setting_window, const QByteArray &property = QByteArray());
    ~DXcbXSettings();
    bool initialized() const;
    bool isEmpty() const;

    bool contains(const QByteArray &property) const;
    QVariant setting(const QByteArray &property) const;
    void setSetting(const QByteArray &property, const QVariant &value);

    typedef void (*PropertyChangeFunc)(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &property, void *handle);
    void registerCallback(PropertyChangeFunc func, void *handle);
    void registerCallbackForProperty(const QByteArray &property, PropertyChangeFunc func, void *handle);
    void removeCallbackForHandle(const QByteArray &property, void *handle);
    void removeCallbackForHandle(void *handle);

    static bool handlePropertyNotifyEvent(const xcb_property_notify_event_t *event);
    static bool handleClientMessageEvent(const xcb_client_message_event_t *event);

    static void clearSettings(xcb_window_t setting_window);
private:
    DXcbXSettingsPrivate *d_ptr;
};

DPP_END_NAMESPACE

#endif // DXCBXSETTINGS_H
