/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2022 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "TConsoleModel.h"

#include "Host.h"
#include "MudletApp.h"
#include "TDebug.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFontInfo>
#include <QTimer>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <utility>

namespace {
double relativeLuminance(const QColor& color)
{
    const auto channel = [](const double value) {
        return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}

// Plain blue is barely legible against the dark background most profiles use,
// so whichever of the two link blues stands out more against this console wins:
TChar standardLinkFormat(const QColor& background)
{
    const QColor lightBlue(80, 160, 255);
    const QColor linkColor = TConsoleModel::contrastRatio(QColor(Qt::blue), background) >= TConsoleModel::contrastRatio(lightBlue, background) ? QColor(Qt::blue) : lightBlue;
    return TChar(linkColor, background, TChar::Underline);
}
} // namespace

TConsoleModel::TConsoleModel(Host* pHost)
: buffer(pHost)
, mpHost(pHost)
, mProfileName(pHost ? pHost->getName() : qsl("debug console"))
, mHyperlinkVisibilityManager(*this)
{
    // Not in the buffer's constructor: the buffer is built before the managers
    // it would then be pointed at, and reaches for them while it is.
    buffer.mpModel = this;
}

QStringList TConsoleModel::lines(int from, int to) const
{
    QStringList ret;
    // 64-bit, as from - to overflows an int for a script's extreme arguments
    const qint64 first = from;
    const qint64 end = std::min(first + qAbs(first - to), static_cast<qint64>(buffer.lineBuffer.size()));
    for (qint64 i = std::max<qint64>(first, 0); i < end; ++i) {
        ret << buffer.lineBuffer.at(i);
    }
    return ret;
}

bool TConsoleModel::moveCursor(int x, int y)
{
    QPoint P(x, y);
    if (buffer.moveCursor(P)) {
        mUserCursor = P;
        return true;
    }
    return false;
}

void TConsoleModel::moveCursorEnd()
{
    const int y = buffer.getLastLineNumber();
    int x = buffer.line(y).size() - 1;
    x = x >= 0 ? x : 0;
    moveCursor(x, y);
}

void TConsoleModel::deleteLineAtCursor()
{
    const int deletedLine = mUserCursor.y();
    if (!buffer.deleteLine(deletedLine)) {
        return;
    }
    forgetHeldMirrorTextOnLine(deletedLine);
    // The selection is held by line number, so it moves up with its line or goes with it
    if (P_begin.y() == deletedLine) {
        deselect();
    } else if (P_begin.y() > deletedLine) {
        P_begin.ry()--;
        P_end.ry()--;
    }
}

void TConsoleModel::clear()
{
    buffer.clear();
    // --mirror's pending line went with the buffer.
    mMirrorPendingLine.clear();
    mMirrorTriggerRuns.clear();
    mMirrorTriggerWriteEnd = QPoint(-1, -1);
    mUserCursor = QPoint();
    // Past the line left, so isPrompt() sees that the line a trigger runs for went too, even when it was line 0
    if (mTriggerEngineMode) {
        mEngineCursor = buffer.size();
    }
}

void TConsoleModel::deselect()
{
    P_begin = QPoint();
    P_end = QPoint();
}

bool TConsoleModel::selectSection(int from, int to)
{
    if (TDebug::wants(TDebug::Category::Selection)) {
        TDebug(Qt::darkMagenta, Qt::black, TDebug::Category::Selection) << "selectSection(" << from << "," << to << "): line under current user cursor: " << buffer.line(mUserCursor.y()) << "\n"
                >> mpHost;
    }
    if (from < 0) {
        return false;
    }
    // a negative length would put the selection's end before its start
    if (to < 0) {
        return false;
    }
    if (mUserCursor.y() >= static_cast<int>(buffer.buffer.size())) {
        return false;
    }
    const int s = buffer.buffer[mUserCursor.y()].size();
    // Not `from + to > s`: that overflows for a large `to`, and a wrapped negative sum passes the check.
    if (from > s || to > s - from) {
        return false;
    }
    P_begin = QPoint(from, mUserCursor.y());
    P_end = QPoint(from + to, mUserCursor.y());

    if (TDebug::wants(TDebug::Category::Selection)) {
        TDebug(Qt::darkMagenta, Qt::black, TDebug::Category::Selection) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y() << ") selectedText:\n\""
                                                                        << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\"\n"
                >> mpHost;
    }
    return true;
}

void TConsoleModel::selectCurrentLine()
{
    selectSection(0, buffer.line(mUserCursor.y()).size());
}

int TConsoleModel::selectString(const QString& text, int numOfMatch)
{
    if (mUserCursor.y() < 0 || mUserCursor.y() >= buffer.size()) {
        deselect();
        return -1;
    }

    if (TDebug::wants(TDebug::Category::Selection)) {
        TDebug(Qt::darkMagenta, Qt::black, TDebug::Category::Selection) << "line under current user cursor: " >> mpHost;
        TDebug(Qt::red, Qt::black, TDebug::Category::Selection) << TDebug::csmContinue << mUserCursor.y() << "#:" >> mpHost;
        TDebug(Qt::gray, Qt::black, TDebug::Category::Selection) << TDebug::csmContinue << buffer.line(mUserCursor.y()) << "\n" >> mpHost;
    }

    const QString li = buffer.line(mUserCursor.y());
    if (li.isEmpty()) {
        deselect();
        return -1;
    }

    int begin = -1;
    for (int i = 0; i < numOfMatch; i++) {
        begin = li.indexOf(text, begin + 1);

        if (begin == -1) {
            deselect();
            return -1;
        }
    }
    if (begin < 0) {
        deselect();
        return -1;
    }

    const int end = begin + text.size();
    P_begin = QPoint(begin, mUserCursor.y());
    P_end = QPoint(end, mUserCursor.y());

    if (TDebug::wants(TDebug::Category::Selection)) {
        TDebug(Qt::darkRed, Qt::black, TDebug::Category::Selection) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y()
                                                                    << ") selectedText = " << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\n"
                >> mpHost;
    }
    return begin;
}

std::tuple<bool, QString, int, int> TConsoleModel::selection()
{
    if (P_begin.y() >= static_cast<int>(buffer.buffer.size())) {
        return {false, qsl("the selection is no longer valid"), 0, 0};
    }

    const auto start = P_begin.x();
    const auto length = P_end.x() - P_begin.x();
    const auto line = buffer.line(P_begin.y());
    if (line.size() < start) {
        return {false, qsl("the selection is no longer valid"), 0, 0};
    }

    const auto text = line.mid(start, length);
    return {true, text, start, length};
}

QPair<quint8, TChar> TConsoleModel::textAttributes() const
{
    int x = P_begin.x();
    int y = P_begin.y();

    // Fallback to cursor position if no selection is active
    if (P_begin == P_end) {
        x = mUserCursor.x();
        y = mUserCursor.y();
    }

    if (y < 0 || x < 0 || y >= static_cast<int>(buffer.buffer.size())) {
        return qMakePair(2, TChar());
    }

    const auto& line = buffer.buffer.at(y);
    if (x >= static_cast<int>(line.size())) {
        return qMakePair(2, TChar());
    }

    return qMakePair(0, line.at(x));
}

const TChar* TConsoleModel::selectionStartChar() const
{
    const int x = P_begin.x();
    const int y = P_begin.y();
    if (y < 0 || x < 0 || y >= static_cast<int>(buffer.buffer.size())) {
        return nullptr;
    }

    const auto& line = buffer.buffer.at(y);
    if (x >= static_cast<int>(line.size())) {
        return nullptr;
    }
    return &line.at(x);
}

void TConsoleModel::resetFormat()
{
    deselect();
    mFormatCurrent.setColors(mFgColor, mBgColor);
    mFormatCurrent.setAllDisplayAttributes(TChar::None);
}

bool TConsoleModel::setSelectionBgColor(const QColor& newColor)
{
    mFormatCurrent.setBackground(newColor);
    return buffer.applyBgColor(P_begin, P_end, newColor);
}

bool TConsoleModel::setSelectionFgColor(const QColor& newColor)
{
    mFormatCurrent.setForeground(newColor);
    return buffer.applyFgColor(P_begin, P_end, newColor);
}

void TConsoleModel::print(const QString& msg)
{
    buffer.append(msg, 0, msg.size(), mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
    mirrorToStdOut(msg);
}

void TConsoleModel::print(const QString& msg, const QColor& fgColor, const QColor& bgColor, const QString& timeStampOverride)
{
    // A sysBufferShrinkEvent handler run by the append echoes after msg on screen, so it is held until msg is out
    if (Q_UNLIKELY(MudletApp::smMirrorToStdOut)) {
        flushHeldMirrorText();
    }
    buffer.append(msg, 0, msg.size(), fgColor, bgColor, TChar::None, 0, timeStampOverride);
    writeMirrorFragments(msg);
}

void TConsoleModel::printSystemMessage(const QString& msg)
{
    // Kept in TConsole's context, where the translations already are.
    const QString txt = QCoreApplication::translate("TConsole", "System Message: %1").arg(msg);
    print(txt, mSystemMessageFgColor, mSystemMessageBgColor);
}

namespace {
// The first failure (reader gone, stream full) turns --mirror off and says so once, rather than
// silently losing every line.
void writeMirrorLine(const QString& line)
{
    // A caller part way through several lines when an earlier one failed
    if (!MudletApp::smMirrorToStdOut) {
        return;
    }
    QByteArray output = line.toUtf8();
    output.append('\n');
    const size_t length = static_cast<size_t>(output.size());
    if (std::fwrite(output.constData(), 1, length, stdout) == length && std::fflush(stdout) == 0) {
        return;
    }

    MudletApp::smMirrorToStdOut = false;
    qWarning().nospace() << "--mirror: could not write to standard output (" << std::strerror(errno) << "), nothing more will be copied to it";
}

// Every main console is "main", so the profile name is needed too. Both names come from Lua and may
// hold control characters; a line feed would split the record for a line-based reader.
QString mirrorPrefix(const QString& profileName, const QString& consoleName)
{
    QString prefix = qsl("%1.%2| ").arg(profileName, consoleName);
    for (QChar& character : prefix) {
        if (character.category() == QChar::Other_Control) {
            character = QChar::ReplacementCharacter;
        }
    }
    return prefix;
}
} // namespace

TConsoleModel::CommandEcho TConsoleModel::printCommand(QString& msg)
{
    // Skip printing if remote echo is active (e.g., password mode)
    if (mpHost && mpHost->isRemoteEchoingActive()) {
        return {};
    }

    if (mTriggerEngineMode) {
        msg.append(QChar::LineFeed);
        if (buffer.lineBuffer.isEmpty()) {
            buffer.appendEmptyLine();
        }
        if (!buffer.lineBuffer.back().isEmpty()) {
            msg.prepend(QChar::LineFeed);
        }
        const MirrorMark mirrorMark = markTriggerAppendForMirror();
        buffer.appendLine(msg, 0, msg.size() - 1, mCommandFgColor, mCommandBgColor);
        holdTriggerWriteForMirror(mirrorMark);
        return {};
    }

    const int lineBeforeNewContent = buffer.size() - 2;
    if (lineBeforeNewContent >= 0 && buffer.promptBuffer[lineBeforeNewContent]) {
        QPoint P(buffer.buffer.at(lineBeforeNewContent).size(), lineBeforeNewContent);
        const TChar format(mCommandFgColor, mCommandBgColor);
        buffer.insertInLine(P, msg, format);
        // The prompt was mirrored when it arrived, so the command gets a line of its own. Script text
        // still open below the prompt stays open: on screen it is under the line the command went on.
        if (Q_UNLIKELY(MudletApp::smMirrorToStdOut) && !msg.isEmpty()) {
            flushHeldMirrorText();
            const QString prefix = mirrorPrefix(mProfileName, mConsoleName);
            for (const QString& line : msg.split(QChar::LineFeed)) {
                writeMirrorLine(prefix + line);
            }
        }
        const int down = buffer.wrapLine(lineBeforeNewContent);
        buffer.promptBuffer[lineBeforeNewContent] = false;
        return {CommandEcho::Kind::PromptLine, lineBeforeNewContent, lineBeforeNewContent + 1 + down};
    }
    msg.append(QChar::LineFeed);
    print(msg, mCommandFgColor, mCommandBgColor);
    return {CommandEcho::Kind::NewLines};
}

void TConsoleModel::mirrorToStdOut(const QString& text)
{
    if (Q_LIKELY(!MudletApp::smMirrorToStdOut)) {
        return;
    }

    flushHeldMirrorText();
    writeMirrorFragments(text);
}

void TConsoleModel::writeMirrorFragments(const QString& text)
{
    if (Q_LIKELY(!MudletApp::smMirrorToStdOut)) {
        return;
    }
    // Text may be a fragment (Lua's print() sends its newline separately, echo() need not end a line),
    // so like TBuffer::appendLine(), write a line out only once a line feed ends it.
    QStringList fragments = text.split(QChar::LineFeed);
    const QString stillOpen = fragments.takeLast();
    const QString prefix = mirrorPrefix(mProfileName, mConsoleName);
    for (const QString& fragment : fragments) {
        writeMirrorLine(prefix + mMirrorPendingLine + fragment);
        mMirrorPendingLine.clear();
    }
    mMirrorPendingLine.append(stillOpen);
}

void TConsoleModel::mirrorLineToStdOut(const QString& line)
{
    if (Q_LIKELY(!MudletApp::smMirrorToStdOut)) {
        return;
    }

    flushHeldMirrorText();
    ++mMirrorLinesCommitted;
    const QString prefix = mirrorPrefix(mProfileName, mConsoleName);
    // Like TBuffer::commitLineData(), put a committed line below a non-empty open line, not onto it.
    if (!mMirrorPendingLine.isEmpty()) {
        writeMirrorLine(prefix + mMirrorPendingLine);
        mMirrorPendingLine.clear();
    }
    writeMirrorLine(prefix + line);
}

namespace {
QPoint endOfBuffer(const TBuffer& buffer)
{
    return buffer.lineBuffer.isEmpty() ? QPoint(0, 0) : QPoint(buffer.lineBuffer.constLast().size(), buffer.lineBuffer.size() - 1);
}

QString bufferText(const TBuffer& buffer, const QPoint& start, const QPoint& end)
{
    const QStringList& lines = buffer.lineBuffer;
    if (start.y() < 0 || end.y() < start.y() || end.y() >= lines.size()) {
        return {};
    }
    if (start.y() == end.y()) {
        return lines.at(start.y()).mid(start.x(), end.x() - start.x());
    }
    QStringList pieces{lines.at(start.y()).mid(start.x())};
    for (int y = start.y() + 1; y < end.y(); ++y) {
        pieces << lines.at(y);
    }
    pieces << lines.at(end.y()).left(end.x());
    return pieces.join(QChar::LineFeed);
}
} // namespace

TConsoleModel::MirrorMark TConsoleModel::markTriggerWriteForMirror(const QPoint& at) const
{
    if (Q_LIKELY(!MudletApp::smMirrorToStdOut)) {
        return {};
    }
    // TBuffer::insertInLine() appends what it is given for a line that is not there
    if (at.y() < 0 || at.y() >= buffer.lineBuffer.size()) {
        return markTriggerAppendForMirror();
    }
    return {at, static_cast<int>(buffer.lineBuffer.at(at.y()).size()), mMirrorLinesCommitted};
}

TConsoleModel::MirrorMark TConsoleModel::markTriggerAppendForMirror() const
{
    if (Q_LIKELY(!MudletApp::smMirrorToStdOut)) {
        return {};
    }
    return {endOfBuffer(buffer), -1, mMirrorLinesCommitted};
}

void TConsoleModel::holdTriggerWriteForMirror(const MirrorMark& mark)
{
    // Lines committed during the write (the OSC 8 documentation it asked for) were mirrored by their commit
    if (mark.start.y() < 0 || !MudletApp::smMirrorToStdOut || mark.linesCommitted != mMirrorLinesCommitted) {
        return;
    }
    QPoint start = mark.start;
    QPoint end;
    if (mark.lineLength >= 0) {
        if (start.y() >= buffer.lineBuffer.size()) {
            return;
        }
        const int added = static_cast<int>(buffer.lineBuffer.at(start.y()).size()) - mark.lineLength;
        if (added <= 0) {
            return;
        }
        // A write past the end of the line pads it out to where the write goes
        start.setX(std::min(start.x(), mark.lineLength));
        end = QPoint(start.x() + added, start.y());
    } else {
        end = endOfBuffer(buffer);
    }
    const QString text = bufferText(buffer, start, end);
    if (text.isEmpty()) {
        return;
    }

    const bool carriesOn = (start == mMirrorTriggerWriteEnd);
    if (carriesOn && !mMirrorTriggerRuns.isEmpty()) {
        mMirrorTriggerRuns.last().text.append(text);
        mMirrorTriggerRuns.last().lastLine = end.y();
    } else {
        // Text put in ahead of what a line already holds starts a line, as does text carrying on
        // after a line feed already written out
        const bool startsLine = carriesOn ? mMirrorTriggerWriteEndedLine : (start.x() == 0 && mark.lineLength > 0);
        mMirrorTriggerRuns.append({text, start.y(), end.y(), startsLine});
    }
    mMirrorTriggerWriteEnd = end;
    mMirrorTriggerWriteEndedLine = text.endsWith(QChar::LineFeed);
}

void TConsoleModel::forgetHeldMirrorTextOnLine(const int line)
{
    mMirrorTriggerRuns.removeIf([line](const MirrorRun& run) {
        return run.firstLine == line && run.lastLine == line;
    });
    for (MirrorRun& run : mMirrorTriggerRuns) {
        if (run.firstLine > line) {
            --run.firstLine;
        }
        if (run.lastLine > line) {
            --run.lastLine;
        }
    }
    if (mMirrorTriggerWriteEnd.y() == line) {
        mMirrorTriggerWriteEnd = QPoint(-1, -1);
    } else if (mMirrorTriggerWriteEnd.y() > line) {
        mMirrorTriggerWriteEnd.ry()--;
    }
}

void TConsoleModel::flushHeldMirrorText()
{
    // Only the main console runs in trigger mode, and what a trigger wrote there first goes out first
    TConsoleModel* mainConsole = mpHost ? mpHost->mainConsoleModelOrNull() : nullptr;
    if (mainConsole && mainConsole != this) {
        mainConsole->writeHeldMirrorRuns();
    }
    writeHeldMirrorRuns();
}

void TConsoleModel::flushMirroredTriggerText()
{
    writeHeldMirrorRuns();
    mMirrorTriggerWriteEnd = QPoint(-1, -1);
    mMirrorTriggerWriteEndedLine = false;
}

void TConsoleModel::writeHeldMirrorRuns()
{
    if (Q_LIKELY(mMirrorTriggerRuns.isEmpty())) {
        return;
    }

    const QList<MirrorRun> runs = std::exchange(mMirrorTriggerRuns, {});
    const QString prefix = mirrorPrefix(mProfileName, mConsoleName);
    for (const MirrorRun& run : runs) {
        QStringList lines = run.text.split(QChar::LineFeed);
        // Unless the run starts a line, what precedes its first line feed went onto the end of one
        // already there, and what follows its last one ends where the line it is on does, so neither
        // is a line of its own when empty.
        if (!run.startsLine && lines.constFirst().isEmpty()) {
            lines.removeFirst();
        }
        if (!lines.isEmpty() && lines.constLast().isEmpty()) {
            lines.removeLast();
        }
        if (lines.isEmpty()) {
            continue;
        }
        if (!mMirrorPendingLine.isEmpty()) {
            writeMirrorLine(prefix + mMirrorPendingLine);
            mMirrorPendingLine.clear();
        }
        for (const QString& line : lines) {
            writeMirrorLine(prefix + line);
        }
    }
}

// Sets or resets all the given attributes, and no others.
bool TConsoleModel::setSelectionDisplayAttributes(const TChar::AttributeFlags attributes, const bool enabled)
{
    mFormatCurrent.setAllDisplayAttributes((mFormatCurrent.allDisplayAttributes() & ~(attributes)) | (enabled ? attributes : TChar::None));
    return buffer.applyAttribute(P_begin, P_end, attributes, enabled);
}

bool TConsoleModel::setLink(const QStringList& commands, const QStringList& hints, const QVector<int>& luaReferences)
{
    return buffer.applyLink(P_begin, P_end, commands, hints, luaReferences);
}

double TConsoleModel::contrastRatio(const QColor& first, const QColor& second)
{
    const double one = relativeLuminance(first);
    const double other = relativeLuminance(second);
    return (std::max(one, other) + 0.05) / (std::min(one, other) + 0.05);
}

void TConsoleModel::echoLink(const QString& text, QStringList& commands, QStringList& hints, const bool useCurrentFormat, const QVector<int>& luaReferences)
{
    const MirrorMark mirrorMark = mTriggerEngineMode ? markTriggerAppendForMirror() : MirrorMark();
    if (useCurrentFormat) {
        buffer.addLink(mTriggerEngineMode, text, commands, hints, mFormatCurrent, luaReferences);
    } else {
        // the main console's links are coloured against the profile's own background
        const QColor background = (mpHost && mpHost->mainConsoleModelOrNull() == this) ? mpHost->mBgColor : mBgColor;
        buffer.addLink(mTriggerEngineMode, text, commands, hints, standardLinkFormat(background), luaReferences);
    }
    if (mTriggerEngineMode) {
        holdTriggerWriteForMirror(mirrorMark);
    } else {
        mirrorToStdOut(text);
    }
}

TConsoleModel::WriteResult TConsoleModel::insertLink(const QString& text, QStringList& commands, QStringList& hints, const bool useCurrentFormat, const QVector<int>& luaReferences)
{
    const QPoint start = mUserCursor;
    const QPoint end(start.x() + text.size(), start.y());
    const TChar format = useCurrentFormat ? mFormatCurrent : standardLinkFormat(mBgColor);
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(start.x(), text.size());
        QPoint at = start;
        const MirrorMark mirrorMark = markTriggerWriteForMirror(at);
        buffer.insertInLine(at, text, format);
        holdTriggerWriteForMirror(mirrorMark);
        buffer.applyLink(start, end, commands, hints, luaReferences);
        if (start.y() < mEngineCursor) {
            return {false, mUserCursor.y(), mUserCursor.y()};
        }
        return {};
    }
    if (buffer.buffer.empty() || mUserCursor == buffer.getEndPos()) {
        buffer.addLink(mTriggerEngineMode, text, commands, hints, format, luaReferences);
        mirrorToStdOut(text);
        return {true};
    }

    buffer.insertInLine(mUserCursor, text, format);
    buffer.applyLink(start, end, commands, hints, luaReferences);
    const int line = mUserCursor.y();
    int newX = mUserCursor.x() + text.size();
    int down = 0;
    if (text.indexOf(QChar::LineFeed) != -1) {
        down = buffer.wrapLine(line);
        newX = std::max(0, static_cast<int>(text.size() - text.lastIndexOf(QChar::LineFeed) - 1));
    }
    QPoint newCursor(newX, line + down);
    if (buffer.moveCursor(newCursor)) {
        mUserCursor = QPoint(newX, line + down);
    }
    return {false, line, line + down};
}

bool TConsoleModel::echo(QString& text)
{
    // A \r would be kept in the buffer and drawn as a glyph, so \r\n becomes \n and a lone \r goes.
    text.remove(QChar::CarriageReturn);
    buffer.mEchoingText = true;
    bool appended = false;
    if (mTriggerEngineMode) {
        // Line feeds are embedded in the trigger's line rather than starting new lines, so that
        // later echoes and cechoes still land on it; wrapping breaks it at them.
        const MirrorMark mirrorMark = markTriggerAppendForMirror();
        const int y = buffer.size() - 1;
        if (y >= 0) {
            QPoint insertPoint(buffer.lineBuffer.at(y).size(), y);
            buffer.insertInLine(insertPoint, text, mFormatCurrent);
        } else {
            buffer.appendLine(text, 0, text.size() - 1, mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
        }
        holdTriggerWriteForMirror(mirrorMark);
    } else {
        buffer.append(text, 0, text.size(), mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
        appended = true;
    }
    buffer.mEchoingText = false;
    return appended;
}

TConsoleModel::WriteResult TConsoleModel::insertText(const QString& text)
{
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(mUserCursor.x(), text.size());
        QPoint at = mUserCursor;
        const MirrorMark mirrorMark = markTriggerWriteForMirror(at);
        buffer.insertInLine(at, text, mFormatCurrent);
        holdTriggerWriteForMirror(mirrorMark);
        if (at.y() < mEngineCursor) {
            return {false, mUserCursor.y(), mUserCursor.y()};
        }
        return {};
    }
    if (buffer.buffer.empty() || mUserCursor == buffer.getEndPos()) {
        buffer.append(text, 0, text.size(), mFormatCurrent);
        mirrorToStdOut(text);
        return {true};
    }

    buffer.insertInLine(mUserCursor, text, mFormatCurrent);
    const int line = mUserCursor.y();
    int down = 0;
    if (text.indexOf(QChar::LineFeed) != -1) {
        down = buffer.wrapLine(line);
    }
    return {false, line, line + down};
}

void TConsoleModel::replace(const QString& text)
{
    MirrorMark mirrorMark;
    if (mTriggerEngineMode) {
        if (P_begin == P_end) {
            mpHost->getLuaInterpreter()->adjustCaptureGroups(P_begin.x(), text.size());
        } else {
            mpHost->getLuaInterpreter()->adjustCaptureGroupsForReplace(P_begin.x(), P_end.x() - P_begin.x(), text);
        }
        // Only a selection within one line, where the line less the selection is what the text goes into
        if (P_begin.y() == P_end.y() && P_end.x() >= P_begin.x()) {
            mirrorMark = markTriggerWriteForMirror(P_begin);
            mirrorMark.lineLength -= P_end.x() - P_begin.x();
        }
    }
    if (buffer.replaceInLine(P_begin, P_end, text, mFormatCurrent)) {
        holdTriggerWriteForMirror(mirrorMark);
    }
}

// Two gotchas in here:
//
// - the strings destined for the log file itself are translated against the
//   "TMainConsole" context rather than through tr(). The catalogue is keyed on
//   context plus source text, so re-keying them under TConsoleModel would
//   orphan every translation they already have. The messages printed on
//   *screen* live in TMainConsole and keep their context that way.
// - QFontInfo(Host::getDisplayFont()) is what QWidget::fontInfo() reports for
//   the main console, because Host::getDisplayFont() hands back that widget's
//   own QFont - and unlike the widget call it still answers with no widget.
namespace {
// The sentinel's existence makes Host resume logging on the next load, so a stale one repeats a failed
// start on every launch. It may be a directory, which QFile::remove() won't take.
void removeAutologSentinel(const QString& path)
{
    const QFileInfo sentinel(path);
    if (!sentinel.exists()) {
        return;
    }
    if (sentinel.isDir()) {
        QDir().rmdir(path);
        return;
    }
    QFile::remove(path);
}
} // namespace

// A clicked checkable button has already flipped itself, so a failed start must still report the state,
// and why, since the autolog resume on profile load has no button to watch.
void TConsoleModel::reportFailedLogStart(const QString& path, const QString& reason)
{
    mLogStartFailure = qsl("%1: %2").arg(path, reason);
    //: Error shown on the main console when a log file could not be opened. %1 is the file, %2 is the reason
    mpHost->postMessage(QCoreApplication::translate("TConsoleModel", "[ ERROR ] - Could not start logging to \"%1\": %2").arg(path, reason));
    mpHost->raiseLoggingStateChanged(false);
}

void TConsoleModel::scheduleLogFlush()
{
    if (mLogFlushPending) {
        return;
    }
    mLogFlushPending = true;
    QTimer::singleShot(0, &mNotifier, [this]() {
        mLogFlushPending = false;
        // Closing the file flushed the stream already, and flushing a stream
        // whose device is closed latches WriteFailed on it
        if (mLogFile.isOpen()) {
            mLogStream.flush();
        }
    });
}

void TConsoleModel::toggleLogging(bool isMessageEnabled)
{
    // Logging is profile-wide, not per-console: the autolog sentinel, the log
    // directory and the filename format all live on the Host, and TBuffer
    // resolves the stream through Host's main model. Running this against any
    // other model would open a second handle onto the same file and clobber
    // that profile-wide state.
    if (mpHost.isNull() || this != mpHost->mainConsoleModelOrNull()) {
        return;
    }

    const auto loggingPath = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("autolog"));
    QFile file(loggingPath);
    const QDateTime logDateTime = QDateTime::currentDateTime();
    if (!mLogToLogFile) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "TConsoleModel: failed to open autolog file" << loggingPath << "for writing:" << file.errorString();
            removeAutologSentinel(loggingPath);
            reportFailedLogStart(loggingPath, file.errorString());
            return;
        }
        QTextStream out(&file);
        file.close();

        QString directoryLogFile;
        QString logFileName;
        // If no log directory is set, default to Mudlet's replay and log files path
        if (mpHost->mLogDir == nullptr || mpHost->mLogDir.isEmpty()) {
            directoryLogFile = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, mpHost->getName());
        } else {
            directoryLogFile = mpHost->mLogDir;
        }
        // The format being empty is a signal value that means use a specified
        // name:
        if (mpHost->mLogFileNameFormat.isEmpty()) {
            if (mpHost->mLogFileName.isEmpty()) {
                // If no log name is set, use the default placeholder
                //: Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {one of two copies}).
                logFileName = QCoreApplication::translate("TMainConsole", "logfile");
            } else {
                // Otherwise a specific name as one is given
                logFileName = mpHost->mLogFileName;
            }
        } else {
            logFileName = logDateTime.toString(mpHost->mLogFileNameFormat);
        }

        // The preset file name formats are derived from date/times so that
        // alphabetical filename and date sort order are the same...
        const QDir dirLogFile;
        if (!dirLogFile.exists(directoryLogFile)) {
            dirLogFile.mkpath(directoryLogFile);
        }

        mpHost->mIsCurrentLogFileInHtmlFormat = mpHost->mIsNextLogFileInHtmlFormat;
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogFileName = qsl("%1/%2.html").arg(directoryLogFile, logFileName);
        } else {
            mLogFileName = qsl("%1/%2.txt").arg(directoryLogFile, logFileName);
        }
        mLogFile.setFileName(mLogFileName);
        // We do not want to use WriteOnly here:
        // Append = "The device is opened in append mode so that all data is
        // written to the end of the file."
        // WriteOnly = "The device is open for writing. Note that this mode
        // implies Truncate."
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            if (!mLogFile.open(QIODevice::ReadWrite)) {
                qWarning() << "TConsoleModel: failed to open log file" << mLogFileName << "for reading/writing:" << mLogFile.errorString();
                removeAutologSentinel(loggingPath);
                reportFailedLogStart(mLogFileName, mLogFile.errorString());
                return;
            }
        } else {
            if (!mLogFile.open(QIODevice::Append)) {
                qWarning() << "TConsoleModel: failed to open log file" << mLogFileName << "for appending:" << mLogFile.errorString();
                removeAutologSentinel(loggingPath);
                reportFailedLogStart(mLogFileName, mLogFile.errorString());
                return;
            }
        }
        mLogStream.setDevice(&mLogFile);

        if (isMessageEnabled) {
            // The frontend prints this synchronously, and anything printed on
            // the console once the flag below is up is IMMEDIATELY POSTED into
            // the log file - so it has to be raised while logging is still off.
            mpHost->raiseLoggingAnnouncement(true, mLogFile.fileName());
        }
        mLogToLogFile = true;
    } else {
        removeAutologSentinel(loggingPath);
        mLogToLogFile = false;
        if (isMessageEnabled) {
            // Likewise raised only once the flag above is down, or the frontend's
            // print would be posted into the log file it is announcing the end of.
            mpHost->raiseLoggingAnnouncement(false, mLogFile.fileName());
        }
    }

    if (mLogToLogFile) {
        // Logging is being turned on
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            QString log;
            QTextStream logStream(&log);
            // No setting a QTextCodec here, they don't work on QString based QTextStreams
            // The font-family entry for the master css in the header
            QStringList fontsList;
            fontsList << QFontInfo(mpHost->getDisplayFont()).family();
            fontsList << qsl("Courier New");
            fontsList << qsl("Monospace");
            fontsList << qsl("Courier");
            fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

            logStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
            logStream << "<html>\n";
            logStream << " <head>\n";
            logStream << "  <meta http-equiv='content-type' content='text/html; charset=utf-8'>";
            // put the charset as early as possible as the parser MUST restart when it
            // switches away from the ASCII default
            logStream << "  <meta name='generator' content='" << QCoreApplication::translate("TMainConsole", "Mudlet MUD Client version: %1%2").arg(APP_VERSION, MudletApp::buildSuffix()) << "'>\n";
            // Nice to identify what made the file!
            logStream << "  <title>" << QCoreApplication::translate("TMainConsole", "Mudlet, log from %1 profile").arg(mpHost->getName()) << "</title>\n";
            // Web-page title
            logStream << "  <style type='text/css'>\n";
            logStream << "   <!-- body { font-family: '" << fontsList.join("', '") << "'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(" << mpHost->mFgColor.red() << ","
                      << mpHost->mFgColor.green() << "," << mpHost->mFgColor.blue() << "); background-color:rgb(" << mpHost->mBgColor.red() << "," << mpHost->mBgColor.green() << ","
                      << mpHost->mBgColor.blue() << ");}\n";
            logStream << "        span { white-space: pre-wrap; }\n";

            if (mpHost->getEnableBlinkText()) {
                logStream << "        @keyframes blink-slow { 0%, 100% { opacity: 0.4; } 50% { opacity: 1; } }\n";
                logStream << "        @keyframes blink-fast { 0%, 100% { opacity: 0.4; } 50% { opacity: 1; } }\n";
                logStream << "        .blink-slow { animation: blink-slow 2s ease-in-out infinite; }\n";
                logStream << "        .blink-fast { animation: blink-fast 1s ease-in-out infinite; }\n";
            }

            logStream << "     -->\n";
            logStream << "  </style>\n";
            logStream << "  </head>\n";
            bool isAtBody = false;
            bool foundBody = false;
            while (!mLogStream.atEnd()) {
                const QString line = mLogStream.readLine();
                if (line.contains("<body><div>")) {
                    // Begin writing old log to the current log when the body is
                    // found.
                    isAtBody = true;
                    foundBody = true;
                } else if (line.contains("</div></body>")) {
                    // Stop writing to current log once the end of the old log's
                    // <body> is reached.
                    isAtBody = false;
                }

                if (isAtBody) {
                    logStream << line << "\n";
                }
            }
            if (!foundBody) {
                logStream << "  <body><div>\n";
            } else {
                // Put a horizontal line between separate log sessions
                logStream << "  </div><hr><div>\n";
            }
            logStream
                    << qsl("<p>%1</p>\n")
                               .arg(logDateTime.toString(
                                       //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
                                       QCoreApplication::translate("TMainConsole", "'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.")));
            // <div></div> tags required around outside of the body <span></spans> for
            // strict HTML 4 as we do not use <p></p>s or anything else

            if (!mLogFile.resize(0)) {
                qWarning() << "TConsoleModel::toggleLogging(...) ERROR - Failed to resize HTML Logfile - it may now be corrupted...!";
            }
            mLogStream << log;
            mLogFile.flush();
        } else {
            // File is NOT an HTML one but pure text:
            // Put a horizontal line between separate log sessions
            // Unfortunately QLatin1String does not have a repeated() method,
            // but it does mean we can use non-ASCII/Latin1 characters:
            // Using 10x U+23AF Horizontal line extension from "Box drawing characters":
            if (mLogFile.size() > 5) {
                // Allow a few junk characters ("BOM"???) at the very start of
                // file to not trigger the insertion of this line:
                mLogStream << qsl("⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯").repeated(8).append(QChar::LineFeed);
            }
            mLogStream << qsl("%1\n").arg(logDateTime.toString(
                    //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
                    QCoreApplication::translate("TMainConsole", "'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.")));
        }
    } else {
        // Logging is being turned off
        buffer.logRemainingOutput();
        //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
        const QString endDateTimeLine = logDateTime.toString(QCoreApplication::translate("TMainConsole", "'Log session ending at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'."));
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogStream << qsl("<p>%1</p>\n").arg(endDateTimeLine);
            mLogStream << "  </div></body>\n";
            mLogStream << "</html>\n";
        } else {
            // File is NOT an HTML one but pure text:
            mLogStream << endDateTimeLine << "\n";
        }
        mLogFile.flush();
        mLogFile.close();
    }

    mpHost->raiseLoggingStateChanged(mLogToLogFile);
}
