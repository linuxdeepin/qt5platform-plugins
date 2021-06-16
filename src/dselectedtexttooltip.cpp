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

#include "dselectedtexttooltip.h"

#include <QWindow>
#include <QPainter>
#include <QPaintEvent>
#include <QGuiApplication>
#include <QFontMetrics>

#define TEXT_SPACING 5

DPP_BEGIN_NAMESPACE

DSelectedTextTooltip::DSelectedTextTooltip()
    : QRasterWindow()
{
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);

    // 这里借用QLineEdit的翻译
    m_textInfoVec.push_back({Cut, 0, qApp->translate("QLineEdit", "Cu&t").split("(").at(0)});
    m_textInfoVec.push_back({Copy, 0, qApp->translate("QLineEdit", "&Copy").split("(").at(0)});
    m_textInfoVec.push_back({Paste, 0, qApp->translate("QLineEdit", "&Paste").split("(").at(0)});
    m_textInfoVec.push_back({SelectAll, 0, qApp->translate("QLineEdit", "Select All")});

    connect(qApp, &QGuiApplication::fontChanged, this, &DSelectedTextTooltip::onFontChanged);

    // 更新文本信息
    onFontChanged();
}

DSelectedTextTooltip::~DSelectedTextTooltip()
{
}

void DSelectedTextTooltip::onFontChanged()
{
    QFontMetrics font_metrics(qApp->font());
    int tooltip_width = 0;
    for (auto &font_info : m_textInfoVec) {
        int tmp_width = font_metrics.width(font_info.optName);
        font_info.textWidth = tmp_width + TEXT_SPACING;
        tooltip_width += font_info.textWidth;
    }

    resize(tooltip_width, font_metrics.height() + 2 * TEXT_SPACING);
}

DSelectedTextTooltip::OptionType DSelectedTextTooltip::getOptionType(const QPoint &pos) const
{
    int tmp_width = 0;
    for (const auto &info : m_textInfoVec) {
        tmp_width += info.textWidth;
        if (pos.x() < tmp_width) {
            return info.optType;
        }
    }

    return None;
}

void DSelectedTextTooltip::mousePressEvent(QMouseEvent *event)
{
    Q_EMIT optAction(getOptionType(event->pos()));
}

void DSelectedTextTooltip::paintEvent(QPaintEvent *pe)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::gray);
    painter.drawRoundedRect(pe->rect(), 5, 5);

    painter.setFont(qApp->font());
    painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));

    int pos_x = 0;
    for (int index = 0; index < m_textInfoVec.size(); ++index) {
        QRect text_rect(pos_x, 0, m_textInfoVec[index].textWidth, pe->rect().height());
        pos_x += m_textInfoVec[index].textWidth;
        painter.drawText(text_rect, Qt::AlignCenter, m_textInfoVec[index].optName);
    }
}

DPP_END_NAMESPACE
