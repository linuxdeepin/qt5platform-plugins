// Copyright (C) 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDDISPLAY_H
#define QWAYLANDDISPLAY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include <QtCore/QWaitCondition>
#include <QtCore/QLoggingCategory>

#include <wayland-client.h>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylandshm_p.h>

struct wl_cursor_image;

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QSocketNotifier;
class QPlatformScreen;

namespace QtWayland {
    class qt_surface_extension;
    class zwp_text_input_manager_v2;
    class zxdg_output_manager_v1;
}

namespace QtWaylandClient {

Q_WAYLAND_CLIENT_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcQpaWayland);

class QWaylandInputDevice;
class QWaylandBuffer;
class QWaylandScreen;
class QWaylandClientBufferIntegration;
class QWaylandWindowManagerIntegration;
class QWaylandDataDeviceManager;
class QWaylandTouchExtension;
class QWaylandQtKeyExtension;
class QWaylandWindow;
class QWaylandIntegration;
class QWaylandHardwareIntegration;
class QWaylandShellSurface;
class QWaylandCursorTheme;

typedef void (*RegistryListener)(void *data,
                                 struct wl_registry *registry,
                                 uint32_t id,
                                 const QString &interface,
                                 uint32_t version);

class Q_WAYLAND_CLIENT_EXPORT QWaylandDisplay : public QObject, public QtWayland::wl_registry {
    Q_OBJECT

public:
    QWaylandDisplay(QWaylandIntegration *waylandIntegration);
    ~QWaylandDisplay(void) override;

    QList<QWaylandScreen *> screens() const { return mScreens; }

    QWaylandScreen *screenForOutput(struct wl_output *output) const;

    struct wl_surface *createSurface(void *handle);
    QWaylandShellSurface *createShellSurface(QWaylandWindow *window);
    struct ::wl_region *createRegion(const QRegion &qregion);
    struct ::wl_subsurface *createSubSurface(QWaylandWindow *window, QWaylandWindow *parent);

    QWaylandClientBufferIntegration *clientBufferIntegration() const;

    QWaylandWindowManagerIntegration *windowManagerIntegration() const;
#if QT_CONFIG(cursor)
    void setCursor(struct wl_buffer *buffer, struct wl_cursor_image *image, qreal dpr);
    void setCursor(const QSharedPointer<QWaylandBuffer> &buffer, const QPoint &hotSpot, qreal dpr);
    QWaylandCursorTheme *loadCursorTheme(qreal devicePixelRatio);
#endif
    struct wl_display *wl_display() const { return mDisplay; }
    struct ::wl_registry *wl_registry() { return object(); }

    const struct wl_compositor *wl_compositor() const { return mCompositor.object(); }
    QtWayland::wl_compositor *compositor() { return &mCompositor; }
    int compositorVersion() const { return mCompositorVersion; }

    QList<QWaylandInputDevice *> inputDevices() const { return mInputDevices; }
    QWaylandInputDevice *defaultInputDevice() const;
    QWaylandInputDevice *currentInputDevice() const { return defaultInputDevice(); }
#if QT_CONFIG(wayland_datadevice)
    QWaylandDataDeviceManager *dndSelectionHandler() const { return mDndSelectionHandler.data(); }
#endif
    QtWayland::qt_surface_extension *windowExtension() const { return mWindowExtension.data(); }
    QWaylandTouchExtension *touchExtension() const { return mTouchExtension.data(); }
    QtWayland::zwp_text_input_manager_v2 *textInputManager() const { return mTextInputManager.data(); }
    QWaylandHardwareIntegration *hardwareIntegration() const { return mHardwareIntegration.data(); }
    QtWayland::zxdg_output_manager_v1 *xdgOutputManager() const { return mXdgOutputManager.data(); }


    struct RegistryGlobal {
        uint32_t id;
        QString interface;
        uint32_t version;
        struct ::wl_registry *registry = nullptr;
        RegistryGlobal(uint32_t id_, const QString &interface_, uint32_t version_, struct ::wl_registry *registry_)
            : id(id_), interface(interface_), version(version_), registry(registry_) { }
    };
    QList<RegistryGlobal> globals() const { return mGlobals; }
    bool hasRegistryGlobal(const QString &interfaceName);

    /* wl_registry_add_listener does not add but rather sets a listener, so this function is used
     * to enable many listeners at once. */
    void addRegistryListener(RegistryListener listener, void *data);

    QWaylandShm *shm() const { return mShm.data(); }

    static uint32_t currentTimeMillisec();

    void forceRoundTrip();

    bool supportsWindowDecoration() const;

    uint32_t lastInputSerial() const { return mLastInputSerial; }
    QWaylandInputDevice *lastInputDevice() const { return mLastInputDevice; }
    QWaylandWindow *lastInputWindow() const;
    void setLastInputDevice(QWaylandInputDevice *device, uint32_t serial, QWaylandWindow *window);

    void handleWindowActivated(QWaylandWindow *window);
    void handleWindowDeactivated(QWaylandWindow *window);
    void handleKeyboardFocusChanged(QWaylandInputDevice *inputDevice);
    void handleWindowDestroyed(QWaylandWindow *window);

public slots:
    void blockingReadEvents();
    void flushRequests();

private:
    void waitForScreens();
    void exitWithError();
    void checkError() const;

    void handleWaylandSync();
    void requestWaylandSync();

    struct Listener {
        RegistryListener listener = nullptr;
        void *data = nullptr;
    };

    struct wl_display *mDisplay = nullptr;
    QtWayland::wl_compositor mCompositor;
    QScopedPointer<QWaylandShm> mShm;
    QList<QWaylandScreen *> mScreens;
    QList<QWaylandInputDevice *> mInputDevices;
    QList<Listener> mRegistryListeners;
    QWaylandIntegration *mWaylandIntegration = nullptr;
#if QT_CONFIG(cursor)
    QMap<int, QWaylandCursorTheme *> mCursorThemesBySize;
#endif
#if QT_CONFIG(wayland_datadevice)
    QScopedPointer<QWaylandDataDeviceManager> mDndSelectionHandler;
#endif
    QScopedPointer<QtWayland::qt_surface_extension> mWindowExtension;
    QScopedPointer<QtWayland::wl_subcompositor> mSubCompositor;
    QScopedPointer<QWaylandTouchExtension> mTouchExtension;
    QScopedPointer<QWaylandQtKeyExtension> mQtKeyExtension;
    QScopedPointer<QWaylandWindowManagerIntegration> mWindowManagerIntegration;
    QScopedPointer<QtWayland::zwp_text_input_manager_v2> mTextInputManager;
    QScopedPointer<QWaylandHardwareIntegration> mHardwareIntegration;
    QScopedPointer<QtWayland::zxdg_output_manager_v1> mXdgOutputManager;
    QSocketNotifier *mReadNotifier = nullptr;
    int mFd;
    int mWritableNotificationFd;
    QList<RegistryGlobal> mGlobals;
    int mCompositorVersion;
    uint32_t mLastInputSerial = 0;
    QWaylandInputDevice *mLastInputDevice = nullptr;
    QPointer<QWaylandWindow> mLastInputWindow;
    QPointer<QWaylandWindow> mLastKeyboardFocus;
    QVector<QWaylandWindow *> mActiveWindows;
    struct wl_callback *mSyncCallback = nullptr;
    static const wl_callback_listener syncCallbackListener;

    void registry_global(uint32_t id, const QString &interface, uint32_t version) override;
    void registry_global_remove(uint32_t id) override;

    static void shellHandleConfigure(void *data, struct wl_shell *shell,
                                     uint32_t time, uint32_t edges,
                                     struct wl_surface *surface,
                                     int32_t width, int32_t height);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDDISPLAY_H
