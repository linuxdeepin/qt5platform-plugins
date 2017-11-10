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



