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

// CommonMark's inline rules (https://spec.commonmark.org/0.31.2/#inlines) applied to one line, with GFM's
// strikethrough and bare web addresses, as the Markdown Text for MUDs standard narrows them.

#include "TMarkdownInline.h"

#include "TBuffer.h"
#include "TMarkdown.h"
#include "utils.h"

#include <array>

namespace {
bool asciiPunctuation(const QChar c)
{
    static const QString punctuation = qsl("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    return punctuation.contains(c);
}

bool unicodePunctuation(const char32_t c)
{
    return QChar::isPunct(c) || QChar::isSymbol(c);
}

bool startsWithAddress(const QString& text, const int at)
{
    const QStringView rest = QStringView(text).mid(at);
    return rest.startsWith(u"http://", Qt::CaseInsensitive) || rest.startsWith(u"https://", Qt::CaseInsensitive);
}

QString unescaped(QStringView text)
{
    QString result;
    result.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text.at(i) == u'\\' && i + 1 < text.size() && asciiPunctuation(text.at(i + 1))) {
            ++i;
        }
        result.append(text.at(i));
    }
    return result;
}

struct Delimiter
{
    int pos = 0;
    int length = 0;
    int originalLength = 0;
    QChar marker;
    bool canOpen = false;
    bool canClose = false;
};

struct Link
{
    int from = 0;
    int to = 0;
    QString destination;
    QString title;
};

class Line
{
public:
    Line(const QString& text, const std::vector<TChar>& format, const int from)
    : mText(text)
    , mFormat(format)
    , mFrom(from)
    , mSize(static_cast<int>(text.size()))
    , mRemoved(static_cast<size_t>(mSize), false)
    , mLiteral(static_cast<size_t>(mSize), false)
    , mFlags(static_cast<size_t>(mSize), TChar::None)
    , mLink(static_cast<size_t>(mSize), -1)
    {
    }

    // Code spans, escapes, autolinks and links, in one pass from left to right as CommonMark reads them, so that a
    // link's destination and title are never read as anything else
    void scanInlines();
    // At the ] at, closes the link the last [ opened, if what follows is a destination; returns where to go on from
    int closeLink(int at);
    void findBareAddresses();
    void emphasis(int from, int to);
    QList<TMarkdownLink> build(QString& text, std::vector<TChar>& format) const;

private:
    bool free(const int i) const { return !mRemoved[static_cast<size_t>(i)] && !mLiteral[static_cast<size_t>(i)]; }
    // A character has one link, and an OSC 8 one already there wins
    bool osc8Covers(const int from, const int to) const
    {
        for (int i = from; i < to; ++i) {
            if (mFormat[static_cast<size_t>(i)].linkIndex() != 0) {
                return true;
            }
        }
        return false;
    }
    void removeRange(int from, int to);
    void literalRange(int from, int to);
    int addLink(int from, int to, const QString& destination, const QString& title);
    // Where the link's closing parenthesis is, or -1 when what follows ] is not a destination
    int parseDestination(int at, QString& destination, QString& title);
    // The whole code points either side of a run, so a symbol outside the basic plane is read as one
    char32_t codePointBefore(int at) const;
    char32_t codePointAt(int at) const;
    // A game can send a line on which every search for a destination or a code span's closing run fails at its end;
    // these are worked out once for the line, so that no search goes over the same text twice
    void matchParentheses();
    void findLastRuns();

    const QString& mText;
    const std::vector<TChar>& mFormat;
    const int mFrom;
    const int mSize;
    std::vector<bool> mRemoved;
    // Not markup: escaped characters, code, and the text of addresses
    std::vector<bool> mLiteral;
    std::vector<TChar::AttributeFlags> mFlags;
    std::vector<int> mLink;
    QList<Link> mLinks;
    // The ) that closes each ( in the same stretch without spaces, or -1
    std::vector<int> mParenMatch;
    // Where the last run of backticks of each length starts
    QHash<int, int> mLastRun;
    struct Opener
    {
        int pos = 0;
        bool image = false;
        bool active = true;
    };
    QList<Opener> mOpeners;
};

char32_t Line::codePointBefore(const int at) const
{
    if (at <= mFrom) {
        return U' ';
    }
    const QChar low = mText.at(at - 1);
    if (low.isLowSurrogate() && at - 2 >= mFrom && mText.at(at - 2).isHighSurrogate()) {
        return QChar::surrogateToUcs4(mText.at(at - 2), low);
    }
    return low.unicode();
}

char32_t Line::codePointAt(const int at) const
{
    if (at >= mSize) {
        return U' ';
    }
    const QChar high = mText.at(at);
    if (high.isHighSurrogate() && at + 1 < mSize && mText.at(at + 1).isLowSurrogate()) {
        return QChar::surrogateToUcs4(high, mText.at(at + 1));
    }
    return high.unicode();
}

void Line::removeRange(const int from, const int to)
{
    for (int i = from; i < to; ++i) {
        mRemoved[static_cast<size_t>(i)] = true;
    }
}

void Line::literalRange(const int from, const int to)
{
    for (int i = from; i < to; ++i) {
        mLiteral[static_cast<size_t>(i)] = true;
    }
}

void Line::matchParentheses()
{
    mParenMatch.assign(static_cast<size_t>(mSize), -1);
    QList<int> open;
    for (int i = mFrom; i < mSize; ++i) {
        const QChar c = mText.at(i);
        if (c.isSpace()) {
            open.clear();
        } else if (c == u'\\' && i + 1 < mSize && asciiPunctuation(mText.at(i + 1))) {
            ++i;
        } else if (c == u'(') {
            open.append(i);
        } else if (c == u')' && !open.isEmpty()) {
            mParenMatch[static_cast<size_t>(open.takeLast())] = i;
        }
    }
}

void Line::findLastRuns()
{
    for (int i = mFrom; i < mSize;) {
        if (mText.at(i) != u'`') {
            ++i;
            continue;
        }
        int run = 0;
        while (i + run < mSize && mText.at(i + run) == u'`') {
            ++run;
        }
        mLastRun.insert(run, i);
        i += run;
    }
}

int Line::addLink(const int from, const int to, const QString& destination, const QString& title)
{
    mLinks.append({from, to, destination, title});
    const int index = static_cast<int>(mLinks.size()) - 1;
    for (int i = from; i < to; ++i) {
        if (mLink[static_cast<size_t>(i)] < 0) {
            mLink[static_cast<size_t>(i)] = index;
        }
    }
    return index;
}

void Line::scanInlines()
{
    int i = mFrom;
    while (i < mSize) {
        const QChar c = mText.at(i);
        if (c == u'[') {
            const bool image = i > mFrom && mText.at(i - 1) == u'!' && free(i - 1);
            mOpeners.append({image ? i - 1 : i, image, true});
            ++i;
            continue;
        }
        if (c == u']' && !mOpeners.isEmpty()) {
            i = closeLink(i);
            continue;
        }
        if (c == u'\\') {
            if (i + 1 < mSize && asciiPunctuation(mText.at(i + 1))) {
                mRemoved[static_cast<size_t>(i)] = true;
                mLiteral[static_cast<size_t>(i + 1)] = true;
                i += 2;
            } else {
                mLiteral[static_cast<size_t>(i)] = true;
                ++i;
            }
            continue;
        }
        if (c == u'`') {
            int run = 0;
            while (i + run < mSize && mText.at(i + run) == u'`') {
                ++run;
            }
            if (mLastRun.isEmpty()) {
                findLastRuns();
            }
            int close = -1;
            for (int j = i + run; j < mSize && mLastRun.value(run, -1) > i;) {
                if (mText.at(j) != u'`') {
                    ++j;
                    continue;
                }
                int other = 0;
                while (j + other < mSize && mText.at(j + other) == u'`') {
                    ++other;
                }
                if (other == run) {
                    close = j;
                    break;
                }
                j += other;
            }
            if (close < 0) {
                literalRange(i, i + run);
                i += run;
                continue;
            }
            int contentFrom = i + run;
            int contentTo = close;
            bool allSpaces = true;
            for (int j = contentFrom; j < contentTo; ++j) {
                allSpaces = allSpaces && mText.at(j) == u' ';
            }
            if (contentTo - contentFrom >= 2 && !allSpaces && mText.at(contentFrom) == u' ' && mText.at(contentTo - 1) == u' ') {
                ++contentFrom;
                --contentTo;
            }
            removeRange(i, contentFrom);
            removeRange(contentTo, close + run);
            literalRange(contentFrom, contentTo);
            for (int j = contentFrom; j < contentTo; ++j) {
                mFlags[static_cast<size_t>(j)] |= TChar::Code;
            }
            i = close + run;
            continue;
        }
        if (c == u'<') {
            int close = i + 1;
            while (close < mSize && mText.at(close) != u'>' && mText.at(close) != u'<' && !mText.at(close).isSpace()) {
                ++close;
            }
            if (close < mSize && mText.at(close) == u'>') {
                const QString address = mText.mid(i + 1, close - i - 1);
                if ((startsWithAddress(address, 0) || address.startsWith(qsl("mailto:"), Qt::CaseInsensitive)) && !osc8Covers(i, close + 1)) {
                    mRemoved[static_cast<size_t>(i)] = true;
                    mRemoved[static_cast<size_t>(close)] = true;
                    literalRange(i + 1, close);
                    addLink(i + 1, close, address, {});
                    i = close + 1;
                    continue;
                }
            }
        }
        ++i;
    }
}

int Line::parseDestination(int at, QString& destination, QString& title)
{
    if (at >= mSize || mText.at(at) != u'(') {
        return -1;
    }
    ++at;
    const auto skipSpaces = [this](int& pos) {
        while (pos < mSize && (mText.at(pos) == u' ' || mText.at(pos) == u'\t')) {
            ++pos;
        }
    };
    skipSpaces(at);
    if (at < mSize && mText.at(at) == u'<') {
        int close = at + 1;
        while (close < mSize && mText.at(close) != u'>' && mText.at(close) != u'<') {
            close += (mText.at(close) == u'\\' && close + 1 < mSize) ? 2 : 1;
        }
        if (close >= mSize || mText.at(close) != u'>') {
            return -1;
        }
        destination = unescaped(QStringView(mText).mid(at + 1, close - at - 1));
        at = close + 1;
    } else {
        if (mParenMatch.empty()) {
            matchParentheses();
        }
        const int start = at;
        while (at < mSize && !mText.at(at).isSpace() && mText.at(at) != u')') {
            const QChar c = mText.at(at);
            if (c == u'\\' && at + 1 < mSize && asciiPunctuation(mText.at(at + 1))) {
                at += 2;
            } else if (c == u'(') {
                const int match = mParenMatch[static_cast<size_t>(at)];
                if (match < 0) {
                    return -1;
                }
                at = match + 1;
            } else {
                ++at;
            }
        }
        destination = unescaped(QStringView(mText).mid(start, at - start));
    }
    const int afterDestination = at;
    skipSpaces(at);
    if (at < mSize && (mText.at(at) == u'"' || mText.at(at) == u'\'') && at > afterDestination) {
        const QChar quote = mText.at(at);
        int close = at + 1;
        while (close < mSize && mText.at(close) != quote) {
            close += (mText.at(close) == u'\\' && close + 1 < mSize) ? 2 : 1;
        }
        if (close >= mSize) {
            return -1;
        }
        title = unescaped(QStringView(mText).mid(at + 1, close - at - 1));
        at = close + 1;
        skipSpaces(at);
    }
    return (at < mSize && mText.at(at) == u')') ? at : -1;
}

int Line::closeLink(const int at)
{
    const Opener opener = mOpeners.takeLast();
    if (!opener.active) {
        return at + 1;
    }
    QString destination;
    QString title;
    const int close = parseDestination(at + 1, destination, title);
    if (close < 0) {
        return at + 1;
    }
    const int textFrom = opener.pos + (opener.image ? 2 : 1);
    // A link with no text would vanish, destination and all, so all of it is shown as typed
    if (textFrom == at) {
        literalRange(opener.pos, close + 1);
        return close + 1;
    }
    // A link cannot hold another, whether an autolink or an OSC 8 link
    bool covered = osc8Covers(textFrom, at);
    for (int j = textFrom; j < at; ++j) {
        covered = covered || mLink[static_cast<size_t>(j)] >= 0;
    }
    if (covered) {
        return at + 1;
    }
    emphasis(textFrom, at);
    removeRange(opener.pos, textFrom);
    removeRange(at, close + 1);
    addLink(textFrom, at, destination, title);
    if (!opener.image) {
        for (Opener& earlier : mOpeners) {
            earlier.active = false;
        }
    }
    return close + 1;
}

void Line::findBareAddresses()
{
    static const QString trailing = qsl(".,:;!?*_~");
    for (int i = mFrom; i < mSize; ++i) {
        if (!free(i) || mLink[static_cast<size_t>(i)] >= 0 || !startsWithAddress(mText, i)) {
            continue;
        }
        if (i > mFrom && !mText.at(i - 1).isSpace() && !qsl("*_~(").contains(mText.at(i - 1))) {
            continue;
        }
        // An escaping backslash, already taken out, does not end the address; any other markup does
        const auto partOfAddress = [this](const int at) {
            return !mText.at(at).isSpace() && mText.at(at) != u'<' && (!mRemoved[static_cast<size_t>(at)] || mText.at(at) == u'\\');
        };
        int end = i;
        int unmatchedClosing = 0;
        while (end < mSize && partOfAddress(end)) {
            if (!mRemoved[static_cast<size_t>(end)]) {
                unmatchedClosing += mText.at(end) == u')' ? 1 : mText.at(end) == u'(' ? -1 : 0;
            }
            ++end;
        }
        while (end > i) {
            const QChar last = mText.at(end - 1);
            if (mRemoved[static_cast<size_t>(end - 1)] || trailing.contains(last)) {
                --end;
            } else if (last == u')' && unmatchedClosing > 0) {
                --unmatchedClosing;
                --end;
            } else {
                break;
            }
        }
        const int schemeLength = mText.at(i + 4) == u's' || mText.at(i + 4) == u'S' ? 8 : 7;
        if (end - i <= schemeLength || osc8Covers(i, end)) {
            continue;
        }
        QString destination;
        for (int j = i; j < end; ++j) {
            if (!mRemoved[static_cast<size_t>(j)]) {
                destination.append(mText.at(j));
            }
        }
        literalRange(i, end);
        addLink(i, end, destination, {});
        i = end - 1;
    }
}

void Line::emphasis(const int from, const int to)
{
    QList<Delimiter> delimiters;
    for (int i = from; i < to;) {
        const QChar c = mText.at(i);
        if (!free(i) || (c != u'*' && c != u'_' && c != u'~')) {
            ++i;
            continue;
        }
        int run = 0;
        while (i + run < to && mText.at(i + run) == c && free(i + run)) {
            ++run;
        }
        // Strikethrough takes exactly two tildes
        if (c == u'~' && run != 2) {
            literalRange(i, i + run);
            i += run;
            continue;
        }
        const char32_t before = codePointBefore(i);
        const char32_t after = codePointAt(i + run);
        const bool leftFlanking = !QChar::isSpace(after) && (!unicodePunctuation(after) || QChar::isSpace(before) || unicodePunctuation(before));
        const bool rightFlanking = !QChar::isSpace(before) && (!unicodePunctuation(before) || QChar::isSpace(after) || unicodePunctuation(after));
        Delimiter delimiter{i, run, run, c, leftFlanking, rightFlanking};
        if (c == u'_') {
            delimiter.canOpen = leftFlanking && (!rightFlanking || unicodePunctuation(before));
            delimiter.canClose = rightFlanking && (!leftFlanking || unicodePunctuation(after));
        }
        delimiters.append(delimiter);
        i += run;
    }

    // CommonMark's openers_bottom: where a search for an opener of this kind has already failed, a later closer of the
    // same kind need not look again, which keeps a line of many unmatched markers from taking quadratic time
    std::array<int, 18> bottom;
    bottom.fill(-1);
    const auto kind = [](const Delimiter& delimiter) {
        const int marker = delimiter.marker == u'*' ? 0 : delimiter.marker == u'_' ? 1 : 2;
        return marker * 6 + (delimiter.canOpen ? 3 : 0) + delimiter.originalLength % 3;
    };
    for (int closer = 0; closer < delimiters.size();) {
        Delimiter& close = delimiters[closer];
        if (!close.canClose || close.length == 0) {
            ++closer;
            continue;
        }
        int opener = closer - 1;
        for (; opener > bottom[static_cast<size_t>(kind(close))]; --opener) {
            const Delimiter& open = delimiters.at(opener);
            if (open.marker != close.marker || !open.canOpen || open.length == 0) {
                continue;
            }
            if (open.marker != u'~' && (open.canClose || close.canOpen) && (open.originalLength + close.originalLength) % 3 == 0 && !(open.originalLength % 3 == 0 && close.originalLength % 3 == 0)) {
                continue;
            }
            break;
        }
        if (opener <= bottom[static_cast<size_t>(kind(close))]) {
            bottom[static_cast<size_t>(kind(close))] = closer - 1;
            ++closer;
            continue;
        }
        Delimiter& open = delimiters[opener];
        const int used = (close.marker == u'~' || (open.length >= 2 && close.length >= 2)) ? 2 : 1;
        const TChar::AttributeFlag flag = close.marker == u'~' ? TChar::StrikeOut : (used == 2 ? TChar::Bold : TChar::Italic);
        const int openEnd = open.pos + open.length;
        removeRange(openEnd - used, openEnd);
        removeRange(close.pos, close.pos + used);
        for (int j = openEnd; j < close.pos; ++j) {
            mFlags[static_cast<size_t>(j)] |= flag;
        }
        open.length -= used;
        close.pos += used;
        close.length -= used;
        for (int between = opener + 1; between < closer; ++between) {
            delimiters[between].length = 0;
        }
    }
    for (const Delimiter& delimiter : delimiters) {
        literalRange(delimiter.pos, delimiter.pos + delimiter.length);
    }
}

QList<TMarkdownLink> Line::build(QString& text, std::vector<TChar>& format) const
{
    QString builtText;
    std::vector<TChar> builtFormat;
    builtText.reserve(mSize);
    builtFormat.reserve(static_cast<size_t>(mSize));
    std::vector<int> column(static_cast<size_t>(mSize) + 1, 0);
    for (int i = 0; i < mSize; ++i) {
        column[static_cast<size_t>(i)] = static_cast<int>(builtText.size());
        if (i >= mFrom && mRemoved[static_cast<size_t>(i)]) {
            continue;
        }
        builtText.append(mText.at(i));
        TChar character = mFormat[static_cast<size_t>(i)];
        if (mFlags[static_cast<size_t>(i)] != TChar::None) {
            character.setAttributes(mFlags[static_cast<size_t>(i)], true);
        }
        builtFormat.push_back(std::move(character));
    }
    column[static_cast<size_t>(mSize)] = static_cast<int>(builtText.size());

    QList<TMarkdownLink> links;
    for (const Link& link : mLinks) {
        const int start = column[static_cast<size_t>(link.from)];
        const int length = column[static_cast<size_t>(link.to)] - start;
        if (length > 0) {
            links.append({start, length, link.destination, link.title});
        }
    }
    text = std::move(builtText);
    format = std::move(builtFormat);
    return links;
}
} // namespace

QList<TMarkdownLink> TMarkdownInline::render(QString& text, std::vector<TChar>& format, const int from)
{
    if (from < 0 || text.size() - from > csmMaxInlineLength || static_cast<size_t>(text.size()) != format.size()) {
        return {};
    }
    Line line(text, format, from);
    line.scanInlines();
    line.findBareAddresses();
    line.emphasis(from, static_cast<int>(text.size()));
    QString result;
    std::vector<TChar> resultFormat;
    QList<TMarkdownLink> links = line.build(result, resultFormat);
    text = std::move(result);
    format = std::move(resultFormat);
    return links;
}
