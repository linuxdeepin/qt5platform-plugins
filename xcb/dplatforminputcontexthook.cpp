/*
* Copyright (C) 2020 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     wangpeng <wangpeng@uniontech.com>
*
* Maintainer: wangpeng <wangpeng@uniontech.com>
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
#include "dplatforminputcontexthook.h"

#include "global.h"
#include <qpa/qplatforminputcontext.h>

DPP_BEGIN_NAMESPACE

void DPlatformInputContextHook::showInputPanel(QPlatformInputContext *inputContext)
{
    Q_UNUSED(inputContext)
    instance()->setImActive(true);
}

void DPlatformInputContextHook::hideInputPanel(QPlatformInputContext *inputContext)
{
    Q_UNUSED(inputContext)
    instance()->setImActive(false);
}

bool DPlatformInputContextHook::isInputPanelVisible(QPlatformInputContext *inputContext)
{
    Q_UNUSED(inputContext)
    return instance()->imActive();
}

QRectF DPlatformInputContextHook::keyboardRect(QPlatformInputContext *inputContext)
{
    Q_UNUSED(inputContext)
    return instance()->geometry();
}

Q_GLOBAL_STATIC_WITH_ARGS(ComDeepinImInterface, __imInterface,
                          (QString("com.deepin.im"), QString("/com/deepin/im"), QDBusConnection::sessionBus()))

ComDeepinImInterface* DPlatformInputContextHook::instance()
{
    return __imInterface;
}

DPP_END_NAMESPACE
