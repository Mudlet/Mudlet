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

#include "TMarkdown.h"

#include "TMarkdownInline.h"

#include "utils.h"

TMarkdownMark TMarkdownMark::parse(QStringView value, bool* valid)
{
    static const QList<std::pair<QString, Kind>> names{{qsl("none"), Kind::None},
                                                       {qsl("neutral"), Kind::Neutral},
                                                       {qsl("ok"), Kind::Ok},
                                                       {qsl("info"), Kind::Info},
                                                       {qsl("warn"), Kind::Warn},
                                                       {qsl("error"), Kind::Error},
                                                       {qsl("busy"), Kind::Busy}};
    TMarkdownMark mark;
    for (const auto& [name, kind] : names) {
        if (value == name) {
            mark.kind = kind;
            if (valid) {
                *valid = true;
            }
            return mark;
        }
    }
    bool hex = value.size() == 6;
    for (const QChar c : value) {
        hex = hex && ((c >= QLatin1Char('0') && c <= QLatin1Char('9')) || (c >= QLatin1Char('a') && c <= QLatin1Char('f')) || (c >= QLatin1Char('A') && c <= QLatin1Char('F')));
    }
    if (hex) {
        mark.kind = Kind::Color;
        mark.color = qRgb(value.mid(0, 2).toInt(nullptr, 16), value.mid(2, 2).toInt(nullptr, 16), value.mid(4, 2).toInt(nullptr, 16));
    }
    if (valid) {
        *valid = hex;
    }
    return mark;
}

QString TMarkdownMark::name() const
{
    switch (kind) {
    case Kind::None:
        return {};
    case Kind::Neutral:
        return qsl("neutral");
    case Kind::Ok:
        return qsl("ok");
    case Kind::Info:
        return qsl("info");
    case Kind::Warn:
        return qsl("warn");
    case Kind::Error:
        return qsl("error");
    case Kind::Busy:
        return qsl("busy");
    case Kind::Color:
        return qsl("%1%2%3").arg(qRed(color), 2, 16, QLatin1Char('0')).arg(qGreen(color), 2, 16, QLatin1Char('0')).arg(qBlue(color), 2, 16, QLatin1Char('0'));
    }
    return {};
}

QString TMarkdownBlock::kindName(const Kind kind)
{
    switch (kind) {
    case Kind::None:
        return {};
    case Kind::Blank:
        return qsl("blank");
    case Kind::Paragraph:
        return qsl("paragraph");
    case Kind::Heading:
        return qsl("heading");
    case Kind::Item:
        return qsl("item");
    case Kind::Continuation:
        return qsl("continuation");
    case Kind::Code:
        return qsl("code");
    case Kind::TableHeader:
        return qsl("tableHeader");
    case Kind::TableDelimiter:
        return qsl("tableDelimiter");
    case Kind::TableRow:
        return qsl("tableRow");
    case Kind::Rule:
        return qsl("rule");
    case Kind::Summary:
        return qsl("summary");
    }
    return {};
}

quint32 TMarkdownMessages::begin(const QString& id, const TMarkdownMark mark)
{
    // The message before, if it showed no line and has no id, has nothing left to mark
    if (const auto previous = mRecords.find(mLastMessage); previous != mRecords.end() && previous->id.isEmpty() && !mWithLines.contains(mLastMessage)) {
        mRecords.erase(previous);
    }
    const quint32 message = ++mLastMessage;
    if (id.isEmpty() && mark.kind == TMarkdownMark::Kind::None) {
        return message;
    }
    mRecords.insert(message, {id, mark});
    if (!id.isEmpty()) {
        if (mMessageById.contains(id)) {
            mIdsInOrder.removeOne(id);
        }
        mMessageById.insert(id, message);
        mIdsInOrder.enqueue(id);
        while (mIdsInOrder.size() > csmKeptIds) {
            const QString oldest = mIdsInOrder.dequeue();
            const quint32 oldMessage = mMessageById.take(oldest);
            if (auto it = mRecords.find(oldMessage); it != mRecords.end()) {
                if (mWithLines.contains(oldMessage)) {
                    it->id.clear();
                } else {
                    mRecords.erase(it);
                }
            }
        }
    }
    return message;
}

quint32 TMarkdownMessages::change(const QString& id, const TMarkdownMark mark)
{
    const quint32 message = mMessageById.value(id);
    if (message) {
        mRecords[message].mark = mark;
    }
    return message;
}

void TMarkdownMessages::forgetThrough(const quint32 message)
{
    for (auto it = mRecords.begin(); it != mRecords.end();) {
        if (it.key() <= message) {
            if (!it->id.isEmpty() && mMessageById.value(it->id) == it.key()) {
                mMessageById.remove(it->id);
                mIdsInOrder.removeOne(it->id);
            }
            it = mRecords.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = mWithLines.begin(); it != mWithLines.end();) {
        it = *it <= message ? mWithLines.erase(it) : std::next(it);
    }
}

void TMarkdownMessages::forgetIds()
{
    for (auto it = mRecords.begin(); it != mRecords.end();) {
        if (mWithLines.contains(it.key())) {
            it->id.clear();
            ++it;
        } else {
            it = mRecords.erase(it);
        }
    }
    mMessageById.clear();
    mIdsInOrder.clear();
}

namespace {
bool spaceOrTab(const QChar c)
{
    return c == u' ' || c == u'\t';
}

// Counts the spaces at pos as rule 3 does, a tab as four, and moves pos past them
int skipIndent(const QString& text, int& pos)
{
    int width = 0;
    while (pos < text.size() && spaceOrTab(text.at(pos))) {
        width += text.at(pos) == u'\t' ? 4 : 1;
        ++pos;
    }
    return width;
}

int trimmedEnd(const QString& text, const int from)
{
    int end = static_cast<int>(text.size());
    while (end > from && spaceOrTab(text.at(end - 1))) {
        --end;
    }
    return end;
}

// The level of a heading starting at pos, or 0
int headingLevel(const QString& text, const int pos, const int end)
{
    int level = 0;
    while (pos + level < end && text.at(pos + level) == u'#') {
        ++level;
    }
    if (level == 0 || level > 6) {
        return 0;
    }
    return (pos + level == end || spaceOrTab(text.at(pos + level))) ? level : 0;
}

bool ruleLine(const QString& text, const int pos, const int end)
{
    const QChar marker = text.at(pos);
    if (marker != u'-' && marker != u'*' && marker != u'_') {
        return false;
    }
    int count = 0;
    for (int i = pos; i < end; ++i) {
        if (text.at(i) == marker) {
            ++count;
        } else if (!spaceOrTab(text.at(i))) {
            return false;
        }
    }
    return count >= 3;
}
} // namespace

TMarkdownParser::Text TMarkdownParser::Text::mid(const int from, const int to) const
{
    Text piece;
    piece.text = text.mid(from, to - from);
    piece.format.assign(format.cbegin() + from, format.cbegin() + to);
    return piece;
}

QString TMarkdownParser::quotePrefix(const quint8 quote)
{
    QString prefix;
    for (int level = 0; level < quote; ++level) {
        prefix.append(qsl("│ "));
    }
    return prefix;
}

TMarkdownParser::Output TMarkdownParser::make(const TMarkdownBlock::Kind kind, const quint8 quote, const QString& prefix, Text content) const
{
    Output line;
    line.text = prefix + content.text;
    line.format.assign(static_cast<size_t>(prefix.size()), mPlain);
    line.format.insert(line.format.end(), content.format.begin(), content.format.end());
    line.block.kind = kind;
    line.block.quote = quote;
    line.block.contentStart = static_cast<int>(prefix.size());
    line.block.message = mMessage;
    switch (kind) {
    case TMarkdownBlock::Kind::Paragraph:
    case TMarkdownBlock::Kind::Heading:
    case TMarkdownBlock::Kind::Item:
    case TMarkdownBlock::Kind::Continuation:
    case TMarkdownBlock::Kind::Summary:
        line.block.links = TMarkdownInline::render(line.text, line.format, line.block.contentStart);
        break;
    default:
        break;
    }
    return line;
}

TMarkdownParser::Output TMarkdownParser::blank(const quint8 quote) const
{
    return make(TMarkdownBlock::Kind::Blank, quote, quotePrefix(quote).trimmed(), {});
}

void TMarkdownParser::closeItemsBeyond(const int column)
{
    while (!mItems.isEmpty() && mItems.constLast().textColumn > column) {
        mItems.removeLast();
    }
}

bool TMarkdownParser::closesFence(const QString& line) const
{
    int pos = 0;
    skipIndent(line, pos);
    int run = 0;
    while (pos < line.size() && line.at(pos) == mFenceMarker) {
        ++run;
        ++pos;
    }
    return run >= mFenceLength && trimmedEnd(line, pos) == pos;
}

QList<TMarkdownParser::Output> TMarkdownParser::addLine(const QString& sourceText, const TLineFormat& sourceFormat, const quint32 message)
{
    QList<Output> out;
    mMessage = message;
    Text line;
    line.text = sourceText;
    line.format.reserve(static_cast<size_t>(sourceText.size()));
    for (qsizetype i = 0; i < sourceText.size(); ++i) {
        line.format.push_back(static_cast<size_t>(i) < sourceFormat.size() ? sourceFormat[static_cast<size_t>(i)] : mPlain);
    }

    // Rule 1
    if (mInFence) {
        if (closesFence(line.text)) {
            mInFence = false;
            return out;
        }
        int pos = 0;
        while (pos < mFenceIndent && pos < line.text.size() && line.text.at(pos) == u' ') {
            ++pos;
        }
        Output code = make(TMarkdownBlock::Kind::Code, 0, {}, line.mid(pos, static_cast<int>(line.text.size())));
        code.block.language = mFenceLanguage;
        out << code;
        return out;
    }

    // Rules 2 to 4
    int pos = 0;
    int indent = skipIndent(line.text, pos);
    const int end = trimmedEnd(line.text, pos);
    quint8 quote = 0;
    while (pos < end && line.text.at(pos) == u'>' && quote < csmMaxQuoteLevels) {
        ++quote;
        ++pos;
        if (pos < end && spaceOrTab(line.text.at(pos))) {
            ++pos;
        }
        indent = skipIndent(line.text, pos);
    }
    if (pos >= end) {
        out << blank(quote);
        return out;
    }
    if (quote != mItemsQuote) {
        mItems.clear();
        mItemsQuote = quote;
    }
    const QString prefix = quotePrefix(quote);
    const QString& text = line.text;

    if (quote == 0) {
        // Rule 6
        if (text.at(pos) == u'`' || text.at(pos) == u'~') {
            const QChar marker = text.at(pos);
            int run = 0;
            while (pos + run < end && text.at(pos + run) == marker) {
                ++run;
            }
            const QString info = text.mid(pos + run, end - pos - run).trimmed();
            if (run >= 3 && !(marker == u'`' && info.contains(u'`'))) {
                mInFence = true;
                mFenceMarker = marker;
                mFenceLength = run;
                mFenceIndent = indent;
                mFenceLanguage = info;
                mItems.clear();
                return out;
            }
        }
    }

    // Rule 8
    if (const int level = headingLevel(text, pos, end)) {
        int from = pos + level;
        while (from < end && spaceOrTab(text.at(from))) {
            ++from;
        }
        int to = end;
        int run = to;
        while (run > from && text.at(run - 1) == u'#') {
            --run;
        }
        if (run == from) {
            to = from;
        } else if (run < to && spaceOrTab(text.at(run - 1))) {
            to = trimmedEnd(text.left(run), from);
        }
        mItems.clear();
        Output heading = make(TMarkdownBlock::Kind::Heading, quote, prefix, line.mid(from, to));
        heading.block.level = static_cast<quint8>(level);
        for (size_t i = static_cast<size_t>(heading.block.contentStart); i < heading.format.size(); ++i) {
            heading.format[i].setAttributes(TChar::Bold, true);
        }
        out << heading;
        return out;
    }

    // Rule 9
    if (ruleLine(text, pos, end)) {
        mItems.clear();
        out << make(TMarkdownBlock::Kind::Rule, quote, prefix + QString(std::max(3, mRuleWidth - static_cast<int>(prefix.size())), QChar(0x2500)), {});
        return out;
    }

    // Rule 10
    {
        int markerEnd = pos;
        bool ordered = false;
        if (text.at(pos) == u'-' || text.at(pos) == u'*' || text.at(pos) == u'+') {
            markerEnd = pos + 1;
        } else {
            while (markerEnd < end && markerEnd - pos < 9 && text.at(markerEnd).isDigit() && text.at(markerEnd).unicode() < 128) {
                ++markerEnd;
            }
            if (markerEnd > pos && markerEnd < end && (text.at(markerEnd) == u'.' || text.at(markerEnd) == u')')) {
                ++markerEnd;
                ordered = true;
            } else {
                markerEnd = pos;
            }
        }
        if (markerEnd > pos && markerEnd < end && spaceOrTab(text.at(markerEnd))) {
            int from = markerEnd;
            while (from < end && spaceOrTab(text.at(from))) {
                ++from;
            }
            const int textColumn = indent + (markerEnd - pos) + 1;
            closeItemsBeyond(indent);
            if (from < end && mItems.size() < csmMaxListLevels) {
                TMarkdownBlock::Task task = TMarkdownBlock::Task::None;
                if (end - from >= 4 && text.at(from) == u'[' && text.at(from + 2) == u']' && spaceOrTab(text.at(from + 3))) {
                    const QChar box = text.at(from + 1);
                    int after = from + 3;
                    while (after < end && spaceOrTab(text.at(after))) {
                        ++after;
                    }
                    if (after < end && (box == u' ' || box == u'x' || box == u'X')) {
                        task = box == u' ' ? TMarkdownBlock::Task::Open : TMarkdownBlock::Task::Done;
                        from = after;
                    }
                }
                QString bullet;
                if (ordered) {
                    bullet = text.mid(pos, markerEnd - pos);
                    if (task != TMarkdownBlock::Task::None) {
                        bullet.append(u' ');
                    }
                }
                if (task == TMarkdownBlock::Task::Open) {
                    bullet.append(QChar(0x2610));
                } else if (task == TMarkdownBlock::Task::Done) {
                    bullet.append(QChar(0x2611));
                } else if (!ordered) {
                    bullet = QChar(0x2022);
                }
                const QString itemPrefix = QString(2 * mItems.size(), u' ') + bullet + u' ';
                mItems.append({textColumn, static_cast<int>(itemPrefix.size())});
                Output item = make(TMarkdownBlock::Kind::Item, quote, prefix + itemPrefix, line.mid(from, end));
                item.block.list = static_cast<quint8>(mItems.size());
                item.block.marker = ordered ? TMarkdownBlock::Marker::Ordered : TMarkdownBlock::Marker::Bullet;
                item.block.number = ordered ? text.mid(pos, markerEnd - pos - 1).toInt() : 0;
                item.block.task = task;
                out << item;
                return out;
            }
        }
    }

    // Rule 11
    if (!mItems.isEmpty()) {
        closeItemsBeyond(indent);
        if (!mItems.isEmpty()) {
            Output continuation = make(TMarkdownBlock::Kind::Continuation, quote, prefix + QString(mItems.constLast().prefixWidth, u' '), line.mid(pos, end));
            continuation.block.list = static_cast<quint8>(mItems.size());
            out << continuation;
            return out;
        }
    }

    // Rule 12
    out << make(TMarkdownBlock::Kind::Paragraph, quote, prefix, line.mid(pos, end));
    return out;
}

QList<TMarkdownParser::Output> TMarkdownParser::addPrompt()
{
    return std::exchange(mHeld, {});
}

QList<TMarkdownParser::Output> TMarkdownParser::endMessage()
{
    mInFence = false;
    mItems.clear();
    mItemsQuote = 0;
    return std::exchange(mHeld, {});
}

QList<TMarkdownParser::Output> TMarkdownParser::flushHeld()
{
    return std::exchange(mHeld, {});
}

quint32 TMarkdown::store(TMarkdownBlock&& block)
{
    // 0 means untagged, so the count skips it when it wraps
    if (++mLastBlockId == 0) {
        ++mLastBlockId;
    }
    mMessages.noteLine(block.message);
    mBlocks.insert(mLastBlockId, std::move(block));
    return mLastBlockId;
}

const TMarkdownBlock* TMarkdown::block(const quint32 id) const
{
    const auto it = mBlocks.constFind(id);
    return it == mBlocks.cend() ? nullptr : &it.value();
}
