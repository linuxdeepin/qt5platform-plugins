#include "genericplugin.h"
#include "xcbwindowhook.h"
#include "dxcbbackingstore.h"
#include "global.h"

#include "qxcbscreen.h"
#include "qxcbbackingstore.h"

#include <private/qwidgetwindow_p.h>

#include <QWidget>

GenericPlugin::GenericPlugin(const QStringList &parameters, int &argc, char **argv)
    : QXcbIntegration(parameters, argc, argv)
{

}

QPlatformWindow *GenericPlugin::createPlatformWindow(QWindow *window) const
{
    qDebug() << __FUNCTION__ << window << window->type() << window->parent();

    bool isUseDxcb = window->type() != Qt::Desktop && window->property(useDxcb).toBool();

    if (isUseDxcb) {
        QSurfaceFormat format = window->format();

        const int oldAlpha = format.alphaBufferSize();
        const int newAlpha = 8;

        if (oldAlpha != newAlpha) {
            format.setAlphaBufferSize(newAlpha);
            window->setFormat(format);
        }
    }

    QPlatformWindow *w = QXcbIntegration::createPlatformWindow(window);

    if (isUseDxcb) {
        (void)new XcbWindowHook(dynamic_cast<QXcbWindow*>(w));
    }

    return w;
}

QPlatformBackingStore *GenericPlugin::createPlatformBackingStore(QWindow *window) const
{
    qDebug() << __FUNCTION__ << window << window->type() << window->parent();

    QPlatformBackingStore *store = QXcbIntegration::createPlatformBackingStore(window);

    if (window->type() != Qt::Desktop && window->property(useDxcb).toBool())
        return new DXcbBackingStore(window, static_cast<QXcbBackingStore*>(store));

    return store;
}
