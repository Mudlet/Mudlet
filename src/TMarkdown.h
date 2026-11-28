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

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QQueue>
#include <QRgb>
#include <QSet>
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
    // A new message, numbered from 1. The two named, which a line under way or held lines may still be committed in,
    // keep their records; any other message nothing can reach any more does not.
    quint32 begin(const QString& id, TMarkdownMark mark, quint32 stillInUse, quint32 alsoStillInUse);
    // The message whose mark changed, or 0 when no message has that id
    quint32 change(const QString& id, TMarkdownMark mark);
    quint32 current() const { return mLastMessage; }
    TMarkdownMark mark(quint32 message) const { return mRecords.value(message).mark; }
    QString id(quint32 message) const { return mRecords.value(message).id; }
    // A new connection: no id it sends names an earlier message
    void forgetIds();
    void lineStored(quint32 message) { ++mLines[message]; }
    void lineRemoved(quint32 message);
    // While held, the message keeps its record whatever happens to its lines, as more are still being committed
    void hold(quint32 message) { mHeld.insert(message); }
    void release(quint32 message);

    // For the Markdown tests
    int recordCount() const { return static_cast<int>(mRecords.size()); }

    static constexpr int csmKeptIds = 256;

private:
    // A record goes once nothing can show it or reach it: no line, no id that names it, not being committed
    void retireIfUnused(quint32 message);

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
    QHash<quint32, int> mLines;
    QSet<quint32> mHeld;
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
    bool holdsLines() const { return mHavePendingRow || !mHeldRows.isEmpty(); }
    // The OSC 8 links in the rows held back, which are on no line yet
    QSet<int> heldLinkIds() const;
    void setRuleWidth(int columns) { mRuleWidth = columns; }
    // The colors of the characters the grid adds, such as a bullet
    void setPlainFormat(const TChar& format) { mPlain = format; }
    // The profile's setting for East Asian characters of ambiguous width, as wrapping reads it
    void setWideAmbiguous(bool wide) { mWideAmbiguous = wide; }

    static constexpr int csmMaxQuoteLevels = 8;
    static constexpr int csmMaxListLevels = 8;
    static constexpr int csmMaxTableColumns = 64;
    static constexpr int csmMaxSectionLevels = 4;
    // A table wider than this, in columns, is not padded, so a game cannot make every row thousands of spaces long
    static constexpr int csmMaxPaddedTableWidth = 1000;
    // How long a row waits for a delimiter row once the game has gone quiet, before it is shown as it is
    static constexpr int csmPendingRowWaitMs = 1000;
    static constexpr int csmMaxHeldRows = 500;

private:
    struct Text
    {
        QString text;
        std::vector<TChar> format;
        Text mid(int from, int to) const;
    };
    struct OpenItem
    {
        // One column past the marker, counted in spaces from the start of the line after its quote markers: the column a
        // line must be indented to to continue the item
        int textColumn = 0;
        // Of the grid's prefix, after the quote bars
        int prefixWidth = 0;
    };

    struct Row
    {
        TMarkdownBlock::Kind kind = TMarkdownBlock::Kind::TableRow;
        QList<Text> cells;
        // Of each cell, in the cell's own columns
        QList<QList<TMarkdownLink>> links;
    };

    Output make(TMarkdownBlock::Kind kind, quint8 quote, const QString& prefix, Text content) const;
    static QList<Text> splitRow(const Text& row);
    static bool delimiterRow(const QList<Text>& cells);
    Row renderedRow(TMarkdownBlock::Kind kind, QList<Text> cells) const;
    Output rowLine(const Row& row) const;
    // The table and a row waiting to be its header end, and what was held is returned to be committed
    QList<Output> endTable();
    QList<Output> takePendingRow();
    void holdRow(Row&& row, QList<Output>& out);
    // The summary line of the section just opened, with a label of the client's when the line after the opening gave none
    Output summary(const Text& content, bool given) const;
    Output blank(quint8 quote) const;
    void closeItemsBeyond(int column);
    bool closesFence(const QString& line) const;
    static QString quotePrefix(quint8 quote);
    // The columns the console draws the text in, which a wide character or a combining mark makes differ from its length
    int columns(const QString& text) const;

    bool mHavePendingRow = false;
    QElapsedTimer mPendingSince;
    // What the row waiting to be a header is shown as if no delimiter row comes: a paragraph, or the continuation of an item
    Output mPendingOutput;
    QList<Text> mPendingCells;
    bool mInTable = false;
    // Set once rows have been shown, after which later rows are padded to them
    bool mWidthsFixed = false;
    quint32 mTable = 0;
    QList<TMarkdownBlock::Align> mAlign;
    QList<int> mWidths;
    QList<Row> mHeldRows;
    // The sections open, innermost last, and how many more are open past the levels a line can be placed in
    QList<quint32> mSections;
    int mSectionsBeyond = 0;
    quint32 mLastSection = 0;
    bool mAwaitingSummary = false;
    bool mSectionStartsOpen = false;
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
    bool mWideAmbiguous = false;
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
    TMarkdownBlock* editableBlock(quint32 id) { return mBlocks.contains(id) ? &mBlocks[id] : nullptr; }
    void forget(quint32 id);

private:
    QHash<quint32, TMarkdownBlock> mBlocks;
    quint32 mLastBlockId = 0;
};

#endif // MUDLET_TMARKDOWN_H
