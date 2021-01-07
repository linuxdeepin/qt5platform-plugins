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
#ifndef DPLATFORMINPUTCONTEXTHOOK_H
#define DPLATFORMINPUTCONTEXTHOOK_H

#include "global.h"

#include <QRectF>
#include <QtGlobal>

#include "im_interface.h"


QT_BEGIN_NAMESPACE
class QPlatformInputContext;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DPlatformInputContextHook
{

public:
    static void showInputPanel(QPlatformInputContext *inputContext);
    static void hideInputPanel(QPlatformInputContext *inputContext);
    static bool isInputPanelVisible(QPlatformInputContext *inputContext);
    static QRectF keyboardRect(QPlatformInputContext *inputContext);

    static ComDeepinImInterface* instance();
};

DPP_END_NAMESPACE

#endif // DPLATFORMINPUTCONTEXTHOOK_H
