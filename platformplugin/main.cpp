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

#include <qpa/qplatformintegrationplugin.h>
#include "dplatformintegration.h"

#include <QDebug>

DPP_USE_NAMESPACE

QT_BEGIN_NAMESPACE

class DPlatformIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dpp.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) Q_DECL_OVERRIDE;
};

QPlatformIntegration* DPlatformIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
#ifdef Q_OS_LINUX
    if (system == "dxcb" || system == "xcb") {
        return new DPlatformIntegration(parameters, argc, argv);
    }
#endif

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
