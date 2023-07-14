// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>
#include <QWindow>

#include "dxcbxsettings.h"
#include "dplatformintegration.h"
#include <QDebug>
DPP_USE_NAMESPACE

class TDXcbXSettings : public testing::Test
{
protected:
    void SetUp();
    void TearDown();

    QWindow *window;
    DXcbXSettings *settings = nullptr;
};

void TDXcbXSettings::SetUp()
{
    window = new QWindow;
    settings = DPlatformIntegration::instance()->xSettings();
}

void TDXcbXSettings::TearDown()
{
    delete window;
}

TEST_F(TDXcbXSettings, getOwner)
{
    xcb_window_t id = DXcbXSettings::getOwner();
    ASSERT_NE(id, 0);
}

TEST_F(TDXcbXSettings, initialized)
{
    bool isInitialized = settings->initialized();
    ASSERT_TRUE(isInitialized);
}

TEST_F(TDXcbXSettings, isEmpty)
{
    bool isEmpty = settings->isEmpty();
    ASSERT_FALSE(isEmpty);
}

TEST_F(TDXcbXSettings, contains)
{
    bool isContained = settings->contains("Net/CursorBlink");
    ASSERT_TRUE(isContained);
}
