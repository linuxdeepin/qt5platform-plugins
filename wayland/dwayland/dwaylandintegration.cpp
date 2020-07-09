/*
 * Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
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

#include "dwaylandintegration.h"
#include "dwaylandinterfacehook.h"
#include "vtablehook.h"

#include <qpa/qplatformnativeinterface.h>

DPP_BEGIN_NAMESPACE

DWaylandIntegration::DWaylandIntegration()
{
    DWaylandInterfaceHook::init();
}

void DWaylandIntegration::initialize()
{
    QWaylandIntegration::initialize();
    // 覆盖wayland的平台函数接口，用于向上层返回一些必要的与平台相关的函数供其使用
    VtableHook::overrideVfptrFun(nativeInterface(), &QPlatformNativeInterface::platformFunction, &DWaylandInterfaceHook::platformFunction);
}

QStringList DWaylandIntegration::themeNames() const
{
    auto list = QWaylandIntegration::themeNames();
    const QByteArray desktop_session = qgetenv("DESKTOP_SESSION");

    // 在lightdm环境中，无此环境变量。默认使用deepin平台主题
    if (desktop_session.isEmpty() || desktop_session == "deepin")
        list.prepend("deepin");

    return list;
}

DPP_END_NAMESPACE
