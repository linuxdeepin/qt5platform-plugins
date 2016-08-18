#include "utility.h"

#include <QPixmap>
#include <QPainter>
#include <QCursor>
#include <QDebug>
#include <QtX11Extras/QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_CANCEL           11   /* cancel operation */

const char kAtomNameMoveResize[] = "_NET_WM_MOVERESIZE";

QT_BEGIN_NAMESPACE
//extern Q_WIDGETS_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QImage Utility::dropShadow(const QPixmap &px, qreal radius, const QColor &color)
{
    if (px.isNull())
        return QImage();

    QImage tmp(px.size() + QSize(radius * 2, radius * 2), QImage::Format_ARGB32_Premultiplied);
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QPoint(radius, radius), px);
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, radius, false, true);
    blurPainter.end();

    if (color == QColor(Qt::black))
        return blurred;

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    return tmp;
}

void Utility::moveWindow(uint WId)
{
    sendMoveResizeMessage(WId, Qt::LeftButton, _NET_WM_MOVERESIZE_MOVE);
}

void Utility::cancelMoveWindow(uint WId)
{
    sendMoveResizeMessage(WId, Qt::LeftButton, _NET_WM_MOVERESIZE_CANCEL);
}

void Utility::sendMoveResizeMessage(uint WId, Qt::MouseButton qbutton, int action)
{
    const auto display = QX11Info::display();
    const auto screen = QX11Info::appScreen();

    int xbtn = qbutton == Qt::LeftButton ? Button1 :
               qbutton == Qt::RightButton ? Button3 :
               AnyButton;

    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    const Atom net_move_resize = XInternAtom(display, kAtomNameMoveResize, false);
    xev.xclient.type = ClientMessage;
    xev.xclient.message_type = net_move_resize;
    xev.xclient.display = display;
    xev.xclient.window = WId;
    xev.xclient.format = 32;

    const auto global_position = QCursor::pos();
    xev.xclient.data.l[0] = global_position.x();
    xev.xclient.data.l[1] = global_position.y();
    xev.xclient.data.l[2] = action;
    xev.xclient.data.l[3] = xbtn;
    xev.xclient.data.l[4] = 0;
    XUngrabPointer(display, QX11Info::appTime());

    XSendEvent(display,
               QX11Info::appRootWindow(screen),
               false,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &xev);

    XFlush(display);
}

void Utility::setWindowExtents(uint WId, const QSize &winSize, const QMargins &margins, const int resizeHandleWidth)
{
    Atom frameExtents;
    unsigned long value[4] = {
        (unsigned long)(margins.left()),
        (unsigned long)(margins.right()),
        (unsigned long)(margins.top()),
        (unsigned long)(margins.bottom())
    };
    frameExtents = XInternAtom(QX11Info::display(), "_GTK_FRAME_EXTENTS", False);
    if (frameExtents == None) {
        qWarning() << "Failed to create atom with name DEEPIN_WINDOW_SHADOW";
        return;
    }
    XChangeProperty(QX11Info::display(),
                    WId,
                    frameExtents,
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *)value,
                    4);

    QRect tmp_rect = QRect(QPoint(0, 0), winSize);

    tmp_rect -= margins;

    XRectangle contentXRect;
    contentXRect.x = 0;
    contentXRect.y = 0;
    contentXRect.width = tmp_rect.width() + resizeHandleWidth * 2;
    contentXRect.height = tmp_rect.height() + resizeHandleWidth * 2;
    XShapeCombineRectangles(QX11Info::display(),
                            WId,
                            ShapeInput,
                            margins.left() - resizeHandleWidth,
                            margins.top() - resizeHandleWidth,
                            &contentXRect, 1, ShapeSet, YXBanded);
}
