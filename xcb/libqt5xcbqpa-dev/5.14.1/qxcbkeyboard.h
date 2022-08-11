// Copyright (C) 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBKEYBOARD_H
#define QXCBKEYBOARD_H

#include "qxcbobject.h"

#include <xcb/xcb_keysyms.h>
#if QT_CONFIG(xkb)
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit
#endif

#include <xkbcommon/xkbcommon.h>
#include <QtXkbCommonSupport/private/qxkbcommon_p.h>

#include <QEvent>

QT_BEGIN_NAMESPACE

class QXcbKeyboard : public QXcbObject
{
public:
    QXcbKeyboard(QXcbConnection *connection);

    ~QXcbKeyboard();

    void initialize();
    void selectEvents();

    void handleKeyPressEvent(const xcb_key_press_event_t *event);
    void handleKeyReleaseEvent(const xcb_key_release_event_t *event);

    Qt::KeyboardModifiers translateModifiers(int s) const;
    void updateKeymap(xcb_mapping_notify_event_t *event);
    void updateKeymap();
    QList<int> possibleKeys(const QKeyEvent *event) const;

    // when XKEYBOARD not present on the X server
    void updateXKBMods();
    xkb_mod_mask_t xkbModMask(quint16 state);
    void updateXKBStateFromCore(quint16 state);
#if QT_CONFIG(xcb_xinput)
    void updateXKBStateFromXI(void *modInfo, void *groupInfo);
#endif
#if QT_CONFIG(xkb)
    // when XKEYBOARD is present on the X server
    int coreDeviceId() const { return core_device_id; }
    void updateXKBState(xcb_xkb_state_notify_event_t *state);
#endif
    void handleStateChanges(xkb_state_component changedComponents);

protected:
    void handleKeyEvent(xcb_window_t sourceWindow, QEvent::Type type, xcb_keycode_t code,
                        quint16 state, xcb_timestamp_t time, bool fromSendEvent);

    void resolveMaskConflicts();

    typedef QMap<xcb_keysym_t, int> KeysymModifierMap;
    struct xkb_keymap *keymapFromCore(const KeysymModifierMap &keysymMods);

    // when XKEYBOARD not present on the X server
    void updateModifiers(const KeysymModifierMap &keysymMods);
    KeysymModifierMap keysymsToModifiers();
    // when XKEYBOARD is present on the X server
    void updateVModMapping();
    void updateVModToRModMapping();

private:
    bool m_config = false;
    bool m_isAutoRepeat = false;
    xcb_keycode_t m_autoRepeatCode = 0;

    struct _mod_masks {
        uint alt;
        uint altgr;
        uint meta;
        uint super;
        uint hyper;
    };

    _mod_masks rmod_masks;

    // when XKEYBOARD not present on the X server
    xcb_key_symbols_t *m_key_symbols = nullptr;
    struct _xkb_mods {
        xkb_mod_index_t shift;
        xkb_mod_index_t lock;
        xkb_mod_index_t control;
        xkb_mod_index_t mod1;
        xkb_mod_index_t mod2;
        xkb_mod_index_t mod3;
        xkb_mod_index_t mod4;
        xkb_mod_index_t mod5;
    };
    _xkb_mods xkb_mods;
#if QT_CONFIG(xkb)
    // when XKEYBOARD is present on the X server
    _mod_masks vmod_masks;
    int core_device_id;
#endif

    QXkbCommon::ScopedXKBState m_xkbState;
    QXkbCommon::ScopedXKBKeymap m_xkbKeymap;
    QXkbCommon::ScopedXKBContext m_xkbContext;

    bool m_superAsMeta = false;
    bool m_hyperAsMeta = false;
};

QT_END_NAMESPACE

#endif
