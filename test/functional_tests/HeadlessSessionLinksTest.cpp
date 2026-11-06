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
 * What THeadlessSession tells an app about the main console's links. Each test
 * makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionLinksTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionLinksTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name, const QString& script = QString())
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSessionLinks</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped())
                               .toUtf8());
        return true;
    }

    // An OSC 8 link around text. Not a raw string: moc drops the class over one holding these escapes.
    static QString osc8(const QString& uri, const QString& text)
    {
        const QString esc = QChar(0x1b);
        return qsl("%1]8;;%2%1\\%3%1]8;;%1\\").arg(esc, uri, text);
    }

    static QString feed(const QString& line) { return qsl("feedTriggers([==[%1\n]==])").arg(line); }

    static QList<THeadlessSession::TextRun> runsOf(const QSignalSpy& spy, const QString& text)
    {
        for (const auto& arguments : spy) {
            if (arguments.at(1).toString() == text) {
                return arguments.at(3).value<QList<THeadlessSession::TextRun>>();
            }
        }
        return {};
    }

    // The link of each character of a line, from its runs
    static QList<int> linkOfEachCharacter(const QList<THeadlessSession::TextRun>& runs)
    {
        QList<int> links;
        for (const THeadlessSession::TextRun& run : runs) {
            for (int i = 0; i < run.length; ++i) {
                links << run.link;
            }
        }
        return links;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void test_onlyALinksRunsCarryIt()
    {
        const QString name = qsl("Headless Link Runs");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("a %1 c").arg(osc8(qsl("send:look"), qsl("b"))))));
        for (const QSignalSpy* spy : {&output, &shown}) {
            const QList<int> links = linkOfEachCharacter(runsOf(*spy, qsl("a b c")));
            QCOMPARE(links.size(), 5);
            QVERIFY(links.at(2) > 0);
            QCOMPARE(links, (QList<int>{0, 0, links.at(2), 0, 0}));
        }
        QVERIFY(session.shutdown(false));
    }

    void test_touchingLinksThatLookAlikeAreRunsOfTheirOwn()
    {
        const QString name = qsl("Headless Touching Links");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(osc8(qsl("send:north"), qsl("north")) + osc8(qsl("send:south"), qsl("south")))));
        const QList<THeadlessSession::TextRun> runs = runsOf(output, qsl("northsouth"));
        QCOMPARE(runs.size(), 2);
        QCOMPARE(runs.at(0).length, 5);
        QVERIFY(runs.at(0).link > 0);
        QVERIFY(runs.at(1).link > 0);
        QVERIFY(runs.at(0).link != runs.at(1).link);
        QCOMPARE(runs.at(0).foreground, runs.at(1).foreground);
        QCOMPARE(runs.at(0).styles, runs.at(1).styles);
        QVERIFY(session.shutdown(false));
    }

    void test_aLinkATriggerWritesIsShown()
    {
        const QString name = qsl("Headless Trigger Link");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless trigger line$", [[echoLink(" more", "headlessFollowed = true", "hint")]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless trigger line"))));
        const QList<int> links = linkOfEachCharacter(runsOf(shown, qsl("headless trigger line more")));
        QCOMPARE(links.size(), 26);
        QCOMPARE(links.first(), 0);
        QVERIFY(links.last() > 0);
        QCOMPARE(links.mid(21), QList<int>(5, links.last()));
        QVERIFY(session.shutdown(false));
    }

    void test_aPlainLineHasNoLink()
    {
        const QString name = qsl("Headless Plain Line");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(qsl("just a plain line"))));
        const QList<THeadlessSession::TextRun> runs = runsOf(output, qsl("just a plain line"));
        QVERIFY(!runs.isEmpty());
        for (const THeadlessSession::TextRun& run : runs) {
            QCOMPARE(run.link, 0);
        }
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionLinksTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionLinksTest)
