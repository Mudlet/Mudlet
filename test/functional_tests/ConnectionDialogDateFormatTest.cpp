/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * The connection dialog dates a profile's saves without the time zone, since formatting the
 * zone looks it up in the platform's time zone database for every entry. See issue #12984.
 *
 * Run with: ctest -R ConnectionDialogDateFormatTest -V
 */

#include "dlgConnectionProfiles.h"

#include <QtTest/QtTest>

#include <QLocale>

#include "GroupedTest.h"

class ConnectionDialogDateFormatTest : public QObject
{
    Q_OBJECT

private:
    static bool hasUnquotedZone(const QString& format)
    {
        bool quoted = false;
        for (const QChar c : format) {
            if (c == u'\'') {
                quoted = !quoted;
            } else if (!quoted && c == u't') {
                return true;
            }
        }
        return false;
    }

private slots:
    void test_zoneIsRemoved_data()
    {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("expected");

        QTest::newRow("long name at the end") << qsl("dddd, MMMM d, yyyy h:mm:ss Ap tttt") << qsl("dddd, MMMM d, yyyy h:mm:ss Ap");
        QTest::newRow("abbreviation at the end") << qsl("dd/MM/yyyy HH:mm:ss t") << qsl("dd/MM/yyyy HH:mm:ss");
        QTest::newRow("between date and time") << qsl("yyyy年M月d日dddd tttt HH:mm:ss") << qsl("yyyy年M月d日dddd HH:mm:ss");
        QTest::newRow("at the start") << qsl("tttt HH:mm:ss") << qsl("HH:mm:ss");
        QTest::newRow("in parentheses") << qsl("HH:mm:ss (tttt)") << qsl("HH:mm:ss");
        QTest::newRow("in brackets") << qsl("HH:mm:ss [tttt]") << qsl("HH:mm:ss");
        QTest::newRow("after a comma") << qsl("h:mm:ss Ap, tttt") << qsl("h:mm:ss Ap");
        QTest::newRow("t inside a quoted literal stays") << qsl("dddd, d 'ta' MMMM yyyy HH:mm:ss tttt") << qsl("dddd, d 'ta' MMMM yyyy HH:mm:ss");
        QTest::newRow("escaped quote inside a literal") << qsl("d 'o''t' MMMM tttt") << qsl("d 'o''t' MMMM");
        QTest::newRow("no zone") << qsl("dd.MM.yyyy HH:mm") << qsl("dd.MM.yyyy HH:mm");
    }

    void test_zoneIsRemoved()
    {
        QFETCH(QString, format);
        QFETCH(QString, expected);

        QCOMPARE(dlgConnectionProfiles::dateTimeFormatWithoutZone(format), expected);
    }

    void test_noLocaleKeepsAZone()
    {
        const QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);
        QVERIFY(!locales.isEmpty());
        for (const QLocale& locale : locales) {
            const QString format = locale.dateTimeFormat();
            const QString stripped = dlgConnectionProfiles::dateTimeFormatWithoutZone(format);
            const QByteArray context = qsl("%1: \"%2\" -> \"%3\"").arg(locale.name(), format, stripped).toUtf8();
            QVERIFY2(!hasUnquotedZone(stripped), context.constData());
            QVERIFY2(stripped == stripped.trimmed(), context.constData());
            QVERIFY2(!stripped.contains(qsl("  ")), context.constData());
        }
    }
};

#include "ConnectionDialogDateFormatTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogDateFormatTest)
