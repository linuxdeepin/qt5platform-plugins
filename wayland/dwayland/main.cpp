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
#include "global.h"

#include "dwaylandintegration.h"

#include <qpa/qplatformintegrationplugin.h>

DPP_USE_NAMESPACE

QT_BEGIN_NAMESPACE

class DWaylandIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dwayland.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration* DWaylandIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
#ifdef Q_OS_LINUX
    Q_UNUSED(system);
    Q_UNUSED(paramList);
    auto *integration =  new DWaylandIntegration();

    if (integration->hasFailed()) {
        delete integration;
        integration = nullptr;
    }

    return integration;
#endif

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
