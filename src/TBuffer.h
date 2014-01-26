/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
 *                                                                         *
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




#include <QPoint>
#include <QColor>
#include <QChar>
#include <QString>
#include <QStringList>
#include <deque>
#include <QTime>
#include "Host.h"
#include <QXmlStreamReader>

#ifndef TBUFFER_H
#define TBUFFER_H

class Host;


class TChar
{
public:
           TChar();
           TChar( int, int, int, int, int, int, bool, bool, bool, int _link = 0 );
           TChar( Host * );
           TChar( const TChar & copy );
    bool   operator==( const TChar & c );
    int    fgR;
    int    fgG;
    int    fgB;
    int    bgR;
    int    bgG;
    int    bgB;
    bool   italics;
    bool   bold;
    bool   underline;
    int    link;
    bool   invers;
};


class Host;


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
public:

    TBuffer( Host * pH );
    QPoint insert( QPoint &, QString text, int,int,int, int, int, int, bool bold, bool italics, bool underline );
    bool insertInLine( QPoint & cursor, QString & what, TChar & format );
    void expandLine( int y, int count, TChar & );
    int wrap( int startLine, int screenWidth, int indentSize, TChar & format );
    int wrapLine( int startLine, int screenWidth, int indentSize, TChar & format );
    void log( int, int );
    int skipSpacesAtBeginOfLine( int i, int i2 );
    void addLink( bool, QString & text, QStringList & command, QStringList & hint, TChar format );
//    void appendLink( QString & text,
//                     int sub_start,
//                     int sub_end,
//                     int fgColorR,
//                     int fgColorG,
//                     int fgColorB,
//                     int bgColorR,
//                     int bgColorG,
//                     int bgColorB,
//                     bool bold,
//                     bool italics,
//                     bool underline );
    QString bufferToHtml( QPoint P1, QPoint P2 );
    int size(){ return static_cast<int>(buffer.size()); }
    QString & line( int n );
    int find( int line, QString what, int pos );
    int wrap( int );
    QStringList split( int line, QString splitter );
    QStringList split( int line, QRegExp splitter );
    bool replace( int line, QString what, QString with );
    bool replaceInLine( QPoint & start, QPoint & end, QString & with, TChar & format );
    bool deleteLine( int );
    bool deleteLines( int from, int to );
    bool applyFormat( QPoint &, QPoint &, TChar & format );
    bool applyUnderline( QPoint & P_begin, QPoint & P_end, bool bold );
    bool applyBold( QPoint & P_begin, QPoint & P_end, bool bold );
    bool applyLink( QPoint & P_begin, QPoint & P_end, QString linkText, QStringList &, QStringList & );
    bool applyItalics( QPoint & P_begin, QPoint & P_end, bool bold );
    bool applyFgColor( QPoint &, QPoint &, int, int, int );
    bool applyBgColor( QPoint &, QPoint &, int, int, int );
    void appendBuffer( TBuffer chunk );
    bool moveCursor( QPoint & where );
    QPoint & insertText( QString & what, QPoint & where );
    int getLastLineNumber();
    QStringList getEndLines( int );
    void clear();
    void resetFontSpecs();
    QPoint getEndPos();
    void translateToPlainText( std::string & s );
    void append( QString & chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, int linkID=0 );
    void appendLine( QString & chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline, int linkID=0 );
    int lookupColor( QString & s, int pos );
    void set_text_properties(int tag);
    void setWrapAt( int i ){ mWrapAt = i; }
    void setWrapIndent( int i ){ mWrapIndent = i; }
    void updateColors();
    TBuffer copy( QPoint &, QPoint & );
    TBuffer cut( QPoint &, QPoint & );
    void paste( QPoint &, TBuffer );
    std::deque<TChar> bufferLine;
    std::deque< std::deque<TChar> > buffer;
    QStringList            timeBuffer;
    QStringList            lineBuffer;
    QList<bool>            promptBuffer;
    QList<bool>            dirty;
    QMap<int, QStringList> mLinkStore;
    QMap<int, QStringList> mHintStore;
    int                    mLinkID;
    int                    mLinesLimit;
    int                    mBatchDeleteSize;
    int                    newLines;
    int               mUntriggered;
    int               mWrapAt;
    int               mWrapIndent;
    void              setBufferSize( int s, int batch );
    void              messen();
    int               speedTP;
    int               speedSequencer;
    int               speedAppend;
    int               msLength;
    int               msPos;

    int               mCursorY;
    bool              mMXP;

    bool              mAssemblingToken;
    std::string       currentToken;
    int               openT;
    int               closeT;
    QMap<QString,TMxpElement *> mMXP_Elements;
    TMxpElement       mCurrentElement;
    bool              mMXP_LINK_MODE;
    bool              mIgnoreTag;
    std::string       mSkip;
    bool              mParsingVar;
    char              mOpenMainQuote;
    bool              mMXP_SEND_NO_REF_MODE;
    std::string       mAssembleRef;

private:
    inline void       shrinkBuffer();
    inline int        calcWrapPos( int line, int begin, int end );
    void              handleNewLine();
    bool              gotESC;
    bool              gotHeader;
    QString           code;
    int               codeRet;
    std::string       tempLine;
    bool              mWaitingForHighColorCode;
    bool              mHighColorModeForeground;
    bool              mHighColorModeBackground;
    bool              mIsHighColorMode;
    bool              mIsDefaultColor;
    bool              isUserScrollBack;
    int               currentFgColorProperty;
    QString           mFormatSequenceRest;
    QColor            mBlack;
    int               mBlackR;
    int               mBlackG;
    int               mBlackB;
    QColor            mLightBlack;
    int               mLightBlackR;
    int               mLightBlackG;
    int               mLightBlackB;
    QColor            mRed;
    int               mRedR;
    int               mRedG;
    int               mRedB;
    QColor            mLightRed;
    int               mLightRedR;
    int               mLightRedG;
    int               mLightRedB;
    QColor            mLightGreen;
    int               mLightGreenR;
    int               mLightGreenG;
    int               mLightGreenB;
    QColor            mGreen;
    int               mGreenR;
    int               mGreenG;
    int               mGreenB;
    QColor            mLightBlue;
    int               mLightBlueR;
    int               mLightBlueG;
    int               mLightBlueB;
    QColor            mBlue;
    int               mBlueR;
    int               mBlueG;
    int               mBlueB;
    QColor            mLightYellow;
    int               mLightYellowR;
    int               mLightYellowG;
    int               mLightYellowB;
    QColor            mYellow;
    int               mYellowR;
    int               mYellowG;
    int               mYellowB;
    QColor            mLightCyan;
    int               mLightCyanR;
    int               mLightCyanG;
    int               mLightCyanB;
    QColor            mCyan;
    int               mCyanR;
    int               mCyanG;
    int               mCyanB;
    QColor            mLightMagenta;
    int               mLightMagentaR;
    int               mLightMagentaG;
    int               mLightMagentaB;
    QColor            mMagenta;
    int               mMagentaR;
    int               mMagentaG;
    int               mMagentaB;
    QColor            mLightWhite;
    int               mLightWhiteR;
    int               mLightWhiteG;
    int               mLightWhiteB;
    QColor            mWhite;
    int               mWhiteR;
    int               mWhiteG;
    int               mWhiteB;
    QColor            mFgColor;
    int               fgColorR;
    int               fgColorLightR;
    int               fgColorG;
    int               fgColorLightG;
    int               fgColorB;
    int               fgColorLightB;
    int               bgColorR;
    int               bgColorG;
    int               bgColorB;
    QColor            mBgColor;

    Host *            mpHost;
    int               maxx;
    int               maxy;
    bool              hadLF;
    int               mLastLine;
    bool              mCursorMoved;

    QTime             mTime;

    bool              mBold;
    bool              mItalics;
    bool              mUnderline;
    bool              mFgColorCode;
    bool              mBgColorCode;
    int               mFgColorR;
    int               mFgColorG;
    int               mFgColorB;
    int               mBgColorR;
    int               mBgColorG;
    int               mBgColorB;
    QString           mMudLine;
    std::deque<TChar> mMudBuffer;
    int               mCode[1024];//FIXME: potential overflow bug
};

#endif
