#include "blureffect.h"
#include "shell_common.h"

#include "QtWaylandClient/private/qwaylandwindow_p.h"
#include <QWindow>
#include <QScreen>

#include <KWayland/Client/blur.h>
#include <KWayland/Client/region.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/compositor.h>

DPP_BEGIN_NAMESPACE

using namespace QtWaylandClient;
using namespace DWaylandPointer;

namespace BlurEffect {

BlurEffectEntity *BlurEffectEntity::ensureBlurEffectEntity(QWaylandWindow *wlWindow) {
    using namespace BlurEffect;
    if (BlurEffectEntity *blurEffect = wlWindow->findChild<BlurEffectEntity*>(QString(), Qt::FindDirectChildrenOnly)) {
        return blurEffect;
    }
    return new BlurEffectEntity(wlWindow);
}

void BlurEffectEntity::enableBlur(bool enable, bool update) {
    if (!kwayland_compositor) {
        return;
    }
    QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());
    if (!wlWindow->window()) {
        return;
    }
    if  (enable) {
        // 创建铺满整个窗口的模糊区域
        auto surface = kwayland_surface->fromWindow(wlWindow->window());
        auto blur = kwayland_blur_manager->createBlur(surface, surface);
        // createRegion 不加参数即默认创建覆盖全部窗口的
        auto region = kwayland_compositor->createRegion();

        // 到 kwayland 中，最终要转换为 QRect 或 QRegion
        // QRegion qregion;
        // for (QRect rect : qregion) {
        //     // qregion << rect;
        // }

        blur->setRegion(region);
        blur->commit();
        kwayland_surface->commit(KWayland::Client::Surface::CommitFlag::None);

        // 一个窗口对应唯一的 region 和 blur
        m_blurRegion = {blur, region};
        QObject::connect(wlWindow, &QWindow::destroyed, [this](QObject *) {
            if (auto blur = m_blurRegion.first) {
                KWayland::Client::Region *region = m_blurRegion.second;
                blur->deleteLater();
                region->deleteLater();
                m_blurRegion = {nullptr, nullptr};
            }
        });
    } else {
        QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());
        // 取消 blur 以后应用要主动 update
        if (auto blur = m_blurRegion.first) {
            KWayland::Client::Region *region = m_blurRegion.second;
            region->subtract(region->region().boundingRect());
            blur->commit();

            Surface *surface = kwayland_surface->fromWindow(wlWindow->window());
            kwayland_blur_manager->removeBlur(surface);
            kwayland_surface->commit(KWayland::Client::Surface::CommitFlag::None);

            blur->deleteLater();
            region->deleteLater();
            m_blurRegion = {nullptr, nullptr};

            if (update) {
                updateWidget();
            }
        }
    }
}

bool BlurEffectEntity::blurEnabled() {
    return m_blurRegion.first != nullptr && m_blurRegion.second != nullptr;
}

bool BlurEffectEntity::updateBlurAreas(const QVector<WlUtility::BlurArea> &areas) {
    // 只要任何一个不为空就进行更新
    if (areas.isEmpty() && m_blurAreaList.isEmpty())
        return false;
    // 新旧数据完全相同就不要更新，其中一个为空
    if (isSameContent(m_blurAreaList, areas)) {
        return false;
    }
    // update old data
    m_blurAreaList = areas; // std::move(paths);

    if (blurEnabled()) {
        updateWindowBlurAreasForWM();
    }
    return true;
}

bool BlurEffectEntity::updateBlurPaths(const QList<QPainterPath> &paths) {
    // 只要任何一个不为空就进行更新
    if (paths.isEmpty() && m_blurPathList.isEmpty())
        return false;
    // 新旧数据完全相同就不要更新，其中一个为空
    if (isSameContent(m_blurPathList, paths)) {
        return false;
    }
    // update old data
    m_blurPathList = paths; // std::move(paths);

    if (blurEnabled()) {
        updateWindowBlurPathsForWM();
    }
    return true;
}

bool BlurEffectEntity::updateClipPaths(const QList<QPainterPath> &paths) {
    // 只要任何一个不为空就进行更新
    if (paths.isEmpty() && m_clipPathList.isEmpty())
        return false;
    // 新旧数据完全相同就不要更新，其中一个为空
    if (isSameContent(m_clipPathList, paths)) {
        return false;
    }
    // update old data
    m_clipPathList = paths; // std::move(paths);

    // if (blurEnabled()) {
        // updateWindowBlurPathsForWM();
        updateWindowClipPathsForWM();
        qInfo() << "updateWindowBlurPathsForWM";
    // }
    return true;
}

void BlurEffectEntity::clipPathForWindow() {

}
void BlurEffectEntity::blurWindowBackground() {

}

void BlurEffectEntity::updateWindowBlurAreasForWM()
{
    using namespace WlUtility;
    QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());
    qreal device_pixel_ratio = wlWindow->window()->screen()->devicePixelRatio();

//    // 获得窗口背景可更新范围
//    QVector<BlurArea> newAreas;
//    const QRect &windowValidRect = QRect(QPoint(0, 0), wlWindow->window()->size() * device_pixel_ratio);
//    BlurArea area;
//    area.x = windowValidRect.x() /*+ offset.x()*/;
//    area.y = windowValidRect.y() /*+ offset.y()*/;
//    area.width = windowValidRect.width();
//    area.height = windowValidRect.height();
//    area.xRadius = 0;
//    area.yRaduis = 0;
//    newAreas.append(std::move(area));

    // 打开背景区域模糊效果
    // blurWindowBackgroundByArea(top_level_w, newAreas);

    QList<QPainterPath> newPathList;

    newPathList.reserve(m_blurAreaList.size());

    // area convert and append to newPath
    foreach (WlUtility::BlurArea area, m_blurAreaList) {
        QPainterPath path;

        area *= device_pixel_ratio;
        path.addRoundedRect(area.x /*+ offset.x()*/, area.y /*+ offset.y()*/, area.width, area.height,
                            area.xRadius, area.yRaduis);

        if (!path.isEmpty())
            newPathList << path;
    }

    // paths append to newPath
    if (!m_blurPathList.isEmpty()) {
        newPathList.reserve(newPathList.size() + m_blurPathList.size());

        foreach (const QPainterPath &path, m_blurPathList) {
            newPathList << (path * device_pixel_ratio)/*.translated(offset)*/;
        }
    }

    if (newPathList.isEmpty())
        return;

    updateBlurRegion(newPathList);
}

void BlurEffectEntity::updateWindowBlurPathsForWM() {
    using namespace WlUtility;
    using namespace QtWaylandClient;
    QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());

    qreal device_pixel_ratio = wlWindow->window()->screen()->devicePixelRatio();

    QList<QPainterPath> newPathList;

    newPathList.reserve(m_blurAreaList.size());

    foreach (BlurArea area, m_blurAreaList) {
        QPainterPath path;

        area *= device_pixel_ratio;
        path.addRoundedRect(area.x/* + offset.x()*/, area.y/* + offset.y()*/, area.width, area.height,
                            area.xRadius, area.yRaduis);

        if (!path.isEmpty())
            newPathList << path;
    }

    if (!m_blurPathList.isEmpty()) {
        newPathList.reserve(newPathList.size() + m_blurPathList.size());

        foreach (const QPainterPath &path, m_blurPathList) {
            newPathList << (path * device_pixel_ratio)/*.translated(offset)*/;
        }
    }

    if (newPathList.isEmpty())
        return;

    updateBlurRegion(newPathList);
}

void BlurEffectEntity::updateWindowClipPathsForWM() {
    using namespace WlUtility;
    QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());
    qreal device_pixel_ratio = wlWindow->window()->screen()->devicePixelRatio();

//    // 获得窗口背景可更新范围
//    QVector<BlurArea> newAreas;
//    const QRect &windowValidRect = QRect(QPoint(0, 0), wlWindow->window()->size() * device_pixel_ratio);
//    BlurArea area;
//    area.x = windowValidRect.x() /*+ offset.x()*/;
//    area.y = windowValidRect.y() /*+ offset.y()*/;
//    area.width = windowValidRect.width();
//    area.height = windowValidRect.height();
//    area.xRadius = 0;
//    area.yRaduis = 0;
//    newAreas.append(std::move(area));

    // 打开背景区域模糊效果
    // blurWindowBackgroundByArea(top_level_w, newAreas);

    QList<QPainterPath> newPathList;

    newPathList.reserve(m_blurAreaList.size());

    // area convert and append to newPath
    foreach (WlUtility::BlurArea area, m_blurAreaList) {
        QPainterPath path;

        area *= device_pixel_ratio;
        path.addRoundedRect(area.x /*+ offset.x()*/, area.y /*+ offset.y()*/, area.width, area.height,
                            area.xRadius, area.yRaduis);

        if (!path.isEmpty())
            newPathList << path;
    }

    // paths append to newPath
    if (!m_clipPathList.isEmpty()) {
        newPathList.reserve(newPathList.size() + m_clipPathList.size());

        foreach (const QPainterPath &path, m_clipPathList) {
            newPathList << (path * device_pixel_ratio)/*.translated(offset)*/;
        }
    }

    if (newPathList.isEmpty())
        return;
    updateBlurRegion(newPathList);
}

// 将 paths、areas 都转到这里作为 path 来调用
void BlurEffectEntity::updateBlurRegion(const QList<QPainterPath> &newPathList) {
    QVector<QRect> rects;
    foreach (const QPainterPath &path, newPathList) {
        foreach(const QPolygonF &polygon, path.toFillPolygons()) {
            foreach(const QRect &rect, QRegion(polygon.toPolygon()).rects()) {
                rects << rect;
            }
        }
    }

    // 更新 blur 以后应用要主动 update
    if (auto blur = m_blurRegion.first) {
        // enableBlur(false, false);
        // enableBlur(true, false);

        KWayland::Client::Region *region = m_blurRegion.second;
        for (const QRect &r : qAsConst(rects)) {
            region->add(r);
        }
        blur->setRegion(region);
        blur->commit();
        kwayland_surface->commit(KWayland::Client::Surface::CommitFlag::None);

        updateWidget();
    }
}

void BlurEffectEntity::updateWidget() {
    QWaylandWindow *wlWindow = static_cast<QWaylandWindow *>(this->parent());
    // 取消模糊效果的更新需要主动调用应用侧的窗口
    if (QWidgetWindow *widgetWin = static_cast<QWidgetWindow*>(wlWindow->window())) {
        if (auto widget = widgetWin->widget()) {
            widget->update();
        }
    }
}

DPP_END_NAMESPACE

}// BlurEffect
