#ifndef SHELL_COMMON
#define SHELL_COMMON

#include "vtablehook.h"

#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#include "QtWaylandClient/private/qwaylandcursor_p.h"
#undef private

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/server_decoration.h>
#include <KWayland/Client/ddeshell.h>
#include <KWayland/Client/ddeseat.h>
#include <KWayland/Client/ddekeyboard.h>
#include <KWayland/Client/strut.h>
#include <KWayland/Client/fakeinput.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <private/qwidgetwindow_p.h>

#include <KWayland/Client/blur.h>
#include <KWayland/Client/region.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/compositor.h>

#pragma GCC diagnostic ignored "-Wc++17-extensions"

DPP_USE_NAMESPACE
using namespace KWayland::Client;

namespace DWaylandPointer {
// kwayland中PlasmaShell的全局对象，用于使用kwayland中的扩展协议
inline PlasmaShell *kwayland_shell = nullptr;
// kwin合成器提供的窗口边框管理器
inline ServerSideDecorationManager *kwayland_ssd = nullptr;
// 创建ddeshell
inline DDEShell *ddeShell = nullptr;
// kwayland
inline Strut *kwayland_strut = nullptr;
inline DDESeat *kwayland_dde_seat = nullptr;
inline DDETouch *kwayland_dde_touch = nullptr;
inline DDEPointer *kwayland_dde_pointer = nullptr;
inline FakeInput *kwayland_dde_fake_input = nullptr;
inline DDEKeyboard *kwayland_dde_keyboard = nullptr;

inline BlurManager *kwayland_blur_manager = nullptr;
inline Surface *kwayland_surface = nullptr;
inline Compositor *kwayland_compositor = nullptr;
}

#endif//SHELL_COMMON
