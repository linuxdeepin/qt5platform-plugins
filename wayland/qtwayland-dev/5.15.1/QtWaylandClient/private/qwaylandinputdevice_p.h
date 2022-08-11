// Copyright (C) 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDINPUTDEVICE_H
#define QWAYLANDINPUTDEVICE_H

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

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <QSocketNotifier>
#include <QObject>
#include <QTimer>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <wayland-client.h>

#include <QtWaylandClient/private/qwayland-wayland.h>

#if QT_CONFIG(xkbcommon_evdev)
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#endif

#include <QtCore/QDebug>
#include <QPointer>

#if QT_CONFIG(cursor)
struct wl_cursor_image;
#endif

#if QT_CONFIG(xkbcommon_evdev)
struct xkb_compose_state;
struct xkb_compose_table;
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandDisplay;
class QWaylandDataDevice;
class QWaylandTextInput;

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice
                            : public QObject
                            , public QtWayland::wl_seat
{
    Q_OBJECT
public:
    class Keyboard;
    class Pointer;
    class Touch;

    QWaylandInputDevice(QWaylandDisplay *display, int version, uint32_t id);
    ~QWaylandInputDevice() override;

    uint32_t capabilities() const { return mCaps; }

    struct ::wl_seat *wl_seat() { return QtWayland::wl_seat::object(); }

#if QT_CONFIG(cursor)
    void setCursor(const QCursor &cursor, QWaylandScreen *screen);
    void setCursor(struct wl_buffer *buffer, struct ::wl_cursor_image *image, int bufferScale);
    void setCursor(struct wl_buffer *buffer, const QPoint &hotSpot, const QSize &size, int bufferScale);
    void setCursor(const QSharedPointer<QWaylandBuffer> &buffer, const QPoint &hotSpot, int bufferScale);
#endif
    void handleWindowDestroyed(QWaylandWindow *window);
    void handleEndDrag();

#if QT_CONFIG(wayland_datadevice)
    void setDataDevice(QWaylandDataDevice *device);
    QWaylandDataDevice *dataDevice() const;
#endif

    void setTextInput(QWaylandTextInput *textInput);
    QWaylandTextInput *textInput() const;

    void removeMouseButtonFromState(Qt::MouseButton button);

    QWaylandWindow *pointerFocus() const;
    QWaylandWindow *keyboardFocus() const;
    QWaylandWindow *touchFocus() const;

    Qt::KeyboardModifiers modifiers() const;

    uint32_t serial() const;
    uint32_t cursorSerial() const;

    virtual Keyboard *createKeyboard(QWaylandInputDevice *device);
    virtual Pointer *createPointer(QWaylandInputDevice *device);
    virtual Touch *createTouch(QWaylandInputDevice *device);

private:
    void setCursor(Qt::CursorShape cursor, QWaylandScreen *screen);

    QWaylandDisplay *mQDisplay = nullptr;
    struct wl_display *mDisplay = nullptr;

    int mVersion;
    uint32_t mCaps = 0;

    struct wl_surface *pointerSurface = nullptr;

#if QT_CONFIG(wayland_datadevice)
    QWaylandDataDevice *mDataDevice = nullptr;
#endif

    Keyboard *mKeyboard = nullptr;
    Pointer *mPointer = nullptr;
    Touch *mTouch = nullptr;

    QWaylandTextInput *mTextInput = nullptr;

    uint32_t mTime = 0;
    uint32_t mSerial = 0;

    void seat_capabilities(uint32_t caps) override;
    void handleTouchPoint(int id, double x, double y, Qt::TouchPointState state);

    QTouchDevice *mTouchDevice = nullptr;

    QSharedPointer<QWaylandBuffer> mPixmapCursor;

    friend class QWaylandTouchExtension;
    friend class QWaylandQtKeyExtension;
};

inline uint32_t QWaylandInputDevice::serial() const
{
    return mSerial;
}


class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Keyboard : public QObject, public QtWayland::wl_keyboard
{
    Q_OBJECT

public:
    Keyboard(QWaylandInputDevice *p);
    ~Keyboard() override;

    void stopRepeat();

    void keyboard_keymap(uint32_t format,
                         int32_t fd,
                         uint32_t size) override;
    void keyboard_enter(uint32_t time,
                        struct wl_surface *surface,
                        struct wl_array *keys) override;
    void keyboard_leave(uint32_t time,
                        struct wl_surface *surface) override;
    void keyboard_key(uint32_t serial, uint32_t time,
                      uint32_t key, uint32_t state) override;
    void keyboard_modifiers(uint32_t serial,
                            uint32_t mods_depressed,
                            uint32_t mods_latched,
                            uint32_t mods_locked,
                            uint32_t group) override;

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandWindow> mFocus;
#if QT_CONFIG(xkbcommon_evdev)
    xkb_context *mXkbContext = nullptr;
    xkb_keymap *mXkbMap = nullptr;
    xkb_state *mXkbState = nullptr;
    xkb_compose_table *mXkbComposeTable = nullptr;
    xkb_compose_state *mXkbComposeState = nullptr;
#endif
    uint32_t mNativeModifiers = 0;

    int mRepeatKey;
    uint32_t mRepeatCode;
    uint32_t mRepeatTime;
    QString mRepeatText;
#if QT_CONFIG(xkbcommon_evdev)
    xkb_keysym_t mRepeatSym;
#endif
    QTimer mRepeatTimer;

    Qt::KeyboardModifiers modifiers() const;

private slots:
    void repeatKey();

private:
#if QT_CONFIG(xkbcommon_evdev)
    bool createDefaultKeyMap();
    void releaseKeyMap();
    void createComposeState();
    void releaseComposeState();
#endif

};

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Pointer : public QtWayland::wl_pointer
{

public:
    Pointer(QWaylandInputDevice *p);
    ~Pointer() override;

    void pointer_enter(uint32_t serial, struct wl_surface *surface,
                       wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_leave(uint32_t time, struct wl_surface *surface) override;
    void pointer_motion(uint32_t time,
                        wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_button(uint32_t serial, uint32_t time,
                        uint32_t button, uint32_t state) override;
    void pointer_axis(uint32_t time,
                      uint32_t axis,
                      wl_fixed_t value) override;

    void releaseButtons();

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandWindow> mFocus;
    uint32_t mEnterSerial = 0;
#if QT_CONFIG(cursor)
    uint32_t mCursorSerial = 0;
#endif
    QPointF mSurfacePos;
    QPointF mGlobalPos;
    Qt::MouseButtons mButtons = Qt::NoButton;
#if QT_CONFIG(cursor)
    wl_buffer *mCursorBuffer = nullptr;
    Qt::CursorShape mCursorShape = Qt::BitmapCursor;
#endif
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Touch : public QtWayland::wl_touch
{
public:
    Touch(QWaylandInputDevice *p);
    ~Touch() override;

    void touch_down(uint32_t serial,
                    uint32_t time,
                    struct wl_surface *surface,
                    int32_t id,
                    wl_fixed_t x,
                    wl_fixed_t y) override;
    void touch_up(uint32_t serial,
                  uint32_t time,
                  int32_t id) override;
    void touch_motion(uint32_t time,
                      int32_t id,
                      wl_fixed_t x,
                      wl_fixed_t y) override;
    void touch_frame() override;
    void touch_cancel() override;

    bool allTouchPointsReleased();
    void releasePoints();

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandWindow> mFocus;
    QList<QWindowSystemInterface::TouchPoint> mTouchPoints;
    QList<QWindowSystemInterface::TouchPoint> mPrevTouchPoints;
};

class QWaylandPointerEvent
{
public:
    enum Type {
        Enter,
        Motion,
        Wheel
    };
    inline QWaylandPointerEvent(Type t, ulong ts, const QPointF &l, const QPointF &g, Qt::MouseButtons b, Qt::KeyboardModifiers m)
        : type(t)
        , timestamp(ts)
        , local(l)
        , global(g)
        , buttons(b)
        , modifiers(m)
    {}
    inline QWaylandPointerEvent(Type t, ulong ts, const QPointF &l, const QPointF &g, const QPoint &pd, const QPoint &ad, Qt::KeyboardModifiers m)
        : type(t)
        , timestamp(ts)
        , local(l)
        , global(g)
        , modifiers(m)
        , pixelDelta(pd)
        , angleDelta(ad)
    {}

    Type type;
    ulong timestamp;
    QPointF local;
    QPointF global;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    QPoint pixelDelta;
    QPoint angleDelta;
};

}

QT_END_NAMESPACE

#endif
