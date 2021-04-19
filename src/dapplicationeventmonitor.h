/*
 * Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
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

#ifndef DAPPLICATIONEVENTMONITOR_H
#define DAPPLICATIONEVENTMONITOR_H

#include "global.h"

#include <QObject>
#include <QEvent>

DPP_BEGIN_NAMESPACE

class DApplicationEventMonitor : public QObject
{
    Q_OBJECT

public:
    enum InputDeviceType {
        None = 0, // 初始状态
        Mouse = 1, //鼠标
        Tablet = 2, //平板外围设备
        Keyboard = 3, //键盘
        TouchScreen = 4 //触摸屏
    };

    DApplicationEventMonitor(QObject *parent = nullptr);
    ~DApplicationEventMonitor() override;

    InputDeviceType lastInputDeviceType() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void lastInputDeviceTypeChanged();

private:
    InputDeviceType m_lastInputDeviceType;
};

DPP_END_NAMESPACE

#endif // DAPPLICATIONEVENTMONITOR_H
