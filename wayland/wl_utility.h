#ifndef WL_UTILITY_H
#define WL_UTILITY_H
#include "global.h"

#include <QtGlobal>
#include <QPainterPath>

DPP_BEGIN_NAMESPACE

namespace WlUtility {
struct BlurArea {
    qint32 x;
    qint32 y;
    qint32 width;
    qint32 height;
    qint32 xRadius;
    qint32 yRaduis;

    inline BlurArea operator *(qreal scale)
    {
        if (qFuzzyCompare(1.0, scale))
            return *this;

        BlurArea new_area;

        new_area.x = qRound64(x * scale);
        new_area.y = qRound64(y * scale);
        new_area.width = qRound64(width * scale);
        new_area.height = qRound64(height * scale);
        new_area.xRadius = qRound64(xRadius * scale);
        new_area.yRaduis = qRound64(yRaduis * scale);

        return new_area;
    }

    inline BlurArea &operator *=(qreal scale)
    {
        return *this = *this * scale;
    }
    bool operator==(const BlurArea &other) const {
        return
        other.x == x &&
        other.y == y &&
        other.width == width &&
        other.height == height &&
        other.xRadius == xRadius &&
        other.yRaduis == yRaduis;
    }
    bool operator!=(const BlurArea &other) const {
        return operator==(other);
    }
};

}
DPP_END_NAMESPACE


QT_BEGIN_NAMESPACE

inline QPainterPath operator *(const QPainterPath &path, qreal scale)
{
    if (qFuzzyCompare(1.0, scale))
        return path;

    QPainterPath new_path = path;

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);

        new_path.setElementPositionAt(i, qRound(e.x * scale), qRound(e.y * scale));
    }

    return new_path;
}
QT_END_NAMESPACE
Q_DECLARE_METATYPE(QList<QPainterPath>)
Q_DECLARE_METATYPE(QVector<deepin_platform_plugin::WlUtility::BlurArea>)

#endif//WL_UTILITY_H
