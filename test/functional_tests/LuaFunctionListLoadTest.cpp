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
 * The script editors' API autocompletion comes from the lua-function-list.json
 * resource; a damaged copy must say why it was rejected, once, rather than
 * leave autocompletion silently empty.
 *
 * Run with: ctest -R LuaFunctionListLoadTest -V
 */

#include "mudlet.h"

#include "GroupedTest.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

class LuaFunctionListLoadTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mDir;
    inline static const QString csmConsequence = qsl(", so the script editors will offer no API autocompletion");

private slots:
    void init() { QTest::failOnWarning(QRegularExpression(qsl(".*"))); }

    void test_bundledListLoads()
    {
        mudlet::smLuaFunctionNames.clear();
        QVERIFY(mudlet::loadLuaFunctionList());
        QVERIFY(mudlet::smLuaFunctionNames.contains(qsl("echo")));
        QVERIFY(!mudlet::smLuaFunctionNames.value(qsl("echo")).toString().isEmpty());
    }

    void test_damagedListWarnsOnceWithTheReason_data()
    {
        QTest::addColumn<QByteArray>("contents");
        QTest::addColumn<QString>("reason");

        QTest::newRow("truncated") << QByteArray(R"json({"echo": "echo(text)")json") << qsl("could not be parsed as a JSON object: unterminated object at offset 21");
        QTest::newRow("empty file") << QByteArray() << qsl("is empty");
        QTest::newRow("top-level number") << QByteArray("42") << qsl("could not be parsed as a JSON object: illegal value at offset 0");
        QTest::newRow("array") << QByteArray(R"(["echo"])") << qsl("holds a JSON array, not an object");
        QTest::newRow("empty object") << QByteArray("{}") << qsl("lists no functions");
        QTest::newRow("non-string description") << QByteArray(R"({"echo": 5})") << qsl("gives \"echo\" a description that is not a string");
    }

    void test_damagedListWarnsOnceWithTheReason()
    {
        QFETCH(QByteArray, contents);
        QFETCH(QString, reason);
        QVERIFY(mDir.isValid());

        const QString path = mDir.filePath(qsl("%1.json").arg(QString::fromLatin1(QTest::currentDataTag()).replace(u' ', u'-')));
        QFile file(path);
        QVERIFY(file.open(QFile::WriteOnly));
        QCOMPARE(file.write(contents), contents.size());
        file.close();

        QTest::ignoreMessage(QtWarningMsg, qPrintable(qsl("mudlet::loadLuaFunctionList() WARNING - \"%1\" %2%3").arg(path, reason, csmConsequence)));
        QVERIFY(!mudlet::loadLuaFunctionList(path));
    }

    void test_directoryWarns()
    {
        QVERIFY(mDir.isValid());
        QTest::ignoreMessage(QtWarningMsg, qPrintable(qsl("mudlet::loadLuaFunctionList() WARNING - \"%1\" is a directory, not a file%2").arg(mDir.path(), csmConsequence)));
        QVERIFY(!mudlet::loadLuaFunctionList(mDir.path()));
    }

    void test_missingListWarns()
    {
        const QString path = mDir.filePath(qsl("absent.json"));
        QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(
                        qsl("^mudlet::loadLuaFunctionList\\(\\) WARNING - \"%1\" could not be opened \\(.+\\)%2$").arg(QRegularExpression::escape(path), QRegularExpression::escape(csmConsequence))));
        QVERIFY(!mudlet::loadLuaFunctionList(path));
    }
};

#include "LuaFunctionListLoadTest.moc"
MUDLET_GROUPED_TEST_MAIN(LuaFunctionListLoadTest)
