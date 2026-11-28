/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TTextSelection.h"

#include "Host.h"
#include "TBuffer.h"

#include <QStringList>

#include <algorithm>

TTextSelection::TTextSelection(TBuffer& buffer, Host* pHost)
: mBuffer(buffer)
, mpHost(pHost)
{
}

void TTextSelection::normalise()
{
    if (mDragStart.y() < mDragSelectionEnd.y() || ((mDragStart.y() == mDragSelectionEnd.y()) && (mDragStart.x() < mDragSelectionEnd.x()))) {
        mPA = mDragStart;
        mPB = mDragSelectionEnd;
    } else {
        mPA = mDragSelectionEnd;
        mPB = mDragStart;
    }
}

void TTextSelection::expandToWords()
{
    int yind = mPA.y();
    int xind = mPA.x();

    if (yind >= 0 && yind < static_cast<int>(mBuffer.lineBuffer.size())) {
        for (; xind >= 0; --xind) {
            if (xind >= static_cast<int>(mBuffer.lineBuffer.at(yind).size())) {
                break;
            }
            const QChar currentChar = mBuffer.lineBuffer.at(yind).at(xind);
            if (currentChar == QChar::Space || mpHost->mDoubleClickIgnore.contains(currentChar)) {
                break;
            }
        }
    }
    mPA.setX(xind + 1);

    yind = mPB.y();
    xind = mPB.x();

    if (yind >= 0 && yind < static_cast<int>(mBuffer.lineBuffer.size())) {
        for (; xind < static_cast<int>(mBuffer.lineBuffer.at(yind).size()); ++xind) {
            const QChar currentChar = mBuffer.lineBuffer.at(yind).at(xind);
            if (currentChar == QChar::Space || mpHost->mDoubleClickIgnore.contains(currentChar)) {
                break;
            }
        }
    }
    mPB.setX(xind - 1);
}

void TTextSelection::anchorEndToNearestWord(const bool anchorFirst)
{
    QPoint& anchoredEnd = anchorFirst ? mPA : mPB;
    const QString& anchoredLine = mBuffer.lineBuffer.at(anchoredEnd.y());
    const int step = anchorFirst ? 1 : -1;
    while (anchoredEnd.x() >= 0 && anchoredEnd.x() < static_cast<int>(anchoredLine.size())
           && (anchoredLine.at(anchoredEnd.x()) == QChar::Space || mpHost->mDoubleClickIgnore.contains(anchoredLine.at(anchoredEnd.x())))) {
        anchoredEnd.rx() += step;
    }
}

void TTextSelection::selectAll()
{
    mDragStart = QPoint(0, 0);
    mDragSelectionEnd = mBuffer.getEndPos();
    normalise();
}

void TTextSelection::markLines()
{
    for (int y = std::max(0, mPA.y()), endY = std::min((mPB.y() + 1), static_cast<int>(mBuffer.buffer.size())); y < endY; ++y) {
        TLineFormat& line = mBuffer.buffer.at(y);
        const int lineSize = static_cast<int>(line.size());
        const int startX = (y == mPA.y()) ? std::clamp(mPA.x(), 0, lineSize) : 0;
        const int endX = (y == mPB.y()) ? std::min(mPB.x() + 1, lineSize) : lineSize;
        if (startX < endX) {
            line.select(startX, endX);
        }
    }
}

void TTextSelection::unmarkLines()
{
    for (int yIndex = mPA.y(), total = mPB.y(); yIndex <= total; ++yIndex) {
        if (yIndex >= static_cast<int>(mBuffer.buffer.size()) || yIndex < 0) {
            break;
        }
        mBuffer.buffer.at(yIndex).deselect();
    }
    mPBTakingNewline = QPoint(-1, -1);
}

bool TTextSelection::hasSelection() const
{
    // A double-click on a word separator highlights nothing, yet leaves a
    // region behind and mPA past mPB
    const bool endpointsCrossed = mPA.y() > mPB.y() || (mPA.y() == mPB.y() && mPA.x() > mPB.x());
    return !mBuffer.lineBuffer.isEmpty() && !mSelectedRegion.isEmpty() && !endpointsCrossed;
}

QString TTextSelection::text(const QChar newline, const bool showTimestamps)
{
    // try to prevent crash if buffer is batch deleted
    if (const int trimmed = linesTrimmed()) {
        const bool takesNewline = mPB == mPBTakingNewline;
        mPA.ry() -= trimmed;
        mPB.ry() -= trimmed;
        if (takesNewline) {
            mPBTakingNewline = mPB;
        }
    }
    qsizetype startLine = std::max(0, mPA.y());
    qsizetype endLine = std::min<qsizetype>(mPB.y(), (mBuffer.lineBuffer.size() - 1));
    // The batch-delete step above can push a selection left on lines a clearWindow() removed off the top
    if (endLine < startLine) {
        return {};
    }
    qsizetype offset = endLine - startLine;
    qsizetype startPos = std::max(0, mPA.x());
    qsizetype endPos = std::min<qsizetype>(mPB.x(), (mBuffer.lineBuffer.at(endLine).size() - 1));
    QStringList textLines = mBuffer.lineBuffer.mid(startLine, endLine - startLine + 1);
    if (textLines.isEmpty()) {
        return {};
    }

    if (mPA.y() == mPB.y()) {
        // Is a single line, so trim characters off the beginning and end
        // according to startPos and endPos:
        if (!textLines.at(0).isEmpty()) {
            textLines[0] = textLines.at(0).mid(startPos, endPos - startPos + 1);
        }
    } else {
        // trim characters off the front of the first line according to startPos:
        if (!textLines.at(0).isEmpty()) {
            textLines[0] = textLines.at(0).mid(startPos);
        }
        // and chop off the required number of QChars from the end of the last
        // line:
        if (!textLines.at(offset).isEmpty()) {
            textLines[offset] = textLines.at(offset).left(1 + endPos);
        }
    }

    // A line that word wrapping split off goes back on the end of the one
    // before, as the game sent it, rather than being copied as a line of its own
    QString result;
    for (qsizetype i = 0; i < textLines.size(); ++i) {
        const int y = static_cast<int>(startLine + i);
        QStringView line = textLines.at(i);
        if (mBuffer.wrapsFromPreviousLine(y)) {
            const qsizetype columnsNotSelected = (i == 0) ? startPos : 0;
            line = line.sliced(std::clamp<qsizetype>(mBuffer.wrapIndentWidth(y) - columnsNotSelected, 0, line.size()));
            if (i > 0) {
                result.append(QString(mBuffer.wrapGapBefore(y), QChar::Space));
            }
        } else {
            if (i > 0) {
                result.append(newline);
            }
            if (showTimestamps) {
                result.append(mBuffer.timeStampText(y));
            }
        }
        result.append(line);
    }
    if (mPB == mPBTakingNewline && !mBuffer.wrapsFromPreviousLine(static_cast<int>(endLine) + 1)) {
        result.append(newline);
    }
    return result;
}

bool TTextSelection::appendHtml(QString& html, const bool showTimestamps) const
{
    // Is this a single line then we do NOT need to pad the first (and thus
    // only) line to the right:
    const bool isSingleLine = (mPA.y() == mPB.y());
    for (int y = mPA.y(), total = mPB.y(); y <= total; ++y) {
        if (y >= static_cast<int>(mBuffer.buffer.size())) {
            return false;
        }
        if (y == mPA.y()) {
            if (isSingleLine) {
                html.append(mBuffer.bufferToHtml(showTimestamps, y, mPB.x() + 1, mPA.x(), 0));
            } else {
                html.append(mBuffer.bufferToHtml(showTimestamps, y, -1, mPA.x(), mPA.x()));
            }
        } else if (y == mPB.y()) {
            html.append(mBuffer.bufferToHtml(showTimestamps, y, mPB.x() + 1));
        } else {
            html.append(mBuffer.bufferToHtml(showTimestamps, y));
        }
    }
    return true;
}

int TTextSelection::linesTrimmed() const
{
    const qsizetype lastLine = mBuffer.lineBuffer.size() - 1;
    return (mPA.y() > lastLine || mPB.y() > lastLine) ? mBuffer.mBatchDeleteSize : 0;
}

// The characters of the accessible text the selection covers, 0 for none: a double-click on a
// space or a triple-click on an empty line leaves a region but no cells. mPB is the last selected
// cell, and a whole-line selection leaves it on the newline cell (x == line length), which counts
// only when mPBTakingNewline says so. Walks just the selected lines, so it stays cheap however
// long the buffer is.
int TTextSelection::accessibleLength() const
{
    if (mSelectedRegion.isEmpty()) {
        return 0;
    }
    const int trimmed = linesTrimmed();
    const int firstLine = mPA.y() - trimmed;
    const int lastLine = mPB.y() - trimmed;
    const int lineCount = static_cast<int>(std::min(mBuffer.lineBuffer.size(), static_cast<qsizetype>(mBuffer.buffer.size())));
    if (firstLine < 0 || firstLine > lastLine || lastLine >= lineCount) {
        return 0;
    }
    const bool takesNewline = mPB == mPBTakingNewline;
    // the text has no newline after its last line
    if (takesNewline && lastLine == lineCount - 1) {
        return 0;
    }
    const int lastLineLength = static_cast<int>(mBuffer.buffer.at(lastLine).size());
    int length = (takesNewline ? lastLineLength + 1 : std::min(mPB.x() + 1, lastLineLength)) - mPA.x();
    for (int y = firstLine; y < lastLine; ++y) {
        length += mBuffer.line(y).length() + 1;
    }
    return std::max(length, 0);
}

std::pair<int, int> TTextSelection::accessibleRange() const
{
    const int length = accessibleLength();
    if (length == 0) {
        return {0, 0};
    }
    const int start = offsetOf(mPA.y() - linesTrimmed(), mPA.x());
    return {start, start + length};
}

int TTextSelection::offsetOf(const int line, const int column) const
{
    int offset = 0;
    for (int i = 0; i < line; i++) {
        offset += mBuffer.line(i).length() + 1;
    }
    return offset + column;
}
