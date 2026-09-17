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

#include <algorithm>

#include <QRandomGenerator>
#include <QTextBoundaryFinder>
#include <QtTest/QtTest>

#include "TTextProperties.h"

#include "GroupedTest.h"

// TBuffer::getWrapInfo() wraps a printable-ASCII line at the break
// opportunities lineBreakInfo::asciiLineBreaks() reports instead of building a
// QTextBoundaryFinder, so the two must agree on every such line: every single
// character, pair and triple of the range, then random lines and lines built
// from the numbers, quotes, hyphens and brackets that UAX #14's context rules
// are about.
class AsciiLineBreakTest : public QObject
{
    Q_OBJECT

private:
    static QByteArray finderBreaks(const QString& line)
    {
        QByteArray breaks(line.size() + 1, '0');
        QTextBoundaryFinder finder(QTextBoundaryFinder::Line, line);
        for (qsizetype position = finder.toNextBoundary(); position != -1; position = finder.toNextBoundary()) {
            breaks[position] = '1';
        }
        return breaks;
    }

    static QByteArray asciiBreaks(const QString& line)
    {
        const lineBreakInfo::Breaks breaks = lineBreakInfo::asciiLineBreaks(line);
        QByteArray result(breaks.size(), '0');
        for (qsizetype position = 0; position < breaks.size(); ++position) {
            if (breaks[position]) {
                result[position] = '1';
            }
        }
        return result;
    }

    // Describes how the two disagree on the line, or returns an empty string
    // when they agree
    static QString disagreement(const QString& line)
    {
        const QByteArray expected = finderBreaks(line);
        const QByteArray actual = asciiBreaks(line);
        if (expected == actual) {
            return {};
        }
        return qsl("line \"%1\": QTextBoundaryFinder %2, asciiLineBreaks %3").arg(line, QString::fromLatin1(expected), QString::fromLatin1(actual));
    }

    static QString randomLine(QRandomGenerator& rng, const QByteArray& pool, const int length)
    {
        QString line(length, QChar::Space);
        for (int i = 0; i < length; ++i) {
            line[i] = QChar::fromLatin1(pool.at(rng.bounded(int(pool.size()))));
        }
        return line;
    }

private slots:
    void everyCharacterPairAndTriple()
    {
        QString failure;
        for (char16_t a = u' '; a <= u'~' && failure.isEmpty(); ++a) {
            QString line(3, QChar::Space);
            line[0] = QChar(a);
            failure = disagreement(line.left(1));
            for (char16_t b = u' '; b <= u'~' && failure.isEmpty(); ++b) {
                line[1] = QChar(b);
                failure = disagreement(line.left(2));
                for (char16_t c = u' '; c <= u'~' && failure.isEmpty(); ++c) {
                    line[2] = QChar(c);
                    failure = disagreement(line);
                }
            }
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
    }

    void randomLines()
    {
        // Weighted towards what game text holds, with every printable
        // character in the pool. The seed is fixed so a failure repeats.
        QByteArray pool;
        for (int repeat = 0; repeat < 30; ++repeat) {
            pool.append(' ');
        }
        for (int repeat = 0; repeat < 3; ++repeat) {
            pool.append("abcdefghijklmnopqrstuvwxyz");
        }
        pool.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        pool.append("0123456789");
        pool.append("0123456789");
        pool.append("------");
        pool.append("''''");
        pool.append("\"\"\"\"");
        pool.append(".,:;.,:;.,:;");
        pool.append("()[]{}()[]{}");
        pool.append("///");
        pool.append("!?!?");
        pool.append("$%+\\|#&*<=>@^_`~");
        QRandomGenerator rng(20260917);
        QString failure;
        for (int i = 0; i < 100000 && failure.isEmpty(); ++i) {
            failure = disagreement(randomLine(rng, pool, 1 + rng.bounded(8)));
            if (failure.isEmpty()) {
                failure = disagreement(randomLine(rng, pool, 1 + rng.bounded(80)));
            }
            if (failure.isEmpty()) {
                failure = disagreement(randomLine(rng, pool, 1 + rng.bounded(400)));
            }
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
    }

    void structuredLines()
    {
        // The sequences UAX #14's number, quotation, hyphen and bracket rules
        // are about, joined by the separators that decide which rule applies
        const QStringList tokens = {qsl("$1,234.56"), qsl("3.14"),  qsl("-5"),    qsl("12/31/2025"), qsl("(555) 123-4567"),
                                    qsl("10%"),       qsl("+1"),    qsl("1-2"),   qsl("a-b"),        qsl("--"),
                                    qsl("'quoted'"),  qsl("\"q\""), qsl("e.g."),  qsl("..."),        qsl("!!"),
                                    qsl("?!"),        qsl("x/y"),   qsl("[tag]"), qsl("{a}"),        qsl("a|b"),
                                    qsl("1|2"),       qsl(".5"),    qsl("$(1)"),  qsl("$[1]"),       qsl("%1"),
                                    qsl("1."),        qsl("1.$"),   qsl("$.1"),   qsl(",1"),         qsl("1,"),
                                    qsl("1,2,3"),     qsl("(1)2"),  qsl("1)"),    qsl("-1-"),        qsl("1-"),
                                    qsl("a-"),        qsl("-a"),    qsl("$"),     qsl("%"),          qsl("'"),
                                    qsl("\""),        qsl("("),     qsl(")"),     qsl("]"),          qsl("}"),
                                    qsl("/"),         qsl("|"),     qsl("."),     qsl(","),          qsl(":"),
                                    qsl("-"),         qsl("1"),     qsl("a"),     qsl("$1"),         qsl("1$"),
                                    qsl("$(1"),       qsl("$(.1"),  qsl("1/2"),   qsl("1:2"),        qsl("1;2"),
                                    qsl("(1"),        qsl("1)."),   qsl("'1'"),   qsl("\"1\""),      qsl("a'b"),
                                    qsl("a\"b"),      qsl("a''b"),  qsl("''"),    qsl("'a"),         qsl("a'"),
                                    qsl("(a)"),       qsl("[1]"),   qsl("{1}")};
        const QStringList separators = {qsl(""), qsl(" "), qsl("  "), qsl("-"), qsl(", "), qsl(" - "), qsl("/"), qsl("."), qsl(" ."), qsl("'"), qsl(" '")};
        QRandomGenerator rng(17092026);
        QString failure;
        for (int i = 0; i < 100000 && failure.isEmpty(); ++i) {
            QString line;
            const int count = 1 + rng.bounded(12);
            for (int t = 0; t < count; ++t) {
                if (t > 0) {
                    line.append(separators.at(rng.bounded(int(separators.size()))));
                }
                line.append(tokens.at(rng.bounded(int(tokens.size()))));
            }
            failure = disagreement(line);
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
    }

    // lineBreakInfo::printableAscii() decides whether a line takes the
    // ASCII path at all, four QChars at a time, so every value must be judged
    // as the one-at-a-time check judges it wherever it sits within a word.
    void printableAsciiScan()
    {
        const auto reference = [](const QString& line) {
            return std::all_of(line.cbegin(), line.cend(), [](const QChar c) {
                return c.unicode() >= u' ' && c.unicode() <= u'~';
            });
        };
        const auto check = [&reference](const QString& line) {
            if (lineBreakInfo::printableAscii(line) == reference(line)) {
                return QString();
            }
            QString codes;
            for (const QChar c : line) {
                codes.append(qsl(" %1").arg(uint(c.unicode()), 4, 16, QLatin1Char('0')));
            }
            return qsl("printableAscii disagrees on [%1]").arg(codes.trimmed());
        };
        QString failure;
        // Every QChar value in each lane of a word and in the tail after it
        for (int length = 0; length <= 9 && failure.isEmpty(); ++length) {
            for (int position = 0; position < length && failure.isEmpty(); ++position) {
                QString line(length, QLatin1Char('a'));
                for (int value = 0; value <= 0xFFFF && failure.isEmpty(); ++value) {
                    line[position] = QChar(char16_t(value));
                    failure = check(line);
                }
            }
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        QVERIFY(lineBreakInfo::printableAscii(QString()));
        // A failing lane next to a boundary value in the neighbouring lane,
        // where a carry or borrow between lanes could hide or invent a failure
        const QList<char16_t> probes = {0x0000, 0x001F, 0x0020, 0x007E, 0x007F, 0x0080, 0x00FF, 0x0100, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF};
        for (const char16_t first : probes) {
            for (const char16_t second : probes) {
                for (int position = 0; position < 7 && failure.isEmpty(); ++position) {
                    QString line(8, QLatin1Char('x'));
                    line[position] = QChar(first);
                    line[position + 1] = QChar(second);
                    failure = check(line);
                }
            }
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        QRandomGenerator rng(20260917);
        for (int i = 0; i < 100000 && failure.isEmpty(); ++i) {
            QString line(rng.bounded(300), QLatin1Char(' '));
            for (QChar& c : line) {
                c = QChar(char16_t(u' ' + rng.bounded(95)));
            }
            if (!line.isEmpty() && rng.bounded(2)) {
                line[rng.bounded(int(line.size()))] = QChar(char16_t(rng.bounded(0x10000)));
            }
            failure = check(line);
        }
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
    }
};

#include "AsciiLineBreakTest.moc"
MUDLET_GROUPED_TEST_MAIN(AsciiLineBreakTest)
