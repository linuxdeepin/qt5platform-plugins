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
