<a name=""></a>
## 1.1.6.1 (2018-02-02)


#### Features

*   support for using the environment variable "DXCB_REDIRECT_CONTENT" to configure whether to redirect the contents of the window ([aeec2d9d](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/aeec2d9d60042bbcaa58e1d96281f38032cbf2df))
*   support config redirect content ([77859430](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/778594309ed3f9633f828838a76d779002d1c261))
*   auto clean virtual table list ([1a4c427f](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/1a4c427f5adf4921055f27dfda37cb521ecd330e))
*   add pluginVersion native function ([aac0d071](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/aac0d0716311fc7757eca2a21ec39778c1970622))
*   use x11 composite and damage extensions to clip content window ([5cc7b296](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/5cc7b296fbaa6e42455fe886eb8eec0ab6f12173))

#### Bug Fixes

*   set miniumu/maximum size inoperative for window ([6404e5e4](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/6404e5e4c3a42cb725c7958e2a599931d2130bd9))
*   no debug package ([e5767453](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/e57674533bc44ea7562d903f52777dbce9ac9cd0))
*   window content no repaint ([faa4106c](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/faa4106cbdb648f31e5e05c312058e30d725d8ca))
*   update xpixmap of content window when frame window map ([191129dc](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/191129dcdb38bcfbb09c064a1f23378f4bc1b490))
*   update readme ([eb55797c](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/eb55797c2a560d7ef79a69baf51c77721b66b1c8))
*   "DXCB_VERSION" is empty of qmake ([4479f404](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/4479f404add8724ad7fd95027e7adadcb6503f67))
*   set the window size into an infinite loop ([a8cb6e92](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/a8cb6e92ac59dcee69b28ec41ba251b009a3e3c7))
*   can not maximized on openbox ([36894681](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/36894681a4dc55e12df5c94125237e8bb1d1deb4))
*   cairo surface not refresh on window repaint ([e02b8355](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/e02b8355eb3eaf74957915c5fb050381ab523739))
*   not draw content window if nativeWindowXSurface is null ([1a021a76](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/1a021a76c7b68266176757c8d79ca41791835c0e))
*   crash at ~DPlatformWindowHelper() ([01e77576](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/01e7757670e32802a5235f7bd7bafc030a81a281))
*   content window geometry update is not timely ([a1416a78](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/a1416a784415c5f197d66057d313ab8e99e503e6))
*   QApplication::topLevelAt return null ([6f84bbba](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/6f84bbba08ae81bc2547902e8a05ca0582e56fb6))



<a name=""></a>
##  1.1.6 (2017-12-28)


#### Bug Fixes

*   send resize event to content window on frame window size changed ([fc88f619](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/fc88f6198c3273acc49f566f95efc459b5a357ba))
*   the content window frame margins is invalid ([bb069366](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/bb069366555c07d1750f376677b20b19fa09e8d3))
*   content window position is wrong at show ([2f74cd2c](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/2f74cd2ce4db37009141d181f9752f3b124dee09))
*   add break for switch/case ([ca84b062](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/ca84b0622ebee89d7454401d261f99f577850f36))
*   fcitx input method can not work in QTextEdit ([cbd74f2a](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/cbd74f2a6a9f137d0081848a53a40bd50d350ca6))
*   window blur area is a rounded rect if set clip path ([a89e7d0c](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/a89e7d0c035137bf1f85c7993eb14cb190f98ce0))
*   LICENSE ([f8013823](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/f801382355dd62618fa0548c87cf6752b53673e8))
*   after moving the window release the mouse and then move the mouse will receive a mouse release event ([10a73727](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/10a73727fe067bbe395d38c46890508ddd1ef559))
*   Adapt lintian ([1f862c11](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/1f862c1175a2342520a3340621cb122e4478f88a))
*   also search for libqt5xcbqpa private headers in system ([e47d510f](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/e47d510f9c2d371f59c33a290d27347840318f44))
*   do not paint shadow on window maximized ([b1483de1](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/b1483de187d26ac360aa6b6f08469fad61b32077))

#### Features

*   eat focus event on grab/ungrab ([936da4ca](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/936da4ca99900ee4256ede4ca33b8767f189e5a2))
*   support for Qt 5.10.0 ([247cabb7](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/247cabb70d2f97de4e6cec062e1106c9678d97a5))



<a name=""></a>
##  1.1.5 (2017-12-07)


#### Bug Fixes

*   visable black border when maximize window ([9a25eccc](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/9a25eccc5e50ccd2cf196671d898ca83ed468f19))
*   output "QPainter::begin: Paint device returned engine == 0, type: 3" ([d424dd61](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/d424dd61e2adc6cb4f96315354128751a2ac60cf))



<a name=""></a>
##  1.1.4 (2017-12-06)


#### Features

*   support for Qt 5.9.3 ([32ec6503](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/32ec650304f12e3107c7dd13677c0cbb0afb1023))

#### Bug Fixes

*   compiling failed in Qt5.7.0 and above ([790377a5](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/790377a5ecadbcd86a0bb62bc71cf71144873984))



<a name=""></a>
##  1.1.3.1 (2017-12-06)


#### Features

*   not clean when begin paint on opaque window ([635a8252](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/635a8252efe0500e2a299b0f336bab6b0362da1e))

#### Bug Fixes

*   visible lines at bottom/right on frame window ([40f53663](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/40f536630fac0222f6e2ae29d0cfcd604f9e6b77))
*   content window area path on window maximized/fullscreen ([4dd1dc20](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/4dd1dc204125a62137ad735bc8e387ef0150314b))
*   mouse position error on automatic adsorption ([196e34cb](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/196e34cbade9e665b4b766e9c62332930cb34f54))



<a name=""></a>
##  1.1.3 (2017-11-28)


#### Features

*   add native function: popupSystemWindowMenu ([0e741b1e](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/0e741b1ef87e77df108c9cccd3fb24e5a9010a88))
*   add showWindowSystemMenu function ([915c6d3f](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/915c6d3f75723c4574c00a786511ea098345d0d8))
*   support for Qt 5.9.2 ([715bb1e4](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/715bb1e411ebf5283a4aec5416c9b7d19d09a55d))

#### Bug Fixes

*   window vaild path is empty ([c2b1a1b5](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/c2b1a1b52324f668a45213bebf1282aeec2c64e2))
*   blur area overstep the content window area ([69f5edef](https://github.com/linuxdeepin/qt5dxcb-plugin/commit/69f5edef4453780c1566bc000b63307a5417b14d))



<a name=""></a>
##  1.1.2 (2017-11-16)


#### Features

*   support set the window radius for QOpenGLWindow ([431fa688](https://github.com/linuxdeepin/qt5integration/commit/431fa688a6fad3a5057c89396342dfcc53987b04))
*   use fake platform name when set DXCB_FAKE_PLATFORM_NAME_XCB ([471216b2](https://github.com/linuxdeepin/qt5integration/commit/471216b22d3ab08129d6d6e5c9e47696947adaaa))

#### Bug Fixes

*   the drag window position when dragging from one screen to another ([3905a0a0](https://github.com/linuxdeepin/qt5integration/commit/3905a0a02e1f17092b6c324516ae130172e247c9))
*   the DFrameWindow's border path is wrong ([4882b916](https://github.com/linuxdeepin/qt5integration/commit/4882b91676593eadb8dcc98fb2a86ae82d0fe87a))
*   the content window screen is wrong ([20f7695f](https://github.com/linuxdeepin/qt5integration/commit/20f7695f59a3a07981fc9bc5dde874385e215328))
*   update screen for content window when the frame window screen changed ([f40a66ab](https://github.com/linuxdeepin/qt5integration/commit/f40a66abdaea1af1d8c00079717dc8d8c2622748))
*   can not move window on openbox wm ([af64d84d](https://github.com/linuxdeepin/qt5integration/commit/af64d84dfc7d98e2bf5b8ea5a11507e200111431))



<a name=""></a>
##  1.1.1 (2017-11-10)


#### Bug Fixes

*   crash on use the QWebEngineView ([c13bdeee](https://github.com/linuxdeepin/qt5integration/commit/c13bdeee242b0cbfebef78df925028f285913f2c))
*   mouse enter of the window to receive the mouse to release the event ([7a6989e5](https://github.com/linuxdeepin/qt5integration/commit/7a6989e5cf8a2be0cc84644c9289e647704b06c6))
*   mouse over the right border of the window to receive the mouse to leave the event ([cad2e037](https://github.com/linuxdeepin/qt5integration/commit/cad2e0379abf745e5d59fbca5902a1008edfff4f))



<a name=""></a>
##  1.1 (2017-11-06)


#### Features

*   print info on create new window ([b68d0371](https://github.com/linuxdeepin/qt5integration/commit/b68d03713feb1d77640b35891e38e05736becba9))

#### Bug Fixes

*   update the content window clip path by it size ([070e3bc4](https://github.com/linuxdeepin/qt5integration/commit/070e3bc4d9c715d1c8c4e1531077286c43658815))
*   the content window position is wrong when content margins hint changed ([9b1a28e6](https://github.com/linuxdeepin/qt5integration/commit/9b1a28e65d60e7286687943511a58485e0937bf5))
*   window focus out when mouse leave ([14c14df2](https://github.com/linuxdeepin/qt5integration/commit/14c14df2c7a92be746d5479558ae2c61765052a9))
*   the DFrameWindow's shadow and border ([e35a87a9](https://github.com/linuxdeepin/qt5integration/commit/e35a87a93872acde909383b6f5b4ca3baf63bfe5))
*   the chile window is blocked by modal window ([db20016c](https://github.com/linuxdeepin/qt5integration/commit/db20016c6baa38ea561cddb9a3676252a19d8af6))
*   warning: "W: Ignoring Provides line with non-equal DepCompareOp for package dde-qt5integration" for apt-get ([a074941e](https://github.com/linuxdeepin/qt5integration/commit/a074941e238476cf5e93fe2400e72d0901474d64))
*   disable update frame mask ([5d136679](https://github.com/linuxdeepin/qt5integration/commit/5d136679f4886523dc3bc779ac8d0347611028bc))
*   can not install ([1ac9975a](https://github.com/linuxdeepin/qt5integration/commit/1ac9975a9169b47f185ef5e33d79e3e2de2dbc50))



<a name=""></a>
##  1.0 (2017-11-02)


#### Others

*   setup .clog.toml ([d5d065d4](https://github.com/linuxdeepin/qt5integration/commit/d5d065d4099973a2d82aca0e16acd26c99621bd9))

#### Features

*   keep the window border is 1px for hiDPI ([da8040c4](https://github.com/linuxdeepin/qt5integration/commit/da8040c42df0cbde97e21693f472a288ef46a9d8))
*   QMenu hiDPI support ([a045b17e](https://github.com/linuxdeepin/qt5integration/commit/a045b17e86735779b48ea734bd18f2b2e25b4251))
* **style:**  override drawItemPixmap ([a580b9bf](https://github.com/linuxdeepin/qt5integration/commit/a580b9bf2024cc5e9564283f4880229bac1d780a))
* **theme plugin:**  read settings from config file ([aacc2995](https://github.com/linuxdeepin/qt5integration/commit/aacc299512f4006ff174c5700d471d344d7155d0))
* **theme style:**  auto update the widgets font when theme config file changed ([d478074e](https://github.com/linuxdeepin/qt5integration/commit/d478074e73d8e22e2d70080ad1430c565261d9af))

#### Bug Fixes

*   set cursor is invaild ([a2e235be](https://github.com/linuxdeepin/qt5integration/commit/a2e235bed7873d35030be380e8cd73acc0192e89))
*   menu can not hide on dde-dock ([ecae595e](https://github.com/linuxdeepin/qt5integration/commit/ecae595e29f1da24654c3128f13321c22412a7bd))
*   compatibility with Qt 5.7.1+ ([774ffb89](https://github.com/linuxdeepin/qt5integration/commit/774ffb89733232b8728b897b58db3eab6c7a6e05))
* **dxcb:**
  *  crash when screen changed ([7e1627c2](https://github.com/linuxdeepin/qt5integration/commit/7e1627c20fd92d7b4c1d7a69c55b4de5869cf6b6))
  *  the window border size is wrong ([83d6ac50](https://github.com/linuxdeepin/qt5integration/commit/83d6ac500b7e070f5c97ff220da16736229fd060))
  *  draw shadow area ([284cae9d](https://github.com/linuxdeepin/qt5integration/commit/284cae9dd90e9b9d7af92480e3a0e217a7c5f478))
  *  the DFrameWindow shadow area ([6e9c8274](https://github.com/linuxdeepin/qt5integration/commit/6e9c8274ef871fe3fa48a60a0861a96b6880dbf0))
  *  update the DFrameWindow's positionAutomatic value ([f6331ee3](https://github.com/linuxdeepin/qt5integration/commit/f6331ee32809d7e548b7d538beefc7ea5582f111))
  *  update the content window motif hints on propagateSizeHints/setVisible ([4e490995](https://github.com/linuxdeepin/qt5integration/commit/4e490995f5cfb564dbec705af114467c0ed374f0))
  *  the DFrameWindow's transientParent is self ([d5d93dba](https://github.com/linuxdeepin/qt5integration/commit/d5d93dbae8cdc21e887da17b8d710b0403f3b0aa))
* **style:**
  *  update all widgets on font changed ([0e28f244](https://github.com/linuxdeepin/qt5integration/commit/0e28f244217f57d454424cc7dfba488ee6be93e3))
  *  update all widgets on font changed ([d4a65deb](https://github.com/linuxdeepin/qt5integration/commit/d4a65deb9e21e18390447c8b26eb429843333be4))
* **theme:**
  *  "DFileSystemWatcherPrivate::addPaths: inotify_add_watch failed" ([a0322a68](https://github.com/linuxdeepin/qt5integration/commit/a0322a68447b7d918af49fbba23ebe1d5f53da25))
  *  set ini codec to "utf8" for QSettings ([3a4f7d76](https://github.com/linuxdeepin/qt5integration/commit/3a4f7d7651f6c433ef9735426d936289ac04d6c9))



