qtHaveModule(DWaylandClient) {
    QT       += DWaylandClient
    LIBS     += -lDWaylandClient

    DEFINES += D_DEEPIN_IS_DWAYLAND
} else {
qtHaveModule(KWaylandClient) {
    QT       += KWaylandClient
    LIBS     += -lKF5WaylandClient
}
}
