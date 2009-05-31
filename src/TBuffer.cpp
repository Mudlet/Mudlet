/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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
//#define NDEBUG
#include <assert.h>
#include <QDebug>
#include <stdio.h>
#include <iostream>
#include <string>
#include <deque>
#include <queue>
#include "TBuffer.h"
#include "Host.h"
#include "TConsole.h"


using namespace std;

class Host;

const QColor TCharDefaultFgColor = QColor(255,255,255);
const QColor TCharDefaultBgColor = QColor(0,0,0); 


TChar::TChar()
{
    fgColor = QColor(255,255,255);
    bgColor = QColor(0,0,0);
    italics = false;
    bold = false;
    underline = false;
}


TChar::TChar( Host * pH )
{
    if( pH )
    {
        fgColor = pH->mFgColor;
        bgColor = pH->mBgColor;
    }
    else
    {
        
        fgColor = TCharDefaultFgColor;
        bgColor = TCharDefaultBgColor;   
    }
    italics = false;
    bold = false;
    underline = false;    
}


TChar::TChar( TChar const & copy )
{
    fgColor = copy.fgColor;
    bgColor = copy.bgColor;
    italics = copy.italics;
    bold = copy.bold;
    underline = copy.underline;     
}


TBuffer::TBuffer( Host * pH )
: mLinesLimit( 100000 )
, mpHost( pH )
, mCursorMoved( false )
, mWrapAt( 100 )
, mWrapIndent( 0 )
, mFormatSequenceRest( QString("") )
, mBold( false )
, mItalics( false )
, mUnderline( false )
, mFgColorCode( 0 )
, mBgColorCode( 0 )
, mBlack             ( pH->mBlack )
, mLightBlack        ( pH->mLightBlack )
, mRed               ( pH->mRed )
, mLightRed          ( pH->mLightRed )
, mLightGreen        ( pH->mLightGreen )
, mGreen             ( pH->mGreen )
, mLightBlue         ( pH->mLightBlue )
, mBlue              ( pH->mBlue )
, mLightYellow       ( pH->mLightYellow )
, mYellow            ( pH->mYellow )
, mLightCyan         ( pH->mLightCyan )
, mCyan              ( pH->mCyan )
, mLightMagenta      ( pH->mLightMagenta )
, mMagenta           ( pH->mMagenta )
, mLightWhite        ( pH->mLightWhite )
, mWhite             ( pH->mWhite )
, mFgColor           ( pH->mFgColor )
, mBgColor           ( pH->mBgColor )
{   
    buffer.clear();
    lineBuffer.clear();
    newLines = 0;
    mLastLine = 0;
    mTime = QTime::currentTime();
    mTime.start();
}

void TBuffer::resetFontSpecs()
{
    fgColorR = mFgColorR;
    fgColorG = mFgColorG;
    fgColorB = mFgColorB;
    bgColorR = mBgColorR;
    bgColorG = mBgColorG;
    bgColorB = mBgColorB;
    mBold = false;
    mItalics = false;
    mUnderline = false;
}

void TBuffer::updateColors()
{
    Host * pH = mpHost;
    mBlack = pH->mBlack;
    mBlackR=mBlack.red();
    mBlackG=mBlack.green();
    mBlackB=mBlack.blue();
    mLightBlack = pH->mLightBlack;
    mLightBlackR = mLightBlack.red();
    mLightBlackG=mLightBlack.green();
    mLightBlackB=mLightBlack.blue();
    mRed = pH->mRed;
    mRedR=mRed.red();
    mRedG=mRed.green();
    mRedB=mRed.blue();
    mLightRed = pH->mLightRed;
    mLightRedR=mLightRed.red();
    mLightRedG=mLightRed.green();
    mLightRedB=mLightRed.blue();
    mLightGreen = pH->mLightGreen;
    mLightGreenR=mLightGreen.red();
    mLightGreenG=mLightGreen.green();
    mLightGreenB=mLightGreen.blue();
    mGreen = pH->mGreen;
    mGreenR=mGreen.red();
    mGreenG=mGreen.green();
    mGreenB=mGreen.blue();
    mLightBlue = pH->mLightBlue;
    mLightBlueR=mLightBlue.red();
    mLightBlueG=mLightBlue.green();
    mLightBlueB=mLightBlue.blue();
    mBlue = pH->mBlue;
    mBlueR=mBlue.red();
    mBlueG=mBlue.green();
    mBlueB=mBlue.blue();
    mLightYellow  = pH->mLightYellow;
    mLightYellowR=mLightYellow.red();
    mLightYellowG=mLightYellow.green();
    mLightYellowB=mLightYellow.blue();
    mYellow = pH->mYellow;
    mYellowR=mYellow.red();
    mYellowG=mYellow.green();
    mYellowB=mYellow.blue();
    mLightCyan = pH->mLightCyan;
    mLightCyanR=mLightCyan.red();
    mLightCyanG=mLightCyan.green();
    mLightCyanB=mLightCyan.blue();
    mCyan = pH->mCyan;
    mCyanR=mCyan.red();
    mCyanG=mCyan.green();
    mCyanB=mCyan.blue();
    mLightMagenta = pH->mLightMagenta;
    mLightMagentaR=mLightMagenta.red();
    mLightMagentaG=mLightMagenta.green();
    mLightMagentaB=mLightMagenta.blue();
    mMagenta = pH->mMagenta;
    mMagentaR=mMagenta.red();
    mMagentaG=mMagenta.green();
    mMagentaB=mMagenta.blue();
    mLightWhite = pH->mLightWhite;
    mLightWhiteR=mLightWhite.red();
    mLightWhiteG=mLightWhite.green();
    mLightWhiteB=mLightWhite.blue();
    mWhite = pH->mWhite;
    mWhiteR=mWhite.red();
    mWhiteG=mWhite.green();
    mWhiteB=mWhite.blue();
    mFgColor = pH->mFgColor;
    mBgColor = pH->mBgColor;
    mFgColorR = mFgColor.red();
    mFgColorG = mFgColor.green();
    mFgColorB = mFgColor.blue();
    mBgColorR = mBgColor.red();
    mBgColorG = mBgColor.green();
    mBgColorB = mBgColor.blue();
}

int TBuffer::getLastLineNumber()
{
    if( static_cast<int>(buffer.size()) > 0 )
    {
        return static_cast<int>(buffer.size())-1;
    }
    else
    {
        return -1;
    }
}

int speedTP;

inline void TBuffer::set_text_properties(int tag)
{
    //qDebug()<<"tag="<<tag;
    // are we dealing with 256 color mode enabled servers or standard ANSI colors?
  //  QTime speed;
    //speed.start();

    if( mWaitingForHighColorCode )
    {
        if( mHighColorModeForeground )
        {
            if( tag < 16 )
            {
                mHighColorModeForeground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16; // because color 1-15 behave like normal ANSI colors
                // 6x6 RGB color space
                int r = tag / 36;
                int g = (tag-(r*36)) / 6;
                int b = (tag-(r*36))-(g*6);
                fgColorR = r*42;
                fgColorG = g*42;
                fgColorB = b*42;
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                fgColorR = tag*10;
                fgColorG = tag*10;
                fgColorB = tag*10;
            }
            mHighColorModeForeground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;
//speedTP+=speed.elapsed();
            return;
        }
        if( mHighColorModeBackground )
        {
            if( tag < 16 )
            {
                mHighColorModeBackground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16;
                int r = tag / 36;
                int g = (tag-(r*36)) / 6;
                int b = (tag-(r*36))-(g*6);
                bgColorR = r*42;
                bgColorG = g*42;
                bgColorB = b*42;
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                fgColorR = tag*10;
                fgColorG = tag*10;
                fgColorB = tag*10;
            }
            mHighColorModeBackground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;
//speedTP+=speed.elapsed();
            return;
        }
    }

    if( tag == 38 )
    {
        mIsHighColorMode = true;
        mHighColorModeForeground = true;
//speedTP+=speed.elapsed();
        return;
    }
    if( tag == 48 )
    {
        mIsHighColorMode = true;
        mHighColorModeBackground = true;
    }
    if( ( mIsHighColorMode ) && ( tag == 5 ) )
    {
        mWaitingForHighColorCode = true;
//speedTP+=speed.elapsed();
        return;
    }

    // we are dealing with standard ANSI colors
NORMAL_ANSI_COLOR_TAG:

    switch( tag )
    {
    case 0:
        mHighColorModeForeground = false;
        mHighColorModeBackground = false;
        mWaitingForHighColorCode = false;
        mIsHighColorMode = false;
        mIsDefaultColor = true;
        resetFontSpecs();
        break;
    case 1:
        mBold = true;
        break;
    case 2:
        mBold = false;
        break;
    case 3:
        mItalics = true;
        break;
    case 4:
        mUnderline = true;
    case 5:
        break; //FIXME support blinking
    case 6:
        break; //FIXME support fast blinking
    case 7:
        break; //FIXME support inverse
    case 9:
        break; //FIXME support strikethrough
    case 22:
        mBold = false;
        break;
    case 23:
        mItalics = false;
        break;
    case 24:
        mUnderline = false;
        break;
    case 27:
        break; //FIXME inverse off
    case 29:
        break; //FIXME
    case 30:
        fgColorR = mBlackR;
        fgColorG = mBlackG;
        fgColorB = mBlackB;
        fgColorLightR = mLightBlackR;
        fgColorLightG = mLightBlackG;
        fgColorLightB = mLightBlackB;
        mIsDefaultColor = false;
        break;
    case 31:
        fgColorR = mRedR;
        fgColorG = mRedR;
        fgColorB = mRedB;
        fgColorLightR = mLightRedR;
        fgColorLightG = mLightRedG;
        fgColorLightB = mLightRedB;
        mIsDefaultColor = false;
        break;
    case 32:
        fgColorR = mGreenR;
        fgColorG = mGreenG;
        fgColorB = mGreenB;
        fgColorLightR = mLightGreenR;
        fgColorLightG = mLightGreenR;
        fgColorLightB = mLightGreenB;
        mIsDefaultColor = false;
        break;
    case 33:
        fgColorR = mYellowR;
        fgColorG = mYellowG;
        fgColorB = mYellowB;
        fgColorLightR = mLightYellowR;
        fgColorLightG = mLightYellowG;
        fgColorLightB = mLightYellowB;
        mIsDefaultColor = false;
        break;
    case 34:
        fgColorR = mBlueR;
        fgColorG = mBlueG;
        fgColorB = mBlueB;
        fgColorLightR = mLightBlueR;
        fgColorLightG = mLightBlueG;
        fgColorLightB = mLightBlueB;
        mIsDefaultColor = false;
        break;
    case 35:
        fgColorR = mMagentaR;
        fgColorG=mMagentaG;
        fgColorB=mMagentaB;
        fgColorLightR=mLightMagentaR;
        fgColorLightG=mLightMagentaG;
        fgColorLightB=mLightMagentaB;
        mIsDefaultColor = false;
        break;
    case 36:
        fgColorR = mCyanR;
        fgColorG = mCyanG;
        fgColorB = mCyanB;
        fgColorLightR = mLightCyanR;
        fgColorLightG = mLightCyanG;
        fgColorLightB = mLightCyanB;
        mIsDefaultColor = false;
        break;
    case 37:
        fgColorR = mWhiteR;
        fgColorG = mWhiteG;
        fgColorB = mWhiteB;
        fgColorLightR = mLightWhiteR;
        fgColorLightG = mLightWhiteG;
        fgColorLightB = mLightWhiteB;
        mIsDefaultColor = false;
        break;
    case 39:
        bgColorR = mBgColorR;
        bgColorG = mBgColorG;
        bgColorB = mBgColorB;
        break;
    case 40:
        bgColorR = mBlackR;
        bgColorG = mBlackG;
        bgColorB = mBlackB;
        break;
    case 41:
        bgColorR = mRedR;
        bgColorG = mRedG;
        bgColorB = mRedB;
        break;
    case 42:
        bgColorR = mGreenR;
        bgColorG = mGreenG;
        bgColorB = mGreenB;
        break;
    case 43:
        bgColorR = mYellowR;
        bgColorG = mYellowG;
        bgColorB = mYellowB;
        break;
    case 44:
        bgColorR = mBlueR;
        bgColorG = mBlueG;
        bgColorB = mBlueB;
        break;
    case 45:
        bgColorR = mMagentaR;
        bgColorG = mMagentaG;
        bgColorB = mMagentaB;
        break;
    case 46:
        bgColorR = mCyanR;
        bgColorG = mCyanG;
        bgColorB = mCyanB;
        break;
    case 47:
        bgColorR = mWhiteR;
        bgColorG = mWhiteG;
        bgColorB = mWhiteB;
        break;
    };
//speedTP+=speed.elapsed();
}


/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint
      3 italics on
      4 underline on
      5 blink slow
      6 blink fast
      7 inverse on
      9 strikethrough
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      27 inverse off
      28 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 bg default white
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg black

      sequences for 256 Color support:
      38;5;0-256 foreground color
      48;5;0-256 background color */

int speedSequencer;
int speedAppend;

const QChar cESC = '\033';
const QString cDigit = "0123456789";
int msLength;
int msPos;
int mCode[3];

inline int TBuffer::lookupColor( QString & s, int pos )
{
    //QTime speed;
    //speed.start();

    int ret = 0;
    QString code;

    if( s.indexOf('[', pos ) == -1 )
    {
        msPos = pos-1;
        //speedSequencer+=speed.elapsed();
        return -1;
    }
    else
        pos++;


    while( pos < msLength )
    {
        int digit = cDigit.indexOf( s[pos] );
        if( digit > -1 )
        {
            code.append( s[pos] );
            pos++;
            continue;
        }
        else if( s[pos] == ';' )
        {
            ret++;
            mCode[ret] = code.toInt();
            pos++;
            code.clear();
            continue;
        }
        else if( s[pos] == 'm' )
        {
            ret++;
            mCode[ret] = code.toInt();
            /*cout << "color<";
            for(int i=1; i< ret+1; i++)
                cout << mCode[i]<<";";
            cout << ">"<<endl;*/
            msPos = ++pos;
            //speedSequencer+=speed.elapsed();
            return ret;
        }
        else
        {
            if( digit == -1 )
            {
                //qDebug()<<"sequence ERROR --> ignoring";
                //speedSequencer+=speed.elapsed();
                return 0; // unbekannte sequenz
            }
            else
            {
                //qDebug()<<"sequence im naechsten paket";
                //speedSequencer+=speed.elapsed();
                return -1; // unbeendete sequenz
            }
        }
    }
}

void TBuffer::translateToPlainText( QString & s )
{
    speedAppend = 0;
    speedTP = 0;
    speedSequencer = 0;
    //QTime speed;

    if( mFormatSequenceRest.size() > 0 )
    {
        //qDebug()<<"#### prepending<"<<mFormatSequenceRest<<"> to s=<"<<s<<">";
        s.prepend( mFormatSequenceRest );
        mFormatSequenceRest.clear();
    }
    msLength = s.length();
    mFormatSequenceRest="";
    int sequence_begin = 0;
    int sequence_end = 0;
    msPos = 0;
    std::list<int> posList;
    int pos = -1;
    //QTime speedESC;
//speedESC.start();

    while( (pos = s.indexOf( cESC, pos+1 )) > -1 )
    {
        //qDebug()<<"adding esc-seq at pos:"<<pos;
        posList.push_back( pos );
    }
//qDebug()<<"ESC-parser: "<<speedESC.elapsed();
    if( posList.size() == 0 )
    {
        //qDebug()<<"posList.size()==0 appending everything -> no control codes";
        append( s, 0, msLength, fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, mBold, mItalics, mUnderline );
      //  qDebug()<<"translate: "<<speed.elapsed()-speedAppend-speedSequencer<<" append: "<<speedAppend<<" sequencer: "<<speedSequencer;
        return;
    }
    if( posList.front() > 0 )
    {
        //qDebug()<<"####danger:: unsafe append call in translate -- append assembeled sequence from 2 packets";
        append( s, 0, posList.front(), fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, mBold, mItalics, mUnderline );
    }
//speed.start();
    typedef std::list<int>::const_iterator IT;
    for( IT it=posList.begin(); it!=posList.end();  )
    {
        int val = *it;
        int nextVal;
        if( ++it!= posList.end() )
        {
            nextVal = *it;
        }
        else
            nextVal = -1;

        int highCode = lookupColor( s , val+1 );
        if( highCode > 0 )
        {
            for( int i=1; i<highCode+1; i++ )
            {
                set_text_properties( mCode[i] );
            }

            if( nextVal == -1 )
            {
                if( ! mBold || mIsDefaultColor )
                {
                    append( s,
                            msPos,
                            msLength-msPos,
                            fgColorR,
                            fgColorG,
                            fgColorB,
                            bgColorR,
                            bgColorG,
                            bgColorB,
                            mIsDefaultColor ? mBold : false,
                            mItalics,
                            mUnderline );
                }
                else
                {
                    append( s,
                            msPos,
                            msLength-msPos,
                            fgColorLightR,
                            fgColorLightG,
                            fgColorLightB,
                            bgColorR,
                            bgColorG,
                            bgColorB,
                            false,
                            mItalics,
                            mUnderline );
                }
  //              qDebug()<<"translate: "<<speed.elapsed()-speedAppend-speedSequencer-speedTP<<" append: "<<speedAppend<<" sequencer: "<<speedSequencer<<" TP: "<<speedTP;
                return;
            }
            else
            {
                if( ! mBold || mIsDefaultColor )
                {
                    append( s,
                            msPos,
                            nextVal-msPos,
                            fgColorR,
                            fgColorG,
                            fgColorB,
                            bgColorR,
                            bgColorG,
                            bgColorB,
                            mIsDefaultColor ? mBold : false,
                            mItalics,
                            mUnderline );
                }
                else
                {
                    append( s,
                            msPos,
                            nextVal-msPos,
                            fgColorLightR,
                            fgColorLightG,
                            fgColorLightB,
                            bgColorR,
                            bgColorG,
                            bgColorB,
                            false,
                            mItalics,
                            mUnderline );
                }
                continue;
            }
        }
        else if( highCode == -1 )
        {
            // sequence_end is in next TCP/IPpacket keep translation state
            mFormatSequenceRest = s.mid( sequence_begin, -1 );
//qDebug()<<"tranlate() adding mFormatSequenceRest <"<<mFormatSequenceRest<<">";
    //        qDebug()<<"translate: "<<speed.elapsed()-speedAppend-speedSequencer-speedTP<<" append: "<<speedAppend<<" sequencer: "<<speedSequencer<<" TP: "<<speedTP;
            return;
        }
    }//for
    //qDebug()<<"translate: "<<speed.elapsed()-speedAppend-speedSequencer-speedTP<<" append: "<<speedAppend<<" sequencer: "<<speedSequencer<<" TP: "<<speedTP;
}



inline void TBuffer::append( QString & text,
                             int sub_start,
                             int sub_end,
                             int fgColorR,
                             int fgColorG,
                             int fgColorB,
                             int bgColorR,
                             int bgColorG,
                             int bgColorB,
                             bool bold,
                             bool italics,
                             bool underline )
{
    //QTime speed;
    //speed.start();

    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        while( static_cast<int>(buffer.size()) > mLinesLimit-10000 )
        {
            deleteLine( 0 );
        }
    }
//qDebug()<<"append()<"<<text.mid(sub_start, sub_end);
    bool runOncePerLine = true;
    for( int i=sub_start; i<sub_start+sub_end; i++ )
    {
        int last = buffer.size()-1;
        if( last < 0 )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = QColor(fgColorR,fgColorG,fgColorB);    // make the <LF>-marker invisible
            pC->bgColor = QColor(bgColorR,bgColorG,bgColorB);
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            QList<QColor> fgColorList;
            QList<QColor> bgColorList;
            fgColorBuffer << fgColorList;
            bgColorBuffer << bgColorList;
            last = 0;
        }
        if( mCursorMoved )
        {
            if(lineBuffer[last].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[last].at( 0 ) == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        lineBuffer[last].replace( 0, 1, text.at( i ) );
                        TChar * pC = buffer[last][0];
                        pC->fgColor = QColor(fgColorR,fgColorG,fgColorB);
                        pC->bgColor = QColor(bgColorR,bgColorG,bgColorB);
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        //timeBuffer[last] = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[last].append( text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = QColor(fgColorR,fgColorG,fgColorB);
            pC->bgColor = QColor(bgColorR,bgColorG,bgColorB);
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[last].push_back( pC );
            if( runOncePerLine )
            {
                bool doublicate = false;
                for( int k=0; k<fgColorBuffer[last].size(); k++ )
                {
                    if( fgColorBuffer[last][k].red() == fgColorR )
                    {
                        if( fgColorBuffer[last][k].green() == fgColorG )
                        {
                            if( fgColorBuffer[last][k].blue() == fgColorB )
                            {
                                doublicate = true;
                                break;
                            }
                        }
                    }
                }
                if( ! doublicate )
                {
                    fgColorBuffer[last].append( QColor(fgColorR, fgColorG, fgColorB) );
                }
                doublicate = false;
                for( int k=0; k<bgColorBuffer[last].size(); k++ )
                {
                    if( bgColorBuffer[last][k].red() == bgColorR )
                    {
                        if( bgColorBuffer[last][k].green() == bgColorG )
                        {
                            if( bgColorBuffer[last][k].blue() == bgColorB )
                            {
                                doublicate = true;
                                break;
                            }
                        }
                    }
                }
                if( ! doublicate )
                {
                    bgColorBuffer[last] << QColor(bgColorR, bgColorG, bgColorB );
                }
                runOncePerLine = false;
            }
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->bgColor = QColor(fgColorR,fgColorG,fgColorB);
            pC->fgColor = QColor(bgColorR,bgColorG,bgColorB);
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            QString time = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            timeBuffer << time;
            QList<QColor> fgColorList;
            QList<QColor> bgColorList;
            fgColorBuffer << fgColorList;
            bgColorBuffer << bgColorList;
            mLastLine++;
            newLines++;
            mCursorMoved = true;
            runOncePerLine = true;
        }
    }
    //speedAppend+=speed.elapsed();
}


inline void TBuffer::append( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
//    qDebug()<<"calling bad append text=<"<<text<<">";
    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        while( static_cast<int>(buffer.size()) > mLinesLimit-10000 )
        {
            deleteLine( 0 );
        }
    }

    bool runOncePerLine = true;
    for( int i=0; i<text.size(); i++ )
    {
        int last = buffer.size()-1;
        if( last < 0 )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            QList<QColor> fgColorList;
            QList<QColor> bgColorList;
            fgColorBuffer << fgColorList;
            bgColorBuffer << bgColorList;
            last = 0;
        }
        if( mCursorMoved ) 
        {
            if(lineBuffer[last].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[last].at( 0 ) == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        lineBuffer[last].replace( 0, 1, text.at( i ) );
                        TChar * pC = buffer[last][0];
                        pC->fgColor = fgColor;
                        pC->bgColor = bgColor;
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        timeBuffer[last] = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[last].append( text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[last].push_back( pC );
            if( runOncePerLine )
            {
                bool doublicate = false;
                for( int k=0; k<fgColorBuffer[last].size(); k++ )
                {
                    if( fgColorBuffer[last][k].red() == fgColor.red() )
                    {
                        if( fgColorBuffer[last][k].green() == fgColor.green() )
                        {
                            if( fgColorBuffer[last][k].blue() == fgColor.blue() )
                            {
                                doublicate = true;
                                break;
                            }
                        }
                    }
                }
                if( ! doublicate )
                {
                    fgColorBuffer[last].append( fgColor );
                }
                doublicate = false;
                for( int k=0; k<bgColorBuffer[last].size(); k++ )
                {
                    if( bgColorBuffer[last][k].red() == bgColor.red() )
                    {
                        if( bgColorBuffer[last][k].green() == bgColor.green() )
                        {
                            if( bgColorBuffer[last][k].blue() == bgColor.blue() )
                            {
                                doublicate = true;
                                break;
                            }
                        }
                    }
                }
                if( ! doublicate )
                {
                    bgColorBuffer[last] << bgColor;
                }
                runOncePerLine = false;
            }
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            QString time = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            timeBuffer << time;
            QList<QColor> fgColorList;
            QList<QColor> bgColorList;
            fgColorBuffer << fgColorList;
            bgColorBuffer << bgColorList;
            mLastLine++;
            newLines++;
            mCursorMoved = true;
            runOncePerLine = true;
        }
    }
}

QPoint TBuffer::insert( QPoint & where, QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
    QPoint P(-1, -1);
    
    int x = where.x();
    int y = where.y();
    
    if( y < 0 ) return P;
    if( y >= static_cast<int>(buffer.size()) ) return P;
    
    
    for( int i=0; i<text.size(); i++ )
    {
        if( mCursorMoved ) 
        {
            if(lineBuffer[y].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[y][0] == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        x = 0;
                        lineBuffer[y].replace( 0, 1, text.at( i ) );
                        TChar * pC = new TChar;
                        pC->fgColor = fgColor;
                        pC->bgColor = bgColor;
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        buffer[y].push_back( pC );
                        buffer[y].pop_front();
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[y].insert( x, text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            typedef std::deque<TChar *>::iterator IT;
            IT it = buffer[y].begin();
            buffer[y].insert( it+x, pC );
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "-   ";
            fgColorBuffer << QList<QColor>();
            bgColorBuffer << QList<QColor>();
            mLastLine++;
            newLines++;
            x = 0;
            y++;
            mCursorMoved = true;
        }
    }
    P.setX( x );
    P.setY( y );
    return P;
}


bool TBuffer::insertInLine( QPoint & P, QString & text, TChar & format )
{
    if( text.size() < 1 ) return false;
    int x = P.x();
    int y = P.y();
    if( ( y > 0 ) && ( y <= (int)buffer.size()-1 ) )
    {
        if( x < 0 )
        {
            return false;
        }
        if( x >= static_cast<int>(buffer[y].size()) )
        {
            TChar c;
            expandLine( y, x-buffer[y].size(), & c );
        }
        for( int i=0; i<text.size(); i++ )
        {
            lineBuffer[y].insert( x+i, text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = format.fgColor;
            pC->bgColor = format.bgColor;
            pC->italics = format.italics;
            pC->bold = format.bold;
            pC->underline = format.underline;
            typedef std::deque<TChar *>::iterator IT;
            IT it = buffer[y].begin();
            buffer[y].insert( it+x+i, pC );
        }   
    }
    return true;
}

TBuffer TBuffer::copy( QPoint & P1, QPoint & P2 )
{
    TBuffer slice( mpHost );
    slice.clear();
    int y = P1.y();
    int x = P1.x();
    if( y < 0 || y >= static_cast<int>(buffer.size()) )
        return slice;
    
    if( ( x < 0 ) 
        || ( x >= static_cast<int>(buffer[y].size()) )
        || ( P2.x() < 0 ) 
        || ( P2.x() > static_cast<int>(buffer[y].size()) ) )
    {
        x=0;
    }
    for( ; x<P2.x(); x++ )
    {
        if( lineBuffer[y][x] == QChar( 0x21af ) )
        {
            continue;
        }
        slice.append(QString(lineBuffer[y][x]), 
                     buffer[y][x]->fgColor, 
                     buffer[y][x]->bgColor, 
                     (buffer[y][x]->bold == true), 
                     (buffer[y][x]->italics == true), 
                     (buffer[y][x]->underline == true) );
    }
    return slice;
}

TBuffer TBuffer::cut( QPoint & P1, QPoint & P2 )
{
    TBuffer slice = copy( P1, P2 );
    QString nothing = "";
    TChar format;
    replaceInLine( P1, P2, nothing, format );
    return slice;
}

void TBuffer::paste( QPoint & P, TBuffer chunk )
{
    bool needAppend = false;
    bool hasAppended = false;
    int y = P.y();
    int x = P.x();
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    if( y < 0 || y > getLastLineNumber() )
    {
        y = getLastLineNumber();
    }
    if( y == -1 )
    {
        needAppend = true;
    }
    else
    {
        if( x < 0 || x >= static_cast<int>(buffer[y].size()) )
        {
            return;
        }
    }
    for( int cx=0; cx<static_cast<int>(chunk.buffer[0].size()); cx++ )
    {
        QPoint P_current(cx, y);
        if( chunk.buffer[0][cx] )
        {
            if( ( y < getLastLineNumber() ) && ( ! needAppend ) )
            {
                TChar format;
                format.fgColor = chunk.buffer[0][cx]->fgColor;
                format.bgColor = chunk.buffer[0][cx]->bgColor;
                format.bold = chunk.buffer[0][cx]->bold;
                format.italics = chunk.buffer[0][cx]->italics;
                format.underline = chunk.buffer[0][cx]->underline;
                QString s = QString(chunk.lineBuffer[0][cx]);
                insertInLine( P_current, s, format );
            }
            else
            {
                hasAppended = true;
                append(QString(chunk.lineBuffer[0][cx]),
                       chunk.buffer[0][cx]->fgColor,
                       chunk.buffer[0][cx]->bgColor,
                       (chunk.buffer[0][cx]->bold == true),
                       (chunk.buffer[0][cx]->italics == true),
                       (chunk.buffer[0][cx]->underline == true) );
            }
        }
    }
    if( hasAppended )
    {
        TChar format;
        if( y == -1 )
        {
            wrap( y,mWrapAt, mWrapIndent, format );
        }
        else
        {
            wrapLine( y, mWrapAt, mWrapIndent, format );
        }
    }
}

void TBuffer::appendBuffer( TBuffer chunk )
{
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    int startLine = getLastLineNumber() > 0 ? getLastLineNumber() : 0;
    for( int cx=0; cx<static_cast<int>(chunk.buffer[0].size()); cx++ )
    {
        if( chunk.buffer[0][cx] )
        {
            append(QString(chunk.lineBuffer[0][cx]),
                   chunk.buffer[0][cx]->fgColor,
                   chunk.buffer[0][cx]->bgColor,
                   (chunk.buffer[0][cx]->bold == true),
                   (chunk.buffer[0][cx]->italics == true),
                   (chunk.buffer[0][cx]->underline == true) );
        }
    }
    TChar format;
    wrap( startLine, mWrapAt, mWrapIndent, format );
}

int TBuffer::calcWrapPos( int line, int begin, int end )
{
    const QString lineBreaks = ",.- ";
    if( lineBuffer.size() < line ) return -1;
    if( lineBuffer[line].size() < end )
    {
        end = lineBuffer[line].size()-1;
    }
    for( int i=end; i>=begin; i-- )
    {
        if( lineBreaks.indexOf(lineBuffer[line][i]) > -1 )
        {
            return i;
        }
    }
    return -1;
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrap( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( startLine < 0 ) return 0;
    if( static_cast<int>(buffer.size()) < startLine ) return 0;
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    QStringList timeList;
    QList<QList<QColor> > fgList;
    QList<QList<QColor> > bgList;
    int lineCount = 0;
    
    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        assert( static_cast<int>(buffer[i].size()) == lineBuffer[i].size() );
        std::deque<TChar *> newLine;
        QString lineText;
        QString time = timeBuffer[i];
        QList<QColor> fgColorList = fgColorBuffer[i];
        QList<QColor> bgColorList = bgColorBuffer[i];
        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= screenWidth )
        {
            for( int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = format.fgColor;
                pSpace->bgColor = format.bgColor;
                pSpace->italics = format.italics;
                pSpace->bold = format.bold;
                pSpace->underline = format.underline;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = buffer[i].size();
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );
            timeList.append( time );
            fgList.append( fgColorList );
            bgList.append( bgColorList );
            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }
    
    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();    
        lineBuffer.pop_back();
        timeBuffer.pop_back();
        bgColorBuffer.pop_back();
        fgColorBuffer.pop_back();
    }
    
    newLines -= lineCount;
    newLines += queue.size();
    int insertedLines = queue.size()-1;
    
    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }
    
    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
        timeBuffer.append( timeList[i] );
        fgColorBuffer.append( fgList[i] );
        bgColorBuffer.append( bgList[i] );
    }
    return insertedLines > 0 ? insertedLines : 0;
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrapLine( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( startLine < 0 ) return 0;
    if( static_cast<int>(buffer.size()) <= startLine ) return 0;
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    int lineCount = 0;

    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        if( i > startLine ) break; //only wrap one line of text

        assert( static_cast<int>(buffer[i].size()) == lineBuffer[i].size() );
        std::deque<TChar *> newLine;
        QString lineText;

        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= screenWidth )
        {
            for( int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = format.fgColor;
                pSpace->bgColor = format.bgColor;
                pSpace->italics = format.italics;
                pSpace->bold = format.bold;
                pSpace->underline = format.underline;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = static_cast<int>(buffer[i].size());
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );

            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    buffer.erase( buffer.begin()+startLine );
    lineBuffer.removeAt( startLine );
    timeBuffer.removeAt( startLine );
    bgColorBuffer.removeAt( startLine );
    fgColorBuffer.removeAt( startLine );

    int insertedLines = queue.size()-1;

    int i=0;
    while( ! queue.empty() )
    {
        buffer.insert(buffer.begin()+startLine+i, queue.front());
        queue.pop();
        i++;
    }

    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.insert( startLine+i, tempList[i] );
        timeBuffer.insert( startLine+i, QTime::currentTime().toString("hh:mm:ss.zzz")+"   " );
        bgColorBuffer.insert( startLine+i, QList<QColor>() );
        fgColorBuffer.insert( startLine+i, QList<QColor>() );
    }
    return insertedLines > 0 ? insertedLines : 0;
}


bool TBuffer::moveCursor( QPoint & where )
{
    int x = where.x();
    int y = where.y();
    if( y < 0 ) return false;
    if( y >= static_cast<int>(buffer.size()) ) return false;
    
    if( static_cast<int>(buffer[y].size())-1 >  x )
    {
        TChar c;
        expandLine( y, x-buffer[y].size()-1, & c );
    }
    return true;
}

QString badLineError = QString("ERROR: invalid line number");


QString & TBuffer::line( int n )
{
    if( (n >= lineBuffer.size()) || (n<0) ) return badLineError;
    return lineBuffer[n];
}


int TBuffer::find( int line, QString what, int pos=0 )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return -1;
    return lineBuffer[line].indexOf( what, pos );
}


QStringList TBuffer::split( int line, QString splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


QStringList TBuffer::split( int line, QRegExp splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


void TBuffer::expandLine( int y, int count, TChar * pC )
{
    int size = buffer[y].size()-1;
    for( int i=size; i<size+count; i++ )
    {
        if( ! pC ) pC = new TChar;
        buffer[y].push_back( pC );
        lineBuffer[y].append( " " );
    }
}

bool TBuffer::replaceInLine( QPoint & P_begin,
                             QPoint & P_end,
                             QString & with,
                             TChar & format )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if( ( x2 > static_cast<int>(buffer[y2].size()) ) || ( x1 > static_cast<int>(buffer[y1].size()) ) )
    {
        return false;
    }
    if( x1 < 0 || x2 < 0 )
    {
        return false;
    }

    int xb,xe, yb, ye;
    if( y1 <= y2 )
    {
        yb = y1;
        ye = y2;
        xb = x1;
        xe = x2;
    }
    else
    {
        yb = y2;
        ye = y1;
        xb = x2;
        xe = x1;
    }

    for( int y=yb; y<=ye; y++ )
    {
        int x = 0;
        if( y == yb )
        {
            x = xb;
        }
        int x_end = buffer[y].size()-1;
        if( y == ye )
        {
            x_end = xe;
        }
        lineBuffer[y].remove( x, x_end-x );
        typedef std::deque<TChar *>::iterator IT;
        IT it1 = buffer[y].begin()+x;
        IT it2 = buffer[y].begin()+x_end;
        buffer[y].erase( it1, it2 );
    }

    // insert replacement
    insertInLine( P_begin, with, format );
    return true;
}


bool TBuffer::replace( int line, QString what, QString with )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) )
        return false;
    lineBuffer[line].replace( what, with );
    
    // fix size of the corresponding format buffer
    
    int delta = lineBuffer[line].size() - static_cast<int>(buffer[line].size());
    
    if( delta > 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            TChar * pC = new TChar( mpHost ); // cloning default char format according to profile
                                              // because a lookup would be too expensive as
                                              // this is a very often used function and this standard
                                              // behaviour is acceptable. If the user wants special colors
                                              // he can apply format changes
            buffer[line].push_back( pC );    
        }
    }
    else if( delta < 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            buffer[line].pop_back();
        }
    }
    return true;
}

void TBuffer::clear()
{
    while( (getLastLineNumber() > -1 ) )   
    {
        if( ! deleteLines( 0, 0 ) )
        {
            break;
        }
    }
}

bool TBuffer::deleteLine( int y )
{ 
    return deleteLines( y, y );
}


bool TBuffer::deleteLines( int from, int to )
{
    if( ( from >= 0 ) 
     && ( from < static_cast<int>(buffer.size()) )
     && ( from <= to )   
     && ( to >=0 )
     && ( to < static_cast<int>(buffer.size()) ) )
    {
        int delta = to - from + 1;
        
        for( int i=from; i<from+delta; i++ )
        {
            lineBuffer.removeAt( i ); 
            timeBuffer.removeAt( i );
            fgColorBuffer.removeAt( i );
            bgColorBuffer.removeAt( i );
            for( int k=0; k<static_cast<int>(buffer[i].size()); k++ )
            {
                delete buffer[i][k];    
            }
        }
        
        int i = (int)buffer.size();
        // we do reverse lookup as the wanted lines are usually at the end of the buffer
        // std::reverse_iterator is not defined for usage in erase()
        
        typedef std::deque<std::deque<TChar *> >::iterator IT;
        for( IT it=buffer.end(); it!=buffer.begin(); )
        {
            it--;
            i--;
            if( i > to ) 
                continue;
            
            if( --delta >= 0 )
                buffer.erase( it );
            else
                break;
        }
        return true;
    }
    else 
    {
        return false;
    }
}


bool TBuffer::applyFormat( QPoint & P_begin, QPoint & P_end, TChar & format )
{
    if( ( P_begin.x() >= 0 ) 
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }
            
                *buffer[y][x] = format;
                x++;
            }
        }
        return true;
    }
    else 
        return false;            
}

bool TBuffer::applyFgColor( QPoint & P_begin, QPoint & P_end, QColor & fgColor )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x]->fgColor = fgColor;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyBgColor( QPoint & P_begin, QPoint & P_end, QColor & bgColor )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x]->bgColor = bgColor;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

QStringList TBuffer::getEndLines( int n )
{
    QStringList linesList;
    for( int i=getLastLineNumber()-n; i<getLastLineNumber(); i++ )
    {
        linesList << line( i );
    }
    return linesList;
}




