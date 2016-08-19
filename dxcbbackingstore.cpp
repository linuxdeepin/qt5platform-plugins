#include "dxcbbackingstore.h"
#include "xcbwindowhook.h"
#include "vtablehook.h"
#include "utility.h"

#include "qxcbbackingstore.h"

#include <QDebug>
#include <QPainter>
#include <QEvent>
#include <QWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainterPathStroker>

#include <private/qwidgetwindow_p.h>
#include <qpa/qplatformgraphicsbuffer.h>

#define MOUSE_MARGINS 10

#define PUBLIC_CLASS(Class, Target) \
    class D##Class : public Class\
    {friend class Target;}

PUBLIC_CLASS(QMouseEvent, WindowEventListener);
PUBLIC_CLASS(QWheelEvent, WindowEventListener);
PUBLIC_CLASS(QResizeEvent, WindowEventListener);
PUBLIC_CLASS(QWidget, WindowEventListener);
PUBLIC_CLASS(QWindow, WindowEventListener);

class WindowEventListener : public QObject
{
public:
    explicit WindowEventListener(DXcbBackingStore *store)
        : QObject(0)
        , m_store(store)
    {
        store->window()->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE
    {
        QWindow *window = qobject_cast<QWindow*>(obj);

        if (!window)
            return false;

        const QRect &window_geometry = window->geometry();
//        qDebug() << obj << event->type() << window_geometry;

        switch ((int)event->type()) {
        case QEvent::Wheel: {
            DQWheelEvent *e = static_cast<DQWheelEvent*>(event);

            if (!window_geometry.contains(e->globalPos()))
                return true;

            e->p -= m_store->windowOffset();

            break;
        }
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            DQMouseEvent *e = static_cast<DQMouseEvent*>(event);

            if (e->buttons() == Qt::LeftButton) {
                if (e->type() == QEvent::MouseButtonPress)
                    setLeftButtonPressed(true);
                else if (e->type() == QEvent::MouseButtonRelease)
                    setLeftButtonPressed(false);
            } else {
                setLeftButtonPressed(false);
            }

            e->l -= m_store->windowOffset();
            e->w -= m_store->windowOffset();

            if (!window_geometry.contains(e->globalPos())) {
                if (event->type() == QEvent::MouseMove) {
//                    const QMargins &mouseMargins = QMargins(MOUSE_MARGINS, MOUSE_MARGINS, MOUSE_MARGINS, MOUSE_MARGINS);
//                    const QList<QRect> &list = Utility::sudokuByRect(window_geometry + mouseMargins, mouseMargins);

//                    Qt::Corner mouseCorner;

//                    if (e->globalX() <= window_geometry.x()) {
//                        if (e->globalY() <= window_geometry.y()) {

//                        }
//                    }

                    if (leftButtonPressed) {
                        Utility::startWindowSystemResize(window->winId(), Utility::RightEdge, e->globalPos());
                    }
                }

                return true;
            }

            break;
        }
        case QEvent::Resize: {
            DQResizeEvent *e = static_cast<DQResizeEvent*>(event);

            const QRect &rect = QRect(QPoint(0, 0), e->size());

            e->s = (rect - m_store->windowMargins).size();

            break;
        }
        }

        return false;
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        Q_UNUSED(event);

        Utility::moveWindow(reinterpret_cast<QWidget*>(this)->winId());
    }

private:
    void setLeftButtonPressed(bool pressed)
    {
        if (leftButtonPressed == pressed)
            return;

        if (!pressed)
            Utility::cancelMoveWindow(m_store->window()->winId());

        leftButtonPressed = pressed;

        const QWidgetWindow *widgetWindow = m_store->widgetWindow();

        QWidget *widget = widgetWindow->widget();

        if (widget) {
            if (pressed) {
                VtableHook::overrideVfptrFun(static_cast<DQWidget*>(widget), &DQWidget::mouseMoveEvent,
                                             this, &WindowEventListener::mouseMoveEvent);
            } else {
                VtableHook::resetVfptrFun(static_cast<DQWidget*>(widget), &DQWidget::mouseMoveEvent);
            }
        } else {
            QWindow *window = m_store->window();

            if (pressed) {
                VtableHook::overrideVfptrFun(static_cast<DQWindow*>(window), &DQWindow::mouseMoveEvent,
                                             this, &WindowEventListener::mouseMoveEvent);
            } else {
                VtableHook::resetVfptrFun(static_cast<DQWindow*>(window), &DQWindow::mouseMoveEvent);
            }
        }
    }

    bool leftButtonPressed = false;

    DXcbBackingStore *m_store;
};

class DXcbShmGraphicsBuffer : public QPlatformGraphicsBuffer
{
public:
    DXcbShmGraphicsBuffer(QImage *image)
        : QPlatformGraphicsBuffer(image->size(), QImage::toPixelFormat(image->format()))
        , m_access_lock(QPlatformGraphicsBuffer::None)
        , m_image(image)
    { }

    bool doLock(AccessTypes access, const QRect &rect) Q_DECL_OVERRIDE
    {
        Q_UNUSED(rect);
        if (access & ~(QPlatformGraphicsBuffer::SWReadAccess | QPlatformGraphicsBuffer::SWWriteAccess))
            return false;

        m_access_lock |= access;
        return true;
    }
    void doUnlock() Q_DECL_OVERRIDE { m_access_lock = None; }

    const uchar *data() const Q_DECL_OVERRIDE { return m_image->bits(); }
    uchar *data() Q_DECL_OVERRIDE { return m_image->bits(); }
    int bytesPerLine() const Q_DECL_OVERRIDE { return m_image->bytesPerLine(); }

    Origin origin() const Q_DECL_OVERRIDE { return QPlatformGraphicsBuffer::OriginTopLeft; }

private:
    AccessTypes m_access_lock;
    QImage *m_image;
};

DXcbBackingStore::DXcbBackingStore(QWindow *window, QXcbBackingStore *proxy)
    : QPlatformBackingStore(window)
    , m_proxy(proxy)
    , m_eventListener(new WindowEventListener(this))
{
    shadowPixmap.fill(Qt::transparent);

    initUserPropertys();

    //! Warning: At this point you must be initialized window Margins and window Extents
    updateWindowMargins();
    updateWindowExtents();

    QObject::connect(window, &QWindow::destroyed, m_eventListener, [this] {
        delete this;
    });
}

DXcbBackingStore::~DXcbBackingStore()
{
    delete m_proxy;
    delete m_eventListener;

    if (m_graphicsBuffer)
        delete m_graphicsBuffer;
}

QPaintDevice *DXcbBackingStore::paintDevice()
{
    return &m_image;
}

void DXcbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    const QPoint &windowOffset = this->windowOffset();
    QRegion tmp_region = region.translated(windowOffset);

//    qDebug() << "flush" << window << tmp_region << offset;

    QPainter pa(m_proxy->paintDevice());

    pa.setCompositionMode(QPainter::CompositionMode_Source);
    pa.drawPixmap(windowOffset, shadowPixmap, windowGeometry());
    pa.setRenderHint(QPainter::Antialiasing);
    pa.setClipPath(windowClipPath);
    pa.drawImage(windowOffset, m_image);
    pa.end();

    XcbWindowHook *window_hook = XcbWindowHook::getHookByWindow(window->handle());

    if (window_hook)
        window_hook->windowMargins = QMargins(0, 0, 0, 0);

    m_proxy->flush(window, tmp_region, offset);

    if (window_hook)
        window_hook->windowMargins = windowMargins;
}

void DXcbBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                       QPlatformTextureList *textures, QOpenGLContext *context,
                                       bool translucentBackground)
{
    Q_UNUSED(textures);
    Q_UNUSED(context);
    Q_UNUSED(translucentBackground)

    flush(window, region, offset);
}

QImage DXcbBackingStore::toImage() const
{
    return m_image;
}

GLuint DXcbBackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize,
                                   QPlatformBackingStore::TextureFlags *flags) const
{
    return m_proxy->toTexture(dirtyRegion, textureSize, flags);
}

QPlatformGraphicsBuffer *DXcbBackingStore::graphicsBuffer() const
{
    return m_graphicsBuffer;
}

void DXcbBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    qDebug() << "resize" << size << staticContents;

    const int dpr = int(window()->devicePixelRatio());
    const QSize xSize = size * dpr;
    if (xSize == m_image.size() && dpr == m_image.devicePixelRatio())
        return;

    if (m_graphicsBuffer)
        delete m_graphicsBuffer;

    m_image = QImage(xSize, QImage::Format_ARGB32_Premultiplied);
    m_image.setDevicePixelRatio(dpr);
//    m_image.fill(Qt::transparent);
    // Slow path for bgr888 VNC: Create an additional image, paint into that and
    // swap R and B while copying to m_image after each paint.

    m_graphicsBuffer = new DXcbShmGraphicsBuffer(&m_image);

    m_size = QSize(size.width() + windowMargins.left() + windowMargins.right(),
                   size.height() + windowMargins.top() + windowMargins.bottom());

    m_proxy->resize(m_size, staticContents);

    updateClipPath();
    //! TODO: update window margins
//    updateWindowMargins();
    paintWindowShadow();
    updateWindowExtents();
}

void DXcbBackingStore::beginPaint(const QRegion &reg)
{
//    qDebug() << "begin paint" << reg << window()->geometry();

    QPainter pa(&m_image);

    pa.setCompositionMode(QPainter::CompositionMode_Clear);

    for (const QRect &rect : reg.rects()) {
        pa.fillRect(rect, Qt::transparent);
    }

    pa.end();

    m_proxy->beginPaint(reg.translated(windowOffset()));
}

void DXcbBackingStore::endPaint()
{
    m_proxy->endPaint();

//    qDebug() << "end paint";
}

void DXcbBackingStore::initUserPropertys()
{
    const QWindow *window = this->window();

    QVariant v = window->property("shadowRadius");

    bool ok;
    int tmp = v.toInt(&ok);

    if (ok)
        shadowRadius = tmp;

    v = window->property("shadowOffsetX");
    tmp = v.toInt(&ok);

    if (ok)
        shadowOffsetX = tmp;

    v = window->property("shadowOffsetY");
    tmp = v.toInt(&ok);

    if (ok)
        shadowOffsetY = tmp;

    v = window->property("shadowColor");
    QColor color = qvariant_cast<QColor>(v);

    if (color.isValid())
        shadowColor = color;

    v = window->property("clipPath");

    if (v.isValid()) {
        clipPath = qvariant_cast<QPainterPath>(v);

        if (!clipPath.isEmpty())
            isUserSetClipPath = true;
    }
}

void DXcbBackingStore::updateWindowMargins()
{
    setWindowMargins(QMargins(shadowRadius - shadowOffsetX,
                              shadowRadius - shadowOffsetY,
                              shadowRadius + shadowOffsetX,
                              shadowRadius + shadowOffsetY));
}

void DXcbBackingStore::updateWindowExtents()
{
    const QMargins &borderMargins = QMargins(windowBorder, windowBorder, windowBorder, windowBorder);
    const QMargins &extentsMargins = windowMargins - borderMargins;

    Utility::setWindowExtents(window()->winId(), m_size, extentsMargins, MOUSE_MARGINS);
}

void DXcbBackingStore::updateClipPath()
{
    if (isUserSetClipPath) {
        const QVariant &v = window()->property("clipPath");

        if (v.isValid()) {
            clipPath = qvariant_cast<QPainterPath>(v);

            isUserSetClipPath = !clipPath.isEmpty();
        }
    }

    if (!isUserSetClipPath) {
        clipPath = QPainterPath();
        clipPath.addRoundedRect(QRect(QPoint(0, 0), m_image.size()), windowRadius, windowRadius);
    }

    windowClipPath = clipPath.translated(windowOffset());
}

void DXcbBackingStore::setWindowMargins(const QMargins &margins)
{
    if (windowMargins == margins)
        return;

    windowMargins = margins;
    windowClipPath = clipPath.translated(windowOffset());

    XcbWindowHook *hook = XcbWindowHook::getHookByWindow(m_proxy->window()->handle());

    if (!hook) {
        return;
    }

    hook->windowMargins = margins;

    const QSize &tmp_size = window()->handle()->QPlatformWindow::geometry().size();

    m_size = QSize(tmp_size.width() + windowMargins.left() + windowMargins.right(),
                   tmp_size.height() + windowMargins.top() + windowMargins.bottom());

    m_proxy->resize(m_size, QRegion());
}

inline QSize margins2Size(const QMargins &margins)
{
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom());
}

void DXcbBackingStore::paintWindowShadow()
{
    QPixmap pixmap(m_image.size());

    pixmap.fill(Qt::transparent);

    QPainter pa(&pixmap);

    pa.fillPath(clipPath, shadowColor);
    pa.end();

    bool paintShadow = isUserSetClipPath || shadowPixmap.isNull();

    if (!paintShadow) {
        QSize margins_size = margins2Size(windowMargins + windowRadius + windowBorder);

        if (margins_size.width() > qMin(m_size.width(), shadowPixmap.width())
                || margins_size.height() > qMin(m_size.height(), shadowPixmap.height())) {
            paintShadow = true;
        }
    }

    if (paintShadow) {
        QImage image = Utility::dropShadow(pixmap, shadowRadius, shadowColor);

        /// begin paint window border;
        QPainter pa(&image);
        QPainterPathStroker pathStroker;

        pathStroker.setWidth(windowBorder * 2);

        QTransform transform = pa.transform();
        const QRectF &clipRect = clipPath.boundingRect();

        transform.translate(windowMargins.left() + 2, windowMargins.top() + 2);
        transform.scale((clipRect.width() - 4) / clipRect.width(),
                        (clipRect.height() - 4) / clipRect.height());

        pa.setCompositionMode(QPainter::CompositionMode_Source);
        pa.setRenderHint(QPainter::Antialiasing);
        pa.fillPath(pathStroker.createStroke(windowClipPath), windowBorderColor);
        pa.setCompositionMode(QPainter::CompositionMode_Clear);
        pa.setRenderHint(QPainter::Antialiasing, false);
        pa.setTransform(transform);
        pa.fillPath(clipPath, Qt::transparent);
        pa.end();
        /// end

        shadowPixmap = QPixmap::fromImage(image);
    } else {
        shadowPixmap = QPixmap::fromImage(Utility::borderImage(shadowPixmap, windowMargins + windowRadius, m_size));
    }

    /// begin paint window drop shadow
    pa.begin(m_proxy->paintDevice());
    pa.setCompositionMode(QPainter::CompositionMode_Source);
    pa.drawPixmap(0, 0, shadowPixmap);
    pa.end();

    XcbWindowHook *window_hook = XcbWindowHook::getHookByWindow(window()->handle());

    if (window_hook)
        window_hook->windowMargins = QMargins(0, 0, 0, 0);

    QRegion region;

    region += QRect(windowOffset().x(), 0, m_size.width(), windowOffset().y());
    region += QRect(0, 0, windowOffset().x(), m_size.height());

    m_proxy->flush(window(), region, QPoint(0, 0));

    if (window_hook)
        window_hook->windowMargins = windowMargins;
    /// end
}

bool DXcbBackingStore::isWidgetWindow(const QWindow *window)
{
    return window->metaObject()->className() == QStringLiteral("QWidgetWindow");
}

QWidgetWindow *DXcbBackingStore::widgetWindow() const
{
    return static_cast<QWidgetWindow*>(window());
}
