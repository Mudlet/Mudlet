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
#include <QScopeGuard>
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
#include "TTextEdit.h"
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

    void test_aClickedLinkAloneTakesTheVisitedColour()
    {
        label()->setLinkStyle(qsl("#00ffff"), qsl("#ff00ff"), true);
        label()->clearVisitedLinks();
        // the first link is the longer by far, so the middle of the text is on it
        label()->setText(qsl("<a href=\"anchorVisitedOne()\">gogogogogogogo</a><a href=\"anchorVisitedTwo()\">x</a>"));
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("function anchorVisitedOne() end\nfunction anchorVisitedTwo() end\n"));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().toString(), qsl("anchorVisitedOne()"));

        QCOMPARE(label()->text(),
                 qsl("<a href=\"anchorVisitedOne()\" style=\"color: #ff00ff;\">gogogogogogogo</a>"
                     "<a href=\"anchorVisitedTwo()\" style=\"color: #00ffff;\">x</a>"));

        label()->clearVisitedLinks();
        QCOMPARE(label()->text(),
                 qsl("<a href=\"anchorVisitedOne()\" style=\"color: #00ffff;\">gogogogogogogo</a>"
                     "<a href=\"anchorVisitedTwo()\" style=\"color: #00ffff;\">x</a>"));
        label()->resetLinkStyle();
    }

    void test_aLinkStyleLeavesAClickThroughLabelsLinksOutOfReach()
    {
        label()->resetLinkStyle();
        label()->setText(qsl("<a href=\"anchorThrough()\">go</a>"));
        label()->setClickThrough(true);
        auto restore = qScopeGuard([this]() {
            label()->setClickThrough(false);
            label()->resetLinkStyle();
        });
        QCOMPARE(label()->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));

        label()->setLinkStyle(qsl("#00ffff"), qsl("#ff00ff"), true);
        QCOMPARE(label()->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);
        label()->clearVisitedLinks();
        QCOMPARE(label()->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));
        label()->resetLinkStyle();
        QCOMPARE(label()->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));
        label()->setText(qsl("<a href=\"anchorThrough()\">again</a>"));
        QCOMPARE(label()->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);

        label()->setClickThrough(false);
        QCOMPARE(label()->textInteractionFlags(), Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    }

    // The headless frontend a spec can run under does not apply a style sheet's qproperty-text
    void test_aStyleSheetsTextLeavesTheScriptsOwnLinkColourAlone()
    {
        label()->setLinkStyle(qsl("#00ffff"), QString(), true);
        const QString sheet = label()->styleSheet();
        // a style sheet change puts back the font the label had before setFont(), which later cases aim by
        const QFont font = label()->font();
        auto restore = qScopeGuard([this, sheet, font]() {
            label()->setStyleSheet(sheet);
            label()->setFont(font);
            label()->resetLinkStyle();
        });
        label()->setText(qsl("<a href=\"a()\">a</a>"));
        label()->setStyleSheet(qsl("qproperty-text: \"plain\";"));
        QCOMPARE(label()->text(), qsl("plain"));

        label()->resetLinkStyle();
        const QString own = qsl("<a href=\"a()\" style=\"color: #00ffff;\">a</a>");
        label()->setText(own);
        QCOMPARE(label()->text(), own);
    }

    void test_aStyledLinkHoldingQuotesRunsItsCommand()
    {
        label()->setLinkStyle(qsl("#00ffff"), qsl("#ff00ff"), true);
        auto restore = qScopeGuard([this]() {
            label()->resetLinkStyle();
        });
        label()->setText(qsl("<a href=\"anchorQuoted('x')\">gogogo</a>"));
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("anchorQuotedArg = nil\nfunction anchorQuoted(arg) anchorQuotedArg = arg end\n")));
        QVERIFY2(centreIsOnTheLabel(), "the point this case clicks is outside the label, so it would miss the link");

        QSignalSpy activated(label(), &QLabel::linkActivated);
        QTest::mouseClick(label(), Qt::LeftButton, Qt::NoModifier, linkCentre());
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().toString(), qsl("anchorQuoted('x')"));
        QVERIFY2(luaHolds(qsl("anchorQuotedArg == 'x'")), "the link's Lua command did not run with its argument");
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

    // A label over the output must not cost the player the console's context menu,
    // unless the label has a click or release callback of its own to answer with
    void test_aRightClickOnALabelWithoutCallbacksOpensTheConsoleMenu()
    {
        const QString name = qsl("rightClickForwardLabel");
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        const auto removeLabel = qScopeGuard([this, name]() {
            mpHost->mainConsoleView()->deleteLabel(name);
        });
        TLabel* pLabel = mpHost->mainConsoleView()->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();
        TTextEdit* pPane = mpHost->mainConsoleView()->mUpperPane;
        QVERIFY2(pPane->rect().contains(pPane->mapFromGlobal(pLabel->mapToGlobal(pLabel->rect().center()))), "the label is not over the console's text, so there is no menu to reach");
        QVERIFY2(!QApplication::activePopupWidget(), "a popup was already open before this case right-clicked");

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);
        QVERIFY2(menuParent == pPane && consoleMenu, "a right-click on a label with no callbacks did not open the console's context menu");
        // The hit test makes the label transparent to the mouse for a moment, which is
        // also what enableClickthrough() does, so left that way it would stop answering clicks
        QVERIFY2(!pLabel->testAttribute(Qt::WA_TransparentForMouseEvents), "the right-click left the label click-through");

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setLabelClickCallback('%1', function() end)").arg(name)));
        const auto [unexpectedParent, unexpectedMenu] = rightClickLabel(pLabel);
        Q_UNUSED(unexpectedMenu)
        QVERIFY2(!unexpectedParent, "the console's menu opened over a label whose own click callback answers right-clicks");
    }

    // The menu belongs to whatever the label covers, which over a miniconsole is
    // the miniconsole rather than the main console behind it
    void test_aRightClickOnALabelOverAMiniconsoleOpensThatConsolesMenu()
    {
        const QString mini = qsl("rightClickUnderLabelMini");
        const QString name = qsl("rightClickOverMiniLabel");
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('%1', 40, 90, 300, 120)").arg(mini)));
        const auto removeMini = qScopeGuard([this, mini]() {
            mpHost->deleteMiniConsole(mini);
        });
        TConsole* pMini = mpHost->mainConsoleView()->subConsoleWidget(mini);
        QVERIFY(pMini);
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        const auto removeLabel = qScopeGuard([this, name]() {
            mpHost->mainConsoleView()->deleteLabel(name);
        });
        TLabel* pLabel = mpHost->mainConsoleView()->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();
        pLabel->raise();

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);

        QVERIFY2(menuParent == pMini->mUpperPane && consoleMenu, "a right-click on a label over a miniconsole did not open that miniconsole's menu");
    }

    // Labels stacked over the output, none with callbacks, still leave the menu
    // to the console under all of them
    void test_aRightClickOnLabelsStackedWithoutCallbacksOpensTheConsoleMenu()
    {
        const QString lower = qsl("rightClickStackedLowerLabel");
        const QString upper = qsl("rightClickStackedUpperLabel");
        auto [lowerCreated, lowerMessage] = mpHost->createLabel(qsl("main"), lower, 40, 90, 220, 70, true, false);
        QVERIFY2(lowerCreated, qPrintable(lowerMessage));
        const auto removeLower = qScopeGuard([this, lower]() {
            mpHost->mainConsoleView()->deleteLabel(lower);
        });
        auto [upperCreated, upperMessage] = mpHost->createLabel(qsl("main"), upper, 50, 100, 200, 50, true, false);
        QVERIFY2(upperCreated, qPrintable(upperMessage));
        const auto removeUpper = qScopeGuard([this, upper]() {
            mpHost->mainConsoleView()->deleteLabel(upper);
        });
        TLabel* pLower = mpHost->mainConsoleView()->labelWidget(lower);
        TLabel* pUpper = mpHost->mainConsoleView()->labelWidget(upper);
        QVERIFY(pLower && pUpper);
        pLower->show();
        pUpper->show();
        pUpper->raise();
        TTextEdit* pPane = mpHost->mainConsoleView()->mUpperPane;
        QVERIFY2(pLower->geometry().contains(pUpper->geometry().center()), "the labels do not overlap, so the lower one is never what the upper one covers");

        const auto [menuParent, consoleMenu] = rightClickLabel(pUpper);

        QVERIFY2(menuParent == pPane && consoleMenu, "a right-click on a label stacked over another without callbacks did not open the console's context menu");
        QVERIFY2(!pUpper->testAttribute(Qt::WA_TransparentForMouseEvents) && !pLower->testAttribute(Qt::WA_TransparentForMouseEvents), "the right-click left a stacked label click-through");
        QVERIFY2(pUpper->parentWidget()->childAt(pUpper->geometry().center()) == pUpper, "the right-click left the upper label out of the hit test");
    }

    void test_aLabelWithCallbacksUnderTheOneRightClickedKeepsTheConsoleMenuShut_data()
    {
        QTest::addColumn<QString>("setCallback");

        QTest::newRow("click callback") << qsl("setLabelClickCallback");
        QTest::newRow("release callback") << qsl("setLabelReleaseCallback");
    }

    // A label with a click or release callback keeps right-clicks from the console,
    // so one stacked under the clicked label stops the menu, though its callback does not run
    void test_aLabelWithCallbacksUnderTheOneRightClickedKeepsTheConsoleMenuShut()
    {
        QFETCH(QString, setCallback);
        const QString lower = qsl("rightClickAnsweringLowerLabel");
        const QString upper = qsl("rightClickPlainUpperLabel");
        auto [lowerCreated, lowerMessage] = mpHost->createLabel(qsl("main"), lower, 40, 90, 220, 70, true, false);
        QVERIFY2(lowerCreated, qPrintable(lowerMessage));
        const auto removeLower = qScopeGuard([this, lower]() {
            mpHost->mainConsoleView()->deleteLabel(lower);
        });
        auto [upperCreated, upperMessage] = mpHost->createLabel(qsl("main"), upper, 50, 100, 200, 50, true, false);
        QVERIFY2(upperCreated, qPrintable(upperMessage));
        const auto removeUpper = qScopeGuard([this, upper]() {
            mpHost->mainConsoleView()->deleteLabel(upper);
        });
        TLabel* pLower = mpHost->mainConsoleView()->labelWidget(lower);
        TLabel* pUpper = mpHost->mainConsoleView()->labelWidget(upper);
        QVERIFY(pLower && pUpper);
        pLower->show();
        pUpper->show();
        pUpper->raise();
        const auto [menuParentWithoutCallback, consoleMenuWithoutCallback] = rightClickLabel(pUpper);
        QVERIFY2(menuParentWithoutCallback && consoleMenuWithoutCallback, "with no callbacks on either label the console's menu did not open, so this case tests nothing");

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("%1('%2', function() end)").arg(setCallback, lower)));
        const auto [menuParent, consoleMenu] = rightClickLabel(pUpper);
        Q_UNUSED(consoleMenu)

        QVERIFY2(!menuParent, "the console's menu opened through a label with a click or release callback");
    }

    // The player cannot see a link the label hides, so a right-click there must
    // not reveal it or offer its commands, only the console's own menu
    void test_aRightClickOnALabelOverALinkOpensTheConsoleMenuNotTheLinks()
    {
        const QString name = qsl("rightClickOverLinkLabel");
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(
                qsl("for i = 1, 80 do echoPopup(string.rep('L', 300), {[[qaHiddenLinkRan = true]], [[qaHiddenLinkRan = true]]}, {'one', 'two'}) echo('\\n') end")));
        const auto clearOutput = qScopeGuard([this]() {
            mpHost->mainConsoleView()->buffer.clear();
        });
        QTest::qWait(100ms);
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), name, 50, 100, 200, 50, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        const auto removeLabel = qScopeGuard([this, name]() {
            mpHost->mainConsoleView()->deleteLabel(name);
        });
        TLabel* pLabel = mpHost->mainConsoleView()->labelWidget(name);
        QVERIFY(pLabel);
        pLabel->show();

        const auto [menuParent, consoleMenu] = rightClickLabel(pLabel);

        QVERIFY2(menuParent == mpHost->mainConsoleView()->mUpperPane, "a right-click on a label over a link opened no menu from the console");
        QVERIFY2(consoleMenu, "a right-click on a label offered the commands of the link it hides");
    }
};

#include "LabelAnchorInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAnchorInteractionTest)
