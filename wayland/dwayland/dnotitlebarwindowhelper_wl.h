/*
 * Copyright (C) 2017 ~ 2019 Uniontech Technology Co., Ltd.
 *
 * Author:     WangPeng <wangpenga@uniontech.com>
 *
 * Maintainer: AlexOne  <993381@qq.com>
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
#ifndef DNOTITLEBARWINDOWHELPER_WL_H
#define DNOTITLEBARWINDOWHELPER_WL_H

#include "global.h"

#include <QObject>
#include <QPainterPath>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DNativeSettings;
class DNoTitlebarWlWindowHelper : public QObject
{
    Q_OBJECT

public:
    explicit DNoTitlebarWlWindowHelper(QWindow *window);
    ~DNoTitlebarWlWindowHelper();

    inline QWindow *window() const {
        return reinterpret_cast<QWindow*>(const_cast<DNoTitlebarWlWindowHelper*>(this));
    }

    static void setWindowProperty(QWindow *window, const char *name, const QVariant &value);
    static void popupSystemWindowMenu(quintptr wid);

private:
    QWindow *m_window;

public:
    static QHash<const QWindow*, DNoTitlebarWlWindowHelper*> mapped;
};

DPP_END_NAMESPACE

Q_DECLARE_METATYPE(QPainterPath)

#endif // DNOTITLEBARWINDOWHELPER_WL_H
