/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
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
#ifndef DNATIVESETTINGS_H
#define DNATIVESETTINGS_H

#include "global.h"

#include <QSet>
#define protected public
#include <private/qobject_p.h>
#undef protected
#include <private/qmetaobjectbuilder_p.h>

DPP_BEGIN_NAMESPACE

#ifdef Q_OS_LINUX
class DXcbXSettings;
typedef DXcbXSettings NativeSettings;
#endif

class DNativeSettings : public QAbstractDynamicMetaObject
{
public:
    explicit DNativeSettings(QObject *base, NativeSettings *settings, bool global_settings);
    ~DNativeSettings();

    static QByteArray getSettingsProperty(QObject *base);
    bool isValid() const;

private:
    void init(const QMetaObject *meta_object);

    int createProperty(const char *, const char *) override;
    int metaCall(QMetaObject::Call, int _id, void **) override;
    bool isRelaySignal() const;

    static void onPropertyChanged(void *screen, const QByteArray &name, const QVariant &property, DNativeSettings *handle);
    static void onSignal(void *screen, const QByteArray &signal, qint32 data1, qint32 data2, DNativeSettings *handle);

    QObject *m_base;
    QMetaObject *m_metaObject = nullptr;
    QMetaObjectBuilder m_objectBuilder;
    int m_firstProperty;
    int m_propertyCount;
    // propertyChanged信号的index
    int m_propertySignalIndex;
    // VALID_PROPERTIES属性的index
    int m_flagPropertyIndex;
    // ALL_KEYS属性的index
    int m_allKeysPropertyIndex;
    // 用于转发base对象产生的信号的槽，使用native settings的接口将其发送出去. 值为0时表示不转发base对象的所有信号
    int m_relaySlotIndex = 0;
    NativeSettings *m_settings = nullptr;
    bool m_isGlobalSettings = false;

    static QHash<QObject*, DNativeSettings*> mapped;
};

DPP_END_NAMESPACE

#endif // DNATIVESETTINGS_H
