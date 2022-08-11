# qt5dxcb-plugin

qt5dxcb-plugin是一个为Deepin桌面环境开发的qt平台集成插件

## 依赖

### 构建依赖

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

## 安装

### 从源码编译

1. 首先确保你已经安装了所有的构建依赖

2. 构建：

支持禁用某些模块, 在执行qmake时加入CONFIG+=<val>

val:

- DISABLE_WAYLAND
- DISABLE_XCB

```
$ cd qt5dxcb-plugin
$ mkdir build
$ cd build
$ qmake ..
$ make
```

1. 安装
```
$ sudo make install
```

## 使用

待完成

## 获取帮助

如果你碰到了任何其它问题可以在下面的频道中获取帮助

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC Channel](https://webchat.freenode.net/?channels=deepin)
* [Official Forum](https://bbs.deepin.org/)
* [Wiki](https://wiki.deepin.org/)

## 参与其中

我们鼓励您报告问题并贡献更改

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en) (英文)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## 开源许可

qt5dxcb-plugin项目采用[GPLv3](LICENSE)授权。

## 环境变量

* DXCB_PRINT_WINDOW_CREATE 在本地窗口被创建时打印对应的 QWindow 的信息
* DXCB_PAINTENGINE_DISABLE_FEATURES 指定要禁用的 QPaintEngine::PaintEngineFeatures，只对 QPainter 绘图系统有效
* DXCB_FAKE_PLATFORM_NAME_XCB 将 QGuiApplication::platformName 属性的值将被强制设置为 "xcb"
* DXCB_DISABLE_HOOK_CURSOR 禁止使用 QWindow::devicePixelRatio 的值自动缩放光标在此窗口上的大小
* DXCB_REDIRECT_CONTENT 设置是否允许对开启了 dxcb 模式的窗口使用 XDamage 重定向窗口绘制的内容，值为 "true" 表示允许，值为 "false" 表示不允许。否则将使用窗口自己设置的值，如果窗口未设置任何值，则窗口的 surfaceType 为 QSurface::OpenGLSurface 时会开启重定向模式，否则不开启
* DXCB_REDIRECT_CONTENT_WITH_NO_COMPOSITE 强制允许在未开启窗口特效的模式下使用 XDamage 重定向窗口绘制的内容，未设置或值为空时，在窗口管理器不支持 Composite 的情况下，将不使用 XDamage 重定向窗口绘制的内容
