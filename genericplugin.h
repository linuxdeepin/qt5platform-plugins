#ifndef GENERICPLUGIN_H
#define GENERICPLUGIN_H

#include "qxcbintegration.h"

class XcbWindowHook;

class GenericPlugin : public QXcbIntegration
{
public:
    GenericPlugin(const QStringList &parameters, int &argc, char **argv);

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;

    QPlatformTheme *createPlatformTheme(const QString &name) const Q_DECL_OVERRIDE;
};


#endif // GENERICPLUGIN_H
