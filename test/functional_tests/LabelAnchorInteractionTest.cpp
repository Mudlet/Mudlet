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
#include <QMenu>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <QTextDocument>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
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

    TLabel* label() const { return mpHost->mpConsole->labelWidget(mLabelName); }

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

    // Right-clicks a label and closes whatever popup that opened, handing back
    // the menu's parent and whether it was the console's own menu rather than a
    // link's, so a case can clean up before it asserts
    static std::pair<QObject*, bool> rightClickLabel(TLabel* pLabel)
    {
        QTest::mouseClick(pLabel, Qt::RightButton, Qt::NoModifier, pLabel->rect().center());
        auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!popup) {
            return {nullptr, false};
        }
        const std::pair<QObject*, bool> result{popup->parent(), popup->findChild<QAction*>(qsl("consoleSelectAll")) != nullptr};
        popup->close();
        QTest::qWait(50ms);
        return result;
    }

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
        if (mpHost && mpHost->mpConsole) {
            mpHost->mpConsole->deleteLabel(mLabelName);
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

    // A label over the output must not cost the player the console's context
    // menu, unless the label has a click callback of its own to answer with (#10753)
    void test_aRightClickOnALabelWithoutCallbacksOpensTheConsoleMenu()
    {
        const QString name = qsl("rightClickForwardLabel");
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        TLabel* pLabel = mpHost->mpConsole->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();
        TTextEdit* pPane = mpHost->mpConsole->mUpperPane;
        QVERIFY2(pPane->rect().contains(pPane->mapFromGlobal(pLabel->mapToGlobal(pLabel->rect().center()))), "the label is not over the console's text, so there is no menu to reach");
        QVERIFY2(!QApplication::activePopupWidget(), "a popup was already open before this case right-clicked");

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setLabelClickCallback('%1', function() end)").arg(name)));
        const auto [unexpectedParent, unexpectedMenu] = rightClickLabel(pLabel);
        Q_UNUSED(unexpectedMenu)
        mpHost->mpConsole->deleteLabel(name);

        QVERIFY2(menuParent == pPane && consoleMenu, "a right-click on a label with no callbacks did not open the console's context menu");
        QVERIFY2(!unexpectedParent, "the console's menu opened over a label whose own click callback answers right-clicks");
    }

    // The menu belongs to whatever the label covers, which over a miniconsole is
    // the miniconsole rather than the main console behind it
    void test_aRightClickOnALabelOverAMiniconsoleOpensThatConsolesMenu()
    {
        const QString mini = qsl("rightClickUnderLabelMini");
        const QString name = qsl("rightClickOverMiniLabel");
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('%1', 40, 90, 300, 120)").arg(mini)));
        TConsole* pMini = mpHost->mpConsole->subConsoleWidget(mini);
        QVERIFY(pMini);
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        TLabel* pLabel = mpHost->mpConsole->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();
        pLabel->raise();

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);
        mpHost->mpConsole->deleteLabel(name);
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("hideWindow('%1')").arg(mini)));

        QVERIFY2(menuParent == pMini->mUpperPane && consoleMenu, "a right-click on a label over a miniconsole did not open that miniconsole's menu");
    }

    // The player cannot see a link the label hides, so a right-click there must
    // not reveal it or offer its commands, only the console's own menu
    void test_aRightClickOnALabelOverALinkOpensTheConsoleMenuNotTheLinks()
    {
        const QString name = qsl("rightClickOverLinkLabel");
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(
                qsl("for i = 1, 80 do echoPopup(string.rep('L', 300), {[[qaHiddenLinkRan = true]], [[qaHiddenLinkRan = true]]}, {'one', 'two'}) echo('\\n') end")));
        QTest::qWait(100ms);
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        TLabel* pLabel = mpHost->mpConsole->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);
        mpHost->mpConsole->deleteLabel(name);
        mpHost->mpConsole->buffer.clear();

        QVERIFY2(menuParent == mpHost->mpConsole->mUpperPane, "a right-click on a label over a link opened no menu from the console");
        QVERIFY2(consoleMenu, "a right-click on a label offered the commands of the link it hides");
    }
};

#include "LabelAnchorInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAnchorInteractionTest)
