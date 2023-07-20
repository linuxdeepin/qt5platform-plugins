// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>
#include <QWindow>

#include "global.h"


class TGlobal : public testing::Test
{
protected:
    void SetUp();
    void TearDown();

    QWindow *window = nullptr;
};

void TGlobal::SetUp()
{
    window = new QWindow;
}

void TGlobal::TearDown()
{
    delete window;
}

TEST_F(TGlobal, fromQtWinId)
{
    QWindow *w = fromQtWinId(window->winId());
    ASSERT_EQ(w, window);
}


