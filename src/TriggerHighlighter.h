#ifndef TRIGGERHIGHLIGHTER_H
#define TRIGGERHIGHLIGHTER_H

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

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include "edbee/views/texttheme.h"

class TriggerHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit TriggerHighlighter(QTextDocument *parent = nullptr);
    void setHighlightingEnabled(bool enabled);
    void setTheme(const QString&);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat anchorFormat;
    QTextCharFormat charClassFormat;
    QTextCharFormat escapeCharFormat;
    QTextCharFormat groupFormat;
    QTextCharFormat quantifierFormat;

    bool highlightingEnabled;
    void applyFormatting(QTextCharFormat& format, edbee::TextThemeRule* rule);
};

#endif  // TRIGGERHIGHLIGHTER_H
