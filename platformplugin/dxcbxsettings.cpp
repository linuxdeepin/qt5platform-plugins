/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dxcbxsettings.h"
#include "dplatformintegration.h"

#include <QtCore/QByteArray>
#include <QtCore/QtEndian>

#include <vector>
#include <algorithm>
#include <memory>

#define Q_XCB_REPLY_CONNECTION_ARG(connection, ...) connection

struct DStdFreeDeleter {
    void operator()(void *p) const noexcept { return std::free(p); }
};

#define Q_XCB_REPLY(call, ...) \
    std::unique_ptr<call##_reply_t, DStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr) \
    )

#define Q_XCB_REPLY_UNCHECKED(call, ...) \
    std::unique_ptr<call##_reply_t, DStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call##_unchecked(__VA_ARGS__), nullptr) \
    )

DPP_BEGIN_NAMESPACE
/* Implementation of http://standards.freedesktop.org/xsettings-spec/xsettings-0.5.html */

enum XSettingsType {
    XSettingsTypeInteger = 0,
    XSettingsTypeString = 1,
    XSettingsTypeColor = 2
};

struct QXcbXSettingsCallback
{
    DXcbXSettings::PropertyChangeFunc func;
    void *handle;
};

class QXcbXSettingsPropertyValue
{
public:
    QXcbXSettingsPropertyValue()
        : last_change_serial(-1)
    {}

    bool updateValue(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &value, int last_change_serial)
    {
        if (last_change_serial <= this->last_change_serial)
            return false;
        this->value = value;
        this->last_change_serial = last_change_serial;
        for (const auto &callback : callback_links)
            callback.func(screen, name, value, callback.handle);

        return true;
    }

    void addCallback(DXcbXSettings::PropertyChangeFunc func, void *handle)
    {
        QXcbXSettingsCallback callback = { func, handle };
        callback_links.push_back(callback);
    }

    QVariant value;
    int last_change_serial = -1;
    std::vector<QXcbXSettingsCallback> callback_links;
};

class QXcbConnectionGrabber
{
public:
    QXcbConnectionGrabber(QXcbConnection *connection);
    ~QXcbConnectionGrabber();
    void release();
private:
    QXcbConnection *m_connection;
};

QXcbConnectionGrabber::QXcbConnectionGrabber(QXcbConnection *connection)
    :m_connection(connection)
{
    connection->grabServer();
}

QXcbConnectionGrabber::~QXcbConnectionGrabber()
{
    if (m_connection)
        m_connection->ungrabServer();
}

void QXcbConnectionGrabber::release()
{
    if (m_connection) {
        m_connection->ungrabServer();
        m_connection = 0;
    }
}

class DXcbXSettingsPrivate
{
public:
    DXcbXSettingsPrivate(QXcbVirtualDesktop *screen)
        : screen(screen)
        , initialized(false)
    {
        _xsettings_atom = screen->connection()->atom(QXcbAtom::_XSETTINGS_SETTINGS);
    }

    QByteArray getSettings()
    {
        QXcbConnectionGrabber connectionGrabber(screen->connection());

        int offset = 0;
        QByteArray settings;
        while (1) {
            auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property,
                                               screen->xcb_connection(),
                                               false,
                                               x_settings_window,
                                               _xsettings_atom,
                                               _xsettings_atom,
                                               offset/4,
                                               8192);
            bool more = false;
            if (!reply)
                return settings;

            const auto property_value_length = xcb_get_property_value_length(reply.get());
            settings.append(static_cast<const char *>(xcb_get_property_value(reply.get())), property_value_length);
            offset += property_value_length;
            more = reply->bytes_after != 0;

            if (!more)
                break;
        }

        return settings;
    }

    void setSettings(const QByteArray &data)
    {
        QXcbConnectionGrabber connectionGrabber(screen->connection());
        xcb_change_property(screen->xcb_connection(), XCB_PROP_MODE_REPLACE, x_settings_window, _xsettings_atom, _xsettings_atom, 8, data.size(), data.constData());
    }

    static int round_to_nearest_multiple_of_4(int value)
    {
        int remainder = value % 4;
        if (!remainder)
            return value;
        return value + 4 - remainder;
    }

    void populateSettings(const QByteArray &xSettings)
    {
        if (xSettings.length() < 12)
            return;
        char byteOrder = xSettings.at(0);
        if (byteOrder != XCB_IMAGE_ORDER_LSB_FIRST && byteOrder != XCB_IMAGE_ORDER_MSB_FIRST) {
            qWarning("ByteOrder byte %d not 0 or 1", byteOrder);
            return;
        }

#define ADJUST_BO(b, t, x) \
        ((b == XCB_IMAGE_ORDER_LSB_FIRST) ?                          \
         qFromLittleEndian<t>(x) : \
         qFromBigEndian<t>(x))
#define VALIDATE_LENGTH(x)    \
        if ((size_t)xSettings.length() < (offset + local_offset + 12 + x)) { \
            qWarning("Length %d runs past end of data", x); \
            return;                                                     \
        }

        serial = ADJUST_BO(byteOrder, qint32, xSettings.mid(4,4).constData());
        uint number_of_settings = ADJUST_BO(byteOrder, quint32, xSettings.mid(8,4).constData());
        const char *data = xSettings.constData() + 12;
        size_t offset = 0;
        for (uint i = 0; i < number_of_settings; i++) {
            int local_offset = 0;
            VALIDATE_LENGTH(2);
            XSettingsType type = static_cast<XSettingsType>(*reinterpret_cast<const quint8 *>(data + offset));
            local_offset += 2;

            VALIDATE_LENGTH(2);
            quint16 name_len = ADJUST_BO(byteOrder, quint16, data + offset + local_offset);
            local_offset += 2;

            VALIDATE_LENGTH(name_len);
            QByteArray name(data + offset + local_offset, name_len);
            local_offset += round_to_nearest_multiple_of_4(name_len);

            VALIDATE_LENGTH(4);
            int last_change_serial = ADJUST_BO(byteOrder, qint32, data + offset + local_offset);
            Q_UNUSED(last_change_serial);
            local_offset += 4;

            QVariant value;
            if (type == XSettingsTypeString) {
                VALIDATE_LENGTH(4);
                int value_length = ADJUST_BO(byteOrder, qint32, data + offset + local_offset);
                local_offset+=4;
                VALIDATE_LENGTH(value_length);
                QByteArray value_string(data + offset + local_offset, value_length);
                value.setValue(value_string);
                local_offset += round_to_nearest_multiple_of_4(value_length);
            } else if (type == XSettingsTypeInteger) {
                VALIDATE_LENGTH(4);
                int value_length = ADJUST_BO(byteOrder, qint32, data + offset + local_offset);
                local_offset += 4;
                value.setValue(value_length);
            } else if (type == XSettingsTypeColor) {
                VALIDATE_LENGTH(2*4);
                quint16 red = ADJUST_BO(byteOrder, quint16, data + offset + local_offset);
                local_offset += 2;
                quint16 green = ADJUST_BO(byteOrder, quint16, data + offset + local_offset);
                local_offset += 2;
                quint16 blue = ADJUST_BO(byteOrder, quint16, data + offset + local_offset);
                local_offset += 2;
                quint16 alpha= ADJUST_BO(byteOrder, quint16, data + offset + local_offset);
                local_offset += 2;
                QColor color_value(red,green,blue,alpha);
                value.setValue(color_value);
            }
            offset += local_offset;

            updateValue(settings[name], name,value,last_change_serial);
        }

    }

    QByteArray depopulateSettings()
    {
        QByteArray xSettings;
        uint number_of_settings = settings.size();
        xSettings.reserve(12 + number_of_settings * 12);
        char byteOrder = QSysInfo::ByteOrder == QSysInfo::LittleEndian ? XCB_IMAGE_ORDER_LSB_FIRST : XCB_IMAGE_ORDER_MSB_FIRST;

        xSettings.append(byteOrder); //byte-order
        xSettings.append(3, '\0'); //unused
        xSettings.append((char*)&serial, sizeof(serial)); //SERIAL
        xSettings.append((char*)&number_of_settings, sizeof(number_of_settings)); //N_SETTINGS
        uint *number_of_settings_ptr = (uint*)(xSettings.data() + xSettings.size() - sizeof(number_of_settings));

        for (auto i = settings.constBegin(); i != settings.constEnd(); ++i) {
            const QXcbXSettingsPropertyValue &value = i.value();

            if (!value.value.isValid()) {
                --*number_of_settings_ptr;
                continue;
            }

            char type = XSettingsTypeString;
            const QByteArray &key = i.key();
            quint16 key_size = key.size();

            if (value.value.type() == QVariant::Color) {
                type = XSettingsTypeColor;
            } else if (value.value.type() == QVariant::Int) {
                type = XSettingsTypeInteger;
            }

            xSettings.append(type); //type
            xSettings.append('\0'); //unused
            xSettings.append((char*)&key_size, 2); //name-len
            xSettings.append(key.constData()); //name
            xSettings.append(3 - (key_size + 3) % 4, '\0'); //4字节对齐
            xSettings.append((char*)&value.last_change_serial, 4); //last-change-serial

            QByteArray value_data;

            if (type == XSettingsTypeInteger) {
                qint32 int_value = value.value.toInt();
                value_data.append((char*)&int_value, 4);
            } else if (type == XSettingsTypeColor) {
                const QColor &color = qvariant_cast<QColor>(value.value);
                quint16 red = color.red();
                quint16 green = color.green();
                quint16 blue = color.blue();
                quint16 alpha = color.alpha();

                value_data.append((char*)&red, 2);
                value_data.append((char*)&green, 2);
                value_data.append((char*)&blue, 2);
                value_data.append((char*)&alpha, 2);
            } else {
                const QByteArray &string_data = value.value.toByteArray();
                quint32 data_size = string_data.size();
                value_data.append((char*)&data_size, 4);
                value_data.append(string_data);
                value_data.append(3 - (string_data.size() + 3) % 4, '\0'); //4字节对齐
            }

            xSettings.append(value_data);
        }

        if (*number_of_settings_ptr == 0) {
            return QByteArray();
        }

        return xSettings;
    }

    void init(xcb_window_t setting_window, DXcbXSettings *object)
    {
        x_settings_window = setting_window;

        populateSettings(getSettings());
        initialized = true;
        mapped[x_settings_window] = object;
    }

    bool updateValue(QXcbXSettingsPropertyValue &xvalue, const QByteArray &name, const QVariant &value, int last_change_serial)
    {
        if (xvalue.updateValue(screen, name, value, last_change_serial)) {
            for (const auto &callback : callback_links) {
                callback.func(screen, name, value, callback.handle);
            }

            return true;
        }

        return false;
    }

    QXcbVirtualDesktop *screen;
    xcb_window_t x_settings_window;
    qint32 serial = -1;
    QHash<QByteArray, QXcbXSettingsPropertyValue> settings;
    std::vector<QXcbXSettingsCallback> callback_links;
    bool initialized;
    static xcb_atom_t _xsettings_atom;
    static QHash<xcb_window_t, DXcbXSettings*> mapped;
};

xcb_atom_t DXcbXSettingsPrivate::_xsettings_atom;
QHash<xcb_window_t, DXcbXSettings*> DXcbXSettingsPrivate::mapped;

DXcbXSettings::DXcbXSettings(QXcbVirtualDesktop *screen)
    : d_ptr(new DXcbXSettingsPrivate(screen))
{
    QByteArray settings_atom_for_screen("_XSETTINGS_S");
    settings_atom_for_screen.append(QByteArray::number(screen->number()));
    auto atom_reply = Q_XCB_REPLY(xcb_intern_atom,
                                  screen->xcb_connection(),
                                  true,
                                  settings_atom_for_screen.length(),
                                  settings_atom_for_screen.constData());
    if (!atom_reply)
        return;

    xcb_atom_t selection_owner_atom = atom_reply->atom;

    auto selection_result = Q_XCB_REPLY(xcb_get_selection_owner,
                                        screen->xcb_connection(), selection_owner_atom);
    if (!selection_result)
        return;

    if (!selection_result->owner)
        return;

    const uint32_t event = XCB_CW_EVENT_MASK;
    const uint32_t event_mask[] = { XCB_EVENT_MASK_STRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE };
    xcb_change_window_attributes(screen->xcb_connection(), selection_result->owner, event, event_mask);

    d_ptr->init(selection_result->owner, this);
}

DXcbXSettings::DXcbXSettings(QXcbVirtualDesktop *screen, xcb_window_t setting_window)
    : d_ptr(new DXcbXSettingsPrivate(screen))
{
    Q_ASSERT(setting_window);

    d_ptr->init(setting_window, this);
}

DXcbXSettings::DXcbXSettings(xcb_window_t setting_window)
    : DXcbXSettings(DPlatformIntegration::xcbConnection()->primaryVirtualDesktop(), setting_window)
{

}

DXcbXSettings::~DXcbXSettings()
{
    DXcbXSettingsPrivate::mapped.remove(d_ptr->x_settings_window);
    delete d_ptr;
    d_ptr = 0;
}

bool DXcbXSettings::initialized() const
{
    Q_D(const DXcbXSettings);
    return d->initialized;
}

bool DXcbXSettings::isEmpty() const
{
    Q_D(const DXcbXSettings);
    return d->settings.isEmpty();
}

bool DXcbXSettings::contains(const QByteArray &property) const
{
    Q_D(const DXcbXSettings);
    return d->settings.contains(property);
}

bool DXcbXSettings::handlePropertyNotifyEvent(const xcb_property_notify_event_t *event)
{
    if (event->atom != DXcbXSettingsPrivate::_xsettings_atom)
        return false;

    if (DXcbXSettings *self = DXcbXSettingsPrivate::mapped.value(event->window)) {
        self->d_ptr->populateSettings(self->d_ptr->getSettings());

        return true;
    }

    return false;
}

void DXcbXSettings::clearSettings(xcb_window_t setting_window)
{
    if (DXcbXSettingsPrivate::_xsettings_atom == XCB_NONE) {
        DXcbXSettingsPrivate::_xsettings_atom = DPlatformIntegration::xcbConnection()->atom(QXcbAtom::_XSETTINGS_SETTINGS);
    }

    xcb_delete_property(DPlatformIntegration::xcbConnection()->xcb_connection(), setting_window, DXcbXSettingsPrivate::_xsettings_atom);
}

void DXcbXSettings::registerCallbackForProperty(const QByteArray &property, DXcbXSettings::PropertyChangeFunc func, void *handle)
{
    Q_D(DXcbXSettings);
    d->settings[property].addCallback(func,handle);
}

void DXcbXSettings::removeCallbackForHandle(const QByteArray &property, void *handle)
{
    Q_D(DXcbXSettings);
    auto &callbacks = d->settings[property].callback_links;

    auto isCallbackForHandle = [handle](const QXcbXSettingsCallback &cb) { return cb.handle == handle; };

    callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                   isCallbackForHandle),
                    callbacks.end());
}

void DXcbXSettings::removeCallbackForHandle(void *handle)
{
    Q_D(DXcbXSettings);
    for (QHash<QByteArray, QXcbXSettingsPropertyValue>::const_iterator it = d->settings.cbegin();
         it != d->settings.cend(); ++it) {
        removeCallbackForHandle(it.key(),handle);
    }

    auto isCallbackForHandle = [handle](const QXcbXSettingsCallback &cb) { return cb.handle == handle; };
    d->callback_links.erase(std::remove_if(d->callback_links.begin(), d->callback_links.end(), isCallbackForHandle));
}

QVariant DXcbXSettings::setting(const QByteArray &property) const
{
    Q_D(const DXcbXSettings);
    return d->settings.value(property).value;
}

void DXcbXSettings::setSetting(const QByteArray &property, const QVariant &value)
{
    Q_D(DXcbXSettings);

    QXcbXSettingsPropertyValue &xvalue = d->settings[property];

    if (xvalue.value == value)
        return;

    d->updateValue(xvalue, property, value, xvalue.last_change_serial + 1);

    ++d->serial;
    // 更新属性
    d->setSettings(d->depopulateSettings());
}

void DXcbXSettings::registerCallback(DXcbXSettings::PropertyChangeFunc func, void *handle)
{
    Q_D(DXcbXSettings);
    QXcbXSettingsCallback callback = { func, handle };
    d->callback_links.push_back(callback);
}

DPP_END_NAMESPACE
