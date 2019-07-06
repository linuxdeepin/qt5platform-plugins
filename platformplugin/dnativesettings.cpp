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
#include "dnativesettings.h"
#ifdef Q_OS_LINUX
#include "dxcbxsettings.h"
#endif
#include "dplatformintegration.h"

#include <QDebug>
#include <QMetaProperty>
#include <QMetaMethod>

#define VALID_PROPERTIES "validProperties"

DPP_BEGIN_NAMESPACE

QHash<QObject*, DNativeSettings*> DNativeSettings::mapped;
/*
 * 通过覆盖QObject的qt_metacall虚函数，检测base object中自定义的属性列表，将xwindow对应的设置和object对象中的属性绑定到一起使用
 * 将对象通过property/setProperty调用对属性的读写操作转为对xsetting的属性设 置
 */
DNativeSettings::DNativeSettings(QObject *base, quint32 settingsWindow)
    : m_base(base)
    , m_objectBuilder(base->metaObject())
{
    mapped[base] = this;

    QByteArray settings_property;

    {
        // 获取base对象是否指定了native settings的域
        // 默认情况下，native settings的值保存在窗口的_XSETTINGS_SETTINGS属性上
        // 指定域后，会将native settings的值保存到指定的窗口属性。
        // 将域的值转换成窗口属性时，会把 "/" 替换为 "_"，如域："/xxx/xxx" 转成窗口属性为："_xxx_xxx"
        // 且所有字母转换为大写
        int index = base->metaObject()->indexOfClassInfo("Domain");

        if (index >= 0) {
            settings_property = QByteArray(base->metaObject()->classInfo(index).value());
            settings_property = settings_property.toUpper();
            settings_property.replace('/', '_');
        }
    }

    // 当指定了窗口或窗口属性时，应当创建一个新的native settings
    if (settingsWindow || !settings_property.isEmpty()) {
        m_settings = new NativeSettings(settingsWindow, settings_property);
    } else {
        m_settings = DPlatformIntegration::instance()->xSettings();
    }

    init();
}

DNativeSettings::~DNativeSettings()
{
    if (m_settings != DPlatformIntegration::instance()->xSettings(true)) {
        delete m_settings;
    }

    mapped.remove(m_base);
    free(m_metaObject);
}

void DNativeSettings::init()
{
    m_firstProperty = m_base->metaObject()->propertyOffset();
    m_propertyCount = m_base->metaObject()->propertyCount() - m_base->metaObject()->superClass()->propertyCount();
    // 用于记录属性是否有效的属性, 属性类型为64位整数，最多可用于记录64个属性的状态
    m_flagPropertyIndex = m_base->metaObject()->indexOfProperty(VALID_PROPERTIES);
    qint64 validProperties = 0;

    QMetaObjectBuilder &ob = m_objectBuilder;
    ob.setFlags(ob.flags() | QMetaObjectBuilder::DynamicMetaObject);

    // 先删除所有的属性，等待重构
    while (ob.propertyCount() > 0) {
        ob.removeProperty(0);
    }

    for (int i = 0; i < m_propertyCount; ++i) {
        int index = i + m_firstProperty;

        const QMetaProperty &mp = m_base->metaObject()->property(index);
        // 跳过特殊属性
        if (index == m_flagPropertyIndex) {
            ob.addProperty(mp);
            continue;
        }

        if (m_settings->setting(mp.name()).isValid()) {
            validProperties |= (1 << i);
        }

        switch (mp.type()) {
        case QMetaType::QByteArray:
        case QMetaType::QString:
        case QMetaType::QColor:
        case QMetaType::Int:
        case QMetaType::Double:
        case QMetaType::Bool:
            ob.addProperty(mp);
            break;
        default:
            // 重设属性的类型，只支持Int double color string bytearray
            ob.addProperty(mp.name(), "QByteArray", mp.notifySignalIndex());
            break;
        }

        // 声明支持属性reset
        ob.property(i).setResettable(true);
    }

    // 将属性状态设置给对象
    m_base->setProperty(VALID_PROPERTIES, validProperties);
    m_propertySignalIndex = m_base->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("propertyChanged(const QByteArray&, const QVariant&)"));
    // 监听native setting变化
    m_settings->registerCallback(reinterpret_cast<NativeSettings::PropertyChangeFunc>(onPropertyChanged), this);
    // 支持在base对象中直接使用property/setProperty读写native属性
    QObjectPrivate *op = QObjectPrivate::get(m_base);
    op->metaObject = this;
    m_metaObject = ob.toMetaObject();
    *static_cast<QMetaObject *>(this) = *m_metaObject;
}

int DNativeSettings::createProperty(const char *name, const char *)
{
    // 清理旧数据
    free(m_metaObject);

    // 添加新属性
    auto property = m_objectBuilder.addProperty(name, "QVariant");
    property.setReadable(true);
    property.setWritable(true);
    property.setResettable(true);
    m_metaObject = m_objectBuilder.toMetaObject();
    *static_cast<QMetaObject *>(this) = *m_metaObject;

    return m_firstProperty + property.index();
}

void DNativeSettings::onPropertyChanged(void *screen, const QByteArray &name, const QVariant &property, DNativeSettings *handle)
{
    Q_UNUSED(screen)

    if (handle->m_propertySignalIndex >= 0) {
        handle->method(handle->m_propertySignalIndex).invoke(handle->m_base, Q_ARG(QByteArray, name), Q_ARG(QVariant, property));
    }

    // 不要直接调用自己的indexOfProperty函数，属性不存在时会导致调用createProperty函数
    int property_index = handle->m_objectBuilder.indexOfProperty(name.constData());

    if (Q_UNLIKELY(property_index < 0)) {
        return;
    }

    if (handle->m_flagPropertyIndex >= 0) {
        bool ok = false;
        qint64 flags = handle->m_base->property(VALID_PROPERTIES).toLongLong(&ok);
        // 更新有效属性的标志位
        if (ok) {
            qint64 flag = (1 << property_index);
            flags = property.isValid() ? flags | flag : flags & ~flag;
            handle->m_base->setProperty(VALID_PROPERTIES, flags);
        }
    }

    const QMetaProperty &p = handle->property(handle->m_firstProperty + property_index);

    if (p.hasNotifySignal()) {
        // 通知属性改变
        p.notifySignal().invoke(handle->m_base);
    }
}

int DNativeSettings::metaCall(QMetaObject::Call _c, int _id, void ** _a)
{
    enum CallFlag {
        ReadProperty = 1 << QMetaObject::ReadProperty,
        WriteProperty = 1 << QMetaObject::WriteProperty,
        ResetProperty = 1 << QMetaObject::ResetProperty,
        AllCall = ReadProperty | WriteProperty | ResetProperty
    };

    if (AllCall & (1 << _c)) {
        const QMetaProperty &p = property(_id);
        const int index = p.propertyIndex();
        // 对于本地属性，此处应该从m_settings中读写
        if (Q_LIKELY(index != m_flagPropertyIndex && index >= m_firstProperty)) {
            switch (_c) {
            case QMetaObject::ReadProperty:
                *reinterpret_cast<QVariant*>(_a[1]) = m_settings->setting(p.name());
                _a[0] = reinterpret_cast<QVariant*>(_a[1])->data();
                break;
            case QMetaObject::WriteProperty:
                m_settings->setSetting(p.name(), *reinterpret_cast<QVariant*>(_a[1]));
                break;
            case QMetaObject::ResetProperty:
                m_settings->setSetting(p.name(), QVariant());
                break;
            default:
                break;
            }

            return -1;
        }
    }

    return m_base->qt_metacall(_c, _id, _a);
}

DPP_END_NAMESPACE
