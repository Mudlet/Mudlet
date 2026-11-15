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
 * TTriggerPrescan offers a line only the slots filed under n-grams it holds, so
 * a slot it misses is a trigger that silently stops firing. A spec reaches it
 * only through lines ending in the newline the trigger engine appends, with
 * patterns that cannot hold one, so it never sees an n-gram that ends the text.
 * This files every n-gram of each line, for every mix of lengths, and needs
 * every one found.
 *
 * Run with: ctest -R TriggerPrescanTest -V
 */

#include <QtTest/QtTest>

#include "TTriggerPrescan.h"
#include "utils.h"

#include "GroupedTest.h"

#include <algorithm>

class TriggerPrescanTest : public QObject
{
    Q_OBJECT

private:
    // Built rather than written out, so that no source file encoding can turn
    // the astral character into something that is not a surrogate pair.
    static QStringList interestingLines()
    {
        const QString dragon = QString(QChar(0xD83D)) + QChar(0xDC09); // U+1F409, a surrogate pair
        const QString tigerDragon = QString(QChar(0x9F8D)) + QChar(0x864E);
        const QString cyrillic = QString(QChar(0x0436)) + QChar(0x0449) + QChar(0x044E) + QChar(0x0444) + QChar(0x044B);
        return QStringList{qsl("qj"),
                           qsl("qjv"),
                           qsl("vqxzk"),
                           qsl("the quick brown fox"),
                           qsl("the ") + tigerDragon + qsl(" banner ") + tigerDragon,
                           qsl("ends in ") + cyrillic,
                           qsl("a dragon ") + dragon,
                           dragon + qsl("qz") + dragon};
    }

    struct Filed
    {
        QString mText;
        int mPosition = 0;
        int mSlot = -1;
    };

    // One slot per n-gram of the line at each length in lengthMask (bit n for length n)
    static std::vector<Filed> fileEveryNgram(TTriggerPrescan& prescan, const QString& line, const int lengthMask)
    {
        std::vector<Filed> filed;
        for (int length = TTriggerPrescan::scmMinGramLength; length <= TTriggerPrescan::scmMaxGramLength; ++length) {
            if (!(lengthMask & (1 << length))) {
                continue;
            }
            for (int i = 0; i + length <= line.size(); ++i) {
                const QString text = line.mid(i, length);
                filed.push_back(Filed{text, i, prescan.fileSlot({TTriggerPrescan::patternGram(text)})});
            }
        }
        return filed;
    }

    static QString missed(const TTriggerPrescan& prescan, const QString& line, const std::vector<Filed>& filed)
    {
        std::vector<int> found;
        prescan.hits(line, found);
        QStringList missing;
        for (const Filed& entry : filed) {
            if (std::find(found.cbegin(), found.cend(), entry.mSlot) == found.cend()) {
                missing << qsl("\"%1\" at %2").arg(entry.mText).arg(entry.mPosition);
            }
        }
        return missing.isEmpty() ? QString() : qsl("of \"%1\": %2").arg(line, missing.join(qsl(", ")));
    }

private slots:
    void findsEveryNgramOfTheLine()
    {
        const int allLengths = ((1 << (TTriggerPrescan::scmMaxGramLength + 1)) - 1) & ~((1 << TTriggerPrescan::scmMinGramLength) - 1);
        for (const QString& line : interestingLines()) {
            // Every mix, so that a length left unused between two in use is covered too
            for (int lengthMask = 1 << TTriggerPrescan::scmMinGramLength; lengthMask <= allLengths; ++lengthMask) {
                if ((lengthMask & allLengths) != lengthMask) {
                    continue;
                }
                TTriggerPrescan prescan;
                const std::vector<Filed> filed = fileEveryNgram(prescan, line, lengthMask);
                const QString missing = missed(prescan, line, filed);
                QVERIFY2(missing.isEmpty(), qPrintable(qsl("lengths 0x%1 missed %2").arg(lengthMask, 0, 16).arg(missing)));
            }
        }
    }

    void findsWhatIsLeftFiledAfterReindexing()
    {
        for (const QString& line : interestingLines()) {
            TTriggerPrescan prescan;
            std::vector<Filed> filed = fileEveryNgram(prescan, line, ~0);
            if (filed.size() < 2) {
                continue;
            }
            const quint64 reindexesBefore = prescan.reindexCount();
            // Every fourth kept, as filed is in length order and the rest have to span every length
            std::vector<Filed> kept;
            std::vector<int> freed;
            for (size_t index = 0; index < filed.size(); ++index) {
                if (index % 4) {
                    freed.push_back(filed[index].mSlot);
                } else {
                    kept.push_back(filed[index]);
                }
            }
            prescan.removeSlots(freed);
            filed = kept;
            QVERIFY2(prescan.reindexCount() > reindexesBefore, "freeing three quarters of the slots should have refiled the rest");
            const QString missing = missed(prescan, line, filed);
            QVERIFY2(missing.isEmpty(), qPrintable(qsl("missed %1").arg(missing)));
        }
    }
};

#include "TriggerPrescanTest.moc"
MUDLET_GROUPED_TEST_GUILESS_MAIN(TriggerPrescanTest)
