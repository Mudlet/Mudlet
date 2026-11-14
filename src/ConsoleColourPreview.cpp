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

#include "ConsoleColourPreview.h"

#include "Host.h"
#include "uiDesign.h"
#include "utils.h"

#include <QEvent>
#include <QFontInfo>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

namespace uiDesign {

namespace {

constexpr int scmPadding = 6;
constexpr int scmCommandLinePadding = 3;
// Below this the words are no longer words, so the font stops shrinking and the lines are clipped instead
constexpr int scmSmallestPixelSize = 6;

struct Run
{
    QString text;
    QColor ink;
    // Invalid where the run is drawn straight on the console's background
    QColor ground;
};
using Line = QList<Run>;

// A translated sentence with %1, %2... standing for runs in colours of their own, so a language can put them in its own
// order
Line runsOf(const QString& sentence, const QList<Run>& placed, const QColor& ink)
{
    Line runs;
    QString plain;
    for (qsizetype at = 0; at < sentence.size(); ++at) {
        if (sentence.at(at) == QLatin1Char('%') && at + 1 < sentence.size() && sentence.at(at + 1).isDigit()) {
            const int index = sentence.at(at + 1).digitValue() - 1;
            if (index >= 0 && index < placed.size()) {
                if (!plain.isEmpty()) {
                    runs.append({plain, ink, QColor()});
                    plain.clear();
                }
                runs.append(placed.at(index));
                ++at;
                continue;
            }
        }
        plain.append(sentence.at(at));
    }
    if (!plain.isEmpty()) {
        runs.append({plain, ink, QColor()});
    }
    return runs;
}

int advanceOf(const Line& line, const QFontMetrics& metrics)
{
    int advance = 0;
    for (const Run& run : line) {
        advance += metrics.horizontalAdvance(run.text);
    }
    return advance;
}

} // namespace

ConsoleColourPreview::ConsoleColourPreview(QWidget* pParent)
: QWidget(pParent)
{
    // As tall as whatever stands beside it and never taller, and given whatever width is left over
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    setFocusPolicy(Qt::NoFocus);
    retranslate();
}

void ConsoleColourPreview::setHost(Host* pHost)
{
    mpHost = pHost;
    update();
}

QSize ConsoleColourPreview::sizeHint() const
{
    return {0, 0};
}

QSize ConsoleColourPreview::minimumSizeHint() const
{
    return {0, 0};
}

void ConsoleColourPreview::retranslate()
{
    //: Screen reader name of the picture beside the sixteen colors on the Main display settings page, showing some game text drawn in the colors chosen there
    setAccessibleName(tr("Preview of the main display colors"));
}

void ConsoleColourPreview::changeEvent(QEvent* pEvent)
{
    QWidget::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange) {
        retranslate();
        update();
    }
}

void ConsoleColourPreview::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED(pEvent)
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    const Host& host = *pHost;

    //: Sample game text in the colors preview on the Main display settings page: a command the player typed, echoed in the command colors
    const Run echo{tr("look"), host.mCommandFgColor, host.mCommandBgColor};
    QList<Line> lines;
    lines.append({echo});
    //: Sample game text in the colors preview on the Main display settings page: the name of a room
    lines.append({{tr("Merchant's Crossing"), host.mFgColor, QColor()}});
    //: Sample game text in the colors preview on the Main display settings page: a room's description
    lines.append({{tr("A busy square where four roads meet."), host.mFgColor, QColor()}});
    lines.append(runsOf(
            //: Sample game text in the colors preview on the Main display settings page: the room's exits, %1 to %4 being the four directions below, drawn in the light cyan color
            tr("Obvious exits: %1, %2, %3, %4."),
            //: Sample game text in the colors preview on the Main display settings page: one of the four exits of the room
            {{tr("north"), host.mLightCyan, QColor()},
             //: Sample game text in the colors preview on the Main display settings page: one of the four exits of the room
             {tr("east"), host.mLightCyan, QColor()},
             //: Sample game text in the colors preview on the Main display settings page: one of the four exits of the room
             {tr("south"), host.mLightCyan, QColor()},
             //: Sample game text in the colors preview on the Main display settings page: one of the four exits of the room
             {tr("west"), host.mLightCyan, QColor()}},
            host.mFgColor));
    lines.append(runsOf(
            //: Sample game text in the colors preview on the Main display settings page: someone speaking, %1 being who (drawn in light yellow) and %2 what they say (drawn in light green)
            tr("%1 says, %2"),
            //: Sample game text in the colors preview on the Main display settings page: the one speaking
            {{tr("A merchant"), host.mLightYellow, QColor()},
             //: Sample game text in the colors preview on the Main display settings page: what the merchant says, with the quotation marks of your language
             {tr("\"Fresh bread!\""), host.mLightGreen, QColor()}},
            host.mFgColor));

    const QList<std::pair<QString, std::pair<QColor, QColor>>> colourNames{//: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("black"), {host.mBlack, host.mLightBlack}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("red"), {host.mRed, host.mLightRed}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("green"), {host.mGreen, host.mLightGreen}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("yellow"), {host.mYellow, host.mLightYellow}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("blue"), {host.mBlue, host.mLightBlue}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("magenta"), {host.mMagenta, host.mLightMagenta}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("cyan"), {host.mCyan, host.mLightCyan}},
                                                                           //: Sample text in the colors preview on the Main display settings page: the name of a color, drawn in that color
                                                                           {tr("white"), {host.mWhite, host.mLightWhite}}};
    Line normalNames;
    Line lightNames;
    for (const auto& [name, inks] : colourNames) {
        if (!normalNames.isEmpty()) {
            normalNames.append({qsl(" "), host.mFgColor, QColor()});
            lightNames.append({qsl(" "), host.mFgColor, QColor()});
        }
        normalNames.append({name, inks.first, QColor()});
        lightNames.append({name, inks.second, QColor()});
    }
    lines.append(normalNames);
    lines.append(lightNames);

    //: Sample game text in the colors preview on the Main display settings page: a command being typed on the command line
    const QString typed = tr("score");

    // The profile's own face, never larger than it is set, and smaller until every line fits across and all of them down
    QFont font = pHost->getDisplayFont();
    int pixelSize = QFontInfo(font).pixelSize();
    for (; pixelSize > scmSmallestPixelSize; --pixelSize) {
        font.setPixelSize(pixelSize);
        const QFontMetrics metrics(font);
        int widest = metrics.horizontalAdvance(typed);
        for (const Line& line : std::as_const(lines)) {
            widest = std::max(widest, advanceOf(line, metrics));
        }
        const int tall = 2 * scmPadding + lines.size() * metrics.lineSpacing() + 1 + 2 * scmCommandLinePadding + metrics.height();
        if (widest + 2 * scmPadding <= width() && tall <= height()) {
            break;
        }
    }
    font.setPixelSize(pixelSize);
    const QFontMetrics metrics(font);

    const ThemeTokens tokens = themeTokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(font);
    const qreal half = scmInputBorderWidth / 2.0;
    const QRectF frame = QRectF(rect()).adjusted(half, half, -half, -half);
    QPainterPath outline;
    outline.addRoundedRect(frame, scmRadiusInput, scmRadiusInput);
    painter.fillPath(outline, host.mBgColor);
    painter.setClipPath(outline);

    // The command line along the foot, under the seam the console draws there in its own colours
    const int commandLineHeight = metrics.height() + 2 * scmCommandLinePadding;
    const QRect commandLine(0, height() - commandLineHeight, width(), commandLineHeight);
    painter.fillRect(commandLine, host.mCommandLineBgColor);
    painter.fillRect(QRect(0, commandLine.top() - 1, width(), 1), blend(host.mCommandLineBgColor, host.mCommandLineFgColor, scmConsoleFootSeamStrength));
    painter.setPen(host.mCommandLineFgColor);
    const int commandBaseline = commandLine.top() + scmCommandLinePadding + metrics.ascent();
    painter.drawText(scmPadding, commandBaseline, typed);
    const int caretX = scmPadding + metrics.horizontalAdvance(typed) + 1;
    painter.fillRect(caretX, commandLine.top() + scmCommandLinePadding, 1, metrics.height(), host.mCommandLineFgColor);

    int top = scmPadding;
    for (const Line& line : std::as_const(lines)) {
        int x = scmPadding;
        for (const Run& run : line) {
            const int advance = metrics.horizontalAdvance(run.text);
            if (run.ground.isValid()) {
                painter.fillRect(x, top, advance, metrics.lineSpacing(), run.ground);
            }
            painter.setPen(run.ink);
            painter.drawText(x, top + metrics.ascent(), run.text);
            x += advance;
        }
        top += metrics.lineSpacing();
    }

    painter.setClipping(false);
    painter.setPen(QPen(tokens.border, scmInputBorderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(outline);
}

} // namespace uiDesign
