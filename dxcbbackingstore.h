#ifndef DXCBBACKINGSTORE_H
#define DXCBBACKINGSTORE_H

#define private public
#include <qpa/qplatformbackingstore.h>
#undef private

QT_BEGIN_NAMESPACE
class QXcbBackingStore;
class QWidgetWindow;
QT_END_NAMESPACE

class DXcbShmGraphicsBuffer;
class WindowEventListener;

class DXcbBackingStore : public QPlatformBackingStore
{
public:
    DXcbBackingStore(QWindow *window, QXcbBackingStore *proxy);
    ~DXcbBackingStore();

    QPaintDevice *paintDevice() Q_DECL_OVERRIDE;

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface.
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) Q_DECL_OVERRIDE;
#ifndef QT_NO_OPENGL
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                 QPlatformTextureList *textures, QOpenGLContext *context,
                                 bool translucentBackground) Q_DECL_OVERRIDE;
    QImage toImage() const Q_DECL_OVERRIDE;

    GLuint toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const Q_DECL_OVERRIDE;
#endif

    QPlatformGraphicsBuffer *graphicsBuffer() const Q_DECL_OVERRIDE;

    void resize(const QSize &size, const QRegion &staticContents) Q_DECL_OVERRIDE;

    void beginPaint(const QRegion &) Q_DECL_OVERRIDE;
    void endPaint() Q_DECL_OVERRIDE;

private:
    void initUserPropertys();

    void updateWindowMargins();
    void updateWindowExtents();
    void updateClipPath();

    void setWindowMargins(const QMargins &margins);

    void paintWindowShadow();
    void paintWindowBorder();

    inline bool isWidgetWindow() const
    { return isWidgetWindow(window());}
    static bool isWidgetWindow(const QWindow *window);
    QWidgetWindow *widgetWindow() const;

    inline QPoint windowOffset() const
    { return QPoint(windowMargins.left(), windowMargins.top());}
    inline QRect windowGeometry() const
    { return QRect(windowOffset(), m_image.size());}

    QSize m_size;
    QImage m_image;

    QXcbBackingStore *m_proxy;
    WindowEventListener *m_eventListener;
    DXcbShmGraphicsBuffer *m_graphicsBuffer = Q_NULLPTR;

    int windowRadius = 10;
    int windowBorder = 1;
    bool isUserSetClipPath = false;
    QPainterPath clipPath;
    QPainterPath windowClipPath;
    QColor windowBorderColor = QColor(255, 0, 0, 255 * 0.5);

    int shadowRadius = 20;//40;
    int shadowOffsetX = 0;
    int shadowOffsetY = 0;//10;
    QColor shadowColor = Qt::black;//QColor(0, 0, 0, 255 * 0.5);
    QImage shadowImage;
    QPixmap cornerShadowPixmap;

    QMargins windowMargins;

    friend class WindowEventListener;
};

Q_DECLARE_METATYPE(QPainterPath);

#endif // DXCBBACKINGSTORE_H
