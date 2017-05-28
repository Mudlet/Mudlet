#ifndef MUDLET_TBUFFER_H
#define MUDLET_TBUFFER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017 by Stephen Lyons - slysven@virginmedia.com   *
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
#include <QChar>
#include <QColor>
#include <QMap>
#include <QPoint>
#include <QPointer>
#include <QString>
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
    static const QMap<QString, QVector<QChar>> csmEncodingTable; // private!


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
    QStringList split(int line, QRegExp splitter);
    bool replace(int line, const QString& what, const QString& with);
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
    void resetFontSpecs();
    QPoint getEndPos();
    void translateToPlainText(std::string& s, const bool isFromServer=false);
    void append(const QString& chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout, int linkID = 0);
    void appendLine(const QString& chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout, int linkID = 0);
    void setWrapAt(int i) { mWrapAt = i; }
    void setWrapIndent(int i) { mWrapIndent = i; }
    void updateColors();
    TBuffer copy(QPoint&, QPoint&);
    TBuffer cut(QPoint&, QPoint&);
    void paste(QPoint&, TBuffer);
    void setBufferSize(int s, int batch);
    static const QList<QString> getHardCodedEncodingTableKeys() {return csmEncodingTable.keys();}


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
    bool mMXP;

    bool mAssemblingToken;
    std::string currentToken;
    int openT;
    int closeT;
    QMap<QString, TMxpElement> mMXP_Elements;
    TMxpElement mCurrentElement;
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


    bool gotESC;
    bool gotHeader;
    QString code;
    int codeRet;
    std::string tempLine;
    bool mWaitingForHighColorCode;
    bool mHighColorModeForeground;
    bool mHighColorModeBackground;
    bool mIsHighColorMode;
    bool mIsDefaultColor;
    bool isUserScrollBack;
    int currentFgColorProperty;

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
    QColor mFgColor;
    int fgColorR;
    int fgColorLightR;
    int fgColorG;
    int fgColorLightG;
    int fgColorB;
    int fgColorLightB;
    int bgColorR;
    int bgColorG;
    int bgColorB;
    QColor mBgColor;

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
    bool mFgColorCode;
    bool mBgColorCode;
    int mFgColorR;
    int mFgColorG;
    int mFgColorB;
    int mBgColorR;
    int mBgColorG;
    int mBgColorB;
    QString mMudLine;
    std::deque<TChar> mMudBuffer;
    int mCode[1024]; //FIXME: potential overflow bug
    // Used to hold the incomplete bytes (1-3) that could be left at the end of
    // a packet:
    std::string mIncompleteUtf8SequenceBytes;
};

#endif // MUDLET_TBUFFER_H
