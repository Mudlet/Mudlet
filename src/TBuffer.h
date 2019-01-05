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

#define TCHAR_ITALICS 1
#define TCHAR_BOLD 2
#define TCHAR_UNDERLINE 4
#define TCHAR_INVERSE 8
#define TCHAR_ECHO 16
#define TCHAR_STRIKEOUT 32
// TODO: {Reserve for} use the next four bits for extensions...
// Next one is for ANSI CSI SGR Overline (53 on, 55 off)
//#define TCHAR_OVERLINE 64
// We currently use inverse for selected text, but will need reworking so that
// code fragments currently using TCHAR_INVERSE use TCHAR_SELECT instead and
// and then used TCHAR_INVERSE in new methods as per BOLD/ITALICS - at point
// when used to draw text for display we will need to XOR the two flags together...!
//#define TCHAR_SELECT 128
// Next one is for ANSI CSI SGR Slow blink < 2.5 Hz (5 on, 25 off)
//#define TCHAR_SLOWBLINK 256
// Next one is for ANSI CSI SGR Rapid blink >= 2.5 Hz (6 on, 25 off)), make it
// mutually exclusive with previous (and have priority...)
//#define TCHAR_FASTBLINK 512
// Convience for testing for both Blink types
//#define TCHAR_BLINKMASK 768

class Host;

class QTextCodec;

class TChar
{
public:
    TChar();
    TChar(int, int, int, int, int, int, bool, bool, bool, bool, int _link = 0);
    TChar(Host*);
    TChar(const TChar& copy);
    bool operator==(const TChar& c);


    int fgR;
    int fgG;
    int fgB;
    int bgR;
    int bgG;
    int bgB;
    unsigned flags;
    int link;
};

const QChar cLF = QChar('\n');
const QChar cSPACE = QChar(' ');

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
    int skipSpacesAtBeginOfLine(int i, int i2);
    void addLink(bool, const QString& text, QStringList& command, QStringList& hint, TChar format);
    QString bufferToHtml(QPoint P1, QPoint P2, bool allowedTimestamps, int spacePadding = 0);
    int size() { return static_cast<int>(buffer.size()); }
    QString& line(int n);
    int find(int line, const QString& what, int pos);
    int wrap(int);
    QStringList split(int line, const QString& splitter);
    QStringList split(int line, QRegularExpression splitter);
    bool replaceInLine(QPoint& start, QPoint& end, const QString& with, TChar& format);
    bool deleteLine(int);
    bool deleteLines(int from, int to);
    bool applyFormat(QPoint&, QPoint&, TChar& format);
    bool applyUnderline(QPoint& P_begin, QPoint& P_end, bool bold);
    bool applyBold(QPoint& P_begin, QPoint& P_end, bool bold);
    bool applyLink(QPoint& P_begin, QPoint& P_end, const QString& linkText, QStringList&, QStringList&);
    bool applyItalics(QPoint& P_begin, QPoint& P_end, bool bold);
    bool applyStrikeOut(QPoint& P_begin, QPoint& P_end, bool strikeout);
    bool applyFgColor(QPoint&, QPoint&, int, int, int);
    bool applyBgColor(QPoint&, QPoint&, int, int, int);
    void appendBuffer(const TBuffer& chunk);
    bool moveCursor(QPoint& where);
    int getLastLineNumber();
    QStringList getEndLines(int);
    void clear();
    QPoint getEndPos();
    void translateToPlainText(std::string& s, bool isFromServer=false);
    void append(const QString& chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout, int linkID = 0);
    void appendLine(const QString& chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout, int linkID = 0);
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
    int newLines;
    int mUntriggered;
    int mWrapAt;
    int mWrapIndent;
    int speedTP;
    int speedSequencer;
    int speedAppend;

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
    // This was called code but has been renamed to mCode as that member had
    // be refactored out and the name was available:
    QString mCode;
    bool mIsDefaultColor;
    bool isUserScrollBack;


    QColor mBlack;
    int mBlackR;
    int mBlackG;
    int mBlackB;
    QColor mLightBlack;
    int mLightBlackR;
    int mLightBlackG;
    int mLightBlackB;
    QColor mRed;
    int mRedR;
    int mRedG;
    int mRedB;
    QColor mLightRed;
    int mLightRedR;
    int mLightRedG;
    int mLightRedB;
    QColor mLightGreen;
    int mLightGreenR;
    int mLightGreenG;
    int mLightGreenB;
    QColor mGreen;
    int mGreenR;
    int mGreenG;
    int mGreenB;
    QColor mLightBlue;
    int mLightBlueR;
    int mLightBlueG;
    int mLightBlueB;
    QColor mBlue;
    int mBlueR;
    int mBlueG;
    int mBlueB;
    QColor mLightYellow;
    int mLightYellowR;
    int mLightYellowG;
    int mLightYellowB;
    QColor mYellow;
    int mYellowR;
    int mYellowG;
    int mYellowB;
    QColor mLightCyan;
    int mLightCyanR;
    int mLightCyanG;
    int mLightCyanB;
    QColor mCyan;
    int mCyanR;
    int mCyanG;
    int mCyanB;
    QColor mLightMagenta;
    int mLightMagentaR;
    int mLightMagentaG;
    int mLightMagentaB;
    QColor mMagenta;
    int mMagentaR;
    int mMagentaG;
    int mMagentaB;
    QColor mLightWhite;
    int mLightWhiteR;
    int mLightWhiteG;
    int mLightWhiteB;
    QColor mWhite;
    int mWhiteR;
    int mWhiteG;
    int mWhiteB;

    int mCurrentFgColorR;
    int mCurrentFgColorG;
    int mCurrentFgColorB;
    int mCurrentFgColorLightR;
    int mCurrentFgColorLightG;
    int mCurrentFgColorLightB;
    int mCurrentBgColorR;
    int mCurrentBgColorG;
    int mCurrentBgColorB;

    QPointer<Host> mpHost;
    int maxx;
    int maxy;
    bool hadLF;
    int mLastLine;
    bool mCursorMoved;

    QTime mTime;

    bool mBold;
    bool mItalics;
    bool mUnderline;
    bool mStrikeOut;








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

#endif // MUDLET_TBUFFER_H
