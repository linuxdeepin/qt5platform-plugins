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

#ifndef DSELECTEDTEXTTOOLTIP_H
#define DSELECTEDTEXTTOOLTIP_H

#include "global.h"

#include <QRasterWindow>
#include <QVector>

DPP_BEGIN_NAMESPACE

class DSelectedTextTooltip : public QRasterWindow
{
    Q_OBJECT

public:
    enum OptionType {
        None = 0,
        Cut = 1, //剪切
        Copy = 2, //复制
        Paste = 3, //粘贴
        SelectAll = 4 //全选
    };

    DSelectedTextTooltip();
    ~DSelectedTextTooltip() override;

protected:
    void paintEvent(QPaintEvent *pe) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onFontChanged();

signals:
    void optAction(OptionType type);

private:
    void updateColor();
    OptionType getOptionType(const QPoint &pos) const;

    struct OptionTextInfo {
        OptionType optType;
        int textWidth;
        QString optName;
    };

    QVector<OptionTextInfo> m_textInfoVec;

    QColor m_backgroundColor;
    QColor m_dividerColor;
    QColor m_borderColor;

};

DPP_END_NAMESPACE

#endif // DSELECTEDTEXTTOOLTIP_H
