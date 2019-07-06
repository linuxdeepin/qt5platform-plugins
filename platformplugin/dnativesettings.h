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

#include <QObject>
#include <QSet>
#include <private/qobject_p.h>
#include <private/qmetaobjectbuilder_p.h>

DPP_BEGIN_NAMESPACE

#ifdef Q_OS_LINUX
class DXcbXSettings;
typedef DXcbXSettings NativeSettings;
#endif

class DNativeSettings : public QAbstractDynamicMetaObject
{
public:
    explicit DNativeSettings(QObject *base, quint32 settingsWindow);
    ~DNativeSettings();

private:
    void init();

    int createProperty(const char *, const char *) override;
    int metaCall(QMetaObject::Call, int _id, void **) override;

    static void onPropertyChanged(void *screen, const QByteArray &name, const QVariant &property, DNativeSettings *handle);

    QObject *m_base;
    QMetaObject *m_metaObject;
    QMetaObjectBuilder m_objectBuilder;
    int m_firstProperty;
    int m_propertyCount;
    int m_propertySignalIndex;
    int m_flagPropertyIndex;
    NativeSettings *m_settings = nullptr;

    static QHash<QObject*, DNativeSettings*> mapped;
};

DPP_END_NAMESPACE

#endif // DNATIVESETTINGS_H
