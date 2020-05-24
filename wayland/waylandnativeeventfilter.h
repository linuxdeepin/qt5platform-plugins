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

#ifndef WAYLANDNATIVEEVENTFILTER_H
#define WAYLANDNATIVEEVENTFILTER_H

#include "global.h"

#include <QAbstractNativeEventFilter>
#include <QClipboard>
#include <QHash>

#include <xcb/xproto.h>

QT_BEGIN_NAMESPACE
class QXcbConnection;
class QInputEvent;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class WaylandNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    WaylandNativeEventFilter(QXcbConnection *connection);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

private:
    QXcbConnection *m_connection;
};

DPP_END_NAMESPACE

#endif // WAYLANDNATIVEEVENTFILTER_H
