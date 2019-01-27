#ifndef MUDLET_TBUFFER_H
#define MUDLET_TBUFFER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2018 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "pre_guard.h"
#include <QApplication>
#include <QChar>
#include <QColor>
#include <QDebug>
#include <QMap>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringBuilder>
#include <QStringList>
#include <QTime>
#include <QVector>
#include "post_guard.h"

#include <deque>
#include <string>

class Host;

class QTextCodec;

class TChar
{
    friend class TBuffer;

public:
    enum AttributeFlag {
        None = 0x0,
        // Replaces TCHAR_BOLD 2
        Bold = 0x1,
        // Replaces TCHAR_ITALICS 1
        Italic = 0x2,
        // Replaces TCHAR_UNDERLINE 4
        Underline = 0x4,
        // New, TCHAR_OVERLINE had not been previous done, is
        // ANSI CSI SGR Overline (53 on, 55 off)
        Overline = 0x8,
        // Replaces TCHAR_STRIKEOUT 32
        StrikeOut = 0x10,
        // NOT a replacement for TCHAR_INVERSE, that is now covered by the
        // separate isSelected bool but they must be EX-ORed at the point of
        // painting the Character
        Reverse = 0x20,
        // The attributes that are currently user settable and what should be
        // consider in HTML generation:
        TestMask = 0x3f,
        // Replaces TCHAR_ECHO 16
        Echo = 0x100
    };
    Q_DECLARE_FLAGS(AttributeFlags, AttributeFlag)

    TChar();
    TChar(const QColor& fg, const QColor& bg, const TChar::AttributeFlags flags = TChar::None, const int linkIndex = 0);
    TChar(Host*);
    TChar(const TChar&);

    bool operator==(const TChar&);
    void setColors(const QColor& newForeGroundColor, const QColor& newBackGroundColor) {
        mFgColor=newForeGroundColor;
        mBgColor=newBackGroundColor;
    }
    // Only considers the following flags: Bold, Italic, Overline, Reverse,
    // Strikeout, Underline, does not consider Echo:
    void setAllDisplayAttributes(const AttributeFlags newDisplayAttributes) { mFlags = (mFlags & ~TestMask) | (newDisplayAttributes & TestMask); }
    void setForeground(const QColor& newColor) { mFgColor = newColor; }
    void setBackground(const QColor& newColor) { mBgColor = newColor; }
    void setTextFormat(const QColor& newFgColor, const QColor& newBgColor, const AttributeFlags newDisplayAttributes) {
        setColors(newFgColor, newBgColor);
        setAllDisplayAttributes(newDisplayAttributes);
    }

    const QColor& foreground() const { return mFgColor; }
    const QColor& background() const { return mBgColor; }
    AttributeFlags allDisplayAttributes() const { return mFlags & TestMask; }
    void select() { mIsSelected = true; }
    void deselect() { mIsSelected = false; }
    bool isSelected() const { return mIsSelected; }
    int linkIndex () const { return mLinkIndex; }

private:
    QColor mFgColor;
    QColor mBgColor;
    AttributeFlags mFlags;
    // Kept as a separate flag because it must often be handled separately
    bool mIsSelected;
    int mLinkIndex;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(TChar::AttributeFlags)

struct TMxpElement
{
    QString name;
    QString href;
    QString hint;
};

class TBuffer
{
    // need to use tr() on encoding names in csmEncodingTable
    Q_DECLARE_TR_FUNCTIONS(TBuffer)

    // private - a map of computer-friendly encoding names as keys,
    // values are a pair of human-friendly name + encoding data
    static const QMap<QString, QPair<QString, QVector<QChar>>> csmEncodingTable;

    static const QMap<QString, QVector<QString>> mSupportedMxpElements;


public:
    TBuffer(Host* pH);
    QPoint insert(QPoint&, const QString& text, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout);
    bool insertInLine(QPoint& cursor, const QString& what, TChar& format);
    void expandLine(int y, int count, TChar&);
    int wrapLine(int startLine, int screenWidth, int indentSize, TChar& format);
    void log(int, int);
    int skipSpacesAtBeginOfLine(const int row, const int column);
    void addLink(bool, const QString& text, QStringList& command, QStringList& hint, TChar format);
    QString bufferToHtml(const bool showTimeStamp = false, const int row = -1, const int endColumn = -1, const int startColumn = 0,  int spacePadding = 0);
    int size() { return static_cast<int>(buffer.size()); }
    QString& line(int n);
    int find(int line, const QString& what, int pos);
    int wrap(int);
    QStringList split(int line, const QString& splitter);
    QStringList split(int line, QRegularExpression splitter);
    bool replaceInLine(QPoint& start, QPoint& end, const QString& with, TChar& format);
    bool deleteLine(int);
    bool deleteLines(int from, int to);
    bool applyAttribute(const QPoint& P_begin, const QPoint& P_end, const TChar::AttributeFlags attributes, const bool state);
    bool applyLink(const QPoint& P_begin, const QPoint& P_end, const QStringList& linkFunction, const QStringList& linkHist);
    bool applyFgColor(const QPoint&, const QPoint&, const QColor&);
    bool applyBgColor(const QPoint&, const QPoint&, const QColor&);
    void appendBuffer(const TBuffer& chunk);
    bool moveCursor(QPoint& where);
    int getLastLineNumber();
    QStringList getEndLines(int);
    void clear();
    QPoint getEndPos();
    void translateToPlainText(std::string& s, bool isFromServer=false);
    void append(const QString& chunk, int sub_start, int sub_end, const QColor& fg, const QColor& bg, const TChar::AttributeFlags flags = TChar::None, const int linkID = 0);
    // Only the bits within TChar::TestMask are considered for formatting:
    void append(const QString& chunk, const int sub_start, const int sub_end, const TChar format, const int linkID = 0);
    void appendLine(const QString& chunk, const int sub_start, const int sub_end, const QColor& fg, const QColor& bg, TChar::AttributeFlags flags = TChar::None, const int linkID = 0);
    void setWrapAt(int i) { mWrapAt = i; }
    void setWrapIndent(int i) { mWrapIndent = i; }
    void updateColors();
    TBuffer copy(QPoint&, QPoint&);
    TBuffer cut(QPoint&, QPoint&);
    void paste(QPoint&, TBuffer);
    void setBufferSize(int s, int batch);
    static const QList<QString> getComputerEncodingNames() { return csmEncodingTable.keys(); }
    static const QList<QString> getFriendlyEncodingNames();
    static const QString& getComputerEncoding(const QString& encoding);
    void logRemainingOutput();
    // It would have been nice to do this with Qt's signals and slots but that
    // is apparently incompatible with using a default constructor - sigh!
    void encodingChanged(const QString &);


    std::deque<TChar> bufferLine;
    std::deque<std::deque<TChar>> buffer;
    QStringList timeBuffer;
    QStringList lineBuffer;
    QList<bool> promptBuffer;
    QList<bool> dirty;
    QMap<int, QStringList> mLinkStore;
    QMap<int, QStringList> mHintStore;
    int mLinkID;
    int mLinesLimit;
    int mBatchDeleteSize;
    int mWrapAt;
    int mWrapIndent;

    int mCursorY;

    /*
     * The documentation at https://www.zuggsoft.com/zmud/mxp.htm says: "
     * * 0 - OPEN LINE - initial default mode: only MXP commands in the 'open'
     *     category are allowed.  When a newline is received from the MUD, the
     *     mode reverts back to the Default mode.  OPEN mode starts as the
     *     default mode until changes with one of the 'lock mode' tags listed
     *     below.
     * * 1 - SECURE LINE (until next newline) all tags and commands in MXP are
     *     allowed within the line.  When a newline is received from the MUD,
     *     the mode reverts back to the Default mode.
     * * 2 - LOCKED LINE (until next newline) no MXP or HTML commands are
     *     allowed in the line.  The line is not parsed for any tags at all.
     *     This is useful for "verbatim" text output from the MUD.  When a
     *     newline is received from the MUD, the mode reverts back to the
     *     Default mode.
     * The following additional modes were added to the v0.4 MXP spec:
     * * 3 - RESET close all open tags.  Set mode to Open.  Set text color and
     *     properties to default.
     * * 4 - TEMP SECURE MODE set secure mode for the next tag only.  Must be
     *     immediately followed by a < character to start a tag.  Remember to
     *     set secure mode when closing the tag also.
     * * 5 - LOCK OPEN MODE set open mode.  Mode remains in effect until
     *     changed.  OPEN mode becomes the new default mode.
     * * 6 - LOCK SECURE MODE set secure mode.  Mode remains in effect until
     *     changed.  Secure mode becomes the new default mode.
     * * 7 - LOCK LOCKED MODE set locked mode.  Mode remains in effect until
     *     changed.  Locked mode becomes the new default mode."
     */

    // State of MXP systen:
    bool mMXP;

    bool mAssemblingToken;
    std::string currentToken;
    int openT;
    int closeT;

    QMap<QString, TMxpElement> mMXP_Elements;

    bool mMXP_LINK_MODE;
    bool mIgnoreTag;
    std::string mSkip;
    bool mParsingVar;
    char mOpenMainQuote;
    bool mMXP_SEND_NO_REF_MODE;
    std::string mAssembleRef;
    bool mEchoText;


private:
    void shrinkBuffer();
    int calcWrapPos(int line, int begin, int end);
    void handleNewLine();
    bool processUtf8Sequence(const std::string&, bool, size_t, size_t&, bool&);
    bool processGBSequence(const std::string&, bool, bool, size_t, size_t&, bool&);
    bool processBig5Sequence(const std::string&, bool, size_t, size_t&, bool&);
    QString processSupportsRequest(const QString &attributes);
    void decodeSGR(const QString&);
    void decodeSGR38(QStringList, bool isColonSeparated = true);
    void decodeSGR48(QStringList, bool isColonSeparated = true);
    void decodeOSC(const QString&);
    void resetColors();

    // First stage in decoding SGR/OCS sequences - set true when we see the
    // ASCII ESC character:
    bool mGotESC;
    // Second stage in decoding SGR sequences - set true when we see the ASCII
    // ESC character followed by the '[' one:
    bool mGotCSI;
    // Second stage in decoding OSC sequences - set true when we see the ASCII
    // ESC character followed by the ']' one:
    bool mGotOSC;
    bool mIsDefaultColor;


    QColor mBlack;
    QColor mLightBlack;
    QColor mRed;
    QColor mLightRed;
    QColor mLightGreen;
    QColor mGreen;
    QColor mLightBlue;
    QColor mBlue;
    QColor mLightYellow;
    QColor mYellow;
    QColor mLightCyan;
    QColor mCyan;
    QColor mLightMagenta;
    QColor mMagenta;
    QColor mLightWhite;
    QColor mWhite;
    // These three replace three sets of three integers that were used to hold
    // colour components during the parsing of SGR sequences, they were called:
    // fgColor{R|G|B}, fgColorLight{R|G|B} and bgColor{R|G|B} apart from
    // anything else, the first and last sets had the same names as arguments
    // to several of the methods which meant the latter shadowed and masked
    // them off!
    QColor mForeGroundColor;
    QColor mForeGroundColorLight;
    QColor mBackGroundColor;

    QPointer<Host> mpHost;

    bool mBold;
    bool mItalics;
    bool mOverline;
    bool mReverse;
    bool mStrikeOut;
    bool mUnderline;

    QString mMudLine;
    std::deque<TChar> mMudBuffer;
    // Used to hold the unprocessed bytes that could be left at the end of a
    // packet if we detect that there should be more - will be prepended to the
    // next chunk of data - PROVIDED it is flagged as coming from the MUD Server
    // and is not generated locally {because both pass through
    // translateToPlainText()}:
    std::string mIncompleteSequenceBytes;

    // keeps track of the previously logged buffer lines to ensure no log duplication
    // happens when you enter a command
    int lastLoggedFromLine;
    int lastloggedToLine;
    QString lastTextToLog;

    QString mEncoding;
    QTextCodec* mMainIncomingCodec;
};

#ifndef QT_NO_DEBUG_STREAM
// Dumper for the TChar::AttributeFlags - so that qDebug gives a detailed broken
// down results when presented with the value rather than just a hex value.
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const TChar::AttributeFlags& attributes)
{
    QDebugStateSaver saver(debug);
    QString result = QLatin1String("TChar::AttributeFlags(");
    QStringList presentAttributes;
    if (attributes & TChar::Bold) {
        presentAttributes << QLatin1String("Bold (0x01)");
    }
    if (attributes & TChar::Italic) {
        presentAttributes << QLatin1String("Italic (0x02)");
    }
    if (attributes & TChar::Underline) {
        presentAttributes << QLatin1String("Underline (0x04)");
    }
    if (attributes & TChar::Overline) {
        presentAttributes << QLatin1String("Overline (0x08)");
    }
    if (attributes & TChar::StrikeOut) {
        presentAttributes << QLatin1String("StrikeOut (0x10)");
    }
    if (attributes & TChar::Reverse) {
        presentAttributes << QLatin1String("Reverse (0x20)");
    }
    if (attributes & TChar::Echo) {
        presentAttributes << QLatin1String("Echo (0x100)");
    }
    result.append(presentAttributes.join(", "));
    result.append(QLatin1String(")"));
    debug.nospace() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TBUFFER_H
