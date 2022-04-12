/*
   * Copyright (C) 2022 Uniontech Software Technology Co.,Ltd.
   *
   * Author:     chenke <chenke@uniontech.com>
   *
   * Maintainer: chenke <chenke@uniontech.com>
   *
   * This program is free software: you can redistribute it and/or modify
   * it under the terms of the GNU Lesser General Public License as published by
   * the Free Software Foundation, either version 3 of the License, or
   * any later version.
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU Lesser General Public License
   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */
#ifndef DKEYBOARD_H
#define DKEYBOARD_H

#include <QObject>
#include <KWayland/Client/ddekeyboard.h>
using namespace KWayland::Client;

class DKeyboard : public QObject
{
    Q_OBJECT
public:
    explicit DKeyboard(QObject *parent = nullptr);
    virtual ~DKeyboard();

public slots:
    void handleKeyEvent(quint32 key, DDEKeyboard::KeyState state, quint32 time);
    void handleModifiersChanged(quint32 depressed, quint32 latched, quint32 locked, quint32 group);

signals:

public slots:
};

#endif // DKEYBOARD_H
