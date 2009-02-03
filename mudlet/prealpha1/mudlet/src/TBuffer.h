/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
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
    bool insertInLine( QPoint & cursor, QString & what );
    void expandLine( int y, int count, TChar * pC );
    void wrap( int startLine, int screenWidth, int indentSize );
    int size(){ return buffer.size(); }    
    QString & line( int n );
    int find( int line, QString what, int pos );
    QStringList split( int line, QString splitter );
    QStringList split( int line, QRegExp splitter );
    bool replace( int line, QString what, QString with );
    bool replace( QPoint & start, QPoint & end, QString & with );
    bool deleteLine( int );
    bool deleteLines( int from, int to );
    bool applyFormat( int line, int x1, int x2, TChar & format );
    bool moveCursor( QPoint & where );
    QPoint & insertText( QString & what, QPoint & where );
    int getLastLineNumber();
    
    std::deque<TChar *> bufferLine;
    std::deque< std::deque<TChar*> > buffer; 
    
    QStringList lineBuffer;
    
    int newLines;

    
private:  
    
    void handleNewLine();
    
    Host * mpHost;
    int maxx;
    int maxy;
    bool hadLF;
    int mLastLine;
    bool mCursorMoved;
    
    
};

#endif
