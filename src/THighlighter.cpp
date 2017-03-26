/***************************************************************************
 *   Copyright (C) 2009-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "THighlighter.h"


THighlighter::THighlighter( QTextDocument *parent )
: QSyntaxHighlighter( parent )
{
    //isString = false;
    HighlightingRule rule;

    keywordFormat.setForeground( Qt::darkBlue );
    keywordFormat.setFontWeight( QFont::Bold );
    QStringList keywordPatterns;
    keywordPatterns << "\\btrue\\b" << "\\bfalse\\b" << "\\bnil\\b"
                    << "\\bnot\\b" << "\\band\\b" << "\\bor\\b"
                    << "\\bfunction\\b" << "\\blocal\\b" << "\\end\\b"
                    << "\\bwhile\\b" << "\\bdo\\b" << "\\bif\\b"
                    << "\\bthen\\b" << "\\belse\\b" << "\\bwhile\\b"
                    << "\\brepeat\\b" << "\\bfor\\b" << "\\bpairs\\b"
                    << "\\bipairs\\b" << "\\bin\\b" << "\\buntil\\b"
                    << "\\bbreak\\b" << "\\breturn\\b" << "\\belseif\\b";
    foreach( QString pattern, keywordPatterns )
    {
        rule.pattern = QRegExp( pattern );
        rule.format = keywordFormat;
        highlightingRules.append( rule );
    }



    classFormat.setFontWeight( QFont::Bold );
    classFormat.setForeground( Qt::darkMagenta );
    rule.pattern = QRegExp( "\\bQ[A-Za-z]+\\b" );
    rule.format = classFormat;
    highlightingRules.append( rule );

    functionFormat.setFontItalic( false );
    functionFormat.setFontWeight( QFont::Bold );
    functionFormat.setForeground( Qt::black );
    rule.pattern = QRegExp( "\\b[A-Za-z0-9_]+(?=\\()" );
    rule.format = functionFormat;
    highlightingRules.append( rule );

    quotationFormat.setForeground( Qt::darkGreen );
    rule.pattern = QRegExp( "\"[^\"]*\"" );
    rule.format = quotationFormat;
    highlightingRules.append( rule );

    rule.pattern = QRegExp( "\\[\\[[^\\]\\]]*\\]\\]" );
    rule.format = quotationFormat;
    highlightingRules.append( rule );

    singleLineCommentFormat.setForeground( Qt::darkGreen );
    rule.pattern = QRegExp( "--[^\n]*" );//\\[\\]]*" );
    rule.format = singleLineCommentFormat;
    highlightingRules.append( rule );

    multiLineCommentFormat.setForeground( Qt::darkGreen );

    stringStart = QRegExp( "\\[\\[" );
    stringEnd = QRegExp( "\\]\\]" );

    commentStartExpression = QRegExp( "\\[\\[" );
    commentEndExpression = QRegExp( "\\]\\]" );
    //highlightingRules.append( rule );


    searchFormat.setForeground( QColor(0,0,0) );
    searchFormat.setBackground( QColor(255,255,0) );
    mSearchPattern = "MudletTheMUDClient";

    searchFormat.setForeground( QColor(0,0,0) );
    searchFormat.setBackground( QColor(255,255,0) );

    rule.pattern = QRegExp( mSearchPattern );
    rule.format = searchFormat;
    highlightingRules.append( rule );
}

void THighlighter::setSearchPattern( QString p )
{
    HighlightingRule rule;
    mSearchPattern = QRegExp::escape( p );
    searchFormat.setForeground( QColor(0,0,0) );
    searchFormat.setBackground( QColor(255,255,0) );

    rule.pattern = QRegExp( mSearchPattern );
    rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
    rule.format = searchFormat;
    highlightingRules.last() = rule;
}

void THighlighter::highlightBlock( const QString & text )
{
    if( text.size() < 1 ) return;
    foreach( HighlightingRule rule, highlightingRules )
    {
        QRegExp expression( rule.pattern );
        int index = text.indexOf( expression );
        while( index >= 0 )
        {
            int length = expression.matchedLength();
            setFormat( index, length, rule.format );
            index = text.indexOf( expression, index + length );
        }
    }
    setCurrentBlockState( 0 );
    int startIndex = 0;
    if( previousBlockState() != 1 )
    {
        startIndex = text.indexOf( commentStartExpression );
    }
    while( startIndex >= 0 )
    {
        int endIndex = text.indexOf( commentEndExpression, startIndex );
        int commentLength;
        if( endIndex == -1 )
        {
            setCurrentBlockState( 1 );
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex + 2;
        }
        setFormat( startIndex, commentLength, multiLineCommentFormat );
        startIndex = text.indexOf( commentStartExpression, startIndex + commentLength );
    }


    QRegExp expression( highlightingRules.last().pattern );
    int index = text.indexOf( mSearchPattern, 0, Qt::CaseInsensitive );//expression );
    while( index >= 0 )
    {
        int length = mSearchPattern.length();//expression.matchedLength();
        setFormat( index, length, highlightingRules.last().format );
        //index = text.indexOf( expression, index + length );
        index = text.indexOf( mSearchPattern, index + length, Qt::CaseInsensitive );
    }
}
