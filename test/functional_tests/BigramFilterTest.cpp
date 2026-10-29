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
 * TNeedle, the search that follows the filter, has to answer as QStringMatcher
 * does. No spec can hand it an embedded NUL or empty text, so this covers those.
 *
 * Run with: ctest -R BigramFilterTest -V
 */

#include <QRandomGenerator>
#include <QStringMatcher>
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

    static QString units(const QString& text)
    {
        QStringList hex;
        for (const QChar unit : text) {
            hex << QString::number(unit.unicode(), 16);
        }
        return hex.join(QLatin1Char(' '));
    }

    static bool searchesAgree(const QString& needle, const QString& haystack)
    {
        const qsizetype expected = QStringMatcher(needle, Qt::CaseSensitive).indexIn(haystack);
        const qsizetype found = TNeedle(needle).indexIn(haystack);
        if (found == expected) {
            return true;
        }
        qWarning().noquote() << qsl("needle [%1] in [%2]: TNeedle %3, QStringMatcher %4").arg(units(needle), units(haystack), QString::number(found), QString::number(expected));
        return false;
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

    void needleFindsWhatQStringMatcherFinds()
    {
        const QString dragon = QString(QChar(0xD83D)) + QChar(0xDC09); // U+1F409
        const QString wyrm = QString(QChar(0xD83D)) + QChar(0xDC32);   // U+1F432, the same high surrogate
        const QString nul(QChar(0));
        const QList<std::pair<QString, QString>> cases{
                {qsl("abcd"), qsl("abc bcd")},
                {qsl("dog."), qsl("lazy dog.")},
                {qsl("wholeline"), qsl("wholeline")},
                {qsl("aab"), qsl("aaab")},
                {qsl("----X"), qsl("-----X -----X")},
                {qsl("--- Exits ---"), QString(80, QLatin1Char('-'))},
                {qsl("ab"), QString()},
                {QString(), QString()},
                {QString(), qsl("ab")},
                {qsl("x"), qsl("abx")},
                {qsl("cd"), qsl("ab") + nul + qsl("cd")},
                {qsl("a") + nul + qsl("b"), qsl("ab a") + nul + qsl("b")},
                {dragon + wyrm, wyrm + qsl(" ") + dragon + dragon},
                {QString(QChar(0xDC09)) + qsl("x"), dragon + qsl("x")},
        };
        for (const auto& [needle, haystack] : cases) {
            QVERIFY(searchesAgree(needle, haystack));
        }

        const QString alphabet = qsl("abe -.Z9") + QChar(0x00E9) + QChar(0x0436) + nul + dragon + wyrm;
        QRandomGenerator random(12159);
        const auto randomText = [&](const int length) {
            QString text;
            for (int i = 0; i < length; ++i) {
                text += alphabet.at(random.bounded(alphabet.size()));
            }
            return text;
        };
        for (int round = 0; round < 20000; ++round) {
            const QString haystack = randomText(random.bounded(24));
            QString needle;
            if (round % 2) {
                needle = randomText(random.bounded(6));
            } else {
                const qsizetype start = random.bounded(haystack.size() + 1);
                needle = haystack.mid(start, random.bounded(haystack.size() - start + 1));
                if (!needle.isEmpty() && round % 4 == 0) {
                    needle[random.bounded(needle.size())] = alphabet.at(random.bounded(alphabet.size()));
                }
            }
            QVERIFY(searchesAgree(needle, haystack));
        }
    }
};

#include "BigramFilterTest.moc"
MUDLET_GROUPED_TEST_MAIN(BigramFilterTest)
