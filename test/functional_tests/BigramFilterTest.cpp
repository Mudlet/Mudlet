/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * TBigramFilter lets a substring trigger dismiss a line without searching it.
 * A wrong "yes" only costs the search that would have happened anyway, but a
 * wrong "no" silently stops a trigger firing, so the property worth pinning is
 * that there is never one: whatever the line is made of, every substring of it
 * has to survive the filter.
 *
 * A Lua spec cannot see the filter's verdict, only the match it leads to, so it
 * can only cover the character shapes someone thought to write a trigger for.
 * This covers every substring of each shape instead.
 *
 * Run with: ctest -R BigramFilterTest -V
 */

#include <QtTest/QtTest>

#include "TTrigger.h"

#include "GroupedTest.h"

class BigramFilterTest : public QObject
{
    Q_OBJECT

private:
    // Built rather than written out, so that no source file encoding can turn
    // the astral character into something that is not a surrogate pair.
    static QStringList interestingLines()
    {
        const QString dragon = QString(QChar(0xD83D)) + QChar(0xDC09); // U+1F409, a surrogate pair
        const QString acute = QChar(0x0301);                           // a combining mark
        return QStringList{QString(),
                           qsl("x"),
                           qsl("the quick brown fox jumps over the lazy dog"),
                           qsl("caf") + QChar(0x00E9) + qsl(" touch") + QChar(0x00E9),
                           qsl("a dragon ") + dragon + qsl(" appears"),
                           dragon + dragon,
                           qsl("aaaaaaaa"),
                           QString(QChar(0)) + qsl("a null leads this line"),
                           qsl("e") + acute + qsl(" is two characters")};
    }

private slots:
    void noSubstringOfALineIsEverDismissed()
    {
        for (const QString& line : interestingLines()) {
            const TBigramFilter filter(line, TBigramFilter::scmQuestionsWorthSummarising);
            for (qsizetype start = 0; start <= line.size(); ++start) {
                for (qsizetype length = 0; start + length <= line.size(); ++length) {
                    const QString substring = line.mid(start, length);
                    QVERIFY2(filter.couldContain(line, TBigramFilter::bitsFor(substring)),
                             qPrintable(qsl("'%1' occurs in '%2' at %3 but the filter dismissed it").arg(substring, line, QString::number(start))));
                }
            }
        }
    }

    void dismissesTextTheLineCannotContain()
    {
        const QString line = qsl("the quick brown fox");
        const TBigramFilter filter(line, TBigramFilter::scmQuestionsWorthSummarising);
        // every character of "xorb" is in the line, so only the pairs can tell
        QVERIFY2(!filter.couldContain(line, TBigramFilter::bitsFor(qsl("xorb"))), "a filter that dismisses nothing would save no searches at all");
    }

    void summarisesOnlyOnceEnoughPatternsAsk()
    {
        const QString line = qsl("the quick brown fox");
        const TBigramFilter::Bits absent = TBigramFilter::bitsFor(qsl("xorb"));

        const TBigramFilter tooFewAsked(line, TBigramFilter::scmQuestionsWorthSummarising - 1);
        QVERIFY2(tooFewAsked.couldContain(line, absent), "below the threshold the line is not summarised, so nothing can be dismissed");

        const TBigramFilter enoughAsked(line, TBigramFilter::scmQuestionsWorthSummarising);
        QVERIFY(!enoughAsked.couldContain(line, absent));
    }

    void countsQuestionsEvenWhenNotSummarising()
    {
        // the count is what turns summarising on for the next line, so a line
        // that is not summarised itself still has to do the counting
        const QString line = qsl("the quick brown fox");
        const TBigramFilter filter(line, 0);
        QCOMPARE(filter.questionsAsked(), 0);
        filter.couldContain(line, TBigramFilter::bitsFor(qsl("xorb")));
        filter.couldContain(line, TBigramFilter::bitsFor(qsl("quick")));
        QCOMPARE(filter.questionsAsked(), 2);
    }
};

#include "BigramFilterTest.moc"
MUDLET_GROUPED_TEST_MAIN(BigramFilterTest)
