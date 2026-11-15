/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "AlphaColourWell.h"

#include "uiDesign.h"
#include "utils.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFontInfo>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPushButton>

#include <algorithm>

namespace uiDesign {

namespace {

// What a plain well leaves above and below its words, so the two kinds of well
// come out one height - see mudlet::mTEXT_ON_BG_STYLESHEET. It is also the
// field left showing round the swatch, and between the swatch and the words.
constexpr int scmWellPaddingVertical = 3;
// ...and the swatch's corner, an input's corner taken in by that inset so the
// smaller square keeps the same curve beside the well's own
constexpr int scmSwatchRadius = std::max(2, scmRadiusInput - scmWellPaddingVertical);

// Black and white squares four pixels on a side. The picture's own squares are
// three times as tall as they are wide, as it was drawn to be squeezed into a
// 3:1 icon, so it is squeezed by the same amount here.
QPixmap checkerboardTile(const qreal ratio)
{
    QPixmap tile = QPixmap(qsl(":/icons/black_white_transparent_check_1x3_ratio.png")).scaled(QSize(48, 16) * ratio, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    tile.setDevicePixelRatio(ratio);
    return tile;
}

} // namespace

AlphaColourWell::AlphaColourWell(QPushButton* pButton)
: QObject(pButton)
, mpButton(pButton)
{
    pButton->installEventFilter(this);
}

AlphaColourWell* AlphaColourWell::on(QPushButton* pButton)
{
    if (AlphaColourWell* pWell = find(pButton)) {
        return pWell;
    }
    return new AlphaColourWell(pButton);
}

AlphaColourWell* AlphaColourWell::find(const QPushButton* pButton)
{
    if (!pButton) {
        return nullptr;
    }
    for (QObject* pChild : pButton->children()) {
        if (auto* pWell = dynamic_cast<AlphaColourWell*>(pChild)) {
            return pWell;
        }
    }
    return nullptr;
}

void AlphaColourWell::setColour(const QColor& colour)
{
    mColour = colour;
    applyLook();
    mStale = true;
    mpButton->update();
}

QRect AlphaColourWell::inside() const
{
    return mpButton->rect().adjusted(scmInputBorderWidth, scmInputBorderWidth, -scmInputBorderWidth, -scmInputBorderWidth);
}

void AlphaColourWell::applyLook()
{
    QString words;
    QString sheet;
    if (mColour.isValid()) {
        const auto opacity = [](const int percent) {
            // The number takes three places, so in a fixed-pitch face every well's words are one length and the
            // word as well as the percent sign stands under the one above it
            //: Words inside a map colour well in the settings, beside a swatch of the colour: how opaque the colour is, %1 being a whole percentage - 100 for a colour that is not see-through at all - padded with spaces to three characters
            return QCoreApplication::translate("AlphaColourWell", "Opacity %1%").arg(percent, 3);
        };
        words = opacity(qRound(mColour.alphaF() * 100));
        const ThemeTokens tokens = themeTokens();
        // The well's own face, the one a plain well is measured in, is the parent's
        const QFont plain = mpButton->parentWidget() ? mpButton->parentWidget()->font() : QApplication::font();
        // The words are set in the design's fixed-pitch face, a step down, so the digits are one width. A sheet
        // can carry only a family, so the face is resolved to the family it comes to here: a platform may name
        // it generically, which a sheet would not resolve to a fixed-pitch one.
        QFont fixed = fixedPitchFont(plain);
        fixed.setPointSize(typeSize(TypeStep::Caption));
        fixed.setFixedPitch(true);
        fixed.setStyleHint(QFont::Monospace);
        fixed.setFamily(QFontInfo(fixed).family());
        const QFontMetrics plainMetrics(plain);
        const QFontMetrics fixedMetrics(fixed);
        // The swatch is the inside of a plain well less the padding above and below it, and the words keep that
        // padding from it at least
        const int swatch = plainMetrics.height();
        // As wide as the widest words would need, whatever these are: the words stand at the trailing edge and
        // any room past that is left between them and the swatch. Measured the way a push button measures its
        // words, so the two agree to the pixel.
        const auto measured = [&fixedMetrics](const QString& text) {
            return fixedMetrics.size(Qt::TextShowMnemonic, text).width();
        };
        const int shortfall = measured(opacity(100)) - measured(words);
        // ...and as tall as a plain well whatever face the platform gives: the content is held to the plain face's
        // line, as a stylesheet measures a face's text in a way of its own
        const int top = scmWellPaddingVertical;
        const int bottom = scmWellPaddingVertical;
        sheet = qsl("QPushButton { color: %1; background-color: transparent; border: %2px solid %3; border-radius: %4px; font-family: \"%5\"; font-size: %6pt;"
                    " padding: %7px %8px %9px ")
                        .arg(tokens.text.name())
                        .arg(scmInputBorderWidth)
                        .arg(tokens.border.name())
                        .arg(scmRadiusInput)
                        .arg(fixed.family())
                        .arg(typeSize(TypeStep::Caption))
                        .arg(top)
                        .arg(scmInputPaddingHorizontal)
                        .arg(bottom)
                + qsl("%1px; min-height: %3px; max-height: %3px; text-align: right; } QPushButton:disabled { color: %2; }")
                          .arg(scmWellPaddingVertical + swatch + scmWellPaddingVertical + std::max(0, shortfall))
                          .arg(tokens.disabledText.name())
                          .arg(plainMetrics.height());
    }
    // Setting either brings the events that call this back again, so only what differs is set
    if (mpButton->text() != words) {
        mpButton->setText(words);
    }
    if (mpButton->styleSheet() != sheet) {
        mpButton->setStyleSheet(sheet);
    }
}

void AlphaColourWell::regenerate()
{
    mStale = false;
    mIcon = QIcon();
    const QSize size = inside().size();
    if (!mColour.isValid() || size.isEmpty()) {
        return;
    }

    const qreal ratio = mpButton->devicePixelRatioF();
    QPixmap picture(size * ratio);
    picture.setDevicePixelRatio(ratio);
    picture.fill(Qt::transparent);
    QPainter painter(&picture);
    painter.setRenderHint(QPainter::Antialiasing);
    const ThemeTokens tokens = themeTokens();
    const bool available = mpButton->isEnabled();
    // The field, cut to the corner inside the hairline
    const qreal innerRadius = scmRadiusInput - scmInputBorderWidth;
    QPainterPath field;
    field.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), innerRadius, innerRadius);
    painter.fillPath(field, tokens.field);
    // The swatch, square, with the field showing round it
    const int side = size.height() - 2 * scmWellPaddingVertical;
    if (side > 0) {
        const QRect swatch(scmWellPaddingVertical, scmWellPaddingVertical, side, side);
        QPainterPath swatchCorner;
        swatchCorner.addRoundedRect(QRectF(swatch), scmSwatchRadius, scmSwatchRadius);
        painter.setClipPath(swatchCorner);
        painter.drawTiledPixmap(swatch, checkerboardTile(ratio));
        // Unavailable, the colour keeps its hue and loses most of its saturation
        const QColor shown = available ? mColour : QColor::fromHsl(mColour.hslHue(), mColour.hslSaturation() / 4, mColour.lightness(), mColour.alpha());
        painter.fillRect(swatch, shown);
        painter.setClipping(false);
        // A hairline over the colour, so a swatch as light as the field still has an edge
        painter.setPen(QPen(available ? tokens.border : disabledBorderColour(tokens), scmInputBorderWidth));
        painter.setBrush(Qt::NoBrush);
        const qreal half = scmInputBorderWidth / 2.0;
        painter.drawRoundedRect(QRectF(swatch).adjusted(half, half, -half, -half), scmSwatchRadius - half, scmSwatchRadius - half);
    }
    painter.end();
    // The unavailable picture has to be given to the icon as the one for its
    // Disabled mode, explicitly: given as the normal one, the built-in icon
    // engine would assume it is for the normal state and grey it out
    // completely by automagic means instead of showing the partial
    // (desaturating) effect drawn above
    mIcon.addPixmap(picture, available ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
}

bool AlphaColourWell::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched != mpButton) {
        return QObject::eventFilter(pWatched, pEvent);
    }
    switch (pEvent->type()) {
    case QEvent::FontChange:
    case QEvent::ApplicationFontChange:
    case QEvent::LanguageChange:
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
        if (mColour.isValid()) {
            applyLook();
        }
        mStale = true;
        break;
    case QEvent::Resize:
    case QEvent::Show:
    case QEvent::EnabledChange:
    case QEvent::StyleChange:
    case QEvent::DevicePixelRatioChange:
        mStale = true;
        break;
    case QEvent::Paint: {
        if (mStale) {
            regenerate();
        }
        if (!mIcon.isNull()) {
            // Before the button paints: its words and its hairline are then
            // drawn over this
            QPainter painter(mpButton);
            mIcon.paint(&painter, inside(), Qt::AlignCenter, mpButton->isEnabled() ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
        }
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(pWatched, pEvent);
}

} // namespace uiDesign
