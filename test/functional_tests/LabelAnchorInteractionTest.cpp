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
 * Neither a label's text interaction flags, a synthesised click on a link, nor a
 * context menu event is reachable from Lua, so they all have to be asked of the
 * TLabel directly.
 *
 * Run with: ctest -R LabelAnchorInteractionTest -V
 */

#include <QContextMenuEvent>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <QTextDocument>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LabelAnchorInteractionTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("LabelAnchorInteraction-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLabelName = qsl("anchorInteractionLabel");
    // a configuration directory of its own, so the profile this opens is never
    // one of the developer's
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;

    TLabel* label() const { return mpHost->mainConsoleView()->labelWidget(mLabelName); }

    // Read back through the return value rather than getLuaString(), which reports
    // an absolute stack slot and so only answers correctly for the first call in a
    // process.
    bool luaHolds(const QString& condition) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(%1)").arg(condition)); }

    // Where the label's only word of text sits, so a synthesised click lands on
    // the link rather than the empty space around it
    QPoint linkCentre() const
    {
        QTextDocument document;
        document.setHtml(label()->text());
        document.setDefaultFont(label()->font());
        return QPoint(static_cast<int>(document.idealWidth()) / 2, static_cast<int>(document.size().height()) / 2);
    }

    // The document above is laid out on its own, without the label's contents
    // margins or alignment, so a click aimed by it can miss the widget - which
    // would read as a callback that did not fire.
    bool centreIsOnTheLabel() const { return label()->rect().contains(linkCentre()); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        QVERIFY(mConfigDir.isValid());
        // setupConfig() only adopts $XDG_CONFIG_HOME once the profiles
        // directory under it is there to be adopted
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1s)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        auto [created, createMessage] = mpHost->createLabel(qsl("main"), mLabelName, 10, 10, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        QVERIFY(label());
    }

    void cleanupTestCase()
    {
        if (mpHost && mpHost->mainConsoleView()) {
            mpHost->deleteLabel(mLabelName);
        }
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        if (mSavedXdgConfigHome.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    void test_whichTextsCountAsCarryingAnAnchor_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("interactive");

        QTest::newRow("plain text") << qsl("just some words") << false;
        QTest::newRow("markup with no anchor") << qsl("<b>bold</b> and <i>italic</i>") << false;
        QTest::newRow("anchor") << qsl("<a href='https://example.com'>go</a>") << true;
        QTest::newRow("anchor in upper case") << qsl("<A HREF='https://example.com'>go</A>") << true;
        QTest::newRow("anchor after other markup") << qsl("<b>x</b> <a href='https://example.com'>go</a>") << true;
        // HTML lets any whitespace follow the tag name, and Qt renders these as
        // links, so a label carrying one has to be told it has a link in it
        QTest::newRow("anchor split by a newline") << qsl("<a\nhref='https://example.com'>go</a>") << true;
        QTest::newRow("anchor split by a tab") << qsl("<a\thref='https://example.com'>go</a>") << true;
        QTest::newRow("upper case anchor split by a newline") << qsl("<A\nHREF='https://example.com'>go</A>") << true;
        QTest::newRow("anchor split by a carriage return") << qsl("<a\rhref='https://example.com'>go</a>") << true;
        QTest::newRow("anchor split by a vertical tab") << qsl("<a\vhref='https://example.com'>go</a>") << true;
        QTest::newRow("anchor split by a form feed") << qsl("<a\fhref='https://example.com'>go</a>") << true;
        // Qt's own HTML parser and QChar::isSpace() agree on the Unicode spaces,
        // so a narrower separator test would take the flags off a text Qt links.
        QTest::newRow("anchor split by a no-break space") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x00a0)) << true;
        QTest::newRow("anchor split by a thin space") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x2009)) << true;
        // They agree the other way on the ASCII separators, which are control
        // characters rather than spaces: Qt draws no link here.
        QTest::newRow("an ASCII file separator is not a tag separator") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x001c)) << false;
        // an anchor needs attributes, and a bare "<a" cannot be the start of one
        QTest::newRow("anchor tag name only") << qsl("truncated at <a") << false;
        // prose holds "<a" and whitespace too, with nothing Qt draws as a link
        QTest::newRow("prose holding a bare <a") << qsl("five <a\nb, ten <a\nc") << false;
        QTest::newRow("prose holding a bare <a and a space") << qsl("five <a b, ten <a c") << false;
        QTest::newRow("an anchor that is never closed") << qsl("<a href='https://example.com'") << false;
        QTest::newRow("a bare <a ahead of a real anchor") << qsl("five <a b <a href='https://example.com'>go</a>") << true;
        QTest::newRow("a word starting with a") << qsl("<abbr title='x'>ab</abbr>") << false;
        // No second '<' to turn the bare "<a" away with, so this answers yes. It
        // costs the flags and nothing else: the label's own callbacks run either
        // way.
        QTest::newRow("prose with a bare <a and a later >") << qsl("five <a b, ten> c") << true;
        // An attribute value holding a '<' of its own is turned away by the same
        // rule. HTML asks for that character to be written &lt;.
        QTest::newRow("an anchor whose attribute value holds a <") << qsl("<a href='a<b'>go</a>") << false;
        QTest::newRow("consecutive angle brackets") << qsl("<<a href='https://example.com'>go</a>") << true;
    }

    void test_whichTextsCountAsCarryingAnAnchor()
    {
        QFETCH(QString, text);
        QFETCH(bool, interactive);

        // start from a text of the other kind, so a flag left over from the
        // previous row cannot pass this one
        label()->setText(interactive ? qsl("nothing here") : qsl("<a href='https://example.com'>go</a>"));
        label()->setText(text);

        // The whole flag set, not just the link bit: a selectable label swallows
        // the press its click callback needs.
        const Qt::TextInteractionFlags expected = interactive ? (Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard) : Qt::TextInteractionFlags(Qt::NoTextInteraction);
        QCOMPARE(label()->textInteractionFlags(), expected);
        // QLabel derives its focus policy from those flags, and that policy is the
        // whole of a link's keyboard reachability.
        QCOMPARE(label()->focusPolicy(), interactive ? Qt::StrongFocus : Qt::NoFocus);
    }

    void test_aLinkSplitByWhitespaceIsClickable()
    {
        // no link style configured, which is the default: with one, the styling
        // pass rewrites the tag and puts the plain "<a " back into the text
        label()->resetLinkStyle();
        label()->setText(qsl("<a\nhref='https://example.com'>go</a>"));

        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");
        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().toString(), qsl("https://example.com"));
    }

    void test_clickthroughRoundTripKeepsALinkSplitByWhitespaceClickable()
    {
        label()->resetLinkStyle();
        label()->setText(qsl("<a\nhref='https://example.com'>go</a>"));
        label()->setClickThrough(true);
        label()->setClickThrough(false);

        QCOMPARE(label()->textInteractionFlags(), Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    }

    void test_aClickOnALinkStillReachesTheLabelsOwnCallbacks()
    {
        label()->resetLinkStyle();
        label()->setText(qsl("<a\nhref='https://example.com'>go</a>"));

        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorClicks = 0\n"
                                                                 "anchorReleases = 0\n"
                                                                 "anchorRegistered = setLabelClickCallback('%1', function() anchorClicks = anchorClicks + 1 end)\n"
                                                                 "anchorRegistered = anchorRegistered and setLabelReleaseCallback('%1', function() anchorReleases = anchorReleases + 1 end)\n")
                                                                     .arg(mLabelName));
        // a registration that quietly failed would make the counts below prove
        // nothing
        QVERIFY2(luaHolds(qsl("anchorRegistered")), "the callbacks did not reach the label");
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());

        QCOMPARE(activated.count(), 1);
        QVERIFY2(luaHolds(qsl("anchorClicks == 1")), "the label's click callback did not fire");
        QVERIFY2(luaHolds(qsl("anchorReleases == 1")), "the label's release callback did not fire");
    }

    void cleanup()
    {
        if (mpHost) {
            mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorSchemeRan, anchorColon, anchorColonText, anchorSent, anchorSendHandler = nil\n"
                                                                     "javascript, JAVASCRIPT, ftp, file, steam, about, magnet, data, mailto, tel, SMS = nil\n"));
        }
    }

    void test_aLinkOfANonLuaSchemeIsNotRun_data()
    {
        QTest::addColumn<QString>("link");

        QTest::newRow("javascript") << qsl("javascript:void(0)");
        QTest::newRow("upper-case JAVASCRIPT") << qsl("JAVASCRIPT:void(0)");
        QTest::newRow("javascript after a space") << qsl(" javascript:void(0)");
        QTest::newRow("ftp") << qsl("ftp://host/x");
        QTest::newRow("file") << qsl("file:///tmp/x");
        QTest::newRow("steam") << qsl("steam://run/1");
        QTest::newRow("about") << qsl("about:blank");
        QTest::newRow("magnet") << qsl("magnet:?xt=urn:btih:abc");
        QTest::newRow("data") << qsl("data:text/plain,hi");
        QTest::newRow("mailto") << qsl("mailto:someone@example.com");
        QTest::newRow("tel") << qsl("tel:+15551234");
    }

    // Each scheme is also a global whose every method sets a sentinel, which catches a javascript: link run as Lua;
    // the other links do not parse as Lua, so only the [ERROR:] check would catch one being run
    void test_aLinkOfANonLuaSchemeIsNotRun()
    {
        QFETCH(QString, link);

        label()->resetLinkStyle();
        label()->setText(qsl("<a href=\"%1\">go</a>").arg(link));
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorSchemeRan = false\n"
                                                                 "local ran = function() anchorSchemeRan = true end\n"
                                                                 "local scheme = setmetatable({}, {__index = function() return ran end})\n"
                                                                 "javascript, JAVASCRIPT, ftp, file, steam = scheme, scheme, scheme, scheme, scheme\n"
                                                                 "about, magnet, data, mailto, tel = scheme, scheme, scheme, scheme, scheme\n"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QStringList errorsConsole;
        auto collect = connect(mpHost, &Host::signal_errorConsolePrint, this, [&errorsConsole](const QString& text) {
            errorsConsole << text;
        });
        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());
        disconnect(collect);

        QCOMPARE(activated.count(), 1);
        QVERIFY2(luaHolds(qsl("anchorSchemeRan == false")), "the link was run as Lua");
        const QString printed = errorsConsole.join(QString());
        QVERIFY2(!printed.contains(qsl("[ERROR:]")), qPrintable(qsl("the link was reported as a Lua error: %1").arg(printed)));
        QVERIFY2(printed.contains(link), qPrintable(qsl("nothing in the Errors console names the ignored link: \"%1\"").arg(printed)));
    }

    void test_aLuaCommandLinkWithAColonRuns_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("ran");

        QTest::newRow("method call") << qsl("anchorColon:go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("colon in a string") << qsl("anchorColonText = 'at 12:00'") << qsl("anchorColonText == 'at 12:00'");
        QTest::newRow("URL-like string") << qsl("anchorColonText = 'at://home'") << qsl("anchorColonText == 'at://home'");
        QTest::newRow("method of a table named tel") << qsl("tel:go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method of a table named SMS") << qsl("SMS:go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method of a table named data") << qsl("data:go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method of a field") << qsl("anchorColon.inner:go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method call with a string argument") << qsl("anchorColon:go 'x'") << qsl("anchorColon.ran == 1");
        QTest::newRow("method call with a space after the colon") << qsl("anchorColon: go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method call with a tab after the colon") << qsl("anchorColon:\tgo()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method call with a comment after the colon") << qsl("anchorColon:--[[c]]go()") << qsl("anchorColon.ran == 1");
        QTest::newRow("method call with a comment before the arguments") << qsl("anchorColon:go --[==[c]==] ()") << qsl("anchorColon.ran == 1");
    }

    void test_aLuaCommandLinkWithAColonRuns()
    {
        QFETCH(QString, command);
        QFETCH(QString, ran);

        label()->resetLinkStyle();
        label()->setText(qsl("<a href=\"%1\">go</a>").arg(command));
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorColon = {ran = 0}\n"
                                                                 "function anchorColon:go() self.ran = self.ran + 1 end\n"
                                                                 "anchorColon.inner, tel, SMS, data = anchorColon, anchorColon, anchorColon, anchorColon\n"
                                                                 "anchorColonText = nil\n"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());

        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().toString(), command);
        QVERIFY2(luaHolds(ran), "the link's Lua command did not run");
    }

    void test_aPromptLinkKeepsItsTrailingSpace()
    {
        TCommandLine* commandLine = mpHost->mainConsoleView()->mpCommandLine;
        QVERIFY(commandLine);
        commandLine->clear();
        label()->resetLinkStyle();
        label()->setText(qsl("<a href=\"prompt:tell Bob \">go</a>"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());

        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().toString(), qsl("prompt:tell Bob "));
        QCOMPARE(commandLine->toPlainText(), qsl("tell Bob "));
        commandLine->clear();
    }

    void test_aSendLinkKeepsItsTrailingSpace()
    {
        label()->resetLinkStyle();
        label()->setText(qsl("<a href=\"send:say hi \">go</a>"));
        mpHost->getLuaInterpreter()->compileAndExecuteScript(
                qsl("anchorSent = {}\n"
                    "anchorSendHandler = registerAnonymousEventHandler('sysDataSendRequest', function(_, command) anchorSent[#anchorSent + 1] = command end)\n"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("killAnonymousEventHandler(anchorSendHandler)"));

        QCOMPARE(activated.count(), 1);
        QVERIFY2(luaHolds(qsl("#anchorSent == 1 and anchorSent[1] == 'say hi '")), "the send: link did not send its text with the trailing space kept");
    }

    void test_aRightClickOpensNoMenuOfQtsOwn_data()
    {
        QTest::addColumn<QString>("text");

        QTest::newRow("plain text") << qsl("just some words");
        QTest::newRow("prose holding a bare <a") << qsl("five <a\nb, ten <a\nc");
        QTest::newRow("a link") << qsl("<a\nhref='https://example.com'>go</a>");
    }

    void test_aRightClickOpensNoMenuOfQtsOwn()
    {
        QFETCH(QString, text);

        label()->resetLinkStyle();
        label()->setText(text);

        // activePopupWidget() is process wide, so a popup left open by anything
        // earlier would be read as this label's menu
        QVERIFY2(!QApplication::activePopupWidget(), "a popup was already open before this case sent its context menu event");

        // right on the text, which is where Qt looks for an anchor to offer
        const QPoint where = linkCentre();
        QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, where, label()->mapToGlobal(where));
        QApplication::sendEvent(label(), &menuEvent);

        // Qt's menu is a popup window of its own, so it is there to be found
        // whether or not the event was accepted
        QWidget* popup = QApplication::activePopupWidget();
        if (popup) {
            popup->close();
            QTest::qWait(50ms);
        }
        QVERIFY2(!popup, "Qt opened a context menu of its own over the label");
        // An ignored context menu event is offered to each ancestor in turn, so
        // this covers the label and everything it sits in.
        QVERIFY2(!menuEvent.isAccepted(), "the label, or a widget it sits in, took the context menu event instead of passing it on");
    }
};

#include "LabelAnchorInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAnchorInteractionTest)
