#include <private/qguiapplication_p.h>
#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qwindowsysteminterface.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <KWayland/Client/registry.h>
#include <private/qxkbcommon_p.h>
#endif

int main(int argc, char *argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QWaylandDisplay *display =
        static_cast<QWaylandIntegration *>(
            QGuiApplicationPrivate::platformIntegration())
            ->display();
    display->usingInputContextFromCompositor();
#endif
    return 0;
}
