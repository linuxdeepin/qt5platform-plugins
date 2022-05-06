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

#include "dwaylandintegration.h"
#include "dwaylandinterfacehook.h"
#include "../../src/vtablehook.h"
#include "../../src/dxcbxsettings.h"

#define private public
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandcursor_p.h>
#include <QtWaylandClient/private/qwaylandinputdevice_p.h>
#undef private

#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

#include <wayland-cursor.h>
#include <QtWaylandClientVersion>

DPP_BEGIN_NAMESPACE

#define XSETTINGS_DOUBLE_CLICK_TIME QByteArrayLiteral("Net/DoubleClickTime")
#define XSETTINGS_CURSOR_THEME_NAME QByteArrayLiteral("Gtk/CursorThemeName")
#define XSETTINGS_PRIMARY_MONITOR_NAME QByteArrayLiteral("Gdk/PrimaryMonitorName")

enum XSettingType:quint64 {
    Gtk_CursorThemeName,
    Gdk_PrimaryMonitorName
};

static void overrideChangeCursor(QPlatformCursor *cursorHandle, QCursor * cursor, QWindow * widget)
{
    if (!(widget && widget->handle()))
        return;

    if (widget->property(disableOverrideCursor).toBool())
        return;

    // qtwayland里面判断了，如果没有设置环境变量，就用默认大小32, 这里通过设在环境变量将大小设置我们需要的24
    static bool xcursorSizeIsSet = qEnvironmentVariableIsSet("XCURSOR_SIZE");
    if (!xcursorSizeIsSet) {
        qputenv("XCURSOR_SIZE", QByteArray::number(24 * qApp->devicePixelRatio()));
    }

    VtableHook::callOriginalFun(cursorHandle, &QPlatformCursor::changeCursor, cursor, widget);

    // 调用changeCursor后，控制中心窗口的光标没有及时更新，这里强制刷新一下
    for (auto *inputDevice : DWaylandIntegration::instance()->display()->inputDevices()) {
        if (inputDevice->pointer()) {
            inputDevice->pointer()->updateCursor();
        }
    }
}

static void onXSettingsChanged(xcb_connection_t *connection, const QByteArray &name, const QVariant &property, void *handle)
{
    Q_UNUSED(connection)
    Q_UNUSED(property)
    Q_UNUSED(name)

    quint64 type = reinterpret_cast<quint64>(handle);

    switch (type) {
    case Gtk_CursorThemeName:
        const QByteArray &cursor_name = DWaylandInterfaceHook::globalSettings()->setting(name).toByteArray();

#if QTWAYLANDCLIENT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        const auto &cursor_map = DWaylandIntegration::instance()->display()->mCursorThemesBySize;
#elif QTWAYLANDCLIENT_VERSION < QT_VERSION_CHECK(5, 16, 0)
        const auto &cursor_map = DWaylandIntegration::instance()->display()->mCursorThemes;
#endif
        // 处理光标主题变化
        for (auto cursor = cursor_map.constBegin(); cursor != cursor_map.constEnd(); ++cursor) {
            QtWaylandClient::QWaylandCursorTheme *ct = cursor.value();
            // 根据大小记载新的主题

#if QTWAYLANDCLIENT_VERSION < QT_VERSION_CHECK(5, 13, 0)
            auto theme = wl_cursor_theme_load(cursor_name.constData(), cursor.key(), DWaylandIntegration::instance()->display()->shm()->object());
#elif QTWAYLANDCLIENT_VERSION < QT_VERSION_CHECK(5, 16, 0)
            auto theme = wl_cursor_theme_load(cursor_name.constData(), cursor.key().second, DWaylandIntegration::instance()->display()->shm()->object());
#endif
            // 如果新主题创建失败则不更新数据
            if (!theme)
                continue;

            // 先尝试销毁旧主题
            if (ct->m_theme) {
                wl_cursor_theme_destroy(ct->m_theme);
            }

            // 清理缓存数据
            ct->m_cursors.clear();
            ct->m_theme = theme;
        }

        // 更新窗口光标
        for (auto s : DWaylandIntegration::instance()->display()->screens()) {
            for (QWindow *w : s->windows()) {
                QCursor cursor = w->cursor();
                // 为窗口重新设置光标
                s->cursor()->changeCursor(&cursor, w);
            }
        }

        break;

    }
}

static void onPrimaryScreenChanged(xcb_connection_t *connection, const QByteArray &name, const QVariant &property, void *handle)
{
    Q_UNUSED(connection)
    Q_UNUSED(property)

    quint64 type = reinterpret_cast<quint64>(handle);
    switch (type) {
    case Gdk_PrimaryMonitorName:
    {
        // 如果设置的主屏与当前获取的主屏一样，则不处理
        const QString &primaryScreenName = DWaylandInterfaceHook::globalSettings()->setting(name).toString();
        if (QGuiApplication::primaryScreen()->model().startsWith(primaryScreenName))
            break;

        // 设置新的主屏
        auto screens = DWaylandIntegration::instance()->display()->screens();
        for (auto screen : screens) {
            if (screen->model().startsWith(primaryScreenName)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
                QWindowSystemInterface::handlePrimaryScreenChanged(screen);
#endif
            }
        }
        break;
    }
    default:
        break;
    }
    qDebug() << "primary screen info:" << QGuiApplication::primaryScreen()->model() << QGuiApplication::primaryScreen()->geometry();
}

DWaylandIntegration::DWaylandIntegration()
{
    DWaylandInterfaceHook::instance()->initXcb();
}

void DWaylandIntegration::initialize()
{
    // 由于Qt代码中可能写死了判断是不是wayland平台，所以只能伪装成是wayland
    if (qgetenv("DXCB_FAKE_PLATFORM_NAME_XCB") != "0") {
        *QGuiApplicationPrivate::platform_name = "wayland";
    }
    qApp->setProperty("_d_isDwayland", true);

    QWaylandIntegration::initialize();
    // 覆盖wayland的平台函数接口，用于向上层返回一些必要的与平台相关的函数供其使用
    VtableHook::overrideVfptrFun(nativeInterface(), &QPlatformNativeInterface::platformFunction, &DWaylandInterfaceHook::platformFunction);
    // hook qtwayland的改变光标的函数，用于设置环境变量并及时更新
    for (QScreen *screen : qApp->screens()) {
        //在没有屏幕的时候，qtwayland会创建一个虚拟屏幕，此时不能调用screen->handle()->cursor()
        if (screen && screen->handle() && screen->handle()->cursor()) {
             VtableHook::overrideVfptrFun(screen->handle()->cursor(), &QPlatformCursor::changeCursor, &overrideChangeCursor);
        }
    }
    // 监听xsettings的信号，用于更新程序状态（如更新光标主题）
    DWaylandInterfaceHook::globalSettings()->registerCallbackForProperty(XSETTINGS_CURSOR_THEME_NAME, onXSettingsChanged, reinterpret_cast<void*>(XSettingType::Gtk_CursorThemeName));
    // 处理主屏设置，并监听xsettings的信号用于更新
    onPrimaryScreenChanged(nullptr, XSETTINGS_PRIMARY_MONITOR_NAME, QVariant(), reinterpret_cast<void*>(XSettingType::Gdk_PrimaryMonitorName));
    DWaylandInterfaceHook::globalSettings()->registerCallbackForProperty(XSETTINGS_PRIMARY_MONITOR_NAME, onPrimaryScreenChanged, reinterpret_cast<void*>(XSettingType::Gdk_PrimaryMonitorName));
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

QVariant DWaylandIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
#define GET_VALID_XSETTINGS(key) { \
    auto value = DWaylandInterfaceHook::globalSettings()->setting(key); \
    if (value.isValid()) return value; \
}

#ifdef Q_OS_LINUX
    switch (hint) {
    case MouseDoubleClickInterval:
        GET_VALID_XSETTINGS(XSETTINGS_DOUBLE_CLICK_TIME);
        break;
    default:
        break;
    }
#endif

    return QtWaylandClient::QWaylandIntegration::styleHint(hint);
}


DPP_END_NAMESPACE
