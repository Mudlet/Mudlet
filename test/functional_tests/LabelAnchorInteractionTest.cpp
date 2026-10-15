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
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
// A move a widget ignores is offered to its parent next, which is where a filter on the parent sees it
class MoveCounter : public QObject
{
public:
    int mMoves = 0;

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::MouseMove) {
            ++mMoves;
        }
        return QObject::eventFilter(watched, event);
    }
};
} // namespace

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

    // Where the label's only word of text sits, so a synthesised click or move lands
    // on the link rather than the empty space around it. QLabel lays its text out
    // with no document margin and puts it where its alignment says within the
    // contents rect, and aiming at a stand-alone document's middle misses a small
    // font's line. Its margin() and indent() are not modelled.
    QPoint linkCentre() const
    {
        QTextDocument document;
        document.setDocumentMargin(0);
        document.setHtml(label()->text());
        document.setDefaultFont(label()->font());
        const QRect area = label()->contentsRect();
        const int width = static_cast<int>(document.idealWidth());
        const int height = static_cast<int>(document.size().height());
        const Qt::Alignment alignment = label()->alignment();
        const int spareWidth = area.width() - width;
        const int spareHeight = area.height() - height;
        const int left = area.left() + ((alignment & Qt::AlignHCenter) ? spareWidth / 2 : (alignment & Qt::AlignRight) ? spareWidth : 0);
        const int top = area.top() + ((alignment & Qt::AlignVCenter) ? spareHeight / 2 : (alignment & Qt::AlignBottom) ? spareHeight : 0);
        return QPoint(left + width / 2, top + height / 2);
    }

    // The document above is laid out on its own, so a click aimed by it can still
    // miss the widget - which would read as a callback that did not fire.
    bool centreIsOnTheLabel() const { return label()->rect().contains(linkCentre()); }

    // Sent rather than made with the real pointer, which a headless run has no control over
    void moveTo(const QPoint& where) const
    {
        QMouseEvent move(QEvent::MouseMove, where, label()->mapToGlobal(where), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(label(), &move);
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

    void test_aLinkShowsThePointingHand()
    {
        label()->resetLinkStyle();
        label()->unsetCursor();
        label()->setText(qsl("<a href='https://example.com'>go</a>"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case hovers is outside the label, so it would miss the link");

        moveTo(QPoint(label()->width() - 1, label()->height() - 1));
        QVERIFY(label()->cursor().shape() != Qt::PointingHandCursor);
        // with no move callback the move goes on to the label's parent
        MoveCounter underneath;
        QWidget* parent = label()->parentWidget();
        // Qt drops a buttonless move on its way up at a widget that does not track the mouse
        const bool parentTracked = parent->hasMouseTracking();
        parent->setMouseTracking(true);
        parent->installEventFilter(&underneath);
        // a failed check returns early, and the cases after this one share the parent
        auto restoreParent = qScopeGuard([parent, parentTracked, &underneath] {
            parent->removeEventFilter(&underneath);
            parent->setMouseTracking(parentTracked);
        });
        moveTo(linkCentre());
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
        QVERIFY2(underneath.mMoves == 1, "a label with no move callback kept a move over its link to itself");
        moveTo(QPoint(label()->width() - 1, label()->height() - 1));
        QVERIFY(label()->cursor().shape() != Qt::PointingHandCursor);

        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorMoves = 0\n"
                                                                 "anchorMoveRegistered = setLabelMoveCallback('%1', function() anchorMoves = anchorMoves + 1 end)\n")
                                                                     .arg(mLabelName));
        // a registered move callback keeps every later move from the parent, and the cases after this one share the label
        auto dropCallback = qScopeGuard([this] {
            mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setLabelMoveCallback('%1', nil)").arg(mLabelName));
        });
        QVERIFY2(luaHolds(qsl("anchorMoveRegistered")), "the move callback did not reach the label");
        moveTo(linkCentre());
        QVERIFY2(luaHolds(qsl("anchorMoves == 1")), "the label's move callback did not fire over its link");
        QVERIFY2(underneath.mMoves == 2, "a label's move callback did not keep the move to itself");
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
    }

    void test_aCursorSetOverALinkOutlastsTheHover_data()
    {
        QTest::addColumn<QString>("before");
        QTest::addColumn<QString>("onTheLink");
        QTest::addColumn<int>("expected");

        QTest::newRow("set") << QString() << qsl("setLabelCursor('%1', mudlet.cursor.Busy)") << static_cast<int>(Qt::BusyCursor);
        QTest::newRow("reset") << qsl("setLabelCursor('%1', mudlet.cursor.Cross)") << qsl("resetLabelCursor('%1')") << static_cast<int>(Qt::ArrowCursor);
        const QString cursorImage = qsl("%1/customCursor.png").arg(mConfigDir.path());
        QPixmap image(16, 16);
        image.fill(Qt::red);
        QVERIFY(image.save(cursorImage));
        QTest::newRow("custom") << QString() << qsl("setLabelCustomCursor('%1', [[%2]], 0, 0)").arg(qsl("%1"), cursorImage) << static_cast<int>(Qt::BitmapCursor);
    }

    // QLabel saves the cursor when the hover begins and puts that copy back when it ends
    void test_aCursorSetOverALinkOutlastsTheHover()
    {
        QFETCH(QString, before);
        QFETCH(QString, onTheLink);
        QFETCH(int, expected);

        label()->resetLinkStyle();
        label()->unsetCursor();
        label()->setText(qsl("<a href='https://example.com'>go</a>"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case hovers is outside the label, so it would miss the link");
        const QPoint corner(label()->width() - 1, label()->height() - 1);
        // QLabel keeps its hover across a failed check, and the next case comes straight back to the link
        auto cleanUp = qScopeGuard([this] {
            label()->endLinkHover();
            label()->unsetCursor();
        });

        moveTo(corner);
        if (!before.isEmpty()) {
            mpHost->getLuaInterpreter()->compileAndExecuteScript(before.arg(mLabelName));
        }
        moveTo(linkCentre());
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
        mpHost->getLuaInterpreter()->compileAndExecuteScript(onTheLink.arg(mLabelName));
        moveTo(corner);
        QCOMPARE(static_cast<int>(label()->cursor().shape()), expected);
        // the next hover starts from the script's cursor and gives it back
        moveTo(linkCentre());
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
        moveTo(corner);
        QCOMPARE(static_cast<int>(label()->cursor().shape()), expected);
    }

    void test_leavingFromALinkDropsTheHand()
    {
        label()->resetLinkStyle();
        label()->unsetCursor();
        label()->setText(qsl("<a href='https://example.com'>go</a>"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case hovers is outside the label, so it would miss the link");

        moveTo(linkCentre());
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(label(), &leave);
        // re-entering over plain text would show the label's cursor before any move arrives
        QVERIFY(label()->cursor().shape() != Qt::PointingHandCursor);
        // and coming straight back onto the link starts a new hover
        moveTo(linkCentre());
        QCOMPARE(label()->cursor().shape(), Qt::PointingHandCursor);
        QApplication::sendEvent(label(), &leave);
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
