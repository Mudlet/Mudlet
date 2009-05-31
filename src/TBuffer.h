/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
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




#ifndef TBUFFER_H
#define TBUFFER_H
#include <QPoint>
#include <QColor>
#include <QChar>
#include <QString>
#include <QStringList>
#include <deque>
#include <QTime>
#include "Host.h"

class Host;


class TChar
{
public:
           TChar();
           TChar( Host * );
           TChar( TChar const & copy );
    QColor fgColor;
    QColor bgColor;
    bool   italics;
    bool   bold;
    bool   underline;
};

class Host;


const QChar cLF = QChar('\n');
const QChar cSPACE = QChar(' ');

class TBuffer
{
public: 
    
    TBuffer( Host * pH );
    void append( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline );
    QPoint insert( QPoint &, QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline );
    bool insertInLine( QPoint & cursor, QString & what, TChar & format );
    void expandLine( int y, int count, TChar * pC );
    int wrap( int startLine, int screenWidth, int indentSize, TChar & format );
    int wrapLine( int startLine, int screenWidth, int indentSize, TChar & format );
    int size(){ return static_cast<int>(buffer.size()); }
    QString & line( int n );
    int find( int line, QString what, int pos );
    QStringList split( int line, QString splitter );
    QStringList split( int line, QRegExp splitter );
    bool replace( int line, QString what, QString with );
    bool replaceInLine( QPoint & start, QPoint & end, QString & with, TChar & format );
    bool deleteLine( int );
    bool deleteLines( int from, int to );
    bool applyFormat( QPoint &, QPoint &, TChar & format );
    bool applyFgColor( QPoint &, QPoint &, QColor & );
    bool applyBgColor( QPoint &, QPoint &, QColor & );
    void appendBuffer( TBuffer chunk );
    bool moveCursor( QPoint & where );
    QPoint & insertText( QString & what, QPoint & where );
    int getLastLineNumber();
    QStringList getEndLines( int );
    void clear();
    void resetFontSpecs();
    void translateToPlainText( QString & s );
    void append( QString & chunk, int sub_start, int sub_end, int, int, int, int, int, int, bool bold, bool italics, bool underline );
    int lookupColor( QString & s, int pos );
    void set_text_properties(int tag);
    void setWrapAt( int i ){ mWrapAt = i; }
    void setWrapIndent( int i ){ mWrapIndent = i; }
    void updateColors();
    TBuffer copy( QPoint &, QPoint & );
    TBuffer cut( QPoint &, QPoint & );
    void paste( QPoint &, TBuffer );
    std::deque<TChar *> bufferLine;
    std::deque< std::deque<TChar*> > buffer; 
    QStringList timeBuffer;
    QStringList lineBuffer;
    QList<QList<QColor> > fgColorBuffer;
    QList<QList<QColor> > bgColorBuffer;
    int mLinesLimit;
    int newLines;

    
private:  
    
    inline int calcWrapPos( int line, int begin, int end );
    void handleNewLine();
    
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
    int               mWrapAt;
    int               mWrapIndent;
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
};

#endif
