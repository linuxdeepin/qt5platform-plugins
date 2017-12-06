/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
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

#include "dplatformbackingstorehelper.h"
#include "vtablehook.h"
#include "dplatformwindowhelper.h"
#include "dframewindow.h"
#include "dwmsupport.h"

#ifdef Q_OS_LINUX
#define private public
#include "qxcbbackingstore.h"
#undef private
#endif

#include <qpa/qplatformbackingstore.h>

#include <QPainter>
#include <QOpenGLPaintDevice>
#include <QThreadStorage>

DPP_BEGIN_NAMESPACE

DPlatformBackingStoreHelper::DPlatformBackingStoreHelper()
{

}

bool DPlatformBackingStoreHelper::addBackingStore(QPlatformBackingStore *store)
{
    QObject::connect(store->window(), &QWindow::destroyed, store->window(), [store] {
        VtableHook::clearGhostVtable(store);
    });

    VtableHook::overrideVfptrFun(store, &QPlatformBackingStore::beginPaint, this, &DPlatformBackingStoreHelper::beginPaint);
    VtableHook::overrideVfptrFun(store, &QPlatformBackingStore::paintDevice, this, &DPlatformBackingStoreHelper::paintDevice);

    return VtableHook::overrideVfptrFun(store, &QPlatformBackingStore::flush, this, &DPlatformBackingStoreHelper::flush);
}

static QThreadStorage<bool> _d_dxcb_overridePaintDevice;

QPaintDevice *DPlatformBackingStoreHelper::paintDevice()
{
    QPlatformBackingStore *store = backingStore();

    if (_d_dxcb_overridePaintDevice.hasLocalData() && _d_dxcb_overridePaintDevice.localData()) {
        static QImage device;

        return &device;
    }

    return VtableHook::callOriginalFun(store, &QPlatformBackingStore::paintDevice);
}

void DPlatformBackingStoreHelper::beginPaint(const QRegion &region)
{
    QPlatformBackingStore *store = backingStore();
    bool has_alpha = store->window()->property("_d_dxcb_TransparentBackground").toBool();

    if (!has_alpha)
        _d_dxcb_overridePaintDevice.setLocalData(true);

    VtableHook::callOriginalFun(store, &QPlatformBackingStore::beginPaint, region);

    _d_dxcb_overridePaintDevice.setLocalData(false);
}

void DPlatformBackingStoreHelper::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (Q_LIKELY(DWMSupport::instance()->hasComposite())) {
        DPlatformWindowHelper *window_helper = DPlatformWindowHelper::mapped.value(window->handle());

        if (window_helper && (window_helper->m_isUserSetClipPath || window_helper->m_windowRadius > 0)) {
            qreal device_pixel_ratio = window_helper->m_nativeWindow->window()->devicePixelRatio();
            QPainterPath path;

            path.addRegion(region);
            path -= window_helper->m_clipPath * device_pixel_ratio;

            if (path.isEmpty())
                goto end;

            QPainter pa(backingStore()->paintDevice());

            if (!pa.isActive())
                goto end;

            QBrush border_brush(window_helper->m_frameWindow->platformBackingStore->toImage());
            const QPoint &offset = window_helper->m_frameWindow->contentOffsetHint() * device_pixel_ratio;

            border_brush.setMatrix(QMatrix(1, 0, 0, 1, -offset.x(), -offset.y()));

            pa.setCompositionMode(QPainter::CompositionMode_Source);
            pa.setRenderHint(QPainter::Antialiasing);
            pa.fillPath(path, border_brush);
            pa.end();
        }
    }

end:
    return VtableHook::callOriginalFun(this->backingStore(), &QPlatformBackingStore::flush, window, region, offset);
}

DPP_END_NAMESPACE
