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

namespace {
bool asciiPunctuation(const QChar c)
{
    static const QString punctuation = qsl("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    return punctuation.contains(c);
}

bool unicodePunctuation(const QChar c)
{
    return c.isPunct() || c.isSymbol();
}

bool startsWithAddress(const QString& text, const int at)
{
    return text.mid(at, 7).compare(qsl("http://"), Qt::CaseInsensitive) == 0 || text.mid(at, 8).compare(qsl("https://"), Qt::CaseInsensitive) == 0;
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

    void scanCodeEscapesAndAutolinks();
    void resolveLinks();
    void findBareAddresses();
    void emphasis(int from, int to);
    QList<TMarkdownLink> build(QString& text, std::vector<TChar>& format) const;

private:
    bool free(const int i) const { return !mRemoved[static_cast<size_t>(i)] && !mLiteral[static_cast<size_t>(i)]; }
    void removeRange(int from, int to);
    void literalRange(int from, int to);
    int addLink(int from, int to, const QString& destination, const QString& title);
    // Where the link's closing parenthesis is, or -1 when what follows ] is not a destination
    int parseDestination(int at, QString& destination, QString& title) const;

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
};

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

void Line::scanCodeEscapesAndAutolinks()
{
    int i = mFrom;
    while (i < mSize) {
        const QChar c = mText.at(i);
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
            int close = -1;
            for (int j = i + run; j < mSize;) {
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
                if (startsWithAddress(address, 0) || address.startsWith(qsl("mailto:"), Qt::CaseInsensitive)) {
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

int Line::parseDestination(int at, QString& destination, QString& title) const
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
        const int start = at;
        int depth = 0;
        while (at < mSize && !mText.at(at).isSpace()) {
            const QChar c = mText.at(at);
            if (c == u'\\' && at + 1 < mSize && asciiPunctuation(mText.at(at + 1))) {
                at += 2;
                continue;
            }
            if (c == u'(') {
                ++depth;
            } else if (c == u')') {
                if (depth == 0) {
                    break;
                }
                --depth;
            }
            ++at;
        }
        if (depth != 0) {
            return -1;
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

void Line::resolveLinks()
{
    struct Opener
    {
        int pos = 0;
        bool image = false;
        bool active = true;
    };
    QList<Opener> openers;
    for (int i = mFrom; i < mSize; ++i) {
        if (!free(i)) {
            continue;
        }
        const QChar c = mText.at(i);
        if (c == u'[') {
            const bool image = i > mFrom && mText.at(i - 1) == u'!' && free(i - 1);
            openers.append({image ? i - 1 : i, image, true});
            continue;
        }
        if (c != u']' || openers.isEmpty()) {
            continue;
        }
        const Opener opener = openers.takeLast();
        if (!opener.active) {
            continue;
        }
        QString destination;
        QString title;
        const int close = parseDestination(i + 1, destination, title);
        if (close < 0) {
            continue;
        }
        const int textFrom = opener.pos + (opener.image ? 2 : 1);
        // A character has one link, and an OSC 8 one already there wins
        bool covered = false;
        for (int j = textFrom; j < i; ++j) {
            covered = covered || mFormat[static_cast<size_t>(j)].linkIndex() != 0;
        }
        if (covered) {
            continue;
        }
        emphasis(textFrom, i);
        removeRange(opener.pos, textFrom);
        removeRange(i, close + 1);
        addLink(textFrom, i, destination, title);
        if (!opener.image) {
            for (Opener& earlier : openers) {
                earlier.active = false;
            }
        }
        i = close;
    }
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
        int end = i;
        while (end < mSize && !mText.at(end).isSpace() && mText.at(end) != u'<' && !mRemoved[static_cast<size_t>(end)]) {
            ++end;
        }
        while (end > i) {
            const QChar last = mText.at(end - 1);
            if (trailing.contains(last)) {
                --end;
            } else if (last == u')' && mText.mid(i, end - i).count(u')') > mText.mid(i, end - i).count(u'(')) {
                --end;
            } else {
                break;
            }
        }
        const int schemeLength = mText.at(i + 4) == u's' || mText.at(i + 4) == u'S' ? 8 : 7;
        if (end - i <= schemeLength) {
            continue;
        }
        literalRange(i, end);
        addLink(i, end, mText.mid(i, end - i), {});
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
        const QChar before = i > mFrom ? mText.at(i - 1) : QChar(u' ');
        const QChar after = i + run < mSize ? mText.at(i + run) : QChar(u' ');
        const bool leftFlanking = !after.isSpace() && (!unicodePunctuation(after) || before.isSpace() || unicodePunctuation(before));
        const bool rightFlanking = !before.isSpace() && (!unicodePunctuation(before) || after.isSpace() || unicodePunctuation(after));
        Delimiter delimiter{i, run, run, c, leftFlanking, rightFlanking};
        if (c == u'_') {
            delimiter.canOpen = leftFlanking && (!rightFlanking || unicodePunctuation(before));
            delimiter.canClose = rightFlanking && (!leftFlanking || unicodePunctuation(after));
        }
        delimiters.append(delimiter);
        i += run;
    }

    for (int closer = 0; closer < delimiters.size();) {
        Delimiter& close = delimiters[closer];
        if (!close.canClose || close.length == 0) {
            ++closer;
            continue;
        }
        int opener = closer - 1;
        for (; opener >= 0; --opener) {
            const Delimiter& open = delimiters.at(opener);
            if (open.marker != close.marker || !open.canOpen || open.length == 0) {
                continue;
            }
            if (open.marker != u'~' && (open.canClose || close.canOpen) && (open.originalLength + close.originalLength) % 3 == 0 && !(open.originalLength % 3 == 0 && close.originalLength % 3 == 0)) {
                continue;
            }
            break;
        }
        if (opener < 0) {
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
        links.append({column[static_cast<size_t>(link.from)], column[static_cast<size_t>(link.to)] - column[static_cast<size_t>(link.from)], link.destination, link.title});
    }
    text = std::move(builtText);
    format = std::move(builtFormat);
    return links;
}
} // namespace

QList<TMarkdownLink> TMarkdownInline::render(QString& text, std::vector<TChar>& format, const int from)
{
    if (text.size() - from > csmMaxInlineLength || static_cast<size_t>(text.size()) != format.size()) {
        return {};
    }
    Line line(text, format, from);
    line.scanCodeEscapesAndAutolinks();
    line.resolveLinks();
    line.findBareAddresses();
    line.emphasis(from, static_cast<int>(text.size()));
    QString result;
    std::vector<TChar> resultFormat;
    QList<TMarkdownLink> links = line.build(result, resultFormat);
    text = std::move(result);
    format = std::move(resultFormat);
    return links;
}
