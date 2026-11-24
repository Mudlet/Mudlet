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

#include "ReplacedBinaryWatcher.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cstdio>

// Stands in for a package manager replacing bin/mudlet with scripts that answer --version.
// Each one leaves a file named "asked" beside bin, outside the watched directory, so a test can
// tell the watcher ran it without that counting as a change.
class ReplacedBinaryWatcherTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mDir;

    QString path(const QString& name) const { return mDir.filePath(name); }
    QString bin(const QString& name) const { return mDir.filePath(QStringLiteral("bin/") + name); }

    void writeScript(const QString& name, const QByteArray& body)
    {
        QFile script(bin(name));
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write("#!/bin/sh\ntouch \"$(dirname \"$0\")/../asked\"\n" + body);
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }

    // The way dpkg and rpm install: a new file, renamed over the old one
    void install(const QByteArray& body)
    {
        writeScript(QStringLiteral("mudlet.new"), body);
        QCOMPARE(std::rename(QFile::encodeName(bin(QStringLiteral("mudlet.new"))).constData(), QFile::encodeName(bin(QStringLiteral("mudlet"))).constData()), 0);
    }

    static QByteArray version(const QByteArray& line) { return "echo '" + line + "'\n"; }

private slots:
    void init()
    {
        QVERIFY(mDir.isValid());
        QVERIFY(QDir(mDir.path()).mkpath(QStringLiteral("bin")));
        QFile::remove(path(QStringLiteral("asked")));
        QFile::remove(path(QStringLiteral("ready")));
        QFile::remove(bin(QStringLiteral("ready")));
        writeScript(QStringLiteral("mudlet"), version("mudlet 5.0.0 "));
    }

    void versionFromOutput_data()
    {
        QTest::addColumn<QString>("output");
        QTest::addColumn<QString>("expected");
        QTest::newRow("release") << QStringLiteral("mudlet 5.1.0  \nQt libraries 6.11.0 (compilation) 6.11.0 (runtime)\n") << QStringLiteral("5.1.0");
        QTest::newRow("public test build") << QStringLiteral("mudlet 5.1.0 -ptb-abc1234 \n") << QStringLiteral("5.1.0-ptb-abc1234");
        QTest::newRow("debug build") << QStringLiteral("mudlet 5.1.0-dev-abc1234 (with debug symbols, without optimisations)\n") << QStringLiteral("5.1.0-dev-abc1234");
        QTest::newRow("no output") << QString() << QString();
        QTest::newRow("no version") << QStringLiteral("mudlet\n5.1.0\n") << QString();
    }

    void versionFromOutput()
    {
        QFETCH(QString, output);
        QFETCH(QString, expected);
        QCOMPARE(ReplacedBinaryWatcher::versionFromOutput(output), expected);
    }

    void newerVersionInstalled()
    {
        ReplacedBinaryWatcher watcher(bin(QStringLiteral("mudlet")), bin(QStringLiteral("mudlet")), QStringLiteral("5.0.0"));
        QSignalSpy spy(&watcher, &ReplacedBinaryWatcher::signal_newVersionInstalled);
        install(version("mudlet 5.1.0 "));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 10000);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("5.1.0"));
    }

    void sameVersionReinstalled()
    {
        ReplacedBinaryWatcher watcher(bin(QStringLiteral("mudlet")), bin(QStringLiteral("mudlet")), QStringLiteral("5.0.0"));
        QSignalSpy spy(&watcher, &ReplacedBinaryWatcher::signal_newVersionInstalled);
        install(version("mudlet 5.0.0 "));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path(QStringLiteral("asked"))), 10000);
        QTest::qWait(500);
        QCOMPARE(spy.count(), 0);
    }

    void otherFileInDirectoryChanged()
    {
        ReplacedBinaryWatcher watcher(bin(QStringLiteral("mudlet")), bin(QStringLiteral("mudlet")), QStringLiteral("5.0.0"));
        QFile neighbour(bin(QStringLiteral("neighbour")));
        QVERIFY(neighbour.open(QIODevice::WriteOnly));
        neighbour.close();
        QTest::qWait(2500);
        QVERIFY(!QFile::exists(path(QStringLiteral("asked"))));
    }

    void askedAgainOnNextChange()
    {
        ReplacedBinaryWatcher watcher(bin(QStringLiteral("mudlet")), bin(QStringLiteral("mudlet")), QStringLiteral("5.0.0"));
        QSignalSpy spy(&watcher, &ReplacedBinaryWatcher::signal_newVersionInstalled);
        // Fails until the rest of the install lands, as a binary can while its libraries are still unpacking
        install("[ -e \"$(dirname \"$0\")/ready\" ] || exit 1\n" + version("mudlet 5.1.0 "));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path(QStringLiteral("asked"))), 10000);
        QTest::qWait(500);
        QCOMPARE(spy.count(), 0);

        QFile ready(bin(QStringLiteral("ready")));
        QVERIFY(ready.open(QIODevice::WriteOnly));
        ready.close();
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 10000);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("5.1.0"));
    }

    void askedAgainWithoutChange()
    {
        ReplacedBinaryWatcher watcher(bin(QStringLiteral("mudlet")), bin(QStringLiteral("mudlet")), QStringLiteral("5.0.0"));
        QSignalSpy spy(&watcher, &ReplacedBinaryWatcher::signal_newVersionInstalled);
        // What it waits for lands outside the watched directory, as libraries do
        install("[ -e \"$(dirname \"$0\")/../ready\" ] || exit 1\n" + version("mudlet 5.1.0 "));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path(QStringLiteral("asked"))), 10000);

        QFile ready(path(QStringLiteral("ready")));
        QVERIFY(ready.open(QIODevice::WriteOnly));
        ready.close();
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 20000);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("5.1.0"));
    }
};

QTEST_GUILESS_MAIN(ReplacedBinaryWatcherTest)
#include "ReplacedBinaryWatcherTest.moc"
