#include <KWayland/Client/registry.h>
#include <private/qguiapplication_p.h>
#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <private/qxkbcommon_p.h>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qwindowsysteminterface.h>

int main(int argc, char *argv[])
{
    QWaylandDisplay *display =
        static_cast<QWaylandIntegration *>(
            QGuiApplicationPrivate::platformIntegration())
            ->display();
    display->usingInputContextFromCompositor();
    return 0;
}
