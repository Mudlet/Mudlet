#ifndef MUDLET_TMARKDOWN_H
#define MUDLET_TMARKDOWN_H

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

// Markdown Text for MUDs (https://wiki.mudlet.org/w/Standards:Markdown_Text_for_MUDs): a game brackets
// lines with OSC 67 and each line is shown as the Markdown it holds, in the grid as text and in a tag
// any view can draw from.

#include "TBuffer.h"

#include <QHash>
#include <QList>
#include <QQueue>
#include <QRgb>
#include <QString>

#include <vector>

struct TMarkdownLink
{
    int start = 0;
    int length = 0;
    QString destination;
    QString title;
};

struct TMarkdownCell
{
    int start = 0;
    int length = 0;
};

struct TMarkdownMark
{
    enum class Kind : quint8 { None, Neutral, Ok, Info, Warn, Error, Busy, Color };
    Kind kind = Kind::None;
    QRgb color = 0;

    // A value the standard does not define is no mark, and leaves valid false
    static TMarkdownMark parse(QStringView value, bool* valid = nullptr);
    // Empty for no mark, else the name sent, or the six hex digits of a color
    QString name() const;
};

struct TMarkdownBlock
{
    enum class Kind : quint8 { None, Blank, Paragraph, Heading, Item, Continuation, Code, TableHeader, TableDelimiter, TableRow, Rule, Summary };
    enum class Marker : quint8 { None, Bullet, Ordered };
    enum class Task : quint8 { None, Open, Done };
    enum class Align : quint8 { None, Left, Center, Right };

    Kind kind = Kind::None;
    quint8 level = 0;
    quint8 quote = 0;
    quint8 list = 0;
    Marker marker = Marker::None;
    Task task = Task::None;
    quint8 section = 0;
    bool open = false;
    int number = 0;
    // Where the line's own text starts, after the prefix the grid shows
    int contentStart = 0;
    quint32 message = 0;
    quint32 sectionId = 0;
    quint32 table = 0;
    QString language;
    QList<Align> align;
    QList<TMarkdownCell> cells;
    QList<TMarkdownLink> links;

    static QString kindName(Kind kind);
};

class TMarkdownMessages
{
public:
    // A new message, numbered from 1
    quint32 begin(const QString& id, TMarkdownMark mark);
    // The message whose mark changed, or 0 when no message has that id
    quint32 change(const QString& id, TMarkdownMark mark);
    quint32 current() const { return mLastMessage; }
    TMarkdownMark mark(quint32 message) const { return mRecords.value(message).mark; }
    QString id(quint32 message) const { return mRecords.value(message).id; }
    // The lines of every message up to this one have gone
    void forgetThrough(quint32 message);
    // A new connection: no id it sends names an earlier message
    void forgetIds();

    static constexpr int csmKeptIds = 256;

private:
    struct Record
    {
        QString id;
        TMarkdownMark mark;
    };
    quint32 mLastMessage = 0;
    // Only messages with an id or a mark have a record
    QHash<quint32, Record> mRecords;
    QHash<QString, quint32> mMessageById;
    QQueue<QString> mIdsInOrder;
};

// A finished line: its text with the grid's prefix, formats to match, and its tag
struct TMarkdownLine
{
    QString text;
    std::vector<TChar> format;
    TMarkdownBlock block;
};

class TMarkdownParser
{
public:
    using Output = TMarkdownLine;

    QList<Output> addLine(const QString& text, const TLineFormat& format, quint32 message);
    // A prompt ends a table and nothing else
    QList<Output> addPrompt();
    QList<Output> endMessage();
    // The held rows of a table, which stays open for rows still to come
    QList<Output> flushHeld();
    bool holdsLines() const { return !mHeld.isEmpty(); }
    void setRuleWidth(int columns) { mRuleWidth = columns; }
    // The colors of the characters the grid adds, such as a bullet
    void setPlainFormat(const TChar& format) { mPlain = format; }

    static constexpr int csmMaxQuoteLevels = 8;
    static constexpr int csmMaxListLevels = 8;

private:
    struct Text
    {
        QString text;
        std::vector<TChar> format;
        Text mid(int from, int to) const;
    };
    struct OpenItem
    {
        // Where the item's text starts, counted in spaces from the start of the line after its quote markers
        int textColumn = 0;
        // Of the grid's prefix, after the quote bars
        int prefixWidth = 0;
    };

    Output make(TMarkdownBlock::Kind kind, quint8 quote, const QString& prefix, Text content) const;
    Output blank(quint8 quote) const;
    void closeItemsBeyond(int column);
    bool closesFence(const QString& line) const;
    static QString quotePrefix(quint8 quote);

    QList<Output> mHeld;
    QList<OpenItem> mItems;
    quint8 mItemsQuote = 0;
    bool mInFence = false;
    QChar mFenceMarker;
    int mFenceLength = 0;
    int mFenceIndent = 0;
    QString mFenceLanguage;
    quint32 mMessage = 0;
    TChar mPlain;
    int mRuleWidth = 80;
};

// The Markdown state of the main console's buffer
class TMarkdown
{
public:
    TMarkdownParser mParser;
    TMarkdownMessages mMessages;
    // As the last sequence left it
    bool mOn = false;
    // As it stood at the first character of the line under way
    bool mLineInMarkdown = false;
    quint32 mLineMessage = 0;
    // The message the parser has open, 0 for none
    quint32 mOpenMessage = 0;

    quint32 store(TMarkdownBlock&& block);
    const TMarkdownBlock* block(quint32 id) const;
    void forget(quint32 id) { mBlocks.remove(id); }

private:
    QHash<quint32, TMarkdownBlock> mBlocks;
    quint32 mLastBlockId = 0;
};

#endif // MUDLET_TMARKDOWN_H
