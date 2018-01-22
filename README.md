# qt5dxcb-plugin

qt5dxcb-plugin is the Qt platform integration plugin for Deepin Desktop Environment.

## Dependencies

### Build dependencies

* pkg-config
* mtdev
* xcb-xkb
* xcb-render-util
* xcb-image
* xcb-icccm4
* xcb-keysyms1-dev
* egl1-mesa
* xkbcommon-x11
* dbus-1
* udev
* xrender
* xi
* sm
* xcb-xinerama
* fontconfig
* freetype6
* glib2.0
* xcb-damage
* xcb-composite
* cairo2
* Qt5 (>= 5.6)
  * Qt5-Core
  * Qt5-Gui
  * Qt5-OpenGL
  * Qt5-X11extras
  * Qt5-Core-Private

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

2. Build:
```
$ cd qt5dxcb-plugin
$ mkdir build
$ cd build
$ qmake ..
$ make
```

3. Install:
```
$ sudo make install
```

## Usage

To be done.

## Getting help

You may also find these channels useful if you encounter any other issues:

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC Channel](https://webchat.freenode.net/?channels=deepin)
* [Official Forum](https://bbs.deepin.org/)
* [Wiki](https://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

qt5dxcb-plugin is licensed under [GPLv3](LICENSE).
