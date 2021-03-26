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
#include "dxcbxsettings.h"

#define private public
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandcursor_p.h>
#undef private

#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

#include <wayland-cursor.h>

DPP_BEGIN_NAMESPACE

enum XSettingType:quint64 {
    Gtk_CursorThemeName
};

static void onXSettingsChanged(xcb_connection_t *connection, const QByteArray &name, const QVariant &property, void *handle)
{
    Q_UNUSED(connection)
    Q_UNUSED(property)
    Q_UNUSED(name)

    quint64 type = reinterpret_cast<quint64>(handle);

    switch (type) {
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    case Gtk_CursorThemeName:
        const QByteArray &cursor_name = DWaylandInterfaceHook::globalSettings()->setting(name).toByteArray();
        const auto &cursor_map = DWaylandIntegration::instance()->display()->mCursorThemesBySize;

        // 处理光标主题变化
        for (auto cursor = cursor_map.constBegin(); cursor != cursor_map.constEnd(); ++cursor) {
            QtWaylandClient::QWaylandCursorTheme *ct = cursor.value();
            // 根据大小记载新的主题
            auto theme = wl_cursor_theme_load(cursor_name.constData(), cursor.key(), DWaylandIntegration::instance()->display()->shm()->object());

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
#endif
    }
}

DWaylandIntegration *DWaylandIntegration::m_instance = nullptr;
DWaylandIntegration::DWaylandIntegration()
{
    m_instance = this;
    DWaylandInterfaceHook::init();
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

    // 监听xsettings的信号，用于更新程序状态（如更新光标主题）
    DWaylandInterfaceHook::globalSettings()->registerCallbackForProperty("Gtk/CursorThemeName", onXSettingsChanged, reinterpret_cast<void*>(XSettingType::Gtk_CursorThemeName));
}

QStringList DWaylandIntegration::themeNames() const
{
    auto list = QWaylandIntegration::themeNames();
    const QByteArray desktop_session = qgetenv("DESKTOP_SESSION");

    // 在lightdm环境中，无此环境变量。默认使用deepin平台主题
    if (desktop_session.isEmpty() || desktop_session.startsWith("deepin"))
        list.prepend("deepin");

    return list;
}

DPP_END_NAMESPACE
