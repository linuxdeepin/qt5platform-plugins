#ifndef UTILITY_H
#define UTILITY_H

#include <QImage>

class Utility
{
public:
    static QImage dropShadow(const QPixmap &px, qreal radius, const QColor &color);
    static QImage borderImage(const QPixmap &px, const QMargins &borders, const QSize &size,
                              QImage::Format format = QImage::Format_ARGB32_Premultiplied);

    static void moveWindow(uint WId);
    static void cancelMoveWindow(uint WId);
    static void setWindowExtents(uint WId, const QSize &winSize, const QMargins &margins, const int resizeHandleWidth);

private:
    static void sendMoveResizeMessage(uint WId, Qt::MouseButton qbutton, int action);
};

#endif // UTILITY_H
