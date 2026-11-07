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

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
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

class HeadlessLinkUrlCatcher : public QObject
{
    Q_OBJECT

public:
    QList<QUrl> mUrls;

public slots:
    void slot_openUrl(const QUrl& url) { mUrls << url; }
};

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

    // The links of the line outputLine sent starting with prefix, in order
    static QList<int> linksOnLineStarting(const QSignalSpy& spy, const QString& prefix)
    {
        QList<int> links;
        for (const auto& arguments : spy) {
            if (arguments.at(1).toString().startsWith(prefix)) {
                for (const THeadlessSession::TextRun& run : arguments.at(3).value<QList<THeadlessSession::TextRun>>()) {
                    if (run.link && !links.contains(run.link)) {
                        links << run.link;
                    }
                }
                break;
            }
        }
        return links;
    }

    static QString luaGlobal(THeadlessSession& session, const char* name)
    {
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    // What the profile's sysDataSendRequest handler saw sent, each followed by ;
    static QString recordSends() { return qsl(R"lua(registerAnonymousEventHandler("sysDataSendRequest", function(_, sent) headlessSent = (headlessSent or "") .. sent .. ";" end))lua"); }

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

    void test_followingALinkRunsItsCommandOrFunction()
    {
        const QString name = qsl("Headless Follow Link");
        QVERIFY(writeProfile(name, qsl(R"lua(
tempRegexTrigger("^headless command$", [[echoLink(" c", "headlessCommand = 'ran'", "c")]])
tempRegexTrigger("^headless function$", [[echoLink(" f", function() headlessFunction = 'ran' end, "f")]])
tempRegexTrigger("^headless popup$", [[echoPopup(" p", {"headlessPopup = 1", "headlessPopup = 2", ""}, {"one", "two", "three"})]])
)lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless command")) + feed(qsl("headless function")) + feed(qsl("headless popup"))));
        QCOMPARE(session.followLink(lastLinkOf(shown, qsl("headless command c"))), THeadlessSession::LinkResult::ran);
        QCOMPARE(luaGlobal(session, "headlessCommand"), qsl("ran"));
        QCOMPARE(session.followLink(lastLinkOf(shown, qsl("headless function f"))), THeadlessSession::LinkResult::ran);
        QCOMPARE(luaGlobal(session, "headlessFunction"), qsl("ran"));
        const int popup = lastLinkOf(shown, qsl("headless popup p"));
        QVERIFY(!session.link(popup)->visited);
        QCOMPARE(session.followLink(popup, 1), THeadlessSession::LinkResult::ran);
        QCOMPARE(luaGlobal(session, "headlessPopup"), qsl("2"));
        QVERIFY(session.link(popup)->visited);
        QCOMPARE(session.followLink(popup, 2), THeadlessSession::LinkResult::disabled);
        QCOMPARE(session.followLink(popup, 3), THeadlessSession::LinkResult::noSuchItem);
        QCOMPARE(session.followLink(popup, -1), THeadlessSession::LinkResult::noSuchItem);
        QCOMPARE(session.followLink(popup + 1), THeadlessSession::LinkResult::unknown);
        QCOMPARE(luaGlobal(session, "headlessPopup"), qsl("2"));
        QVERIFY(session.shutdown(false));
    }

    void test_aSpoilerIsRevealedBeforeItRunsAndADisabledLinkRunsNothing()
    {
        const QString name = qsl("Headless Follow Spoiler");
        QVERIFY(writeProfile(name, recordSends()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(qsl("spoiler: ") + osc8(qsl("send:look?config={\"spoiler\":true}"), qsl("the butler")))
                               + feed(qsl("disabled: ") + osc8(qsl("send:eat?config={\"disabled\":true}"), qsl("the stew")))));
        const QList<int> spoiler = linksOnLineStarting(output, qsl("spoiler: "));
        QCOMPARE(spoiler.size(), 1);
        QVERIFY(!session.link(spoiler.first())->revealed);
        QCOMPARE(session.followLink(spoiler.first()), THeadlessSession::LinkResult::revealedSpoiler);
        QVERIFY(session.link(spoiler.first())->revealed);
        QVERIFY(luaGlobal(session, "headlessSent").isEmpty());
        QCOMPARE(session.followLink(spoiler.first()), THeadlessSession::LinkResult::ran);
        QCOMPARE(luaGlobal(session, "headlessSent"), qsl("look;"));

        const QList<int> disabled = linksOnLineStarting(output, qsl("disabled: "));
        QCOMPARE(disabled.size(), 1);
        QCOMPARE(session.followLink(disabled.first()), THeadlessSession::LinkResult::disabled);
        QCOMPARE(luaGlobal(session, "headlessSent"), qsl("look;"));
        QVERIFY(session.shutdown(false));
    }

    void test_followingALinkOfAnExclusiveGroupSelectsOnlyIt()
    {
        const QString name = qsl("Headless Follow Group");
        QVERIFY(writeProfile(name, recordSends()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        const QString config = qsl("?config={\"selection\":{\"group\":\"size\",\"value\":\"%1\",\"exclusive\":true}}");
        QVERIFY(session.runLua(feed(qsl("group: ") + osc8(qsl("send:small") + config.arg(qsl("small")), qsl("small")) + qsl(" ") + osc8(qsl("send:large") + config.arg(qsl("large")), qsl("large")))));
        const QList<int> links = linksOnLineStarting(output, qsl("group: "));
        QCOMPARE(links.size(), 2);
        QCOMPARE(session.followLink(links.at(0)), THeadlessSession::LinkResult::ran);
        QVERIFY(session.link(links.at(0))->selected);
        QVERIFY(!session.link(links.at(1))->selected);
        QCOMPARE(session.followLink(links.at(1)), THeadlessSession::LinkResult::ran);
        QVERIFY(!session.link(links.at(0))->selected);
        QVERIFY(session.link(links.at(1))->selected);
        // As the GUI does, the command sent says the choice
        QCOMPARE(luaGlobal(session, "headlessSent"), qsl("small?selected=true;large?selected=true;"));
        QVERIFY(session.shutdown(false));
    }

    void test_followingAWebLinkOpensIt()
    {
        const QString name = qsl("Headless Follow Web Link");
        QVERIFY(writeProfile(name));
        HeadlessLinkUrlCatcher catcher;
        QDesktopServices::setUrlHandler(qsl("https"), &catcher, "slot_openUrl");
        const auto unset = qScopeGuard([] {
            QDesktopServices::unsetUrlHandler(qsl("https"));
        });
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(qsl("web: ") + osc8(qsl("https://example.com/stew"), qsl("the recipe")))));
        const QList<int> links = linksOnLineStarting(output, qsl("web: "));
        QCOMPARE(links.size(), 1);
        QVERIFY(catcher.mUrls.isEmpty());
        QCOMPARE(session.followLink(links.first()), THeadlessSession::LinkResult::ran);
        QCOMPARE(catcher.mUrls, QList<QUrl>{QUrl(qsl("https://example.com/stew"))});
        QVERIFY(session.shutdown(false));
    }

    void test_followingALinkIsRefusedWhereSubmitIs()
    {
        const QString name = qsl("Headless Follow Refused");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless refused$", [[echoLink(" r", "headlessRefused = 'ran'", "r")]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.followLink(1), THeadlessSession::LinkResult::refused);
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless refused"))));
        const int id = lastLinkOf(shown, qsl("headless refused r"));
        QVERIFY(id > 0);

        std::optional<THeadlessSession::LinkResult> followedMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &followedMidWrite, id](const QString&, const QString& text) {
            if (text == qsl("headless again")) {
                followedMidWrite = session.followLink(id);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(feed(qsl("headless again"))));
        QCOMPARE(followedMidWrite, std::optional(THeadlessSession::LinkResult::refused));
        QVERIFY(luaGlobal(session, "headlessRefused").isEmpty());
        QCOMPARE(session.followLink(id), THeadlessSession::LinkResult::ran);
        QCOMPARE(luaGlobal(session, "headlessRefused"), qsl("ran"));
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.followLink(id), THeadlessSession::LinkResult::refused);
    }

    void test_followingWithNoItemIsALeftClick()
    {
        const QString name = qsl("Headless Follow Left Click");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless empty first$", [[echoPopup(" e", {"", "headlessSecond = 'ran'"}, {"nothing", "second"})]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless empty first"))));
        const int id = lastLinkOf(shown, qsl("headless empty first e"));
        QVERIFY(id > 0);
        QVERIFY(!session.link(id)->items.first().enabled);
        QCOMPARE(session.followLink(id, 0), THeadlessSession::LinkResult::disabled);
        QVERIFY(!session.link(id)->visited);
        // The GUI's left click runs the first command, empty or not
        QCOMPARE(session.followLink(id), THeadlessSession::LinkResult::ran);
        QVERIFY(session.link(id)->visited);
        QVERIFY(luaGlobal(session, "headlessSecond").isEmpty());
        QVERIFY(session.shutdown(false));
    }

    void test_aLinksLuaErrorIsGiven()
    {
        const QString name = qsl("Headless Follow Lua Error");
        QVERIFY(writeProfile(name, qsl(R"lua(
tempRegexTrigger("^headless broken$", [[echoLink(" b", "headlessBroken =", "b")]])
tempRegexTrigger("^headless raising$", [[echoLink(" r", function() error("headless boom") end, "r")]])
)lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless broken")) + feed(qsl("headless raising"))));
        const int broken = lastLinkOf(shown, qsl("headless broken b"));
        QCOMPARE(session.followLink(broken), THeadlessSession::LinkResult::failed);
        QVERIFY2(session.errorString().contains(qsl("near '<eof>'")), qPrintable(session.errorString()));
        QVERIFY(session.link(broken)->visited);
        QCOMPARE(session.followLink(lastLinkOf(shown, qsl("headless raising r"))), THeadlessSession::LinkResult::failed);
        QVERIFY2(session.errorString().contains(qsl("headless boom")), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
    }

    void test_aConcealedLinkIsNotFollowed()
    {
        const QString name = qsl("Headless Follow Concealed");
        QVERIFY(writeProfile(name, recordSends()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(feed(qsl("once: ") + osc8(qsl("send:accept?config={\"visibility\":{\"action\":\"conceal\"}}"), qsl("Accept quest")))
                               + feed(qsl("gone: ") + osc8(qsl("send:drop?config={\"visibility\":{\"action\":\"conceal\",\"wholeline\":true}}"), qsl("the lamp")))));
        const QList<int> once = linksOnLineStarting(output, qsl("once: "));
        const QList<int> gone = linksOnLineStarting(output, qsl("gone: "));
        QCOMPARE(once.size(), 1);
        QCOMPARE(gone.size(), 1);
        QCOMPARE(session.followLink(once.first()), THeadlessSession::LinkResult::ran);
        QVERIFY(session.link(once.first())->concealed);
        QCOMPARE(session.followLink(once.first()), THeadlessSession::LinkResult::concealed);
        QCOMPARE(session.followLink(gone.first()), THeadlessSession::LinkResult::ran);
        QCOMPARE(session.followLink(gone.first()), THeadlessSession::LinkResult::unknown);
        QCOMPARE(luaGlobal(session, "headlessSent"), qsl("accept;drop;"));
        QVERIFY(session.shutdown(false));
    }

    // The GUI's menu pick, where the link's id has been handed to another while its menu was open
    void test_aMenuPickRunsNothingOnceItsIdIsReused()
    {
        const QString name = qsl("Headless Menu Pick Reused");
        QVERIFY(writeProfile(name, qsl(R"lua(tempRegexTrigger("^headless picked$", [[echoPopup(" p", {"headlessPicked = 'old'", "headlessPicked = 'old two'"}, {"one", "two"})]]))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(feed(qsl("headless picked"))));
        const int id = lastLinkOf(shown, qsl("headless picked p"));
        TConsoleModel& model = session.host()->mainConsoleModel();
        const TLinkStore& store = model.buffer.mLinkStore;
        const qint64 issue = store.issueOf(id);
        QVERIFY(issue > 0);
        // The store holds 20000 ids, so this many more links come round to this one's
        QVERIFY(session.runLua(qsl("for i = 1, 20000 do echoLink('x', [[headlessPicked = 'new']], '') end")));
        QCOMPARE(store.getCurrentLinkID(), id);
        QCOMPARE(store.getLinksConst(id), QStringList{qsl("headlessPicked = 'new'")});
        QVERIFY(store.issueOf(id) != issue);
        model.runMenuPick(id, issue, 0);
        QVERIFY(luaGlobal(session, "headlessPicked").isEmpty());
        model.runMenuPick(id, store.issueOf(id), 0);
        QCOMPARE(luaGlobal(session, "headlessPicked"), qsl("new"));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionLinksTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionLinksTest)
