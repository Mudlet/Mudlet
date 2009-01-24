/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
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

#include <QDebug>
#include <stdio.h>
#include <iostream>
#include <string>
#include <deque>
#include <queue>
#include "TBuffer.h"
#include "Host.h"

using namespace std;


TBuffer::TBuffer( Host * pH )
: mpHost( pH )
{   
    mpHost->mScreenHeight = 40;
    mpHost->mScreenWidth = 80; //TODO: make this a user option
    
    buffer.clear();
    buffer.push_back( bufferLine );
    lineBuffer.clear();
    QString nothing;
    lineBuffer.append( nothing );
    newLines = 0;
}

void TBuffer::handleNewLine()
{
    hadLF = true;
    std::deque<TChar *> newLine;
    QString nextLine = "";
    buffer.push_back( newLine );
    lineBuffer.append( nextLine );
    newLines++;
}

void TBuffer::addText( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
    QStringList lines = text.split( '\n' );
    for( int i=0; i<lines.size(); i++ )
    {
        // the content of lines[0] is to be added to the last line in the buffer
        // unless lines[0] is itself an <LF>
        // all other lines represent corresponding <LF>
        if( i > 0 ) handleNewLine();
        
        if( lines[i].size() == 0 ) 
        {
            // <LF> 
            continue;
        }
        
        lineBuffer[buffer.size()-1].append( lines[i] );
        
        for( int i2=0; i2<lines[i].size(); i2++ )
        {
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[buffer.size()-1].push_back( pC );
        }
    }
}

void TBuffer::wrap( unsigned int startLine, unsigned int screenWidth, unsigned int indentSize )
{
    if( buffer.size() <= startLine ) return;
    
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    int lineCount = 0;
    
    for( unsigned int i=startLine; i<buffer.size(); i++ )
    {
        std::deque<TChar *> newLine;
        QString lineText;
        
        int indent = 0;
        if( buffer[i].size() >= screenWidth )
        {
            for( unsigned int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = mpHost->mFgColor;
                pSpace->bgColor = mpHost->mBgColor;
                pSpace->italics = false;
                pSpace->bold = false;
                pSpace->underline = false;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        for( unsigned int i2=0; i2<buffer[i].size();  )
        {  
            for( unsigned int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( i2 >= buffer[i].size() )
                {
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
    
    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();    
        lineBuffer.pop_back();
    }
    
    newLines -= lineCount;
    newLines += queue.size();
    
    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }
    
    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
    }
}

QString badLineError = QString("ERROR: invalid line number");

QString & TBuffer::line( int n )
{
    if( (n > lineBuffer.size()) || (n<0) ) return badLineError;
    return lineBuffer[n];
}

/*int TBuffer::find( int line, QString what, int pos=0 )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return -1;
    return lineBuffer[line].indexOf( what, pos );
}

QStringList TBuffer::split( int line, QString splitter )
{
    if( lineBuffer[line].size() >= pos ) return QStringList();
    if( pos < 0 ) return QStringList();
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return QStringList;   
    return lineBuffer[line].split( splitter );
}

QStringList TBuffer::split( int line, QRegExp splitter )
{
    if( lineBuffer[line].size() >= pos ) return QStringList();
    if( pos < 0 ) return QStringList();
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return QStringList;   
    return lineBuffer[line].split( splitter );
}

void TBuffer::replace( int line, QString what, QString with )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return -1;
    lineBuffer[line].replace( what, with );
}*/