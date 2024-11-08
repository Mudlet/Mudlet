/***************************************************************************
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

#include "TriggerHighlighter.h"

TriggerHighlighter::TriggerHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    initialiseRules();
}

// Processing starts from top to bottom, so put higher priority colours last.
void TriggerHighlighter::initialiseRules()
{
    // a muted pastel colour range
    QColor red(255, 239, 239);
    QColor orange(255, 223, 186, 255);
    QColor yellow(255, 255, 186, 255);
    QColor green(186, 255, 201, 255);
    QColor blue(186, 225, 255, 255);
    QColor gray(241, 241, 241);

    // (pattern), (?:pattern), (?=pattern) (?<capture>)
    groupFormat.setForeground(Qt::darkGreen);
    groupFormat.setBackground(QBrush(green));
    groupFormat.setFontWeight(QFont::Bold);
    //highlightingRules.append({QRegularExpression(R"(\([?][=:!<]|\\\(|\>|\))"), groupFormat});
    highlightingRules.append({QRegularExpression(R"(\(.*\))"), groupFormat});
    //highlightingRules.append({QRegularExpression(R"(\(|\))"), groupFormat});

    // ^ $ ? \b \B
    anchorFormat.setForeground(Qt::red);
    anchorFormat.setBackground(QBrush(red));
    anchorFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"(\^|\$|\?|\\[bB])"), anchorFormat});

    // * + ? . [a-z] {m,n}
    quantifierFormat.setForeground(Qt::black);
    quantifierFormat.setBackground(QBrush(orange));
    quantifierFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"([*+.]|{[0-9,]*})"), quantifierFormat});
    highlightingRules.append({QRegularExpression(R"((\\[dDsSwW]|\[[^\]]+\]))"), quantifierFormat});

    // \n \r \t and octal codes
    escapeCharFormat.setForeground(Qt::black);
    escapeCharFormat.setBackground(yellow);
    escapeCharFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"(\\[nrtvfae]|\\[xXuU][0-9A-Fa-f]+|\\[0-7]{1,3})"), escapeCharFormat});

    // highlight spaces
    spaceCharFormat.setBackground(gray);
    highlightingRules.append({QRegularExpression(R"(\s+)"), spaceCharFormat});
}

void TriggerHighlighter::setHighlightingEnabled(bool enabled) {
    highlightingEnabled = enabled;
    rehighlight();
}

void TriggerHighlighter::highlightBlock(const QString &text)
{
    if (!highlightingEnabled) {
        return;
    }

    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
