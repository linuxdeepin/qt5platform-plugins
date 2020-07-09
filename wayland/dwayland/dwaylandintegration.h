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

#ifndef DWAYLANDINTEGRATION_H
#define DWAYLANDINTEGRATION_H

#include "global.h"
#include "QtWaylandClient/private/qwaylandintegration_p.h"

DPP_BEGIN_NAMESPACE

class DWaylandIntegration : public QtWaylandClient::QWaylandIntegration
{
public:
    DWaylandIntegration();

    void initialize() override;
    QStringList themeNames() const override;
};

DPP_END_NAMESPACE

#endif // DWAYLANDINTEGRATION_H
