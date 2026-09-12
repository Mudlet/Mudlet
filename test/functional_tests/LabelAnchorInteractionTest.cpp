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
 * A label gets Qt's link interaction only when its text carries an anchor, so
 * which texts count as carrying one decides which links work - while the label's
 * own click callback and its right-click behaviour must not depend on that answer
 * at all. None of the interaction flags, a synthesised click on a link, or a
 * context menu event are reachable from Lua, so they all have to be asked of the
 * TLabel directly.
 *
 * Run with: ctest -R LabelAnchorInteractionTest -V
 */

#include <QContextMenuEvent>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <QTextDocument>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
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

    TLabel* label() const { return mpHost->mpConsole->labelWidget(mLabelName); }

    // What a Lua callback counted, since the callbacks themselves only exist in
    // the interpreter. Read back through the return value rather than
    // getLuaString(), which reports an absolute stack slot and so only answers
    // correctly for the first call in a process.
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
    // would read as a callback that did not fire rather than as a click that
    // went nowhere.
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
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
        if (!spy.wait(1000)) {
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
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
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
        // QChar::isSpace() and Qt's own HTML parser agree on the Unicode spaces
        // as well as the ASCII ones - Qt draws each of these as a link - so a
        // separator test narrower than isSpace() would take the flags away from
        // text Qt still puts a link in.
        QTest::newRow("anchor split by a no-break space") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x00a0)) << true;
        QTest::newRow("anchor split by a thin space") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x2009)) << true;
        // ...and they agree the other way on the ASCII separators, which are
        // control characters rather than spaces: Qt draws no link here.
        QTest::newRow("an ASCII file separator is not a tag separator") << qsl("<a%1href='https://example.com'>go</a>").arg(QChar(0x001c)) << false;
        // an anchor needs attributes, and a bare "<a" cannot be the start of one
        QTest::newRow("anchor tag name only") << qsl("truncated at <a") << false;
        // prose - and game text echoed into a label - holds "<a" and whitespace
        // too, with nothing Qt draws as a link
        QTest::newRow("prose holding a bare <a") << qsl("five <a\nb, ten <a\nc") << false;
        QTest::newRow("prose holding a bare <a and a space") << qsl("five <a b, ten <a c") << false;
        QTest::newRow("an anchor that is never closed") << qsl("<a href='https://example.com'") << false;
        QTest::newRow("a bare <a ahead of a real anchor") << qsl("five <a b <a href='https://example.com'>go</a>") << true;
        QTest::newRow("a word starting with a") << qsl("<abbr title='x'>ab</abbr>") << false;
        // Prose with nothing after the bare "<a" to say it was not a tag: the
        // walk has no second '<' to turn it away with, so it answers yes, as the
        // contains("<a ") this replaced did. What that costs is the flags and
        // nothing else - the label's own callbacks run either way, and the
        // context menu case below covers every one of these texts.
        QTest::newRow("prose with a bare <a and a later >") << qsl("five <a b, ten> c") << true;
        // An attribute value holding a '<' of its own is turned away by the same
        // rule, so a label carrying one loses its link interaction. HTML asks
        // for that character to be written &lt;.
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

        // The whole flag set, not just the link bit: leaving the label selectable
        // would have QLabel swallow the press the label's click callback needs,
        // and would put Qt's Copy / Select All menu on it. What keeps Qt's
        // smaller Copy Link Location menu away is the contextMenuEvent()
        // override, not these flags.
        const Qt::TextInteractionFlags expected = interactive ? (Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard) : Qt::TextInteractionFlags(Qt::NoTextInteraction);
        QCOMPARE(label()->textInteractionFlags(), expected);
        // QLabel derives its focus policy from those flags, and that policy is
        // the whole of a link's keyboard reachability - which is what the
        // keyboard flag above is there for.
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
        // nothing at all
        QVERIFY2(luaHolds(qsl("anchorRegistered")), "the callbacks did not reach the label");
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());

        // the link the click landed on, and the callbacks the script registered:
        // a label carrying a link is still a clickable label
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
        // this is the label and everything it sits in: a console or main window
        // that grew a menu of its own would report here too.
        QVERIFY2(!menuEvent.isAccepted(), "the label, or a widget it sits in, took the context menu event instead of passing it on");
    }
};

#include "LabelAnchorInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAnchorInteractionTest)
