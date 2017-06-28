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


THighlighter::THighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << R"(\btrue\b)"
                    << R"(\bfalse\b)"
                    << R"(\bnil\b)"
                    << R"(\bnot\b)"
                    << R"(\band\b)"
                    << R"(\bor\b)"
                    << R"(\bfunction\b)"
                    << R"(\blocal\b)"
                    << R"(\end\b)"
                    << R"(\bwhile\b)"
                    << R"(\bdo\b)"
                    << R"(\bif\b)"
                    << R"(\bthen\b)"
                    << R"(\belse\b)"
                    << R"(\bwhile\b)"
                    << R"(\brepeat\b)"
                    << R"(\bfor\b)"
                    << R"(\bpairs\b)"
                    << R"(\bipairs\b)"
                    << R"(\bin\b)"
                    << R"(\buntil\b)"
                    << R"(\bbreak\b)"
                    << R"(\breturn\b)"
                    << R"(\belseif\b)";
    foreach (QString pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }


    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp(R"(\bQ[A-Za-z]+\b)");
    rule.format = classFormat;
    highlightingRules.append(rule);

    functionFormat.setFontItalic(false);
    functionFormat.setFontWeight(QFont::Bold);
    functionFormat.setForeground(Qt::black);
    rule.pattern = QRegExp(R"(\b[A-Za-z0-9_]+(?=\())");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp(R"("[^"]*")");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegExp(R"(\[\[[^\]\]]*\]\])");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("--[^\n]*"); //\\[\\]]*" );
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::darkGreen);

    stringStart = QRegExp(R"(\[\[)");
    stringEnd = QRegExp(R"(\]\])");

    commentStartExpression = QRegExp(R"(\[\[)");
    commentEndExpression = QRegExp(R"(\]\])");


    searchFormat.setForeground(QColor(Qt::black));
    searchFormat.setBackground(QColor(Qt::yellow));
    mSearchPattern = "MudletTheMUDClient";

    searchFormat.setForeground(QColor(Qt::black));
    searchFormat.setBackground(QColor(Qt::yellow));

    rule.pattern = QRegExp(mSearchPattern);
    rule.format = searchFormat;
    highlightingRules.append(rule);
}

void THighlighter::setSearchPattern(QString p)
{
    HighlightingRule rule;
    mSearchPattern = QRegExp::escape(p);
    searchFormat.setForeground(QColor(Qt::black));
    searchFormat.setBackground(QColor(Qt::yellow));

    rule.pattern = QRegExp(mSearchPattern);
    rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
    rule.format = searchFormat;
    highlightingRules.last() = rule;
}

void THighlighter::highlightBlock(const QString& text)
{
    if (text.size() < 1) {
        return;
    }
    foreach (HighlightingRule rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }
    while (startIndex >= 0) {
        int endIndex = text.indexOf(commentEndExpression, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + 2;
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }


    QRegExp expression(highlightingRules.last().pattern);
    int index = text.indexOf(mSearchPattern, 0, Qt::CaseInsensitive);
    while (index >= 0) {
        int length = mSearchPattern.length();
        setFormat(index, length, highlightingRules.last().format);
        index = text.indexOf(mSearchPattern, index + length, Qt::CaseInsensitive);
    }
}
