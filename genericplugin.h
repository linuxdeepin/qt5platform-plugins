#ifndef GENERICPLUGIN_H
#define GENERICPLUGIN_H

#include "qxcbintegration.h"

class XcbWindowHook;

class GenericPlugin : public QXcbIntegration
{
public:
    GenericPlugin(const QStringList &parameters, int &argc, char **argv);

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
};


#endif // GENERICPLUGIN_H
