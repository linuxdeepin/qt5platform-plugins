/*
 * Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dplatformsettings.h"

DPP_BEGIN_NAMESPACE

void DPlatformSettings::registerCallback(DPlatformSettings::PropertyChangeFunc func, void *handle)
{
    Callback callback = { func, handle };
    callback_links.push_back(callback);
}

void DPlatformSettings::removeCallbackForHandle(void *handle)
{
    auto isCallbackForHandle = [handle](const Callback &cb) { return cb.handle == handle; };
    callback_links.erase(std::remove_if(callback_links.begin(), callback_links.end(), isCallbackForHandle));
}

void DPlatformSettings::registerSignalCallback(DPlatformSettings::SignalFunc func, void *handle)
{
    SignalCallback callback = { func, handle };
    signal_callback_links.push_back(callback);
}

void DPlatformSettings::removeSignalCallback(void *handle)
{
    auto isCallbackForHandle = [handle](const SignalCallback &cb) { return cb.handle == handle; };
    signal_callback_links.erase(std::remove_if(signal_callback_links.begin(), signal_callback_links.end(), isCallbackForHandle));
}

void DPlatformSettings::handlePropertyChanged(const QByteArray &property, const QVariant &value)
{
    for (auto callback : callback_links) {
        callback.func(property, value, callback.handle);
    }
}

void DPlatformSettings::handleNotify(const QByteArray &signal, qint32 data1, qint32 data2)
{
    for (auto callback : signal_callback_links) {
        callback.func(signal, data1, data2, callback.handle);
    }
}

DPP_END_NAMESPACE
