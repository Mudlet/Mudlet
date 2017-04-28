/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2017 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TBuffer.h"

#include "Host.h"
#include "TConsole.h"

#include <queue>

#include <assert.h>


TChar::TChar()
{
    fgR = 255;
    fgG = 255;
    fgB = 255;
    bgR = 0;
    bgG = 0;
    bgB = 0;
    flags = 0;
    link = 0;
}

TChar::TChar( int fR, int fG, int fB, int bR, int bG, int bB, bool b, bool i, bool u, bool s, int _link )
: fgR(fR)
, fgG(fG)
, fgB(fB)
, bgR(bR)
, bgG(bG)
, bgB(bB)
, link(_link )
{
    flags = 0;
    if (i)
        flags |= TCHAR_ITALICS;
    if (b)
        flags |= TCHAR_BOLD;
    if (u)
        flags |= TCHAR_UNDERLINE;
    if (s)
        flags |= TCHAR_STRIKEOUT;
}

TChar::TChar( Host * pH )
{
    if( pH )
    {
        fgR = pH->mFgColor.red();
        fgG = pH->mFgColor.green();
        fgB = pH->mFgColor.blue();
        bgR = pH->mBgColor.red();
        bgG = pH->mBgColor.green();
        bgB = pH->mBgColor.blue();
    }
    else
    {

        fgR = 255;
        fgG = 255;
        fgB = 255;
        bgR = 0;
        bgG = 0;
        bgB = 0;
    }
    flags = 0;
    link = 0;
}

bool TChar::operator==( const TChar & c )
{
    if( fgR != c.fgR ) return false;
    if( fgG != c.fgG ) return false;
    if( fgB != c.fgB ) return false;
    if( bgR != c.bgR ) return false;
    if( bgG != c.bgG ) return false;
    if( bgB != c.bgB ) return false;
    if( flags != c.flags ) return false;
    if( link != c.link ) return false;
    return true;
}

TChar::TChar( const TChar & copy )
{
    fgR = copy.fgR;
    fgG = copy.fgG;
    fgB = copy.fgB;
    bgR = copy.bgR;
    bgG = copy.bgG;
    bgB = copy.bgB;
    flags = copy.flags;
    link = copy.link;
    flags &= ~(TCHAR_INVERSE);//for some reason we always clear the inverse, is this a bug?
}


TBuffer::TBuffer( Host * pH )
: mLinkID            ( 0 )
, mLinesLimit        ( 10000 )
, mBatchDeleteSize   ( 1000 )
, mUntriggered       ( 0 )
, mWrapAt            ( 99999999 )
, mWrapIndent        ( 0 )
, mCursorY           ( 0 )
, mMXP               ( false )
, mAssemblingToken   ( false )
, currentToken       ( "" )
, openT              ( 0 )
, closeT             ( 0 )
, mMXP_LINK_MODE     ( false )
, mIgnoreTag         ( false )
, mSkip              ( "" )
, mParsingVar        ( false )
, mMXP_SEND_NO_REF_MODE ( false )
, gotESC             ( false )
, gotHeader          ( false )
, codeRet            ( 0 )
, mFormatSequenceRest( QString("") )
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
, mpHost             ( pH )
, mCursorMoved       ( false )
, mBold              ( false )
, mItalics           ( false )
, mUnderline         ( false )
, mStrikeOut         ( false )
, mFgColorCode       ( false )
, mBgColorCode       ( false )
, mIsHighColorMode   ( false )
, speedTP()
, speedSequencer()
, speedAppend()
, msLength()
, msPos()
, mOpenMainQuote()
, mEchoText()
, mIsDefaultColor()
, isUserScrollBack()
, currentFgColorProperty()
, maxx()
, maxy()
, hadLF()
, mCode()
{
    clear();
    newLines = 0;
    mLastLine = 0;
    updateColors();
    mWaitingForHighColorCode = false;
    mHighColorModeForeground = false;
    mHighColorModeBackground = false;

    TMxpElement _element;
    _element.name = "SEND";
    _element.href = "";
    _element.hint = "";
    mMXP_Elements["SEND"] = _element;
}

void TBuffer::setBufferSize( int s, int batch )
{
    if( s < 100 ) s = 100;
    if( batch >= s ) batch = s/10;
    mLinesLimit = s;
    mBatchDeleteSize = batch;
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
    mStrikeOut = false;
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
    fgColorR = mFgColorR;
    fgColorLightR = fgColorR;
    mFgColorG = mFgColor.green();
    fgColorG = mFgColorG;
    fgColorLightG = fgColorG;
    mFgColorB = mFgColor.blue();
    fgColorB = mFgColorB;
    fgColorLightB = fgColorB;
    mBgColorR = mBgColor.red();
    bgColorR = mBgColorR;
    mBgColorG = mBgColor.green();
    bgColorG = mBgColorG;
    mBgColorB = mBgColor.blue();
    bgColorB = mBgColorB;

}

QPoint TBuffer::getEndPos()
{
    int x = 0;
    int y = 0;
    if (! buffer.empty()) {
        y = buffer.size()-1;
        if (! buffer.at(y).empty()) {
            x = buffer.at(y).size() - 1;
        }
    }
    QPoint P_end(x, y);
    return P_end;
}

int TBuffer::getLastLineNumber()
{
    if( static_cast<int>(buffer.size()) > 0 )
    {
        return static_cast<int>(buffer.size())-1;
    }
    else
    {
        return 0;//-1;
    }
}

void TBuffer::addLink( bool trigMode, const QString & text, QStringList & command, QStringList & hint, TChar format )
{
    mLinkID++;
    if( mLinkID > 1000 )
    {
        mLinkID = 1;
    }
    mLinkStore[mLinkID] = command;
    mHintStore[mLinkID] = hint;
    if( ! trigMode )
    {
        append( text,
                0,
                text.length(),
                format.fgR,
                format.fgG,
                format.fgB,
                format.bgR,
                format.bgG,
                format.bgB,
                format.flags & TCHAR_BOLD,
                format.flags & TCHAR_ITALICS,
                format.flags & TCHAR_UNDERLINE,
                format.flags & TCHAR_STRIKEOUT,
                mLinkID );
    }
    else
    {
        appendLine( text,
                    0,
                    text.length(),
                    format.fgR,
                    format.fgG,
                    format.fgB,
                    format.bgR,
                    format.bgG,
                    format.bgB,
                    format.flags & TCHAR_BOLD,
                    format.flags & TCHAR_ITALICS,
                    format.flags & TCHAR_UNDERLINE,
                    format.flags & TCHAR_STRIKEOUT,
                    mLinkID );
    }
}

//void TBuffer::appendLink( QString & text,
//                          int sub_start,
//                          int sub_end,
//                          int fgColorR,
//                          int fgColorG,
//                          int fgColorB,
//                          int bgColorR,
//                          int bgColorG,
//                          int bgColorB,
//                          bool bold,
//                          bool italics,
//                          bool underline )
//{
//    if( static_cast<int>(buffer.size()) > mLinesLimit )
//    {
//        shrinkBuffer();
//    }
//    int last = buffer.size()-1;
//    if( last < 0 )
//    {
//        std::deque<TChar> newLine;
//        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
//        newLine.push_back( c );
//        buffer.push_back( newLine );
//        lineBuffer.push_back(QString());
//        promptBuffer.push_back(false);
//        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
//        dirty.push_back(true);
//        last = 0;
//    }
//    bool firstChar = (lineBuffer.back().size() == 0);
//    int length = text.size();
//    if( length < 1 ) return;
//    if( sub_end >= length ) sub_end = text.size()-1;

//    for( int i=sub_start; i<=(sub_start+sub_end); i++ )
//    {
//        if( text.at(i) == '\n' )
//        {
//            std::deque<TChar> newLine;
//            buffer.push_back( newLine );
//            lineBuffer.push_back( QString() );
//            QString time = "-----";
//            timeBuffer << time;
//            promptBuffer << false;
//            dirty << true;
//            mLastLine++;
//            newLines++;
//            firstChar = true;
//            continue;
//        }
//        if( lineBuffer.back().size() >= mWrapAt )
//        {
//            //assert(lineBuffer.back().size()==buffer.back().size());
//            const QString lineBreaks = ",.- ";
//            const QString nothing = "";
//            for( int i=lineBuffer.back().size()-1; i>=0; i-- )
//            {
//                if( lineBreaks.indexOf( lineBuffer.back().at(i) ) > -1 )
//                {
//                    QString tmp = lineBuffer.back().mid(0,i+1);
//                    QString lineRest = lineBuffer.back().mid(i+1);
//                    lineBuffer.back() = tmp;
//                    std::deque<TChar> newLine;

//                    int k = lineRest.size();
//                    if( k > 0 )
//                    {
//                        while( k > 0 )
//                        {
//                            newLine.push_front(buffer.back().back());
//                            buffer.back().pop_back();
//                            k--;
//                        }
//                    }

//                    buffer.push_back( newLine );
//                    if( lineRest.size() > 0 )
//                        lineBuffer.append( lineRest );
//                    else
//                        lineBuffer.append( nothing );
//                    QString time = "-----";
//                    timeBuffer << time;
//                    promptBuffer << false;
//                    dirty << true;
//                    mLastLine++;
//                    newLines++;
//                    break;
//                }
//            }
//        }
//        lineBuffer.back().append( text.at( i ) );

//        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline, mLinkID );
//        buffer.back().push_back( c );
//        if( firstChar )
//        {
//            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
//        }
//    }
//}


//int speedTP;

/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint on
      3 italics on
      4 underline on
      5 blink on slow
      6 blink on fast
      7 inverse on
      9 strikethrough on
      10 ? TODO
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      25 blink off
      26 RESERVED (for proportional spacing)
      27 inverse off
      29 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 fg default
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg default
      50 RESERVED (for proportional spacing)
      51 framed on
      52 encircled on
      53 overlined on
      54 framed / encircled off
      55 overlined off

      Notes for code 38/48:
      38:0 implementation defined (48:0 is NOT allowed)

      38:1 transparent foreground
      48:1 transparent background

      sequences for 24-bit Color support:
      38:2:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space foreground color
      48:2:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space background color
      38:3:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space foreground color
      48:3:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space background color
      38:4:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space foreground color
      48:4:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space background color
      The seventh parameter may be used to specify a tolerance value (an integer)
      and the parameter element 8 may be used to specify a colour space associated
      with the tolerance (0 for CIELUV, 1 for CIELAB).

      sequences for (indexed) 256 Color support:
      38:5:0-256 (indexed) foreground color
      48:5:0-256 (indexed) background color:
          0x00-0x07:   0 -   7 standard colors (as in ESC [ 30–37 m)
          0x08-0x0F:   8 -  15 high intensity colors (as in ESC [ 90–97 m)
          0x10-0xE7:  16 - 231 6 × 6 × 6 = 216 colors: 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
          0xE8-0xFF: 232 - 255 grayscale from black to white in 24 steps

      Also note that for the 38 and 48 codes the parameter elements SHOULD be
      separated by ':' but some interpretations erronously use ';'.  Also
      "empty" parameter elements represent a default value and that empty
      elements at the end can be omitted.
      */



const QChar cESC = '\033';
const QString cDigit = "0123456789";


inline int TBuffer::lookupColor(const QString & s, int pos )
{
    int ret = 0;
    QString code;

    msPos = pos;
    while( msPos < msLength )
    {
        int digit = cDigit.indexOf( s[msPos] );
        if( digit > -1 )
        {
            code.append( s[msPos] );
            msPos++;
            continue;
        }
        else if( s[msPos] == ';' )
        {
            ret++;
            mCode[ret] = code.toInt();
            msPos++;
            code.clear();
            continue;
        }
        else if( s[msPos] == 'm' )
        {
            ret++;
            mCode[ret] = code.toInt();
            msPos++;
            return ret;
        }
        else if( s[msPos] == '[' )
        {
            msPos++;
            continue;
        }
        else
        {
            msPos++;
            return 0; // unbekannte sequenz
        }
    }
    msPos = pos-1;
    return -1; // unbeendete sequenz
}

void TBuffer::translateToPlainText( std::string & incoming, const bool isFromServer )
{
    // As well as enabling the prepending of left-over bytes from last packet
    // from the MUD server this may help in high frequency interactions to
    // protect this process from the supplied string being modified
    // asynchronously by the QNetwork code that runs in another thread:
    std::string localBuffer;

    Host * pHost = mpHost;
    if (! pHost) {
        qWarning() << "TBuffer::translateToPlainText(...) ERROR: Cannot access Host instance at this time - data has been lost.";
        return; // We really have a problem
    }

    if (isFromServer
        && pHost->mTelnet.getEncoding() == QLatin1String("UTF-8")) {

        localBuffer = mIncompleteUtf8SequenceBytes + incoming;
        mIncompleteUtf8SequenceBytes.clear();
    } else {
        localBuffer = incoming;
    }

    speedAppend = 0;
    speedTP = 0;
    int numCodes=0;
    speedSequencer = 0;
    mUntriggered = lineBuffer.size()-1;
    msLength = incoming.length();
    mFormatSequenceRest="";
    int msPos = 0;
    QString packetTime = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
    if (msLength < 1) {
        return;
    }

    QString encoding = mpHost->mTelnet.getEncoding();

    while( true )
    {
        DECODE:
        if( msPos >= msLength )
        {
            return;
        }
        char & ch = localBuffer[msPos];
        if( ch == '\033' )
        {
            gotESC = true;
            msPos++;
            continue;
        }
        if( gotESC )
        {
            if( ch == '[' )
            {
                gotHeader = true;
                gotESC = false;
                msPos++;
                continue;
            }
        }

        if( gotHeader )
        {
            while( msPos < msLength )
            {
                QChar ch2 = incoming[msPos];

                if( ch2 == 'z' )
                {
                    gotHeader = false;
                    gotESC = false;
                    // MXP line modes

                    // locked mode
                    if( code == "7" || code == "2" ) mMXP = false;
                    // secure mode
                    if( code == "1" || code == "6" || code == "4" ) mMXP = true;
                    // reset
                    if( code == "3" )
                    {
                        closeT = 0;
                        openT = 0;
                        mAssemblingToken = false;
                        currentToken.clear();
                        mParsingVar = false;
                    }
                    codeRet = 0;
                    code.clear();
                    msPos++;
                    goto DECODE;
                }
                int digit = cDigit.indexOf( ch2 );
                if( digit > -1 )
                {
                    code.append( ch2 );
                    msPos++;
                    continue;
                }
                else if( ch2 == ';' )
                {
                    codeRet++;
                    mCode[codeRet] = code.toInt();
                    code.clear();
                    msPos++;
                    continue;
                }
                else if( ch2 == 'm' )
                {
                    codeRet++;
                    mCode[codeRet] = code.toInt();
                    code.clear();
                    gotHeader = false;
                    msPos++;

                    numCodes += codeRet;
                    for( int i=1; i<codeRet+1; i++ )
                    {
                        int tag = mCode[i];
                        if( mWaitingForHighColorCode )
                        {
                            if( mHighColorModeForeground )
                            {
                                if( tag < 16 )
                                {
                                    mHighColorModeForeground = false;
                                    mWaitingForHighColorCode = false;
                                    mIsHighColorMode = false;

                                    if( tag >= 8 )
                                    {
                                        tag -= 8;
                                        mBold = true;
                                    }
                                    else
                                        mBold = false;
                                    switch(tag)
                                    {
                                    case 0:
                                        fgColorR = mBlackR;
                                        fgColorG = mBlackG;
                                        fgColorB = mBlackB;
                                        fgColorLightR = mLightBlackR;
                                        fgColorLightG = mLightBlackG;
                                        fgColorLightB = mLightBlackB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 1:
                                        fgColorR = mRedR;
                                        fgColorG = mRedG;
                                        fgColorB = mRedB;
                                        fgColorLightR = mLightRedR;
                                        fgColorLightG = mLightRedG;
                                        fgColorLightB = mLightRedB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 2:
                                        fgColorR = mGreenR;
                                        fgColorG = mGreenG;
                                        fgColorB = mGreenB;
                                        fgColorLightR = mLightGreenR;
                                        fgColorLightG = mLightGreenG;
                                        fgColorLightB = mLightGreenB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 3:
                                        fgColorR = mYellowR;
                                        fgColorG = mYellowG;
                                        fgColorB = mYellowB;
                                        fgColorLightR = mLightYellowR;
                                        fgColorLightG = mLightYellowG;
                                        fgColorLightB = mLightYellowB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 4:
                                        fgColorR = mBlueR;
                                        fgColorG = mBlueG;
                                        fgColorB = mBlueB;
                                        fgColorLightR = mLightBlueR;
                                        fgColorLightG = mLightBlueG;
                                        fgColorLightB = mLightBlueB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 5:
                                        fgColorR = mMagentaR;
                                        fgColorG = mMagentaG;
                                        fgColorB = mMagentaB;
                                        fgColorLightR = mLightMagentaR;
                                        fgColorLightG = mLightMagentaG;
                                        fgColorLightB = mLightMagentaB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 6:
                                        fgColorR = mCyanR;
                                        fgColorG = mCyanG;
                                        fgColorB = mCyanB;
                                        fgColorLightR = mLightCyanR;
                                        fgColorLightG = mLightCyanG;
                                        fgColorLightB = mLightCyanB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 7:
                                        fgColorR = mWhiteR;
                                        fgColorG = mWhiteG;
                                        fgColorB = mWhiteB;
                                        fgColorLightR = mLightWhiteR;
                                        fgColorLightG = mLightWhiteG;
                                        fgColorLightB = mLightWhiteB;
                                        mIsDefaultColor = false;
                                        break;
                                     }
                                    continue;
                                }
                                else if( tag < 232 )
                                {
                                    tag-=16; // because color 1-15 behave like normal ANSI colors
                                    // 6x6x6 RGB color space
                                    int r = tag / 36;
                                    int g = (tag-(r*36)) / 6;
                                    int b = (tag-(r*36))-(g*6);
                                    // Did use 42 as a factor but that isn't
                                    // right as it yields:
                                    // 0:0; 1:42; 2:84; 3:126; 4:168; 5:210
                                    // 6 x 42 DOES equal 252 BUT IT IS OUT OF
                                    // RANGE... Instead we should use 51:
                                    // 0:0; 1:51; 2:102; 3:153; 4:204: 5:255
                                    fgColorR = r*51;
                                    fgColorLightR = r*51;
                                    fgColorG = g*51;
                                    fgColorLightG = g*51;
                                    fgColorB = b*51;
                                    fgColorLightB = b*51;
                                }
                                else
                                {
                                    // black + 23 tone grayscale from dark to light gray
                                    // Similiar to RGB case the multipler is a bit off
                                    // we have been using 10 but 23 x 10 = 230
                                    // whereas 23 should map to 255, this requires
                                    // a non-integer multiplier, instead of mulipling
                                    // and rounding we, for speed, can use a look-up table:
                                    int value;
                                    switch( tag )
                                    {
                                    case 232:   value =   0; break; //   0.000
                                    case 233:   value =  11; break; //  11.087
                                    case 234:   value =  22; break; //  22.174
                                    case 235:   value =  33; break; //  33.261
                                    case 236:   value =  44; break; //  44.348
                                    case 237:   value =  55; break; //  55.435
                                    case 238:   value =  67; break; //  66.522
                                    case 239:   value =  78; break; //  77.609
                                    case 240:   value =  89; break; //  88.696
                                    case 241:   value = 100; break; //  99.783
                                    case 242:   value = 111; break; // 110.870
                                    case 243:   value = 122; break; // 121.957
                                    case 244:   value = 133; break; // 133.043
                                    case 245:   value = 144; break; // 144.130
                                    case 246:   value = 155; break; // 155.217
                                    case 247:   value = 166; break; // 166.304
                                    case 248:   value = 177; break; // 177.391
                                    case 249:   value = 188; break; // 188.478
                                    case 250:   value = 200; break; // 199.565
                                    case 251:   value = 211; break; // 210.652
                                    case 252:   value = 222; break; // 221.739
                                    case 253:   value = 233; break; // 232.826
                                    case 254:   value = 244; break; // 243.913
                                    case 255:   value = 255; break; // 255.000
                                    default:
                                        value = 192;
                                        qWarning() << "TBuffer::translateToPlainText() 256 Color mode parsing Grey-scale code for foreground failed, unexpected value encountered (outside of 232-255):" << tag << "mapping to light-grey!";
                                    }

                                    fgColorR = value;
                                    fgColorLightR = value;
                                    fgColorG = value;
                                    fgColorLightG = value;
                                    fgColorB = value;
                                    fgColorLightB = value;
                                }
                                mHighColorModeForeground = false;
                                mWaitingForHighColorCode = false;
                                mIsHighColorMode = false;
                                continue;
                            }

                            if( mHighColorModeBackground )
                            {
                                if( tag < 16 )
                                {
                                    mHighColorModeBackground = false;
                                    mWaitingForHighColorCode = false;
                                    mIsHighColorMode = false;

                                    bool _bold;
                                    if( tag >= 8 )
                                    {
                                        tag -= 8;
                                        _bold = true;
                                    }
                                    else
                                        _bold = false;
                                    int bgColorLightR = 0;
                                    int bgColorLightG = 0;
                                    int bgColorLightB = 0;
                                    switch( tag )
                                    {
                                    case 0:
                                        bgColorR = mBlackR;
                                        bgColorG = mBlackG;
                                        bgColorB = mBlackB;
                                        bgColorLightR = mLightBlackR;
                                        bgColorLightG = mLightBlackG;
                                        bgColorLightB = mLightBlackB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 1:
                                        bgColorR = mRedR;
                                        bgColorG = mRedG;
                                        bgColorB = mRedB;
                                        bgColorLightR = mLightRedR;
                                        bgColorLightG = mLightRedG;
                                        bgColorLightB = mLightRedB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 2:
                                        bgColorR = mGreenR;
                                        bgColorG = mGreenG;
                                        bgColorB = mGreenB;
                                        bgColorLightR = mLightGreenR;
                                        bgColorLightG = mLightGreenG;
                                        bgColorLightB = mLightGreenB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 3:
                                        bgColorR = mYellowR;
                                        bgColorG = mYellowG;
                                        bgColorB = mYellowB;
                                        bgColorLightR = mLightYellowR;
                                        bgColorLightG = mLightYellowG;
                                        bgColorLightB = mLightYellowB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 4:
                                        bgColorR = mBlueR;
                                        bgColorG = mBlueG;
                                        bgColorB = mBlueB;
                                        bgColorLightR = mLightBlueR;
                                        bgColorLightG = mLightBlueG;
                                        bgColorLightB = mLightBlueB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 5:
                                        bgColorR = mMagentaR;
                                        bgColorG = mMagentaG;
                                        bgColorB = mMagentaB;
                                        bgColorLightR = mLightMagentaR;
                                        bgColorLightG = mLightMagentaG;
                                        bgColorLightB = mLightMagentaB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 6:
                                        bgColorR = mCyanR;
                                        bgColorG = mCyanG;
                                        bgColorB = mCyanB;
                                        bgColorLightR = mLightCyanR;
                                        bgColorLightG = mLightCyanG;
                                        bgColorLightB = mLightCyanB;
                                        mIsDefaultColor = false;
                                        break;
                                    case 7:
                                        bgColorR = mWhiteR;
                                        bgColorG = mWhiteG;
                                        bgColorB = mWhiteB;
                                        bgColorLightR = mLightWhiteR;
                                        bgColorLightG = mLightWhiteG;
                                        bgColorLightB = mLightWhiteB;
                                        mIsDefaultColor = false;
                                        break;
                                    }
                                    if( _bold )
                                    {
                                        bgColorR = bgColorLightR;
                                        bgColorG = bgColorLightG;
                                        bgColorB = bgColorLightB;
                                    }
                                    continue;
                                }
                                if( tag < 232 )
                                {
                                    tag-=16;
                                    int r = tag / 36;
                                    int g = (tag-(r*36)) / 6;
                                    int b = (tag-(r*36))-(g*6);
                                    bgColorR = r*51;
                                    bgColorG = g*51;
                                    bgColorB = b*51;
                                }
                                else
                                {
                                    // black + 23 tone grayscale from dark to (NOT light gray, but) white
                                    int value;
                                    switch( tag )
                                    {
                                    case 232:   value =   0; break; //   0.000
                                    case 233:   value =  11; break; //  11.087
                                    case 234:   value =  22; break; //  22.174
                                    case 235:   value =  33; break; //  33.261
                                    case 236:   value =  44; break; //  44.348
                                    case 237:   value =  55; break; //  55.435
                                    case 238:   value =  67; break; //  66.522
                                    case 239:   value =  78; break; //  77.609
                                    case 240:   value =  89; break; //  88.696
                                    case 241:   value = 100; break; //  99.783
                                    case 242:   value = 111; break; // 110.870
                                    case 243:   value = 122; break; // 121.957
                                    case 244:   value = 133; break; // 133.043
                                    case 245:   value = 144; break; // 144.130
                                    case 246:   value = 155; break; // 155.217
                                    case 247:   value = 166; break; // 166.304
                                    case 248:   value = 177; break; // 177.391
                                    case 249:   value = 188; break; // 188.478
                                    case 250:   value = 200; break; // 199.565
                                    case 251:   value = 211; break; // 210.652
                                    case 252:   value = 222; break; // 221.739
                                    case 253:   value = 233; break; // 232.826
                                    case 254:   value = 244; break; // 243.913
                                    case 255:   value = 255; break; // 255.000
                                    default:
                                        value = 64;
                                        qWarning() << "TBuffer::translateToPlainText() 256 Color mode parsing Grey-scale code for background failed, unexpected value encountered (outside of 232-255):" << tag << "mapping to dark-grey!";
                                    }

                                    bgColorR = value;
                                    bgColorG = value;
                                    bgColorB = value;
                                }
                                mHighColorModeBackground = false;
                                mWaitingForHighColorCode = false;
                                mIsHighColorMode = false;
                                continue;
                            }
                        }

                        if( tag == 38 )
                        {
                            mIsHighColorMode = true;
                            mHighColorModeForeground = true;
                            continue;
                        }
                        if( tag == 48 )
                        {
                            mIsHighColorMode = true;
                            mHighColorModeBackground = true;
                            continue;
                        }

                        if( mIsHighColorMode )
                        {
                            switch( tag )
                            {
                            case 5: // Indexed 256 color mode
                                mWaitingForHighColorCode = true;
                                break;
                            case 2: // 24Bit RGB color mode
// TODO:
//                                mWaitingFor24BitColor = true;
//                                break;
                            case 4: // 24Bit CYMB color mode
                            case 3: // 24Bit CYM color mode
                            case 1: // "Transparent" mode
                            case 0: // "Application defined" mode
                                qWarning() << "TBuffer::translateToPlainText(...) Warning unhandled ANSI SGR 38/48 type color code encountered, first parameter is:" << tag;
                                break;
                            default:
                                qWarning() << "TBuffer::translateToPlainText(...) Warning unknown ANSI SGR 38/48 type color code encountered, first parameter is:" << tag;
                                break;
                            }

                            continue;
                        }

                        // we are dealing with standard ANSI colors
                        switch( tag )
                        {
                        case 0:
                            mHighColorModeForeground = false;
                            mHighColorModeBackground = false;
                            mWaitingForHighColorCode = false;
                            mIsHighColorMode = false;
                            mIsDefaultColor = true;
                            fgColorR = mFgColorR;
                            fgColorG = mFgColorG;
                            fgColorB = mFgColorB;
                            bgColorR = mBgColorR;
                            bgColorG = mBgColorG;
                            bgColorB = mBgColorB;
                            mBold = false;
                            mItalics = false;
                            mUnderline = false;
                            mStrikeOut = false;
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
                            break;
                        case 5:
                            // TODO:
                            break; //slow-blinking
                        case 6:
                            // TODO:
                            break; //fast blinking
                        case 7:
                            // TODO:
                            break; //inverse
                        case 9:
                            mStrikeOut = true;
                            break; //strikethrough
                        case 10:
                            break; //default font
                        case 22:
                            mBold = false;
                            break;
                        case 23:
                            mItalics = false;
                            break;
                        case 24:
                            mUnderline = false;
                            break;
                        case 25:
                            break; // blink off
                        case 27:
                            // TODO:
                            break; //inverse off
                        case 29:
                            mStrikeOut = false;
                            break; //not crossed out (strikethrough) text
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
                            fgColorG = mRedG;
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
                            fgColorLightG = mLightGreenG;
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
                            fgColorG = mMagentaG;
                            fgColorB = mMagentaB;
                            fgColorLightR = mLightMagentaR;
                            fgColorLightG = mLightMagentaG;
                            fgColorLightB = mLightMagentaB;
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
                        case 39: //default foreground color
                            fgColorR = mFgColorR;
                            fgColorG = mFgColorG;
                            fgColorB = mFgColorB;
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
                        case 49: // default background color
                            bgColorR = mBgColorR;
                            bgColorG = mBgColorG;
                            bgColorB = mBgColorB;
                            break;
                        case 53: // overline on
                            // TODO:
                            break;
                        case 55: // overline off
                            // TODO:
                            break;
                        };
                    }

                    codeRet = 0;
                    goto DECODE;
                }
                else
                {
                    msPos++;
                    gotHeader = false;
                    goto DECODE;
                }
            }
            // sequenz ist im naechsten tcp paket keep decoder state
            return;
        }

        const QString nothing = "";
        TChar stdCh;

        if( mMXP )
        {
            // ignore < and > inside of parameter strings
            if( openT == 1 )
            {
                if( ch == '\'' || ch == '\"' )
                {
                    if( ! mParsingVar )
                    {
                        mOpenMainQuote = ch;
                        mParsingVar = true;
                    }
                    else
                    {
                        if( ch == mOpenMainQuote )
                        {
                            mParsingVar = false;
                        }
                    }
                }
            }

            if( ch == '<' )
            {
                if( ! mParsingVar )
                {
                    openT++;
                    if( currentToken.size() > 0 )
                    {
                        currentToken += ch;
                    }
                    mAssemblingToken = true;
                    msPos++;
                    continue;
                }
            }

            if( ch == '>' )
            {
                if( ! mParsingVar ) closeT++;

                // sanity check
                if( closeT > openT )
                {
                    closeT = 0;
                    openT = 0;
                    mAssemblingToken = false;
                    currentToken.clear();
                    mParsingVar = false;
                }

                if( ( openT > 0 ) && ( closeT == openT ) )
                {
                    mAssemblingToken = false;
                    std::string::size_type _pfs = currentToken.find_first_of(' ');
                    QString _tn;
                    if( _pfs == std::string::npos )
                    {
                        _tn = currentToken.c_str();
                    }
                    else
                        _tn = currentToken.substr( 0, _pfs ).c_str();
                    _tn = _tn.toUpper();
                    if( _tn == "VERSION" )
                    {
                        mpHost->sendRaw( QString("\n\x1b[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=2.0 REGISTERED=no>\n") );
                    }
                    if( _tn == "BR" )
                    {
                        ch = '\n';
                        openT = 0;
                        closeT = 0;
                        currentToken.clear();
                        goto COMMIT_LINE;
                    }
                    if( _tn.startsWith( "!EL" ) )
                    {
                        QString _tp = currentToken.substr( currentToken.find_first_of(' ') ).c_str();
                        _tn = _tp.section( ' ', 1, 1 ).toUpper();
                        _tp = _tp.section( ' ', 2 ).toUpper();
                        if( ( _tp.indexOf( "SEND" ) != -1 ) )
                        {
                            QString _t2 = _tp;
                            int pRef = _t2.indexOf( "HREF=" );
                            bool _got_ref = false;
                            // wenn kein href angegeben ist, dann gilt das 1. parameter als href
                            if( pRef == -1 )
                            {
                                pRef = _t2.indexOf("SEND ");
                            }
                            else
                                _got_ref = true;

                            if( pRef == -1 ) return;
                            pRef += 5;

                            QChar _quote_type = _t2[pRef];
                            int pRef2;
                            if( _quote_type != '&' )
                                pRef2 = _t2.indexOf( _quote_type, pRef+1 ); //' ', pRef );
                            else
                                pRef2 = _t2.indexOf( ' ', pRef+1 );
                            QString _ref = _t2.mid( pRef, pRef2-pRef );

                            // gegencheck, ob es keine andere variable ist

                            if( _ref.startsWith('\'') )
                            {
                                int pRef3 = _t2.indexOf( '\'', _t2.indexOf( '\'', pRef )+1 );
                                int pRef4 = _t2.indexOf( '=' );
                                if( ( ( pRef4 == -1 ) || ( pRef4 != 0 && pRef4 > pRef3 ) ) || ( _got_ref ) )
                                {
                                    _ref = _t2.mid( pRef, pRef2-pRef );
                                }
                            }
                            else if( _ref.startsWith('\"') )
                            {
                                int pRef3 = _t2.indexOf( '\"', _t2.indexOf( '\"', pRef )+1 );
                                int pRef4 = _t2.indexOf( '=' );
                                if( ( ( pRef4 == -1 ) || ( pRef4 != 0 && pRef4 > pRef3 ) ) || ( _got_ref ) )
                                {
                                    _ref = _t2.mid( pRef, pRef2-pRef );
                                }
                            }
                            else if( _ref.startsWith( '&' ) )
                            {
                                _ref = _t2.mid( pRef, _t2.indexOf( ' ', pRef+1 )-pRef );
                            }
                            else
                                _ref = "";
                            _ref = _ref.replace( ';' , "" );
                            _ref = _ref.replace( "&quot", "" );
                            _ref = _ref.replace( "&amp", "&" );
                            _ref = _ref.replace('\'', "" );//NEU
                            _ref = _ref.replace( '\"', "" );//NEU
                            _ref = _ref.replace( "&#34", "\"" );

                            pRef = _t2.indexOf( "HINT=" );
                            QString _hint;
                            if( pRef != -1 )
                            {
                                pRef += 5;
                                int pRef2 = _t2.indexOf( ' ', pRef );
                                _hint = _t2.mid( pRef, pRef2-pRef );
                                if( _hint.startsWith('\'') || pRef2 < 0 )
                                {
                                    pRef2 = _t2.indexOf( '\'', _t2.indexOf( '\'', pRef )+1 );
                                    _hint = _t2.mid( pRef, pRef2-pRef );
                                }
                                else if( _hint.startsWith('\"') || pRef2 < 0 )
                                {
                                    pRef2 = _t2.indexOf( '\"', _t2.indexOf( '\"', pRef )+1 );
                                    _hint = _t2.mid( pRef, pRef2-pRef );
                                }
                                _hint = _hint.replace( ';' , "" );
                                _hint = _hint.replace( "&quot", "" );
                                _hint = _hint.replace( "&amp", "&" );
                                _hint = _hint.replace('\'', "" );//NEU
                                _hint = _hint.replace( '\"', "" );//NEU
                                _hint = _hint.replace( "&#34", "\"" );
                            }
                            TMxpElement _element;
                            _element.name = _tn;
                            _element.href = _ref;
                            _element.hint = _hint;
                            mMXP_Elements[_tn] = _element;
                        }
                        openT = 0;
                        closeT = 0;
                        currentToken.clear();
                        msPos++;
                        continue;
                    }



                    if( mMXP_LINK_MODE )
                    {
                        if( _tn.indexOf('/') != -1 )
                        {
                            mMXP_LINK_MODE = false;
                        }
                    }

                    if( mMXP_SEND_NO_REF_MODE )
                    {
                        if( _tn.indexOf('/') != -1 )
                        {
                            mMXP_SEND_NO_REF_MODE = false;
                            if( mLinkStore[mLinkID].front() == "send([[]])" )
                            {
                                QString _t_ref = "send([[";
                                _t_ref.append( mAssembleRef.c_str() );
                                _t_ref.append( "]])" );
                                QStringList _t_ref_list;
                                _t_ref_list << _t_ref;
                                mLinkStore[mLinkID] = _t_ref_list;
                            }
                            else
                            {
                                mLinkStore[mLinkID].replaceInStrings( "&text;", mAssembleRef.c_str() );
                            }
                            mAssembleRef.clear();
                        }
                    }
                    else if( mMXP_Elements.contains( _tn ) )
                    {
                        QString _tp;
                        std::string::size_type _fs = currentToken.find_first_of(' ');
                        if( _fs != std::string::npos )
                            _tp = currentToken.substr( _fs ).c_str();
                        else
                            _tp = "";
                        QString _t1 = _tp.toUpper();
                        const TMxpElement & _element = mMXP_Elements[_tn];
                        QString _t2 = _element.href;
                        QString _t3 = _element.hint;
                        bool _userTag = true;
                        if( _t2.size() < 1 ) _userTag = false;
                        QRegExp _rex;
                        QStringList _rl1, _rl2;
                        int _ki1 = _tp.indexOf('\'');
                        int _ki2 = _tp.indexOf('\"');
                        int _ki3 = _tp.indexOf('=');
                        // is the first parameter to send given in the form
                        // send "what" hint="bla" or send href="what" hint="bla"

                        // handle the first case without a variable assignment
                        if( ( _ki3 == -1 ) // no = whatsoever
                         || ( ( _ki3 != -1 ) && ( ( _ki2 < _ki3 ) || ( _ki1 < _ki3 ) ) ) ) // first parameter is given without =
                        {
                            if( ( _ki1 < _ki2  && _ki1 != -1 ) || ( _ki2 == -1 && _ki1 != -1 ) )
                            {
                                if( _ki1 < _ki3 || _ki3 == -1 )
                                {
                                    _rl1 << "HREF";
                                    int _cki1 = _tp.indexOf('\'', _ki1+1);
                                    if( _cki1 > -1 )
                                        _rl2 << _tp.mid(_ki1+1, _cki1-(_ki1+1));
                                }
                            }
                            else if( ( _ki2 < _ki1 && _ki2 != -1 ) || ( _ki1 == -1 && _ki2 != -1 ) )
                            {
                                if( _ki2 < _ki3 || _ki3 == -1 )
                                {
                                    _rl1 << "HREF";
                                    int _cki2 = _tp.indexOf('\"', _ki2+1);
                                    if( _cki2 > -1 )
                                        _rl2 << _tp.mid(_ki2+1, _cki2-(_ki2+1));
                                }
                            }
                        }
                        // parse parameters in the form var="val" or var='val' where val can be given in the form "foo'b'ar" or 'foo"b"ar'
                        if( _tp.contains("=\'") )
                            _rex = QRegExp("\\b(\\w+)=\\\'([^\\\']*) ?");
                        else
                            _rex = QRegExp("\\b(\\w+)=\\\"([^\\\"]*) ?");

                        int _rpos = 0;
                        while( ( _rpos = _rex.indexIn( _tp, _rpos ) ) != -1 )
                        {
                            _rl1 << _rex.cap(1).toUpper();
                            _rl2 << _rex.cap(2);
                            _rpos += _rex.matchedLength();
                        }
                        if( ( _rl1.size() == _rl2.size() ) && ( _rl1.size() > 0 ) )
                        {
                            for( int i=0; i<_rl1.size(); i++ )
                            {
                                QString _var = _rl1[i];
                                _var.prepend('&');
                                if( _userTag || _t2.indexOf( _var ) != -1 )
                                {
                                    _t2 = _t2.replace( _var, _rl2[i] );
                                    _t3 = _t3.replace( _var, _rl2[i] );
                                }
                                else
                                {
                                    if( _rl1[i] == "HREF" )
                                        _t2 = _rl2[i];
                                    if( _rl1[i] == "HINT" )
                                        _t3 = _rl2[i];
                                }
                            }
                        }

                        // handle print to prompt feature PROMPT
                        bool _send_to_command_line = false;
                        if( _t1.endsWith("PROMPT") )
                        {
                            _send_to_command_line = true;
                        }


                        mMXP_LINK_MODE = true;
                        if( _t2.size() < 1 || _t2.contains( "&text;" ) ) mMXP_SEND_NO_REF_MODE = true;
                        mLinkID++;
                        if( mLinkID > 1000 )
                        {
                            mLinkID = 1;
                        }
                        QStringList _tl = _t2.split('|');
                        for( int i=0; i<_tl.size(); i++ )
                        {
                            _tl[i].replace( "|", "" );
                            if (! _send_to_command_line )
                            {
                                _tl[i] = "send([[" + _tl[i] + "]])";
                            }
                            else
                            {
                                _tl[i] = "printCmdLine([[" + _tl[i] + "]])";
                            }

                        }

                        mLinkStore[mLinkID] = _tl;

                        _t3 = _t3.replace( "&quot;", "\"" );
                        _t3 = _t3.replace( "&amp;", "&" );
                        _t3 = _t3.replace( "&apos;", "'" );
                        _t3 = _t3.replace( "&#34;", "\"" );

                        QStringList _tl2 = _t3.split('|');
                        _tl2.replaceInStrings("|", "");
                        if( _tl2.size() >= _tl.size()+1 )
                        {
                            _tl2.pop_front();
                        }
                        mHintStore[mLinkID] = _tl2;
                    }
                    openT = 0;
                    closeT = 0;
                    currentToken.clear();
                }
                msPos++;
                continue;
            }

            if( mAssemblingToken )
            {
                if( ch == '\n' )
                {
                    closeT = 0;
                    openT = 0;
                    mAssemblingToken = false;
                    currentToken.clear();
                    mParsingVar = false;
                }
                else
                {
                    currentToken += ch;
                    msPos++;
                    continue;
                }
            }

            if( ch == '&' || mIgnoreTag )
            {
                if( ( msPos+4 < msLength ) && ( mSkip.size() == 0 ) )
                {
                    if( incoming.substr( msPos, 4 ) == "&gt;" )
                    {
                        msPos += 3;
                        ch = '>';
                        mIgnoreTag = false;
                    }
                    else if( incoming.substr( msPos, 4 ) == "&lt;" )
                    {
                        msPos += 3;
                        ch = '<';
                        mIgnoreTag = false;
                    }
                    else if( incoming.substr( msPos, 5 ) == "&amp;" )
                    {
                        mIgnoreTag = false;
                        msPos += 4;
                        ch = '&';
                    }
                    else if( incoming.substr( msPos, 6 ) == "&quot;" )
                    {
                        msPos += 5;
                        mIgnoreTag = false;
                        mSkip.clear();
                        ch = '"';
                    }
                }
                // if the content is split across package borders
                else if( mSkip == "&gt"  && ch == ';' )
                {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '>';
                }
                else if( mSkip == "&lt"  && ch == ';' )
                {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '<';
                }
                else if( mSkip == "&amp" && ch == ';' )
                {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '&';
                }
                else if( mSkip == "&quot;" && ch == ';' )
                {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '"';
                }
                else
                {
                    mIgnoreTag = true;
                    mSkip += ch;
                    // sanity check
                    if( mSkip.size() > 7 )
                    {
                        mIgnoreTag = false;
                        mSkip.clear();
                    }
                    msPos++;
                    continue;
                }
            }
        }


        if( mMXP_SEND_NO_REF_MODE )
        {
            mAssembleRef += ch;
        }

        COMMIT_LINE: if( ( ch == '\n' ) || ( ch == '\xff') || ( ch == '\r' ) )
        {
            // MUD Zeilen werden immer am Zeilenanfang geschrieben
            if( lineBuffer.back().size() > 0 )
            {
                if( mMudLine.size() > 0 )
                {
                    lineBuffer << mMudLine;
                }
                else
                {
                    if( ch == '\r' )
                    {
                        msPos++;
                        continue; //empty timer posting
                    }
                    lineBuffer << QString();
                }
                buffer.push_back( mMudBuffer );
                dirty << true;
                timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
                if( ch == '\xff' )
                {
                    promptBuffer.append( true );
                }
                else
                {
                    promptBuffer.append( false );
                }
            }
            else
            {
                if( mMudLine.size() > 0 )
                {
                    lineBuffer.back().append( mMudLine );
                }
                else
                {
                    if( ch == '\r' )
                    {
                        msPos++;
                        continue; //empty timer posting
                    }
                    lineBuffer.back().append(QString());
                }
                buffer.back() = mMudBuffer;
                dirty.back() = true;
                timeBuffer.back() = QTime::currentTime().toString("hh:mm:ss.zzz") + "   ";
                if( ch == '\xff' )
                {
                    promptBuffer.back() = true ;
                }
                else
                {
                    promptBuffer.back() = false;
                }
            }

            mMudLine.clear();
            mMudBuffer.clear();
            int line = lineBuffer.size()-1;
            mpHost->mpConsole->runTriggers( line );
            wrap( lineBuffer.size()-1 );
            msPos++;
            std::deque<TChar> newLine;
            buffer.push_back( newLine );
            lineBuffer.push_back(QString());
            timeBuffer.push_back("   ");
            promptBuffer << false;
            dirty << true;
            if( static_cast<int>(buffer.size()) > mLinesLimit )
            {
                shrinkBuffer();
            }
            continue;
        }

        // PLACEMARKER: Incoming text decoding
        // Used to double up the TChars for Utf-8 byte sequences that produce
        // a surrogate pair (non-BMP):
        bool isTwoTCharsNeeded = false;

        // TODO: It ought to be possible to store a pointer to the relevant
        // decoder function for all but the ASCII, ISO 8859-1 and UTF-8 cases
        // so that we do not have to go through this branching each time - as it
        // is not going to change whilst processing a packet of data!

        if        (encoding == QLatin1String("ISO 8859-1")) {
            mMudLine.append(QString(QChar::fromLatin1(ch)));
        } else if (encoding == QLatin1String("ISO 8859-2")) {
            mMudLine.append(QString(decodeByteToIso_8859_2(ch)));
        } else if (encoding == QLatin1String("ISO 8859-3")) {
            mMudLine.append(QString(decodeByteToIso_8859_3(ch)));
        } else if (encoding == QLatin1String("ISO 8859-4")) {
            mMudLine.append(QString(decodeByteToIso_8859_4(ch)));
        } else if (encoding == QLatin1String("ISO 8859-10")) {
           mMudLine.append(QString(decodeByteToIso_8859_10(ch)));
        } else if (encoding == QLatin1String("ISO 8859-15")) {
            mMudLine.append(QString(decodeByteToIso_8859_10(ch)));
        } else if (encoding == QLatin1String("ISO 8859-16")) {
            mMudLine.append(QString(decodeByteToIso_8859_16(ch)));
        } else if (encoding == QLatin1String("WINDOWS-1250")) {
            mMudLine.append(QString(decodeByteToWindows_1250(ch)));
        } else if (encoding == QLatin1String("WINDOWS-1251")) {
            mMudLine.append(QString(decodeByteToWindows_1250(ch)));
        } else if (encoding == QLatin1String("WINDOWS-1252")) {
            mMudLine.append(QString(decodeByteToWindows_1252(ch)));
        } else if (encoding == QLatin1String("UTF-8")) {
            // In Utf-8 mode we have to process the data more than one byte at a
            // time because there is not necessarily a one-byte to one TChar
            // mapping, instead we use one TChar per QChar - and that has to be
            // tweaked for non-BMP characters that use TWO QChars per codepoint.
            if (localBuffer[msPos] & 0x80) {
                // MSB is set, so if this is Utf-8 then assume this is the first byte
                size_t utf8SequenceLength = 1;
                if        ((localBuffer[msPos] & 0xE0) == 0xC0) {
                    // 2 byte sequence - Unicode code-points: U+00000080 to U+000007FF
                    utf8SequenceLength = 2;
                } else if ((localBuffer[msPos] & 0xF0) == 0xE0) {
                    // 3 byte sequence - Unicode code-points: U+00000800 to U+0000FFFF
                    utf8SequenceLength = 3;
                } else if ((localBuffer[msPos] & 0xF8) == 0xF0) {
                    // 4 byte sequence - Unicode code-points: U+00010000 to U+001FFFFF (<= U+0010FFF LEGAL)
                    utf8SequenceLength = 4;
                } else if ((localBuffer[msPos] & 0xFC) == 0xF8) {
                    // 5 byte sequence - Unicode code-points: U+00200000 to U+03FFFFFF (ALL ILLEGAL)
                    utf8SequenceLength = 5;
                } else if ((localBuffer[msPos] & 0xFE) == 0xFC) {
                    // 6 byte sequence - Unicode code-points: U+04000000 to U+7FFFFFFF (ALL ILLEGAL)
                    utf8SequenceLength = 6;
                }

                if (msPos + utf8SequenceLength >= msLength) {
                    // Not enough bytes left in localBuffer to complete the utf-8
                    // sequence - need to save and prepend onto incoming data next
                    // time around.
                    // The absence of a second argument takes all the available
                    // bytes - this is only for data from the Server NOT from
                    // locally generated material from Lua feedTriggers(...)
                    if( isFromServer ) {
                        mIncompleteUtf8SequenceBytes = localBuffer.substr(msPos);
                    }
                    return; // Bail out
                }

                // If we have got here we have enough bytes to work with:
                bool isValid = true;
                bool isToUseReplacementMark = false;
                bool isToUseByteOrderMark = false; // When BOM seen in stream it transcodes as zero characters
                switch( utf8SequenceLength ) {
                case 4:
                    if ((localBuffer[msPos+3] & 0xC0) != 0x80) {
                        qDebug() << "TBuffer::translateToPlainText(...) 4th byte in UTF-8 sequence is invalid!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    }
                    else if((localBuffer[msPos+3] & 0x3F) > 0x04) {
                        // For 4 byte values the bits are distributed:
                        //  Byte 1    Byte 2    Byte 3    Byte 4
                        // 11110ABC  10DEFGHI  10JKLMNO  10PQRSTU   A is MSB
                        // U+10FFFF in binary is: 1 0000 1111 1111 1111 1111
                        // So this (the maximum valid character) is:
                        // -----100  --001111  --111111  --111111

                        qDebug() << "TBuffer::translateToPlainText(...) 4 byte UTF-8 sequence is valid but is beyond range of legal codepoints!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    }

                    // Fall-through
                case 3:
                    if ((localBuffer[msPos+2] & 0xC0) != 0x80) {
                        qDebug() << "TBuffer::translateToPlainText(...) 3rd byte in UTF-8 sequence is invalid!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    } else if(   (static_cast<quint8>(localBuffer[msPos+2]) == 0xbf)
                              && (static_cast<quint8>(localBuffer[msPos+1]) == 0xbb)
                              && (static_cast<quint8>(localBuffer[msPos]  ) == 0xef)) {

                        // Got caught out by this one - it is the Utf-8 BOM and
                        // needs to be ignored as it transcodes to NO codepoints!
                        qDebug() << "TBuffer::translateToPlainText(...) UTF-8 BOM sequence seen and handled!";
                        isValid = false;
                        isToUseByteOrderMark = true;
                    }

                    // Fall-through
                case 2:
                    if ((localBuffer[msPos+1] & 0xC0) != 0x80) {
                        qDebug() << "TBuffer::translateToPlainText(...) 2nd byte in UTF-8 sequence is invalid!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    }

                    // Also test for (and reject) overlong sequences - don't
                    // need to check 5 or 6 ones as those are already rejected:
                    if( (((localBuffer[msPos] & 0xFE) == 0xC0) && ((localBuffer[msPos+1] & 0xC0) == 0x80))
                     || (( localBuffer[msPos]         == 0xE0) && ((localBuffer[msPos+1] & 0xE0) == 0x80))
                     || (( localBuffer[msPos]         == 0xF0) && ((localBuffer[msPos+1] & 0xF0) == 0x80)) ){

                        qDebug().nospace() << "TBuffer::translateToPlainText(...) Overlong "
                                           << utf8SequenceLength
                                           << "-byte sequence as UTF-8 rejected!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    }
                    break;

                default:
                    qDebug().nospace() << "TBuffer::translateToPlainText(...) "
                                       << utf8SequenceLength
                                       << "-byte sequence as UTF-8 rejected!";
                    isValid = false;
                    isToUseReplacementMark = true;
                }

                // Will be one (BMP codepoint) or two (non-BMP codepoints) QChar(s)
                if (isValid) {
                    QString codePoint = QString(localBuffer.substr(msPos,utf8SequenceLength).c_str());
                    switch(codePoint.size()) {
                    default:
                        Q_UNREACHABLE(); // This can't happen, unless we got start or length wrong in std::string::substr()
                        qWarning().nospace() << "TBuffer::translateToPlainText(...) "
                                              << utf8SequenceLength
                                              << "-byte UTF-8 sequence accepted, but it encoded to "
                                              << codePoint.size()
                                              << " QChars which does not make sense!!!";
                        isValid = false;
                        isToUseReplacementMark = true;
                        break;
                    case 2:
                        isTwoTCharsNeeded = true;
                        // Fall-through
                    case 1:
                        qDebug().nospace() << "TBuffer::translateToPlainText(...) "
                                           << utf8SequenceLength
                                           << "-byte UTF-8 sequence accepted, it was "
                                           << codePoint.size()
                                           << " QChar(s) long ["
                                           << codePoint
                                           << "]";
                        mMudLine.append(codePoint);
                        break;
                    case 0:
                        qWarning().nospace() << "TBuffer::translateToPlainText(...) "
                                              << utf8SequenceLength
                                              << "-byte UTF-8 sequence accepted, but it did not encode to ANY QChar(s)!!!";
                        isValid = false;
                        isToUseReplacementMark = true;
                    }
                }

                if (! isValid) {
                    QString debugMsg;
                    for(size_t i = 0; i < utf8SequenceLength; ++i) {
                        debugMsg.append(QStringLiteral("<%1>").arg(static_cast<quint8>(localBuffer[msPos+i]),2,16));
                    }
                    qDebug().nospace() << "    Sequence bytes are: "
                                       << debugMsg.toLatin1().constData();

                    if(isToUseReplacementMark) {
                        mMudLine.append(QChar::ReplacementCharacter);
                    }
                    if(isToUseByteOrderMark) {
                        mMudLine.append(QChar::ByteOrderMark);
                    }
                }

                // As there is already a unit increment at the bottom of loop
                // add one less than the sequence length:
                msPos += utf8SequenceLength - 1;
            } else {
                // Single byte character i.e. Unicode points: U+00000000 to U+0000007F
                mMudLine.append(ch);
            }
        } else {
            // Default - no encoding case - reject anything that has MS Bit set
            // as that isn't ASCII which is what no encoding specifies!
            if (ch & 0x80) {
                // Was going to ignore this byte, not add a TChar instance
                // either and move on:
                // ++msPos;
                // continue;
                // but instead insert the "Replacement Character Marker"
                mMudLine.append(QChar::ReplacementCharacter);
            } else {
                mMudLine.append(ch);
            }
        }

        TChar c( ! mIsDefaultColor && mBold ? fgColorLightR : fgColorR,
                  ! mIsDefaultColor && mBold ? fgColorLightG : fgColorG,
                  ! mIsDefaultColor && mBold ? fgColorLightB : fgColorB,
                  bgColorR,
                  bgColorG,
                  bgColorB,
                  mIsDefaultColor ? mBold : false,
                  mItalics,
                  mUnderline,
                  mStrikeOut );

        if( mMXP_LINK_MODE )
        {
            c.link = mLinkID;
            c.flags |= TCHAR_UNDERLINE;
        }

        mMudBuffer.push_back( c );

        if (isTwoTCharsNeeded) {
            TChar c2( ! mIsDefaultColor && mBold ? fgColorLightR : fgColorR,
                      ! mIsDefaultColor && mBold ? fgColorLightG : fgColorG,
                      ! mIsDefaultColor && mBold ? fgColorLightB : fgColorB,
                      bgColorR,
                      bgColorG,
                      bgColorB,
                      mIsDefaultColor ? mBold : false,
                      mItalics,
                      mUnderline,
                      mStrikeOut );

            // CHECK: Do we need to duplicate stuff for mMXP_LINK_MODE?
            mMudBuffer.push_back( c2 );
        }

        msPos++;
    }
}

void TBuffer::append(const QString & text,
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
                      bool underline,
                      bool strikeout,
                      int linkID )
{
    const QString lineBreaks = QStringLiteral( ",.- " );

    if( static_cast<int>(buffer.size()) > mLinesLimit ) {
        shrinkBuffer();
    }
    int last = buffer.size()-1;
    if( last < 0 ) {
        std::deque<TChar> newLine;
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout);
        if( mEchoText ) {
            c.flags |= TCHAR_ECHO;
        }
        newLine.push_back( c );
        buffer.push_back( newLine );
        lineBuffer.push_back(QString());
        timeBuffer << QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz   "));
        promptBuffer << false;
        dirty << true;
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if( length < 1 ) {
        return;
    }
    if( sub_end >= length ) {
        sub_end = text.size()-1;
    }

    for( int i=sub_start; i<length; i++ ) {//FIXME <=substart+sub_end muss nachsehen, ob wirklich noch teilbereiche gebraucht werden
        if( text.at(i) == '\n' ) {
            log(size()-1, size()-1);
            std::deque<TChar> newLine;
            buffer.push_back( newLine );
            lineBuffer.push_back( QString() );
            timeBuffer << QStringLiteral( "-----" );
            promptBuffer << false;
            dirty << true;
            mLastLine++;
            newLines++;
            firstChar = true;
            continue;
        }
        if( lineBuffer.back().size() >= mWrapAt ) {
            for( int i=lineBuffer.back().size()-1; i>=0; i-- ) {
                if( lineBreaks.indexOf( lineBuffer.back().at(i) ) > -1 ) {
                    QString tmp = lineBuffer.back().mid(0,i+1);
                    QString lineRest = lineBuffer.back().mid(i+1);
                    lineBuffer.back() = tmp;
                    std::deque<TChar> newLine;

                    int k = lineRest.size();
                    if( k > 0 ) {
                        while( k > 0 ) {
                            newLine.push_front(buffer.back().back());
                            buffer.back().pop_back();
                            k--;
                        }
                    }

                    buffer.push_back( newLine );
                    if( lineRest.size() > 0 ) {
                        lineBuffer.append( lineRest );
                    }
                    else {
                        lineBuffer.append( QString() );
                    }
                    timeBuffer << QStringLiteral( "-----" );
                    promptBuffer << false;
                    dirty << true;
                    mLastLine++;
                    newLines++;
                    log(size()-2, size()-2);
                    // Was absent causing loss of all but last line of wrapped
                    // long lines of user input and some other console displayed
                    // text from log file.
                    break;
                }
            }
        }
        lineBuffer.back().append( text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout,linkID);
        if( mEchoText ) {
            c.flags |= TCHAR_ECHO;
        }
        buffer.back().push_back( c );
        if( firstChar ) {
            timeBuffer.back() = QTime::currentTime().toString( QStringLiteral( "hh:mm:ss.zzz   " ) );
            firstChar = false;
        }
    }
}

void TBuffer::appendLine( const QString & text,
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
                          bool underline,
                          bool strikeout,
                          int linkID )
{
    if( sub_end < 0 ) return;
    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        shrinkBuffer();
    }
    int last = buffer.size()-1;
    if( last < 0 )
    {
        std::deque<TChar> newLine;
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout);
        if( mEchoText )
            c.flags |= TCHAR_ECHO;
        newLine.push_back( c );
        buffer.push_back( newLine );
        lineBuffer.push_back(QString());
        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        promptBuffer << false;
        dirty << true;
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if( length < 1 ) return;
    if( sub_end >= length ) sub_end = text.size()-1;

    for( int i=sub_start; i<=(sub_start+sub_end); i++ )
    {
        lineBuffer.back().append( text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout,linkID);
        if( mEchoText )
            c.flags |= TCHAR_ECHO;
        buffer.back().push_back( c );
        if( firstChar )
        {
            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        }
    }
}

QPoint TBuffer::insert( QPoint & where, const QString& text, int fgColorR, int fgColorG, int fgColorB, int bgColorR, int bgColorG, int bgColorB, bool bold, bool italics, bool underline, bool strikeout )
{
    QPoint P(-1, -1);

    int x = where.x();
    int y = where.y();

    if( y < 0 ) return P;
    if( y >= static_cast<int>(buffer.size()) ) return P;


    for(auto character : text)
    {
        if( character == QChar('\n') )
        {
            std::deque<TChar> newLine;
            TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout);
            newLine.push_back( c );
            buffer.push_back( newLine );
            promptBuffer.insert( y, false );
            const QString nothing = "";
            lineBuffer.insert( y, nothing );
            timeBuffer << "-->";//this is intentional -> faster
            dirty.insert( y, true );
            mLastLine++;
            newLines++;
            x = 0;
            y++;
            continue;
        }
        lineBuffer[y].insert( x, character );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline,strikeout);
        auto it = buffer[y].begin();
        buffer[y].insert( it+x, c );
    }
    dirty[y] = true;
    P.setX( x );
    P.setY( y );
    return P;
}


bool TBuffer::insertInLine( QPoint & P, const QString & text, TChar & format )
{
    if( text.size() < 1 ) return false;
    int x = P.x();
    int y = P.y();
    if( ( y >= 0 ) && ( y < static_cast<int>(buffer.size()) ) )
    {
        if( x < 0 )
        {
            return false;
        }
        if( x >= static_cast<int>(buffer[y].size()) )
        {
            TChar c;
            expandLine( y, x-buffer[y].size(), c );
        }
        for( int i=0; i<text.size(); i++ )
        {
            lineBuffer[y].insert( x+i, text.at( i ) );
            TChar c = format;
            auto it = buffer[y].begin();
            buffer[y].insert( it+x+i, c );
        }
    }
    else
    {
        appendLine( text, 0, text.size(), format.fgR, format.fgG, format.fgB, format.bgR, format.bgG, format.bgB, format.flags & TCHAR_BOLD, format.flags & TCHAR_ITALICS, format.flags & TCHAR_UNDERLINE, format.flags & TCHAR_STRIKEOUT );
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
        QString s(lineBuffer[y][x]);
        slice.append(s,
                     0,
                     1,
                     buffer[y][x].fgR,
                     buffer[y][x].fgG,
                     buffer[y][x].fgB,
                     buffer[y][x].bgR,
                     buffer[y][x].bgG,
                     buffer[y][x].bgB,
                     (buffer[y][x].flags & TCHAR_BOLD),
                     (buffer[y][x].flags & TCHAR_ITALICS),
                     (buffer[y][x].flags & TCHAR_UNDERLINE),
                     (buffer[y][x].flags & TCHAR_STRIKEOUT) );
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
        if( ( y < getLastLineNumber() ) && ( ! needAppend ) )
        {
            TChar & format = chunk.buffer[0][cx];
            QString s = QString(chunk.lineBuffer[0][cx]);
            insertInLine( P_current, s, format );
        }
        else
        {
            hasAppended = true;
            QString s(chunk.lineBuffer[0][cx]);
            append(s,
                   0,
                   1,
                   chunk.buffer[0][cx].fgR,
                   chunk.buffer[0][cx].fgG,
                   chunk.buffer[0][cx].fgB,
                   chunk.buffer[0][cx].bgR,
                   chunk.buffer[0][cx].bgG,
                   chunk.buffer[0][cx].bgB,
                   (chunk.buffer[0][cx].flags & TCHAR_BOLD),
                   (chunk.buffer[0][cx].flags & TCHAR_ITALICS),
                   (chunk.buffer[0][cx].flags & TCHAR_UNDERLINE),
                   (chunk.buffer[0][cx].flags & TCHAR_STRIKEOUT) );
        }
    }
    if( hasAppended )
    {
        TChar format;
        if( y == -1 )
        {
        }
        else
        {
            wrapLine( y, mWrapAt, mWrapIndent, format );
        }
    }
}

void TBuffer::appendBuffer( const TBuffer& chunk )
{
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    for( int cx=0; cx<static_cast<int>(chunk.buffer[0].size()); cx++ )
    {
        QString s(chunk.lineBuffer[0][cx]);
        append(s,
               0,
               1,
               chunk.buffer[0][cx].fgR,
               chunk.buffer[0][cx].fgG,
               chunk.buffer[0][cx].fgB,
               chunk.buffer[0][cx].bgR,
               chunk.buffer[0][cx].bgG,
               chunk.buffer[0][cx].bgB,
               (chunk.buffer[0][cx].flags & TCHAR_BOLD),
               (chunk.buffer[0][cx].flags & TCHAR_ITALICS),
               (chunk.buffer[0][cx].flags & TCHAR_UNDERLINE),
               (chunk.buffer[0][cx].flags & TCHAR_STRIKEOUT) );
    }
    QString lf = "\n";
    append( lf,
               0,
               1,
               0,
               0,
               0,
               0,
               0,
               0,
               false,
               false,
               false,
               false );
}

int TBuffer::calcWrapPos( int line, int begin, int end )
{
    const QString lineBreaks = ",.- \n";
    if( lineBuffer.size() < line ) return 0;
    int lineSize = static_cast<int>(lineBuffer[line].size())-1;
    if( lineSize < end )
    {
        end = lineSize;
    }
    for( int i=end; i>=begin; i-- )
    {
        if( lineBreaks.indexOf( lineBuffer[line].at(i) ) > -1 )
        {
            return i;
        }
    }
    return 0;
}

inline int TBuffer::skipSpacesAtBeginOfLine( int i, int i2 )
{
    int offset = 0;
    int i_end = lineBuffer[i].size();
    QChar space = ' ';
    while( i2 < i_end )
    {
        if( buffer[i][i2].flags & TCHAR_ECHO )
            break;
        if( lineBuffer[i][i2] == space )
            offset++;
        else
            break;
        i2++;
    }
    return offset;
}

inline int TBuffer::wrap( int startLine )
{
    if( static_cast<int>(buffer.size()) < startLine || startLine < 0 ) return 0;
    std::queue<std::deque<TChar> > queue;
    QStringList tempList;
    QStringList timeList;
    QList<bool> promptList;
    int lineCount = 0;
    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        bool isPrompt = promptBuffer[i];
        std::deque<TChar> newLine;
        QString lineText = "";
        QString time = timeBuffer[i];
        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= mWrapAt )
        {
            for( int i3=0; i3<mWrapIndent; i3++ )
            {
                TChar pSpace;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = mWrapIndent;
        }
        int lastSpace = 0;
        int wrapPos = 0;
        int length = buffer[i].size();
        if( length == 0 )
        {
            tempList.append(QString());
            std::deque<TChar> emptyLine;
            queue.push( emptyLine );
            timeList.append( time );
        }
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > mWrapAt-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+mWrapAt-indent );
                if( wrapPos > 0 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = 0;
                }
            }
            else
            {
                lastSpace = 0;
            }
            int __wrapPos = lastSpace == 0 ? mWrapAt-indent : lastSpace;
            for( int i3=0; i3<=__wrapPos; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 > lastSpace )
                    {
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i].at(i2) == '\n' )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            if( newLine.size() == 0 )
            {
                tempList.append(QString());
                std::deque<TChar> emptyLine;
                queue.push( emptyLine );
                timeList.append( QString() );
                promptList.append( false );
            }
            else
            {
                queue.push( newLine );
                tempList.append( lineText );
                timeList.append( time );
                promptList.append( isPrompt );
            }
            newLine.clear();
            lineText = "";
            indent = 0;
            i2 += skipSpacesAtBeginOfLine( i, i2 );
        }
        lineCount++;
    }
    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();
        lineBuffer.pop_back();
        timeBuffer.pop_back();
        promptBuffer.pop_back();
        dirty.pop_back();
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
        if( tempList[i].size() < 1 )
        {
            lineBuffer.append( QString() );
            timeBuffer.append( QString() );
            promptBuffer.push_back( false );
        }
        else
        {
            lineBuffer.append( tempList[i] );
            timeBuffer.append( timeList[i] );
            promptBuffer.push_back( promptList[i] );
        }
        dirty.push_back( true );
    }

    log(startLine, startLine+tempList.size());
    return insertedLines > 0 ? insertedLines : 0;
}

void TBuffer::log( int from, int to )
{

    TBuffer * pB = &mpHost->mpConsole->buffer;
    if( pB == this )
    {
        if( mpHost->mpConsole->mLogToLogFile )
        {
            if( from >= size() || from < 0 )
            {
                return;
            }
            if( to >= size() )
            {
                to = size()-1;
            }
            if( to < 0 )
            {
                return;
            }
            for( int i=from; i<=to; i++ )
            {

                QString toLog;
                if( mpHost->mIsCurrentLogFileInHtmlFormat )
                {
                    QPoint P1 = QPoint(0,i);
                    QPoint P2 = QPoint( buffer[i].size(), i);
                    toLog = bufferToHtml(P1, P2);
                }
                else
                {
                    toLog = lineBuffer[i];
                    toLog.append("\n");
                }
                mpHost->mpConsole->mLogStream << toLog;
            }
            mpHost->mpConsole->mLogStream.flush();
        }
    }
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrapLine( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( startLine < 0 ) return 0;
    if( static_cast<int>(buffer.size()) <= startLine ) return 0;
    std::queue<std::deque<TChar> > queue;
    QStringList tempList;
    int lineCount = 0;

    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        if( i > startLine ) break; //only wrap one line of text

        std::deque<TChar> newLine;
        QString lineText;

        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= screenWidth )
        {
            for( int i3=0; i3<indentSize; i3++ )
            {
                TChar pSpace = format;
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

                    if( newLine.size() == 0 )
                    {
                        tempList.append(QString());
                        std::deque<TChar> emptyLine;
                        queue.push( emptyLine );
                    }
                    else
                    {
                        queue.push( newLine );
                        tempList.append( lineText );
                    }
                    goto OPT_OUT_CLEAN;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );

            OPT_OUT_CLEAN: newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    if( lineCount < 1 )
    {
        log( startLine, startLine );
        return 0;
    }

    buffer.erase( buffer.begin()+startLine );
    lineBuffer.removeAt( startLine );
    QString time = timeBuffer.at( startLine );
    timeBuffer.removeAt( startLine );
    bool isPrompt = promptBuffer.at( startLine );
    promptBuffer.removeAt( startLine );
    dirty.removeAt( startLine );

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
        timeBuffer.insert( startLine+i, time );
        promptBuffer.insert( startLine+i, isPrompt );
        dirty.insert( startLine+i, true );
    }
    log( startLine, startLine+tempList.size()-1 );
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
        expandLine( y, x-buffer[y].size()-1, c );
    }
    return true;
}

QString badLineError = QString("ERROR: invalid line number");


QString & TBuffer::line( int n )
{
    if( (n >= lineBuffer.size()) || (n<0) ) return badLineError;
    return lineBuffer[n];
}


int TBuffer::find( int line, const QString& what, int pos=0 )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return -1;
    return lineBuffer[line].indexOf( what, pos );
}


QStringList TBuffer::split( int line, const QString& splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


QStringList TBuffer::split( int line, QRegExp splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


void TBuffer::expandLine( int y, int count, TChar & pC )
{
    int size = buffer[y].size()-1;
    for( int i=size; i<size+count; i++ )
    {
        buffer[y].push_back( pC );
        lineBuffer[y].append( " " );
    }
}

bool TBuffer::replaceInLine( QPoint & P_begin,
                             QPoint & P_end,
                             const QString & with,
                             TChar & format )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if( ( y1 >= static_cast<int>(buffer.size()) ) || ( y2 >= static_cast<int>(buffer.size()) ) )
    {
        return false;
    }
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
        auto it1 = buffer[y].begin()+x;
        auto it2 = buffer[y].begin()+x_end;
        buffer[y].erase( it1, it2 );
    }

    // insert replacement
    insertInLine( P_begin, with, format );
    return true;
}


bool TBuffer::replace( int line, const QString& what, const QString& with )
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
            TChar c( mpHost ); // cloning default char format according to profile
                               // because a lookup would be too expensive as
                               // this is a very often used function and this standard
                               // behaviour is acceptable. If the user wants special colors
                               // he can apply format changes
            buffer[line].push_back( c );
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
    while( buffer.size() > 0 )
    {
        if( ! deleteLines( 0, 0 ) )
        {
            break;
        }
    }
    std::deque<TChar> newLine;
    buffer.push_back( newLine );
    lineBuffer << QString();
    timeBuffer << QString();
    promptBuffer.push_back( false );
    dirty.push_back( true );
}

bool TBuffer::deleteLine( int y )
{
    return deleteLines( y, y );
}

void TBuffer::shrinkBuffer()
{
    for( int i=0; i < mBatchDeleteSize; i++ )
    {
        lineBuffer.pop_front();
        promptBuffer.pop_front();
        timeBuffer.pop_front();
        dirty.pop_front();
        buffer.pop_front();
        mCursorY--;
    }
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
            promptBuffer.removeAt( i );
            dirty.removeAt( i );
        }

        buffer.erase( buffer.begin() + from, buffer.begin() + to + 1 );
        return true;
    }
    else
    {
        return false;
    }
}


bool TBuffer::applyFormat( QPoint & P_begin, QPoint & P_end, TChar & format )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }

                buffer[y][x] = format;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyLink( QPoint & P_begin, QPoint & P_end, const QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    bool incLinkID = false;
    int linkID = 0;
    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

        {
            for( int y=y1; y<=y2; y++ )
            {
                int x = 0;
                if( y == y1 )
                {
                    x = x1;
                }
                while( x < static_cast<int>(buffer[y].size()) )
                {
                    if( y >= y2 )
                    {
                        if( x >= x2 )
                        {
                            return true;
                        }
                    }
                    if( ! incLinkID )
                    {
                        incLinkID = true;
                        mLinkID++;
                        linkID = mLinkID;
                        if( mLinkID > 1000 )
                        {
                            mLinkID = 1;
                        }
                        mLinkStore[mLinkID] = linkFunction;
                        mHintStore[mLinkID] = linkHint;
                    }
                    buffer[y][x].link = linkID;
                    x++;
                }
            }
            return true;
        }
        else
            return false;
}

bool TBuffer::applyBold( QPoint & P_begin, QPoint & P_end, bool bold )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }
                if ( bold )
                    buffer[y][x].flags |= TCHAR_BOLD;
                else
                    buffer[y][x].flags &= ~(TCHAR_BOLD);
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyItalics( QPoint & P_begin, QPoint & P_end, bool bold )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }
                if ( bold )
                    buffer[y][x].flags |= TCHAR_ITALICS;
                else
                    buffer[y][x].flags &= ~(TCHAR_ITALICS);
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyUnderline( QPoint & P_begin, QPoint & P_end, bool bold )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }

                if ( bold )
                    buffer[y][x].flags |= TCHAR_UNDERLINE;
                else
                    buffer[y][x].flags &= ~(TCHAR_UNDERLINE);
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyStrikeOut( QPoint & P_begin, QPoint & P_end, bool strikeout )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }

                if ( strikeout )
                    buffer[y][x].flags |= TCHAR_STRIKEOUT;
                else
                    buffer[y][x].flags &= ~(TCHAR_STRIKEOUT);
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyFgColor( QPoint & P_begin, QPoint & P_end, int fgColorR, int fgColorG, int fgColorB )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }

                buffer[y][x].fgR = fgColorR;
                buffer[y][x].fgG = fgColorG;
                buffer[y][x].fgB = fgColorB;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyBgColor( QPoint & P_begin, QPoint & P_end, int bgColorR, int bgColorG, int bgColorB )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if( ( x1 >= 0 )
        && ( ( y2 < static_cast<int>(buffer.size()) )
        && ( y2 >= 0 ) )
        && ( ( x2 > x1 ) || ( y2 > y1 ) )
        && ( x1 < static_cast<int>(buffer[y1].size()) ) )
        // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
        // && ( x2 < static_cast<int>(buffer[y2].size()) ) )
    {
        for( int y=y1; y<=y2; y++ )
        {
            int x = 0;
            if( y == y1 )
            {
                x = x1;
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= y2 )
                {
                    if( x >= x2 )
                    {
                        return true;
                    }
                }

                (buffer[y][x]).bgR = bgColorR;
                (buffer[y][x]).bgG = bgColorG;
                (buffer[y][x]).bgB = bgColorB;
                x++;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
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


QString TBuffer::bufferToHtml( QPoint P1, QPoint P2 )
{
    int y = P1.y();
    int x = P1.x();
    QString s;
    if( y < 0 || y >= static_cast<int>(buffer.size()) ) {
        return s;
    }

    if( ( x < 0 )
        || ( x >= static_cast<int>(buffer[y].size()) )
        || ( P2.x() >= static_cast<int>(buffer[y].size()) ) ) {
        x=0;
    }
    if( P2.x() < 0 ) {
        P2.setX(buffer[y].size());
    }

    bool bold = false;
    bool italics = false;
    bool underline = false;
//    bool overline = false;
    bool strikeout = false;
//    bool inverse = false;
    int fgR=0;
    int fgG=0;
    int fgB=0;
    int bgR=0;
    int bgG=0;
    int bgB=0;
    // This combination of color values (black on black) cannot usefully be used in practice
    // - so use as initialization values
    QString fontWeight;
    QString fontStyle;
    QString textDecoration;
    bool firstSpan = true;
    for( ; x<P2.x(); x++ ) {
        if( x >= static_cast<int>(buffer[y].size()) ) {
            break;
        }
        if( firstSpan
            || buffer[y][x].fgR != fgR
            || buffer[y][x].fgG != fgG
            || buffer[y][x].fgB != fgB
            || buffer[y][x].bgR != bgR
            || buffer[y][x].bgG != bgG
            || buffer[y][x].bgB != bgB
            || bool( buffer[y][x].flags & TCHAR_BOLD ) != bold
            || bool( buffer[y][x].flags & TCHAR_UNDERLINE ) != underline
            || bool( buffer[y][x].flags & TCHAR_ITALICS ) != italics
            || bool( buffer[y][x].flags & TCHAR_STRIKEOUT ) != strikeout
//            || bool( buffer[y][x].flags & TCHAR_OVERLINE ) != overline
//            || bool( buffer[y][x].flags & TCHAR_INVERSE ) != inverse
            ) { // Can leave this on a separate line until line above uncommented.
            if( firstSpan ) {
                firstSpan = false; // The first span won't need to close the previous one
            }
            else {
                s += "</span>";
            }
            fgR = buffer[y][x].fgR;
            fgG = buffer[y][x].fgG;
            fgB = buffer[y][x].fgB;
            bgR = buffer[y][x].bgR;
            bgG = buffer[y][x].bgG;
            bgB = buffer[y][x].bgB;
            bold = buffer[y][x].flags & TCHAR_BOLD;
            italics = buffer[y][x].flags & TCHAR_ITALICS;
            underline = buffer[y][x].flags & TCHAR_UNDERLINE;
            strikeout = buffer[y][x].flags & TCHAR_STRIKEOUT;
//            overline = buffer[y][x].flags & TCHAR_OVERLINE;
//            inverse = buffer[y][x].flags & TCHAR_INVERSE;
            if( bold ) {
                fontWeight = "bold";
            }
            else {
                fontWeight = "normal";
            }
            if( italics ) {
                fontStyle = "italic";
            }
            else {
                fontStyle = "normal";
            }
            if( ! (underline || strikeout /* || overline */ ) ) {
                textDecoration = "normal";
            }
            else {
                textDecoration = "";
                if( underline ) {
                    textDecoration += "underline ";
                }
                if( strikeout ) {
                    textDecoration += "line-through ";
                }
//                if( overline ) {
//                    textDecoration += "overline ";
//                }
                textDecoration = textDecoration.trimmed();
            }
            s += "<span style=\"";
//            if( inverse )
//            {
//                s += "color: rgb(" + QString::number(bgR) + ","
//                                   + QString::number(bgG) + ","
//                                   + QString::number(bgB) + ");";
//                s += " background: rgb(" + QString::number(fgR) + ","
//                                         + QString::number(fgG) + ","
//                                         + QString::number(fgB) + ");";
//            }
//            else
//            {
            s += "color: rgb(" + QString::number(fgR) + ","
                               + QString::number(fgG) + ","
                               + QString::number(fgB) + ");";
            s += " background: rgb(" + QString::number(bgR) + ","
                                     + QString::number(bgG) + ","
                                     + QString::number(bgB) + ");";
//            }
            s += " font-weight: " + fontWeight +
                 "; font-style: " + fontStyle +
                 "; text-decoration: " + textDecoration + "\">";
        }
        if( lineBuffer[y][x] == '<' ) {
            s.append("&lt;");
        }
        else if( lineBuffer[y][x] == '>' ) {
            s.append("&gt;");
        }
        else {
            s.append(lineBuffer[y][x]);
        }
    }
    if( s.size() > 0 ) {
        s.append("</span>");
        // Needed to balance the very first open <span>, but only if we have
        // included anything. the previously appearing <br /> is an XML tag, NOT
        // a (strict) HTML 4 one
    }

    s.append( QStringLiteral( "<br>\n" ) );
    // Needed to reproduce empty lines in capture, as this method is called for
    // EACH line, even the empty ones, the spans are styled as "pre" so literal
    // linefeeds would be treated as such THERE but we deleberately place the
    // line-feeds OUTSIDE so they come under the <body>s no wrap and as such
    // line-feeds can be used to break the HTML over lots of lines (which is
    // easier to hand edit and examine afterwards) without impacting the
    // formatting. To get the line feeds at the end of displayed HTML lines the
    // <br> is used.  This slightly weird way of doing things is so that some
    // on-line tools preserve the formatting when the HTML-lised selection is
    // pasted to them AND retain the ability to paste the HTML from the
    // clipboard into a plain text editor and not have everything on one line in
    // that editor!

    return s;
}

// These tables were gleaned by comparing side-by-side the given table and the
// Iso-8859-1 one and picking out the DIFFERENCES on Wikipedia!
// Tables at: https://en.wikipedia.org/wiki/ISO/IEC_8859-XX#Codepage_layout
// replacing XX with "1" to "15" as required, each decoder defaults to using
// the built-in single byte QChar::fromLatin1(...) decoding method except for
// those that are different, sequences of differences are grouped for visual
// convenience here:
QChar TBuffer::decodeByteToIso_8859_2(const quint8 value)
{
    switch(value) {
    case 161:   return QChar(0x0104);   break;
    case 162:   return QChar(0x02D8);   break;
    case 163:   return QChar(0x0141);   break;

    case 165:   return QChar(0x013D);   break;
    case 166:   return QChar(0x015A);   break;

    case 169:   return QChar(0x0160);   break;
    case 170:   return QChar(0x015E);   break;
    case 171:   return QChar(0x0164);   break;
    case 172:   return QChar(0x0179);   break;

    case 174:   return QChar(0x017D);   break;
    case 175:   return QChar(0x017B);   break;

    case 177:   return QChar(0x0105);   break;
    case 178:   return QChar(0x02DB);   break;
    case 179:   return QChar(0x0142);   break;

    case 181:   return QChar(0x013E);   break;
    case 182:   return QChar(0x015B);   break;
    case 183:   return QChar(0x02C7);   break;
    case 184:   return QChar(0x00B8);   break;
    case 185:   return QChar(0x0161);   break;
    case 186:   return QChar(0x015F);   break;
    case 187:   return QChar(0x0165);   break;
    case 188:   return QChar(0x017A);   break;
    case 189:   return QChar(0x02DD);   break;
    case 190:   return QChar(0x017E);   break;
    case 191:   return QChar(0x017C);   break;
    case 192:   return QChar(0x0154);   break;

    case 195:   return QChar(0x0102);   break;

    case 197:   return QChar(0x0139);   break;
    case 198:   return QChar(0x0106);   break;

    case 200:   return QChar(0x010C);   break;

    case 202:   return QChar(0x0118);   break;

    case 204:   return QChar(0x011A);   break;

    case 207:   return QChar(0x010E);   break;
    case 208:   return QChar(0x0110);   break;
    case 209:   return QChar(0x0143);   break;
    case 210:   return QChar(0x0147);   break;

    case 213:   return QChar(0x0150);   break;
    case 214:   return QChar(0x00D6);   break;

    case 216:   return QChar(0x0158);   break;
    case 217:   return QChar(0x016E);   break;

    case 219:   return QChar(0x0170);   break;

    case 222:   return QChar(0x0162);   break;

    case 224:   return QChar(0x0155);   break;

    case 227:   return QChar(0x0103);   break;

    case 229:   return QChar(0x013A);   break;
    case 230:   return QChar(0x0107);   break;

    case 232:   return QChar(0x010D);   break;

    case 234:   return QChar(0x0119);   break;

    case 236:   return QChar(0x011B);   break;

    case 239:   return QChar(0x010F);   break;
    case 240:   return QChar(0x0111);   break;
    case 241:   return QChar(0x0144);   break;
    case 242:   return QChar(0x0148);   break;

    case 245:   return QChar(0x0151);   break;

    case 248:   return QChar(0x0159);   break;
    case 249:   return QChar(0x016F);   break;

    case 251:   return QChar(0x0171);   break;

    case 254:   return QChar(0x0163);   break;
    case 255:   return QChar(0x02D9);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToIso_8859_3(const quint8 value)
{
    switch(value) {
    case 161:   return QChar(0x0126);   break;
    case 162:   return QChar(0x02D8);   break;

    case 165:   return QChar::ReplacementCharacter;   break; // Unassigned?
    case 166:   return QChar(0x0124);   break;

    case 169:   return QChar(0x0130);   break;
    case 170:   return QChar(0x015E);   break;
    case 171:   return QChar(0x011E);   break;
    case 172:   return QChar(0x0134);   break;

    case 174:   return QChar::ReplacementCharacter;   break; // Unassigned?
    case 175:   return QChar(0x017B);   break;

    case 177:   return QChar(0x0127);   break;

    case 182:   return QChar(0x0125);   break;

    case 185:   return QChar(0x0131);   break;
    case 186:   return QChar(0x015F);   break;
    case 187:   return QChar(0x011F);   break;
    case 188:   return QChar(0x0135);   break;

    case 190:   return QChar::ReplacementCharacter;   break; // Unassigned?
    case 191:   return QChar(0x017C);   break;

    case 195:   return QChar::ReplacementCharacter;   break; // Unassigned?

    case 197:   return QChar(0x010A);   break;
    case 198:   return QChar(0x0108);   break;

    case 208:   return QChar::ReplacementCharacter;   break; // Unassigned?

    case 216:   return QChar(0x011C);   break;

    case 221:   return QChar(0x016C);   break;
    case 222:   return QChar(0x015C);   break;

    case 227:   return QChar::ReplacementCharacter;   break; // Unassigned?

    case 229:   return QChar(0x010B);   break;
    case 230:   return QChar(0x0109);   break;

    case 240:   return QChar::ReplacementCharacter;   break; // Unassigned?

    case 245:   return QChar(0x0121);   break;

    case 248:   return QChar(0x011D);   break;

    case 253:   return QChar(0x016D);   break;
    case 254:   return QChar(0x015D);   break;
    case 255:   return QChar(0x02D9);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToIso_8859_4(const quint8 value)
{
    switch(value) {
    case 161:   return QChar(0x0104);   break;
    case 162:   return QChar(0x0138);   break;
    case 163:   return QChar(0x0156);   break;

    case 165:   return QChar(0x0128);   break;
    case 166:   return QChar(0x013B);   break;

    case 169:   return QChar(0x0160);   break;
    case 170:   return QChar(0x0112);   break;
    case 171:   return QChar(0x0122);   break;
    case 172:   return QChar(0x0166);   break;

    case 174:   return QChar(0x017D);   break;

    case 177:   return QChar(0x0105);   break;
    case 178:   return QChar(0x02DB);   break;
    case 179:   return QChar(0x0157);   break;

    case 181:   return QChar(0x0129);   break;
    case 182:   return QChar(0x013C);   break;
    case 183:   return QChar(0x02C7);   break;

    case 185:   return QChar(0x0161);   break;
    case 186:   return QChar(0x0113);   break;
    case 187:   return QChar(0x0123);   break;
    case 188:   return QChar(0x0167);   break;
    case 189:   return QChar(0x014A);   break;
    case 190:   return QChar(0x017E);   break;
    case 191:   return QChar(0x014B);   break;
    case 192:   return QChar(0x0100);   break;

    case 199:   return QChar(0x012E);   break;
    case 200:   return QChar(0x010C);   break;

    case 202:   return QChar(0x0118);   break;

    case 204:   return QChar(0x0116);   break;

    case 207:   return QChar(0x012A);   break;
    case 208:   return QChar(0x0110);   break;
    case 209:   return QChar(0x0145);   break;
    case 210:   return QChar(0x014C);   break;
    case 211:   return QChar(0x0136);   break;

    case 217:   return QChar(0x0172);   break;

    case 221:   return QChar(0x0168);   break;
    case 222:   return QChar(0x016A);   break;

    case 224:   return QChar(0x0101);   break;

    case 231:   return QChar(0x012F);   break;
    case 232:   return QChar(0x010D);   break;

    case 234:   return QChar(0x0119);   break;

    case 236:   return QChar(0x0117);   break;

    case 239:   return QChar(0x012B);   break;

    case 240:   return QChar(0x0111);   break;
    case 241:   return QChar(0x0146);   break;
    case 242:   return QChar(0x014D);   break;
    case 243:   return QChar(0x0137);   break;

    case 249:   return QChar(0x0173);   break;

    case 253:   return QChar(0x0169);   break;
    case 254:   return QChar(0x016B);   break;
    case 255:   return QChar(0x02D9);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToIso_8859_10(const quint8 value)
{
    switch(value) {
    case 161:   return QChar(0x0104);   break;
    case 162:   return QChar(0x0112);   break;
    case 163:   return QChar(0x0122);   break;
    case 164:   return QChar(0x012A);   break;
    case 165:   return QChar(0x0128);   break;
    case 166:   return QChar(0x0136);   break;

    case 168:   return QChar(0x013B);   break;
    case 169:   return QChar(0x0110);   break;
    case 170:   return QChar(0x0160);   break;
    case 171:   return QChar(0x0166);   break;
    case 172:   return QChar(0x017D);   break;

    case 174:   return QChar(0x016A);   break;
    case 175:   return QChar(0x014A);   break;

    case 177:   return QChar(0x0105);   break;
    case 178:   return QChar(0x0113);   break;
    case 179:   return QChar(0x0123);   break;
    case 180:   return QChar(0x012B);   break;
    case 181:   return QChar(0x0129);   break;
    case 182:   return QChar(0x0137);   break;

    case 184:   return QChar(0x013C);   break;
    case 185:   return QChar(0x0111);   break;
    case 186:   return QChar(0x0161);   break;
    case 187:   return QChar(0x0167);   break;
    case 188:   return QChar(0x017E);   break;
    case 189:   return QChar(0x2015);   break;
    case 190:   return QChar(0x016B);   break;
    case 191:   return QChar(0x014B);   break;
    case 192:   return QChar(0x0100);   break;

    case 199:   return QChar(0x012E);   break;
    case 200:   return QChar(0x010C);   break;

    case 202:   return QChar(0x0118);   break;

    case 204:   return QChar(0x0116);   break;

    case 209:   return QChar(0x0145);   break;
    case 210:   return QChar(0x014C);   break;

    case 215:   return QChar(0x0168);   break;

    case 217:   return QChar(0x0172);   break;

    case 224:   return QChar(0x0101);   break;

    case 231:   return QChar(0x012F);   break;
    case 232:   return QChar(0x010D);   break;

    case 234:   return QChar(0x0119);   break;

    case 236:   return QChar(0x0117);   break;

    case 239:   return QChar(0x012B);   break;

    case 241:   return QChar(0x0146);   break;
    case 242:   return QChar(0x014D);   break;

    case 247:   return QChar(0x0169);   break;

    case 249:   return QChar(0x0173);   break;

    case 255:   return QChar(0x0138);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToIso_8859_15(const quint8 value)
{
    switch(value) {
    case 164:   return QChar(0x20AC);   break;

    case 166:   return QChar(0x0160);   break;

    case 168:   return QChar(0x0161);   break;

    case 180:   return QChar(0x017D);   break;

    case 184:   return QChar(0x017E);   break;

    case 188:   return QChar(0x0152);   break;
    case 189:   return QChar(0x0153);   break;
    case 190:   return QChar(0x0178);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToIso_8859_16(const quint8 value)
{
    switch(value) {
    case 161:   return QChar(0x0104);   break;
    case 162:   return QChar(0x0105);   break;
    case 163:   return QChar(0x0141);   break;
    case 164:   return QChar(0x20AC);   break;
    case 165:   return QChar(0x201E);   break;
    case 166:   return QChar(0x0160);   break;

    case 168:   return QChar(0x0161);   break;

    case 170:   return QChar(0x0218);   break;

    case 172:   return QChar(0x0179);   break;

    case 174:   return QChar(0x017A);   break;
    case 175:   return QChar(0x017B);   break;

    case 178:   return QChar(0x010C);   break;
    case 179:   return QChar(0x0142);   break;
    case 180:   return QChar(0x017D);   break;
    case 181:   return QChar(0x201D);   break;

    case 184:   return QChar(0x017E);   break;
    case 185:   return QChar(0x010D);   break;
    case 186:   return QChar(0x0219);   break;

    case 188:   return QChar(0x0152);   break;
    case 189:   return QChar(0x0153);   break;
    case 190:   return QChar(0x0178);   break;
    case 191:   return QChar(0x017C);   break;

    case 195:   return QChar(0x0102);   break;

    case 197:   return QChar(0x0106);   break;

    case 208:   return QChar(0x0110);   break;
    case 209:   return QChar(0x0143);   break;

    case 213:   return QChar(0x0150);   break;

    case 215:   return QChar(0x015A);   break;
    case 216:   return QChar(0x0170);   break;

    case 221:   return QChar(0x0118);   break;
    case 222:   return QChar(0x021A);   break;

    case 227:   return QChar(0x0103);   break;

    case 229:   return QChar(0x0107);   break;

    case 240:   return QChar(0x0111);   break;
    case 241:   return QChar(0x0144);   break;

    case 245:   return QChar(0x0151);   break;

    case 247:   return QChar(0x015B);   break;
    case 248:   return QChar(0x0171);   break;

    case 253:   return QChar(0x0119);   break;
    case 254:   return QChar(0x021B);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToWindows_1250(const quint8 value)
{
    switch(value) {
    case 128:   return QChar(0x20AC);   break;
    case 129:   return QChar::ReplacementCharacter; break;
    case 130:   return QChar(0x201A);   break;
    case 131:   return QChar::ReplacementCharacter; break;
    case 132:   return QChar(0x201E);   break;
    case 133:   return QChar(0x2026);   break;
    case 134:   return QChar(0x2020);   break;
    case 135:   return QChar(0x2021);   break;
    case 136:   return QChar::ReplacementCharacter; break;
    case 137:   return QChar(0x2030);   break;
    case 138:   return QChar(0x0160);   break;
    case 139:   return QChar(0x2039);   break;
    case 140:   return QChar(0x015A);   break;
    case 141:   return QChar(0x0164);   break;
    case 142:   return QChar(0x017D);   break;
    case 143:   return QChar(0x0179);   break;
    case 144:   return QChar::ReplacementCharacter; break;
    case 145:   return QChar(0x2018);   break;
    case 146:   return QChar(0x2019);   break;
    case 147:   return QChar(0x201C);   break;
    case 148:   return QChar(0x201D);   break;
    case 149:   return QChar(0x2022);   break;
    case 150:   return QChar(0x2013);   break;
    case 151:   return QChar(0x2014);   break;
    case 152:   return QChar::ReplacementCharacter; break;
    case 153:   return QChar(0x2122);   break;
    case 154:   return QChar(0x0161);   break;
    case 155:   return QChar(0x203A);   break;
    case 156:   return QChar(0x015B);   break;
    case 157:   return QChar(0x0165);   break;
    case 158:   return QChar(0x017E);   break;
    case 159:   return QChar(0x017A);   break;

    case 161:   return QChar(0x02C7);   break;
    case 162:   return QChar(0x02D8);   break;
    case 163:   return QChar(0x0141);   break;

    case 165:   return QChar(0x0104);   break;

    case 170:   return QChar(0x015E);   break;

    case 175:   return QChar(0x017B);   break;

    case 178:   return QChar(0x02DB);   break;
    case 179:   return QChar(0x0142);   break;

    case 185:   return QChar(0x0105);   break;
    case 186:   return QChar(0x015F);   break;

    case 188:   return QChar(0x013D);   break;
    case 189:   return QChar(0x02DD);   break;
    case 190:   return QChar(0x013E);   break;
    case 191:   return QChar(0x017C);   break;
    case 192:   return QChar(0x0154);   break;

    case 195:   return QChar(0x0102);   break;

    case 197:   return QChar(0x0139);   break;
    case 198:   return QChar(0x0106);   break;

    case 200:   return QChar(0x010C);   break;

    case 202:   return QChar(0x0118);   break;

    case 204:   return QChar(0x011A);   break;

    case 207:   return QChar(0x010E);   break;
    case 208:   return QChar(0x0110);   break;
    case 209:   return QChar(0x0143);   break;
    case 210:   return QChar(0x0147);   break;

    case 213:   return QChar(0x0150);   break;
    case 214:   return QChar(0x00D6);   break;

    case 216:   return QChar(0x0158);   break;
    case 217:   return QChar(0x016E);   break;

    case 219:   return QChar(0x0170);   break;

    case 222:   return QChar(0x0162);   break;

    case 224:   return QChar(0x0155);   break;

    case 227:   return QChar(0x0103);   break;

    case 229:   return QChar(0x013A);   break;
    case 230:   return QChar(0x0107);   break;

    case 232:   return QChar(0x010D);   break;

    case 234:   return QChar(0x0119);   break;

    case 236:   return QChar(0x011B);   break;

    case 239:   return QChar(0x010F);   break;
    case 240:   return QChar(0x0111);   break;
    case 241:   return QChar(0x0144);   break;
    case 242:   return QChar(0x0148);   break;

    case 245:   return QChar(0x0151);   break;

    case 248:   return QChar(0x0159);   break;
    case 249:   return QChar(0x016F);   break;

    case 251:   return QChar(0x0171);   break;

    case 254:   return QChar(0x0163);   break;
    case 255:   return QChar(0x02D9);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToWindows_1251(const quint8 value)
{
    switch(value) {
    case 128:   return QChar(0x0402);   break;
    case 129:   return QChar(0x0403);   break;
    case 130:   return QChar(0x201A);   break;
    case 131:   return QChar(0x0453);   break;
    case 132:   return QChar(0x201E);   break;
    case 133:   return QChar(0x2026);   break;
    case 134:   return QChar(0x2020);   break;
    case 135:   return QChar(0x2021);   break;
    case 136:   return QChar(0x20AC);   break;
    case 137:   return QChar(0x2030);   break;
    case 138:   return QChar(0x0409);   break;
    case 139:   return QChar(0x2039);   break;
    case 140:   return QChar(0x040A);   break;
    case 141:   return QChar(0x040C);   break;
    case 142:   return QChar(0x040B);   break;
    case 143:   return QChar(0x040F);   break;
    case 144:   return QChar(0x0452);   break;
    case 145:   return QChar(0x2018);   break;
    case 146:   return QChar(0x2019);   break;
    case 147:   return QChar(0x201C);   break;
    case 148:   return QChar(0x201D);   break;
    case 149:   return QChar(0x2022);   break;
    case 150:   return QChar(0x2013);   break;
    case 151:   return QChar(0x2014);   break;
    case 152:   return QChar::ReplacementCharacter; break;
    case 153:   return QChar(0x2122);   break;
    case 154:   return QChar(0x0459);   break;
    case 155:   return QChar(0x203A);   break;
    case 156:   return QChar(0x045A);   break;
    case 157:   return QChar(0x045C);   break;
    case 158:   return QChar(0x045B);   break;
    case 159:   return QChar(0x045F);   break;

    case 161:   return QChar(0x040E);   break;
    case 162:   return QChar(0x045E);   break;
    case 163:   return QChar(0x0408);   break;

    case 165:   return QChar(0x0490);   break;

    case 168:   return QChar(0x0401);   break;

    case 170:   return QChar(0x0404);   break;

    case 175:   return QChar(0x0407);   break;

    case 178:   return QChar(0x0406);   break;
    case 179:   return QChar(0x0456);   break;
    case 180:   return QChar(0x0491);   break;

    case 184:   return QChar(0x0451);   break;
    case 185:   return QChar(0x2116);   break;
    case 186:   return QChar(0x0454);   break;

    case 188:   return QChar(0x0458);   break;
    case 189:   return QChar(0x0405);   break;
    case 190:   return QChar(0x0455);   break;
    case 191:   return QChar(0x0457);   break;
    case 192:   return QChar(0x0410);   break;
    case 193:   return QChar(0x0411);   break;
    case 194:   return QChar(0x0412);   break;
    case 195:   return QChar(0x0413);   break;
    case 196:   return QChar(0x0414);   break;
    case 197:   return QChar(0x0415);   break;
    case 198:   return QChar(0x0416);   break;
    case 199:   return QChar(0x0417);   break;
    case 200:   return QChar(0x0418);   break;
    case 201:   return QChar(0x0419);   break;
    case 202:   return QChar(0x041A);   break;
    case 203:   return QChar(0x041B);   break;
    case 204:   return QChar(0x041C);   break;
    case 205:   return QChar(0x041D);   break;
    case 206:   return QChar(0x041E);   break;
    case 207:   return QChar(0x041F);   break;
    case 208:   return QChar(0x0420);   break;
    case 209:   return QChar(0x0421);   break;
    case 210:   return QChar(0x0422);   break;
    case 211:   return QChar(0x0423);   break;
    case 212:   return QChar(0x0424);   break;
    case 213:   return QChar(0x0425);   break;
    case 214:   return QChar(0x0426);   break;
    case 215:   return QChar(0x0427);   break;
    case 216:   return QChar(0x0428);   break;
    case 217:   return QChar(0x0429);   break;
    case 218:   return QChar(0x042A);   break;
    case 219:   return QChar(0x042B);   break;
    case 220:   return QChar(0x042C);   break;
    case 221:   return QChar(0x042D);   break;
    case 222:   return QChar(0x042E);   break;
    case 223:   return QChar(0x042F);   break;
    case 224:   return QChar(0x0430);   break;
    case 225:   return QChar(0x0431);   break;
    case 226:   return QChar(0x0432);   break;
    case 227:   return QChar(0x0433);   break;
    case 228:   return QChar(0x0434);   break;
    case 229:   return QChar(0x0435);   break;
    case 230:   return QChar(0x0436);   break;
    case 231:   return QChar(0x0437);   break;
    case 232:   return QChar(0x0438);   break;
    case 233:   return QChar(0x0439);   break;
    case 234:   return QChar(0x043A);   break;
    case 235:   return QChar(0x043B);   break;
    case 236:   return QChar(0x043C);   break;
    case 237:   return QChar(0x043D);   break;
    case 238:   return QChar(0x043E);   break;
    case 239:   return QChar(0x043F);   break;
    case 240:   return QChar(0x0440);   break;
    case 241:   return QChar(0x0441);   break;
    case 242:   return QChar(0x0442);   break;
    case 243:   return QChar(0x0443);   break;
    case 244:   return QChar(0x0444);   break;
    case 245:   return QChar(0x0445);   break;
    case 246:   return QChar(0x0446);   break;
    case 247:   return QChar(0x0447);   break;
    case 248:   return QChar(0x0448);   break;
    case 249:   return QChar(0x0449);   break;
    case 250:   return QChar(0x044A);   break;
    case 251:   return QChar(0x044B);   break;
    case 252:   return QChar(0x044C);   break;
    case 253:   return QChar(0x044D);   break;
    case 254:   return QChar(0x044E);   break;
    case 255:   return QChar(0x044F);   break;

    default:    return QChar::fromLatin1(value);
    }
}

QChar TBuffer::decodeByteToWindows_1252(const quint8 value)
{
    switch(value) {
    case 128:   return QChar(0x20AC);   break;
    case 129:   return QChar::ReplacementCharacter; break;
    case 130:   return QChar(0x201A);   break;
    case 131:   return QChar(0x0192);   break;
    case 132:   return QChar(0x201E);   break;
    case 133:   return QChar(0x2026);   break;
    case 134:   return QChar(0x2020);   break;
    case 135:   return QChar(0x2021);   break;
    case 136:   return QChar(0x02C6);   break;
    case 137:   return QChar(0x2030);   break;
    case 138:   return QChar(0x0160);   break;
    case 139:   return QChar(0x2039);   break;
    case 140:   return QChar(0x0152);   break;
    case 141:   return QChar::ReplacementCharacter; break;
    case 142:   return QChar(0x017D);   break;
    case 143:   return QChar::ReplacementCharacter; break;
    case 144:   return QChar::ReplacementCharacter; break;
    case 145:   return QChar(0x2018);   break;
    case 146:   return QChar(0x2019);   break;
    case 147:   return QChar(0x201C);   break;
    case 148:   return QChar(0x201D);   break;
    case 149:   return QChar(0x2022);   break;
    case 150:   return QChar(0x2013);   break;
    case 151:   return QChar(0x2014);   break;
    case 152:   return QChar(0x02DC);   break;
    case 153:   return QChar(0x2122);   break;
    case 154:   return QChar(0x0161);   break;
    case 155:   return QChar(0x203A);   break;
    case 156:   return QChar(0x0153);   break;
    case 157:   return QChar::ReplacementCharacter; break;
    case 158:   return QChar(0x017E);   break;
    case 159:   return QChar(0x0178);   break;

    default:    return QChar::fromLatin1(value);
    }
}
