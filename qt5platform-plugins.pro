TEMPLATE = subdirs

!DISABLE_WAYLAND {
    SUBDIRS += wayland/wayland.pro
}

!DISABLE_XCB {
    SUBDIRS += xcb/xcb.pro
}
