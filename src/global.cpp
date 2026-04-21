// SPDX-FileCopyrightText: 2017 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

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

DPP_BEGIN_NAMESPACE

RunInThreadProxy::RunInThreadProxy(QObject *parent)
    : QObject(parent)
{
}

void RunInThreadProxy::proxyCall(FunctionType func)
{
    QObject *receiver = parent();
    if (!receiver)
        receiver = qApp;

    QObject scope;
    connect(&scope, &QObject::destroyed, receiver, [func]() {
        (func)();
    }, Qt::QueuedConnection);
}

// 判断起始按压点是否在窗口左/右/下边缘的手势保护区内
bool isInEdgeMargin(const QPoint &pressLocalPos, const QSize &windowSize)
{
    static int margin = []() {
        bool ok;
        int v = qEnvironmentVariableIntValue("D_DXCB_EDGE_MARGIN", &ok);
        return (ok && v >= 0) ? v : 5;
    }();
    return pressLocalPos.x() < margin
        || pressLocalPos.x() >= windowSize.width() - margin
        || pressLocalPos.y() >= windowSize.height() - margin;
}

DPP_END_NAMESPACE
