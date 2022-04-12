/*
   * Copyright (C) 2022 Uniontech Software Technology Co.,Ltd.
   *
   * Author:     chenke <chenke@uniontech.com>
   *
   * Maintainer: chenke <chenke@uniontech.com>
   *
   * This program is free software: you can redistribute it and/or modify
   * it under the terms of the GNU Lesser General Public License as published by
   * the Free Software Foundation, either version 3 of the License, or
   * any later version.
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU Lesser General Public License
   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */
#ifndef DWAYLANDSHELLMANAGERV2_H
#define DWAYLANDSHELLMANAGERV2_H

#include <QMargins>
#include <QObject>
#include <KWayland/Client/registry.h>
//#include <private/qwaylandshellintegration_p.h>

QT_BEGIN_NAMESPACE
class QPlatformWindow;
QT_END_NAMESPACE


using namespace KWayland::Client;

namespace QtWaylandClient {
class QWaylandWindow;
class QWaylandShellSurface;
class QWaylandShellIntegration;

class DWaylandShellManagerV2Private;
class DWaylandShellManagerV2 : public QObject
{
    Q_OBJECT
public:
    explicit DWaylandShellManagerV2(QObject *parent = nullptr);
    virtual ~DWaylandShellManagerV2();
    void init(Registry *registry);

    QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *integration, QWaylandWindow *window);
    void sendProperty(QWaylandWindow *window, const QString &name, const QVariant &value);
    void setPostion(QWaylandShellSurface *surface, const QPoint &pos);
    void setRole(QWaylandShellSurface *surface, int role);
    void setWindowStaysOnTop(QWaylandShellSurface *surface, bool ontop = true);


public slots:
    void createPlasmaShell(quint32 name, quint32 version);
    void createKWaylandSSD(quint32 name, quint32 version);
    void createDDEShell(quint32 name, quint32 version);
    void createDDESeat(quint32 name, quint32 version);
    void createDDEPointer();
    void createDDETouch();
    void createDDEKeyboard();
    void createDDEFakeInput();
    void createServerDecoration(QWaylandWindow *window);
private:
    Q_DECLARE_PRIVATE(DWaylandShellManagerV2)
};
}
#endif // DWAYLANDSHELLMANAGERV2_H
