// SPDX-FileCopyrightText: 2017 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "global.h"
#include <QWindow>
#include <QGuiApplication>

QWindow * fromQtWinId(WId id) {
    QWindow *window = nullptr;

    for (auto win : qApp->allWindows()) {
        if (win->winId() == id) {
            window = win;
            break;
        }
    }
    return window;
};
