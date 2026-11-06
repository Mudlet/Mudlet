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

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TConsoleModel.h"
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

    // The link of the last run of the line lineShown sent with this text, 0 if none
    static int lastLinkOf(const QSignalSpy& spy, const QString& text)
    {
        const QList<THeadlessSession::TextRun> runs = runsOf(spy, text);
        return runs.isEmpty() ? 0 : runs.constLast().link;
    }

    static THeadlessSession::LinkItem item(const QString& label, const bool enabled = true, const bool separator = false) { return {label, enabled, separator}; }

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

    void test_aLinksTooltipAndItemIsItsHint()
    {
        const QString name = qsl("Headless Link Details");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless single$", [[echoLink(" x", "headlessFollowed = true", "the hint")]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless single"))));
        const int id = lastLinkOf(shown, qsl("headless single x"));
        QVERIFY(id > 0);
        const std::optional<THeadlessSession::Link> link = session.link(id);
        QVERIFY(link);
        QCOMPARE(link->tooltip, qsl("the hint"));
        QVERIFY(!link->menu);
        QCOMPARE(link->items, (QList<THeadlessSession::LinkItem>{item(qsl("the hint"))}));
        QVERIFY(!link->spoiler);
        QVERIFY(!link->disabled);
        QVERIFY(!link->visited);
        QVERIFY(!link->selected);
        QVERIFY(!link->foreground);
        QVERIFY(!link->styles);
        QVERIFY(session.shutdown(false));
    }

    void test_aPopupsItemsAreAsItsMenuListsThem()
    {
        const QString name = qsl("Headless Popup Details");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless popup$", [[echoPopup(" p", {"headlessFollowed = 1", "", ""}, {"the tooltip", "First", "", "Third"})]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless popup"))));
        const std::optional<THeadlessSession::Link> link = session.link(lastLinkOf(shown, qsl("headless popup p")));
        QVERIFY(link);
        QCOMPARE(link->tooltip, qsl("the tooltip"));
        QVERIFY(link->menu);
        QVERIFY(link->menuTitle.isEmpty());
        QCOMPARE(link->items, (QList<THeadlessSession::LinkItem>{item(qsl("First")), item(QString(), false, true), item(qsl("Third"), false)}));
        QVERIFY(session.shutdown(false));
    }

    void test_anOsc8LinksMenuHasItsTitleAndLook()
    {
        const QString name = qsl("Headless OSC 8 Details");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        const QString uri = qsl("send:look?config={\"title\":\"Lamb Stew\",\"style\":{\"color\":\"#00ffff\"},\"menu\":[{\"Look\":\"send:look stew\"},{\"Eat\":\"send:eat stew\"}]}");
        QVERIFY(session.runLua(feed(osc8(uri, qsl("stew")))));
        const std::optional<THeadlessSession::Link> link = session.link(lastLinkOf(output, qsl("stew")));
        QVERIFY(link);
        QVERIFY(link->menu);
        QCOMPARE(link->menuTitle, qsl("Lamb Stew"));
        QCOMPARE(link->tooltip, qsl("Right-click for menu"));
        QCOMPARE(link->items, (QList<THeadlessSession::LinkItem>{item(qsl("Look")), item(qsl("Eat"))}));
        QVERIFY(link->foreground);
        QCOMPARE(*link->foreground, QColor(0x00, 0xff, 0xff));
        QVERIFY(link->styles);
        QVERIFY(session.shutdown(false));
    }

    void test_noLinkIsGivenForAnIdTheConsoleDoesNotHold()
    {
        const QString name = qsl("Headless Unknown Link");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.link(1));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(osc8(qsl("send:look"), qsl("look")))));
        const int id = lastLinkOf(output, qsl("look"));
        QVERIFY(session.link(id));
        QVERIFY(!session.link(0));
        QVERIFY(!session.link(-1));
        QVERIFY(!session.link(id + 1));
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.link(id));
    }

    void test_aConcealedLinkIsSaidToBeAndOneWhoseLineWentIsNotGiven()
    {
        const QString name = qsl("Headless Concealed Link");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(qsl("hidden: ") + osc8(qsl("send:take?config={\"visibility\":{\"action\":\"conceal\"}}"), qsl("the key")))
                               + feed(qsl("gone: ") + osc8(qsl("send:drop?config={\"visibility\":{\"action\":\"conceal\",\"wholeline\":true}}"), qsl("the lamp")))));
        const int hidden = lastLinkOf(output, qsl("hidden: the key"));
        const int gone = lastLinkOf(output, qsl("gone: the lamp"));
        QVERIFY(hidden > 0);
        QVERIFY(gone > 0);
        TConsoleModel& model = session.host()->mainConsoleModel();
        QVERIFY(!session.link(hidden)->concealed);
        model.mHyperlinkVisibilityManager.concealLink(hidden);
        QVERIFY(session.link(hidden)->concealed);
        model.mHyperlinkVisibilityManager.revealLink(hidden);
        QVERIFY(!session.link(hidden)->concealed);

        QVERIFY(session.link(gone));
        model.mHyperlinkVisibilityManager.concealLink(gone);
        // Its line went with it, though the store keeps the link until a sweep
        QVERIFY(model.buffer.mLinkStore.contains(gone));
        QVERIFY(!session.link(gone));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionLinksTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionLinksTest)
