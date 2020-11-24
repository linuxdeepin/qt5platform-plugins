/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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

#include "dplatformnativeinterfacehook.h"
#include "global.h"
#include "utility.h"
#include "dplatformwindowhelper.h"
#include "dplatformintegration.h"

#include "dwmsupport.h"

#ifdef Q_OS_LINUX
#include "xcbnativeeventfilter.h"
#include "qxcbnativeinterface.h"
typedef QXcbNativeInterface DPlatformNativeInterface;
#elif defined(Q_OS_WIN)
#include "qwindowsgdinativeinterface.h"
typedef QWindowsGdiNativeInterface DPlatformNativeInterface;
#endif

DPP_BEGIN_NAMESPACE

static QString version()
{
    return QStringLiteral(DXCB_VERSION);
}

#ifdef Q_OS_LINUX
static DeviceType _inputEventSourceDevice(const QInputEvent *event)
{
    return DPlatformIntegration::instance()->eventFilter()->xiEventSource(event);
}
#endif

static QFunctionPointer getFunction(const QByteArray &function)
{
    if (function == setWmBlurWindowBackgroundArea) {
        return reinterpret_cast<QFunctionPointer>(&Utility::blurWindowBackground);
    } else if (function == setWmBlurWindowBackgroundPathList) {
        return reinterpret_cast<QFunctionPointer>(&Utility::blurWindowBackgroundByPaths);
    } else if (function == setWmBlurWindowBackgroundMaskImage) {
        return reinterpret_cast<QFunctionPointer>(&Utility::blurWindowBackgroundByImage);
    } else if (function == setWmWallpaperParameter) {
        return reinterpret_cast<QFunctionPointer>(&Utility::updateBackgroundWallpaper);
    } else if (function == hasBlurWindow) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::hasBlurWindow);
    } else if (function == hasComposite) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::hasComposite);
    } else if (function == hasNoTitlebar) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::hasNoTitlebar);
    } else if (function == hasWindowAlpha) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::hasWindowAlpha);
    } else if (function == hasWallpaperEffect) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::hasWallpaperEffect);
    } else if (function == windowManagerName) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::Global::windowManagerName);
    } else if (function == connectWindowManagerChangedSignal) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectWindowManagerChangedSignal);
    } else if (function == connectHasBlurWindowChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectHasBlurWindowChanged);
    } else if (function == connectHasCompositeChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectHasCompositeChanged);
    } else if (function == connectHasNoTitlebarChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectHasNoTitlebarChanged);
    } else if (function == connectHasWallpaperEffectChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectHasWallpaperEffectChanged);
    } else if (function == getWindows) {
        return reinterpret_cast<QFunctionPointer>(&Utility::getWindows);
    } else if (function == windowFromPoint) {
        return reinterpret_cast<QFunctionPointer>(&Utility::windowFromPoint);
    } else if (function == getCurrentWorkspaceWindows) {
        return reinterpret_cast<QFunctionPointer>(&Utility::getCurrentWorkspaceWindows);
    } else if (function == connectWindowListChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectWindowListChanged);
    } else if (function == setMWMFunctions) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::setMWMFunctions);
    } else if (function == getMWMFunctions) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::getMWMFunctions);
    } else if (function == setMWMDecorations) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::setMWMDecorations);
    } else if (function == getMWMDecorations) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::getMWMDecorations);
    } else if (function == connectWindowMotifWMHintsChanged) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::connectWindowMotifWMHintsChanged);
    } else if (function == popupSystemWindowMenu) {
        return reinterpret_cast<QFunctionPointer>(&DWMSupport::popupSystemWindowMenu);
    } else if (function == setWindowProperty) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::setWindowProperty);
    } else if (function == pluginVersion) {
        return reinterpret_cast<QFunctionPointer>(&version);
    } else if (function == inputEventSourceDevice) {
        return reinterpret_cast<QFunctionPointer>(&_inputEventSourceDevice);
    } else if (function == createGroupWindow) {
        return reinterpret_cast<QFunctionPointer>(&Utility::createGroupWindow);
    } else if (function == destoryGroupWindow) {
        return reinterpret_cast<QFunctionPointer>(&Utility::destoryGroupWindow);
    } else if (function == setWindowGroup) {
        return reinterpret_cast<QFunctionPointer>(&Utility::setWindowGroup);
    } else if (function == clientLeader) {
        return reinterpret_cast<QFunctionPointer>(&Utility::clientLeader);
    } else if (function == enableDxcb) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::enableDxcb);
    } else if (function == isEnableDxcb) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::isEnableDxcb);
    } else if (function == setEnableNoTitlebar) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::setEnableNoTitlebar);
    } else if (function == isEnableNoTitlebar) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::isEnableNoTitlebar);
    } else if (function == buildNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::buildNativeSettings);
    } else if (function == clearNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformIntegration::clearNativeSettings);
    }

    return nullptr;
}

thread_local QHash<QByteArray, QFunctionPointer> DPlatformNativeInterfaceHook::functionCache;
QFunctionPointer DPlatformNativeInterfaceHook::platformFunction(QPlatformNativeInterface *interface, const QByteArray &function)
{
    if (QFunctionPointer f = functionCache.value(function)) {
        return f;
    }

    QFunctionPointer f = getFunction(function);

    if (f) {
        functionCache.insert(function, f);
        return f;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    return static_cast<DPlatformNativeInterface*>(interface)->DPlatformNativeInterface::platformFunction(function);
#endif
    return 0;
}

DPP_END_NAMESPACE
