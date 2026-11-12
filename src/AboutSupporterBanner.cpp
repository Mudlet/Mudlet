/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "AboutSupporterBanner.h"

#include "utils.h"

#include <algorithm>

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int scmPointWidth = 24;
constexpr int scmSwordsHeight = 58;
constexpr int scmPlaqueHeight = 50;
constexpr int scmMaximumWidth = 560;
constexpr int scmGlyphInset = 34;
constexpr int scmGlyphSize = 22;
constexpr qreal scmSwordsNameScale = 1.45;
constexpr qreal scmPlaqueNameScale = 1.25;
} // namespace

namespace uiDesign {

AboutSupporterBanner::AboutSupporterBanner(const QString& name, const bool swords, QWidget* pParent)
: QWidget(pParent)
, mName(name)
, mSwords(swords)
{
    setObjectName(qsl("aboutBanner"));
    setAccessibleName(name);
    // A centring layout gives a widget its size hint rather than stretching it,
    // and a plain QWidget hints at nothing, so the width is fixed outright
    setFixedHeight(swords ? scmSwordsHeight : scmPlaqueHeight);
    setFixedWidth(scmMaximumWidth);
}

void AboutSupporterBanner::applyTokens(const uiDesign::ThemeTokens& tokens)
{
    mTokens = tokens;
    mGlyph = QPixmap();
    if (mSwords) {
        // Scaled before inking: downsampling the 128px raster in paintEvent() is
        // a nearest-neighbour blit, repeated on every repaint
        const qreal ratio = devicePixelRatioF();
        QPixmap blade = uiDesign::glyphPixmap(qsl(":/icons/about-swords.svg")).scaled(qRound(scmGlyphSize * ratio), qRound(scmGlyphSize * ratio), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        blade.setDevicePixelRatio(ratio);
        mGlyph = uiDesign::tintedGlyph(blade, tokens.accentText);
    }
    update();
}

void AboutSupporterBanner::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED(pEvent)
    if (!mTokens.card.isValid()) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Half a pen in from the widget's edge, or the stroke is drawn half off it
    const qreal inset = 0.5;
    const qreal left = inset;
    const qreal top = inset;
    const qreal right = width() - inset;
    const qreal bottom = height() - inset;
    const qreal middle = height() / 2.0;

    QPainterPath pennant;
    pennant.moveTo(left, middle);
    pennant.lineTo(left + scmPointWidth, top);
    pennant.lineTo(right - scmPointWidth, top);
    pennant.lineTo(right, middle);
    pennant.lineTo(right - scmPointWidth, bottom);
    pennant.lineTo(left + scmPointWidth, bottom);
    pennant.closeSubpath();

    painter.setPen(QPen(mTokens.border, 1.0));
    painter.setBrush(mTokens.card);
    painter.drawPath(pennant);

    int wordsLeft = scmGlyphInset;
    int wordsRight = width() - scmGlyphInset;
    if (!mGlyph.isNull()) {
        const int glyphTop = (height() - scmGlyphSize) / 2;
        // Mirrored so the pair reads as crossed, not as the same picture twice
        painter.save();
        painter.translate(scmGlyphInset + scmGlyphSize, glyphTop);
        painter.scale(-1.0, 1.0);
        painter.drawPixmap(QPoint(0, 0), mGlyph);
        painter.restore();
        painter.drawPixmap(QPoint(width() - scmGlyphInset - scmGlyphSize, glyphTop), mGlyph);
        wordsLeft = scmGlyphInset + scmGlyphSize + scmGlyphSize / 2;
        wordsRight = width() - wordsLeft;
    }

    QFont nameFont = font();
    nameFont.setBold(true);
    nameFont.setPointSizeF(font().pointSizeF() * (mSwords ? scmSwordsNameScale : scmPlaqueNameScale));
    painter.setFont(nameFont);
    painter.setPen(mTokens.text);
    const QRect words(wordsLeft, 0, std::max(0, wordsRight - wordsLeft), height());
    painter.drawText(words, Qt::AlignCenter, QFontMetrics(nameFont).elidedText(mName, Qt::ElideRight, words.width()));
}

} // namespace uiDesign
