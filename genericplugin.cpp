#include "genericplugin.h"

#include "xcbwindowhook.h"
#include "dxcbbackingstore.h"

#include "qxcbscreen.h"
#include "qxcbbackingstore.h"

#include <private/qwidgetwindow_p.h>

#include <QWidget>
#include <QOpenGLWidget>
#include <QtOpenGL/QGLWidget>

GenericPlugin::GenericPlugin(const QStringList &parameters, int &argc, char **argv)
    : QXcbIntegration(parameters, argc, argv)
{

}

QPlatformWindow *GenericPlugin::createPlatformWindow(QWindow *window) const
{
    qDebug() << __FUNCTION__ << window << window->type() << window->parent();

//    if (window->type() == Qt::Window) {
        QSurfaceFormat format = window->format();
        const int oldAlpha = format.alphaBufferSize();
        const int newAlpha = 8;
        if (oldAlpha != newAlpha) {
            format.setAlphaBufferSize(newAlpha);
            window->setFormat(format);
        }
//    }

    QPlatformWindow *w = QXcbIntegration::createPlatformWindow(window);

    w->setWindowFlags(Qt::FramelessWindowHint);

    if (window->type() != Qt::Desktop) {
        //            if (widget_window->widget()->property("enableDxcb").toBool())
                        new XcbWindowHook(dynamic_cast<QXcbWindow*>(w));
    }

    return w;
}

QPlatformOpenGLContext *GenericPlugin::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return QXcbIntegration::createPlatformOpenGLContext(context);
}

QPlatformBackingStore *GenericPlugin::createPlatformBackingStore(QWindow *window) const
{
    qDebug() << __FUNCTION__ << window << window->type() << window->parent();

    QPlatformBackingStore *store = QXcbIntegration::createPlatformBackingStore(window);

    if (window->type() == Qt::Desktop)
        return store;

//    if (widget_window->widget()->property("enableDxcb").toBool())
        return new DXcbBackingStore(window, static_cast<QXcbBackingStore*>(store));

    return store;
}

QPlatformTheme *GenericPlugin::createPlatformTheme(const QString &name) const
{
    QPlatformTheme *theme = QXcbIntegration::createPlatformTheme(name);

    qDebug() << __FUNCTION__ << name << themeNames() << theme;

    return theme;
}
