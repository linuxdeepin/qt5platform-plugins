// Copyright (C) 2017 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDABSTRACTDECORATION_H
#define QWAYLANDABSTRACTDECORATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QMargins>
#include <QtCore/QPointF>
#include <QtGui/QGuiApplication>
#include <QtGui/QCursor>
#include <QtGui/QColor>
#include <QtGui/QStaticText>
#include <QtGui/QImage>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <wayland-client.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class QWindow;
class QPaintDevice;
class QPainter;
class QEvent;

namespace QtWaylandClient {

class QWaylandScreen;
class QWaylandWindow;
class QWaylandInputDevice;
class QWaylandAbstractDecorationPrivate;

class Q_WAYLAND_CLIENT_EXPORT QWaylandAbstractDecoration : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandAbstractDecoration)
public:
    QWaylandAbstractDecoration();
    ~QWaylandAbstractDecoration() override;

    void setWaylandWindow(QWaylandWindow *window);
    QWaylandWindow *waylandWindow() const;

    void update();
    bool isDirty() const;

    virtual QMargins margins() const = 0;
    QWindow *window() const;
    const QImage &contentImage();

    virtual bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,Qt::MouseButtons b,Qt::KeyboardModifiers mods) = 0;
    virtual bool handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::TouchPointState state, Qt::KeyboardModifiers mods) = 0;

protected:
    virtual void paint(QPaintDevice *device) = 0;

    void setMouseButtons(Qt::MouseButtons mb);

    void startResize(QWaylandInputDevice *inputDevice,enum wl_shell_surface_resize resize, Qt::MouseButtons buttons);
    void startMove(QWaylandInputDevice *inputDevice, Qt::MouseButtons buttons);

    bool isLeftClicked(Qt::MouseButtons newMouseButtonState);
    bool isLeftReleased(Qt::MouseButtons newMouseButtonState);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDABSTRACTDECORATION_H
