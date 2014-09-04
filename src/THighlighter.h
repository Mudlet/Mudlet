#ifndef MUDLET_THIGHLIGHTER_H
#define MUDLET_THIGHLIGHTER_H

/***************************************************************************
 *   Copyright (C) 2009 by Heiko Koehn - KoehnHeiko@googlemail.com         *
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


#include "pre_guard.h"
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "post_guard.h"

class QTextDocument;


class THighlighter : public QSyntaxHighlighter
{
     Q_OBJECT

 public:
                               THighlighter(QTextDocument *parent = 0);
     void                      setSearchPattern( QString p );


 protected:
     void                      highlightBlock(const QString &text);

 private:
     struct HighlightingRule
     {
         QRegExp pattern;
         QTextCharFormat format;
     };
     QString                   mSearchPattern;
     QVector<HighlightingRule> highlightingRules;
     QRegExp                   commentStartExpression;
     QRegExp                   commentEndExpression;
     QRegExp                   stringStart;
     QRegExp                   stringEnd;
     QTextCharFormat           keywordFormat;
     QTextCharFormat           searchFormat;
     QTextCharFormat           classFormat;
     QTextCharFormat           singleLineCommentFormat;
     QTextCharFormat           multiLineCommentFormat;
     QTextCharFormat           quotationFormat;
     QTextCharFormat           functionFormat;
     //bool isString;
 };

#endif // MUDLET_THIGHLIGHTER_H
