#ifndef BLUREFFECT_H
#define BLUREFFECT_H
#include "global.h"
#include "wl_utility.h"

#include <QtGlobal>
#include <QObject>
#include <QPainterPath>

namespace KWayland {
namespace Client {
class Blur;
class Region;
}
}

QT_BEGIN_NAMESPACE
namespace QtWaylandClient {
class QWaylandWindow;
}
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

namespace BlurEffect {

// 窗口如果有特效，就把这个托管给该窗口，管理关于特效的一些数据
class BlurEffectEntity : public QObject {
Q_OBJECT
    BlurEffectEntity(QObject *parent = nullptr) : QObject(parent) {}
    ~BlurEffectEntity() {}
public:
    static BlurEffectEntity *ensureBlurEffectEntity(QtWaylandClient::QWaylandWindow *wlWindow);

    void enableBlur(bool enable, bool update = true);
    bool blurEnabled();
    bool updateBlurAreas(const QVector<WlUtility::BlurArea> &areas);
    bool updateBlurPaths(const QList<QPainterPath> &paths);
    bool updateClipPaths(const QList<QPainterPath> &paths);

    // TODO
    void clipPathForWindow();
    void blurWindowBackground();

    // TODO: 优化，用户的数据是否没必要存储，做成hash进行比较，减少内存占用？
    QVector<WlUtility::BlurArea> m_blurAreaList;
    QList<QPainterPath> m_blurPathList;
    QList<QPainterPath> m_clipPathList;

private:
    void updateWindowBlurAreasForWM();
    void updateWindowBlurPathsForWM();
    void updateWindowClipPathsForWM();
    void updateBlurRegion(const QList<QPainterPath> &newPathList);
    void updateWidget();

    template <typename T1, typename T2>
    inline bool isSameContent(const T1 &t1, const T2 &t2) {
        if (t1.size() == 0 || t2.size() == 0) {
            return false;
        }
        bool allSame = true;
        if (t1.size() == t2.size()) {
            for (int i = 0; i < t1.size(); ++i) {
                allSame &= (t1.at(i) == t2.at(i));
            }
        }
        return allSame;
    }

    QPair<KWayland::Client::Blur *, KWayland::Client::Region*> m_blurRegion = {nullptr, nullptr};
};
}

#define blurEffect(W) BlurEffect::BlurEffectEntity::ensureBlurEffectEntity(W)

DPP_END_NAMESPACE

#endif//BLUREFFECT_H
