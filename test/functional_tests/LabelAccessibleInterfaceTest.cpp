/***************************************************************************
 *   Copyright (C) 2026 by Michael Conley - sousesider@gmail.com           *
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
 * What a screen reader is told about a script's label: its role, its name and
 * its press action. The role and press action are not reachable from Lua, so
 * they are asked of the label's accessible interface directly.
 *
 * Run with: ctest -R LabelAccessibleInterfaceTest -V
 */

#include <QAccessible>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAccessibleLabel.h"
#include "TCommandLine.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TTextEdit.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LabelAccessibleInterfaceTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("LabelAccessibleInterface-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLabelName = qsl("accessibleLabel");
    // a configuration directory of its own, so the profile this opens is never
    // one of the developer's
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;

    TLabel* label() const { return mpHost->mainConsoleView()->labelWidget(mLabelName); }

    QAccessibleInterface* accessible() const { return QAccessible::queryAccessibleInterface(label()); }

    bool runLua(const QString& script) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }

    bool luaHolds(const QString& condition) const { return runLua(qsl("assert(%1)").arg(condition)); }

    inline static QList<QAccessible::Event> smEvents;
    inline static QObject* smWatched = nullptr;

    static void recordEvent(QAccessibleEvent* event)
    {
        if (event->object() == smWatched) {
            smEvents.append(event->type());
        }
    }

    // Qt only hands events to the handler while accessibility is active, which no screen reader makes it here
    void listenTo(QObject* object)
    {
        smEvents.clear();
        smWatched = object;
        QAccessible::installUpdateHandler(recordEvent);
        QAccessible::setActive(true);
    }

    void stopListening()
    {
        QAccessible::setActive(false);
        QAccessible::installUpdateHandler(nullptr);
        smWatched = nullptr;
    }

private slots:
    void initTestCase()
    {
        QAccessible::installFactory(TAccessibleLabel::labelFactory);

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
        // A ceiling, not a delay: it returns as soon as the profile loads
        if (!spy.wait(10s)) {
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

    void init()
    {
        QVERIFY(runLua(
                qsl("setConfig('accessibleLabels', false); setLabelClickCallback('%1', nil); setLabelReleaseCallback('%1', nil); setLabelAccessibleName('%1', ''); setLabelAccessibleRole('%1', ''); "
                    "disableClickthrough('%1'); resetBackgroundImage('%1'); resetLabelToolTip('%1'); "
                    "echo('%1', 'plain'); accessibleLabelClicks = nil; accessibleLabelReleased = nil; accessibleLinkHit = nil")
                        .arg(mLabelName)));
    }

    void test_aLabelIsStaticTextUntilItHasAClickCallback()
    {
        QAccessibleInterface* pInterface = accessible();
        QVERIFY2(pInterface, "the label has no accessible interface - is TAccessibleLabel's factory installed?");
        QCOMPARE(pInterface->role(), QAccessible::StaticText);
        QVERIFY2(!pInterface->actionInterface() || !pInterface->actionInterface()->actionNames().contains(QAccessibleActionInterface::pressAction()), "a label nobody can click offers a press action");

        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(mLabelName)));
        QCOMPARE(pInterface->role(), QAccessible::Button);
        QVERIFY(pInterface->actionInterface());
        QVERIFY(pInterface->actionInterface()->actionNames().contains(QAccessibleActionInterface::pressAction()));

        QVERIFY(runLua(qsl("setLabelClickCallback('%1', nil)").arg(mLabelName)));
        QCOMPARE(pInterface->role(), QAccessible::StaticText);
    }

    // A mouse user can work a label with only a release callback, so a screen reader user can too
    void test_aReleaseCallbackAloneMakesAButton()
    {
        QVERIFY(runLua(qsl("setLabelReleaseCallback('%1', function() accessibleLabelReleased = true end)").arg(mLabelName)));
        QCOMPARE(accessible()->role(), QAccessible::Button);
        accessible()->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        QVERIFY(luaHolds(qsl("accessibleLabelReleased")));
    }

    // A drag handle has a click callback but is no button, so its script says so
    void test_aScriptSetRoleOverridesTheCallbacks()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() accessibleLabelClicks = 1 end)").arg(mLabelName)));
        QVERIFY(runLua(qsl("assert(setLabelAccessibleRole('%1', 'text'))").arg(mLabelName)));
        QCOMPARE(accessible()->role(), QAccessible::StaticText);
        accessible()->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        QVERIFY(luaHolds(qsl("accessibleLabelClicks == nil")));
    }

    void test_aLabelWithNoTextIsNamedByItsToolTip()
    {
        QVERIFY(runLua(qsl("echo('%1', ''); setLabelToolTip('%1', '<b>Open</b> the map')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Open the map"));
        QVERIFY2(accessible()->text(QAccessible::Description).isEmpty(), "the tooltip is read twice, as the name and as the description");

        // Qt shows this one as it is written, tags and all
        QVERIFY(runLua(qsl("setLabelToolTip('%1', 'Add one for <name> & more')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Add one for <name> & more"));

        QVERIFY(runLua(qsl("echo('%1', 'Map')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Description), qsl("Add one for <name> & more"));
        QVERIFY(runLua(qsl("resetLabelToolTip('%1')").arg(mLabelName)));
    }

    // Adjustable.Container titles its bar "&nbsp;&nbsp;Title"; the markup and entities are read as what they show
    void test_theNameIsTheTextAsItIsShown()
    {
        QVERIFY(runLua(qsl("echo('%1', '&nbsp;&nbsp;<b>Map</b> &amp; <i>Exits</i> &lt;3&gt;')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Map & Exits <3>"));
        QVERIFY(luaHolds(qsl("getLabelAccessibleName('%1') == 'Map & Exits <3>'").arg(mLabelName)));
    }

    // An image or icon shows no text, so its author has to name it
    void test_anAuthoredNameIsUsedUntilItIsCleared()
    {
        QVERIFY(runLua(qsl("echo('%1', '<img src=\"nowhere.png\">')").arg(mLabelName)));
        QVERIFY(runLua(qsl("assert(setLabelAccessibleName('%1', 'Open the map'))").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Open the map"));
        QVERIFY(luaHolds(qsl("getLabelAccessibleName('%1') == 'Open the map'").arg(mLabelName)));

        QVERIFY(runLua(qsl("echo('%1', 'Map')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Open the map"));

        QVERIFY(runLua(qsl("setLabelAccessibleName('%1', '')").arg(mLabelName)));
        QCOMPARE(accessible()->text(QAccessible::Name), qsl("Map"));
    }

    // A screen reader's press is the left click a mouse would make, so a Geyser handler reading event.button still works
    void test_aPressRunsTheClickCallbackAsALeftClick()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function(event) accessibleLabelClicks = (accessibleLabelClicks or 0) + 1; accessibleLabelButton = event.button end)").arg(mLabelName)));
        accessible()->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        QVERIFY(luaHolds(qsl("accessibleLabelClicks == 1")));
        QVERIFY(luaHolds(qsl("accessibleLabelButton == 'LeftButton'")));
    }

    // Off by default: a button joining the Tab order changes how existing packages behave
    void test_aButtonTakesTabFocusOnlyWhileAccessibleLabelsIsOn()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);

        QVERIFY(runLua(qsl("setConfig('accessibleLabels', true)")));
        QCOMPARE(label()->focusPolicy(), Qt::TabFocus);

        QVERIFY(runLua(qsl("setLabelAccessibleRole('%1', 'text')").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);
        QVERIFY(runLua(qsl("setLabelAccessibleRole('%1', '')").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::TabFocus);

        QVERIFY(runLua(qsl("setLabelClickCallback('%1', nil)").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);

        QVERIFY(runLua(qsl("setLabelReleaseCallback('%1', function() end)").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::TabFocus);
        QVERIFY(runLua(qsl("setConfig('accessibleLabels', false)")));
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);
    }

    void test_aButtonCreatedWhileAccessibleLabelsIsOnTakesTabFocus()
    {
        QVERIFY(runLua(qsl("setConfig('accessibleLabels', true)")));
        const QString later = qsl("accessibleLabelLater");
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), later, 10, 110, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(later)));
        QCOMPARE(mpHost->mainConsoleView()->labelWidget(later)->focusPolicy(), Qt::TabFocus);
        mpHost->deleteLabel(later);
    }

    // QLabel gives a label with a link focus of its own, for reaching the link
    void test_aLabelWithALinkKeepsItsFocus()
    {
        QVERIFY(runLua(qsl("echo('%1', '<a href=\"send:look\">look</a>')").arg(mLabelName)));
        QCOMPARE(label()->focusPolicy(), Qt::StrongFocus);
        QVERIFY(runLua(qsl("setConfig('accessibleLabels', true); setConfig('accessibleLabels', false)")));
        QCOMPARE(label()->focusPolicy(), Qt::StrongFocus);
    }

    void test_spaceAndEnterPressAFocusedButton()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function(event) accessibleLabelClicks = (accessibleLabelClicks or 0) + 1; accessibleLabelButton = event.button end)").arg(mLabelName)));
        QTest::keyClick(label(), Qt::Key_Space);
        QVERIFY2(luaHolds(qsl("accessibleLabelClicks == nil")), "Space pressed a button with accessibleLabels off");

        QVERIFY(runLua(qsl("setConfig('accessibleLabels', true)")));
        QTest::keyClick(label(), Qt::Key_Space);
        QTest::keyClick(label(), Qt::Key_Return);
        QTest::keyClick(label(), Qt::Key_Enter, Qt::KeypadModifier);
        QVERIFY(luaHolds(qsl("accessibleLabelClicks == 3")));
        QVERIFY(luaHolds(qsl("accessibleLabelButton == 'LeftButton'")));

        QTest::keyClick(label(), Qt::Key_Space, Qt::ControlModifier);
        QVERIFY2(luaHolds(qsl("accessibleLabelClicks == 3")), "Ctrl+Space pressed the button");
    }

private:
    // Through the keys a player has, sent to the window as a real key press is, so the command line taking
    // Tab for completion and caret mode holding the keyboard both stay in the way
    void key(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) { QTest::keyClick(mudlet::self()->windowHandle(), key, modifiers); }

    TTextEdit* pane() const { return mpHost->mainConsoleView()->mUpperPane; }

    qsizetype lineHolding(const QString& text) const { return mpHost->mainConsoleView()->buffer.lineBuffer.indexOf(text); }

    const QString mLowerName = qsl("aaaAccessibleLower");

    TLabel* lower() const { return mpHost->mainConsoleView()->labelWidget(mLowerName); }

    // Two links in the game text above where caret mode starts, the label as a button, and a second button lower
    // down whose name sorts first, so only the order on screen puts the label first
    bool setUpLinksAndButtons()
    {
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), mLowerName, 10, 110, 200, 40, true, false);
        if (!created) {
            qWarning() << createMessage;
            return false;
        }
        if (!runLua(qsl("clearWindow(); echo('top\\n'); echoLink('LINKONE', '', 'the first link'); echo('\\n'); "
                        "echoLink('LINKTWO', '', 'the second link'); echo('\\nbottom\\n'); "
                        "setConfig('accessibleLabels', true); setLabelClickCallback('%1', function() end); setLabelClickCallback('%2', function() end)")
                            .arg(mLabelName, mLowerName))) {
            return false;
        }
        mudlet::self()->show();
        mudlet::self()->activateWindow();
        if (!QTest::qWaitFor(
                    []() {
                        return QApplication::activeWindow() == mudlet::self();
                    },
                    5000)) {
            qWarning() << "the main window never became active";
            return false;
        }
        // Handing the focus to the command line retries on timers up to 50ms long, which would take it back
        QTest::qWait(80ms);
        mpHost->setCaretEnabled(true);
        return QTest::qWaitFor(
                [this]() {
                    return QApplication::focusWidget() == pane();
                },
                1000);
    }

    void tearDownLinksAndButtons()
    {
        if (mpHost->caretEnabled()) {
            mpHost->setCaretEnabled(false);
        }
        mpHost->mainConsoleView()->forgetCaretOrigin();
        mpHost->mCaretShortcut = Host::CaretShortcut::None;
        mpHost->deleteLabel(mLowerName);
        runLua(qsl("clearWindow()"));
    }

private slots:
    // Caret mode starts below the links, so Tab goes on to the buttons, then round to the first link, the second,
    // and the buttons again; Shift+Tab goes the other way
    void test_tabCyclesThroughTheLinksAndTheButtons()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        const QString lower = mLowerName;
        QWidget* pLower = mpHost->mainConsoleView()->labelWidget(lower);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), pLower);
        key(Qt::Key_Tab);
        QTRY_VERIFY2(mpHost->caretEnabled(), "leaving the last button did not go back to the main window");
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKONE")));
        key(Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKTWO")));
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));

        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKTWO")));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKONE")));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), pLower);
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
    }

    void test_withNoButtonsTheLinksWrapAsBefore()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        const QString lower = mLowerName;
        QVERIFY(runLua(qsl("setConfig('accessibleLabels', false)")));
        QTRY_COMPARE(QApplication::focusWidget(), pane());

        key(Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKONE")));
        key(Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKTWO")));
        key(Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKONE")));
        QCOMPARE(QApplication::focusWidget(), pane());
    }

    // With Tab as the caret shortcut, Tab leaves caret mode, so Ctrl+] and Ctrl+[ are the way to the buttons
    void test_ctrlBracketsReachTheButtonsWhenTabTogglesCaretMode()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        const QString lower = mLowerName;
        mpHost->mCaretShortcut = Host::CaretShortcut::Tab;
        QTRY_COMPARE(QApplication::focusWidget(), pane());

        key(Qt::Key_BracketRight, Qt::ControlModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        mpHost->setCaretEnabled(true);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        // backwards from the bottom, the links come first
        key(Qt::Key_BracketLeft, Qt::ControlModifier);
        QCOMPARE(pane()->mCaretLine, lineHolding(qsl("LINKTWO")));
        key(Qt::Key_BracketLeft, Qt::ControlModifier);
        key(Qt::Key_BracketLeft, Qt::ControlModifier);
        QTRY_COMPARE(QApplication::focusWidget(), mpHost->mainConsoleView()->labelWidget(lower));

        mpHost->setCaretEnabled(true);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        QTRY_VERIFY2(!mpHost->caretEnabled(), "Tab as the caret shortcut no longer leaves caret mode");
    }

    void test_typingOnAButtonCarriesOnInTheCommandLine()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        const QString lower = mLowerName;
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        pCommandLine->clear();
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));

        QTest::keyClick(mudlet::self()->windowHandle(), 'n');
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pCommandLine));
        QCOMPARE(pCommandLine->toPlainText(), qsl("n"));
        pCommandLine->clear();
    }

    // A close button deletes its own label; the focus has to go somewhere the player can still use
    void test_aFocusedButtonThatGoesOrStopsBeingOneHandsTheFocusOn()
    {
        const QString lower = mLowerName;
        const QString third = qsl("accessibleLabelThird");
        auto restore = qScopeGuard([this, third]() {
            mpHost->deleteLabel(third);
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), third, 10, 160, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(third)));
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), mpHost->mainConsoleView()->labelWidget(lower));

        QVERIFY(runLua(qsl("setLabelClickCallback('%1', nil)").arg(lower)));
        QTRY_COMPARE(QApplication::focusWidget(), mpHost->mainConsoleView()->labelWidget(third));

        QVERIFY(runLua(qsl("deleteLabel('%1')").arg(third)));
        QTRY_VERIFY2(mpHost->caretEnabled(), "deleting the last button did not hand the focus back to the main window");
        QTRY_COMPARE(QApplication::focusWidget(), pane());
    }

    // A screen reader keeps hold of what it was last told has the focus
    void test_theHandOffDoesNotAnnounceTheCommandLine()
    {
        auto restore = qScopeGuard([this]() {
            stopListening();
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        listenTo(pCommandLine->focusProxy() ? pCommandLine->focusProxy() : pCommandLine);
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QVERIFY2(!smEvents.contains(QAccessible::Focus), "the command line was announced as focused as the button took the focus");
    }

    // Arriving on a button, Return presses it; Tab then steps through its own links, which Return and Space open,
    // before the next button
    void test_aButtonsOwnLinksComeFirst()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QVERIFY(runLua(qsl("echo('%1', '<a href=\"accessibleLinkNorth = true\">north</a> <a href=\"accessibleLinkSouth = true\">south</a>'); "
                           "setLabelClickCallback('%1', function() accessibleButtonPressed = true end)")
                               .arg(mLabelName)));

        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QVERIFY(!label()->hasSelectedText());
        key(Qt::Key_Return);
        QVERIFY2(luaHolds(qsl("accessibleButtonPressed")), "Return did not press a button that carries links");

        key(Qt::Key_Tab);
        QCOMPARE(label()->selectedText(), qsl("north"));
        key(Qt::Key_Return);
        QVERIFY2(luaHolds(qsl("accessibleLinkNorth")), "Return did not open the focused link");
        key(Qt::Key_Tab);
        QCOMPARE(label()->selectedText(), qsl("south"));
        key(Qt::Key_Space);
        QVERIFY2(luaHolds(qsl("accessibleLinkSouth")), "Space did not open the focused link");
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));

        // the window losing the focus and getting it back leaves QLabel's link focused
        QFocusEvent away(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
        QApplication::sendEvent(label(), &away);
        QFocusEvent back(QEvent::FocusIn, Qt::ActiveWindowFocusReason);
        QApplication::sendEvent(label(), &back);
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(lower()));

        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(label()->selectedText(), qsl("south"));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(label()->selectedText(), qsl("north"));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
    }

    // With nothing to press, the stop is its first link going forwards and its last going back
    void test_aLabelWithOnlyLinksIsATabStop()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', nil); echo('%1', '<a href=\"accessibleLinkNorth = true\">north</a> <a href=\"accessibleLinkSouth = true\">south</a>')").arg(mLabelName)));

        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QCOMPARE(label()->selectedText(), qsl("north"));
        key(Qt::Key_Tab);
        QCOMPARE(label()->selectedText(), qsl("south"));
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(lower()));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QCOMPARE(label()->selectedText(), qsl("south"));

        QVERIFY(runLua(qsl("echo('%1', '<a href=\"accessibleLinkOnly = true\">only</a>')").arg(mLabelName)));
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(lower()));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QCOMPARE(label()->selectedText(), qsl("only"));
        key(Qt::Key_Return);
        QVERIFY2(luaHolds(qsl("accessibleLinkOnly")), "Return did not open a label's only link");
    }

    // Most game text has no links, so caret mode's Tab goes straight on to the buttons and comes back to the main window
    void test_withNoLinksInTheTextTabGoesStraightToTheButtons()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QVERIFY(runLua(qsl("clearWindow(); echo('plain\\n')")));

        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(lower()));
        key(Qt::Key_Tab);
        QTRY_VERIFY(mpHost->caretEnabled());
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
    }

    // Qt asks a widget being hidden with the focus to pass it on, which a label with links must not take as a step
    void test_hidingAFocusedLabelHandsTheFocusOn()
    {
        auto restore = qScopeGuard([this]() {
            runLua(qsl("showWindow('%1')").arg(mLabelName));
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QVERIFY(runLua(qsl("echo('%1', '<a href=\"accessibleNoop = 1\">north</a> <a href=\"accessibleNoop = 2\">south</a> <a href=\"accessibleNoop = 3\">west</a>')").arg(mLabelName)));
        key(Qt::Key_Tab);
        key(Qt::Key_Tab);
        QCOMPARE(label()->selectedText(), qsl("north"));

        QVERIFY(runLua(qsl("hideWindow('%1')").arg(mLabelName)));
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(lower()));
    }

    // Labels at the same corner, as a Geyser gauge's are, read neither before nor after each other
    void test_buttonsAtTheSameCornerAreEachReached()
    {
        const QString twin = qsl("accessibleLabelTwin");
        auto restore = qScopeGuard([this, twin]() {
            mpHost->deleteLabel(twin);
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), twin, 10, 110, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(twin)));
        const QList<QWidget*> pair{lower(), mpHost->mainConsoleView()->labelWidget(twin)};

        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_Tab);
        QTRY_VERIFY(pair.contains(QApplication::focusWidget()));
        QWidget* pFirst = QApplication::focusWidget();
        key(Qt::Key_Tab);
        QTRY_VERIFY(pair.contains(QApplication::focusWidget()) && QApplication::focusWidget() != pFirst);
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), pFirst);
    }

    // Held, or with a modifier, Space is neither a press nor typing
    void test_spaceOnAButtonStaysThere()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        pCommandLine->clear();
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_Space, Qt::ShiftModifier);
        QTest::qWait(20ms);
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QVERIFY(pCommandLine->toPlainText().isEmpty());
    }

    void test_theCaretShortcutAndKeybindingsWorkFromAButton()
    {
        auto restore = qScopeGuard([this]() {
            runLua(qsl("killKey(accessibleF5Key)"));
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        QVERIFY(runLua(qsl("accessibleF5Key = tempKey(mudlet.key.F5, [[accessibleF5 = true]])")));
        mpHost->mCaretShortcut = Host::CaretShortcut::F6;

        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_F5);
        QTRY_VERIFY2(luaHolds(qsl("accessibleF5")), "a keybinding did not run from a focused button");

        mpHost->setCaretEnabled(true);
        QTRY_COMPARE(QApplication::focusWidget(), pane());
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        key(Qt::Key_F6);
        QTRY_VERIFY(mpHost->caretEnabled());
        QTRY_COMPARE(QApplication::focusWidget(), pane());
    }

    // Without caret mode to go back to, the way out of the buttons is the command line
    void test_leavingTheButtonsWithoutCaretModeGoesToTheCommandLine()
    {
        auto restore = qScopeGuard([this]() {
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        mpHost->setCaretEnabled(false);
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pCommandLine));
        lower()->setFocus(Qt::OtherFocusReason);
        key(Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pCommandLine));
        QVERIFY(!mpHost->caretEnabled());
    }

    // Destroyed with a scroll box, a focused button would leave the keyboard with nobody
    void test_aButtonDyingWithItsContainerHandsTheFocusOn()
    {
        const QString box = qsl("accessibleLabelBox");
        const QString boxed = qsl("accessibleLabelBoxed");
        auto restore = qScopeGuard([this, box]() {
            runLua(qsl("deleteScrollBox('%1')").arg(box));
            tearDownLinksAndButtons();
        });
        QVERIFY(setUpLinksAndButtons());
        mpHost->setCaretEnabled(false);
        QVERIFY(runLua(qsl("assert(createScrollBox('%1', 300, 10, 200, 100)); createLabel('%1', '%2', 0, 0, 100, 30, 1); "
                           "setLabelClickCallback('%2', function() end)")
                               .arg(box, boxed)));
        TLabel* pBoxed = mpHost->mainConsoleView()->labelWidget(boxed);
        QVERIFY(pBoxed);
        QTRY_VERIFY(pBoxed->isVisible());
        pBoxed->setFocus(Qt::TabFocusReason);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pBoxed));

        QVERIFY(runLua(qsl("deleteScrollBox('%1')").arg(box)));
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(mpHost->mainConsoleView()->mpCommandLine));
    }

    // A floating user window is a window of its own: neither Tab, the caret shortcut nor typing takes the
    // keyboard from it to the main window
    void test_aFloatingWindowsButtonsAreLeftOut()
    {
        const QString floating = qsl("accessibleLabelFloatingWindow");
        const QString floatingLabel = qsl("accessibleLabelFloating");
        QVERIFY(runLua(qsl("openUserWindow('%1', false, false, 'f'); createLabel('%1', '%2', 0, 0, 100, 30, 1); "
                           "setConfig('accessibleLabels', true); setLabelClickCallback('%2', function() end)")
                               .arg(floating, floatingLabel)));
        auto restore = qScopeGuard([this, floating, floatingLabel]() {
            mpHost->mCaretShortcut = Host::CaretShortcut::None;
            runLua(qsl("deleteLabel('%1'); hideWindow('%2')").arg(floatingLabel, floating));
        });
        TLabel* pFloating = mpHost->mainConsoleView()->labelWidget(floatingLabel);
        QVERIFY(pFloating);
        QTRY_VERIFY(pFloating->isVisible());
        QWidget* pWindow = pFloating->window();
        QVERIFY(pWindow != mpHost->mainConsoleView()->window());
        QVERIFY(!mpHost->mainConsoleView()->keyboardButtons().contains(pFloating));

        pWindow->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(pWindow));
        pFloating->setFocus(Qt::TabFocusReason);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pFloating));
        mpHost->mCaretShortcut = Host::CaretShortcut::F6;
        QTest::keyClick(pWindow->windowHandle(), Qt::Key_F6);
        QVERIFY2(!mpHost->caretEnabled(), "the caret shortcut on a floating window's button turned caret mode on in the main window");
        QTest::keyClick(pWindow->windowHandle(), 'n');
        QVERIFY2(mpHost->mainConsoleView()->mpCommandLine->toPlainText().isEmpty(), "typing on a floating window's button went to the main command line");
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pFloating));
    }

    // A mouse click passes through it to what is underneath, so a screen reader cannot press it either
    void test_aClickThroughLabelIsNotAButton()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() accessibleLabelClicks = 1 end); enableClickthrough('%1')").arg(mLabelName)));
        QCOMPARE(accessible()->role(), QAccessible::StaticText);
        QVERIFY(luaHolds(qsl("getLabelAccessibleRole('%1') == 'text'").arg(mLabelName)));
        QVERIFY(runLua(qsl("disableClickthrough('%1')").arg(mLabelName)));
        QCOMPARE(accessible()->role(), QAccessible::Button);
    }

    // As Qt's own interface for a QLabel says of one
    void test_anImageLabelIsAGraphic()
    {
        QImage image(16, 16, QImage::Format_RGB32);
        image.fill(Qt::red);
        const QString path = mConfigDir.filePath(qsl("accessible-label.png"));
        QVERIFY(image.save(path));
        QVERIFY(runLua(qsl("assert(setBackgroundImage('%1', [[%2]]))").arg(mLabelName, path)));
        QCOMPARE(accessible()->role(), QAccessible::Graphic);
    }

    // The SVG is a layer under the text, so a label with text over it shows text
    void test_anSvgLabelIsAGraphicOnlyWithoutText()
    {
        QFile svg(mConfigDir.filePath(qsl("accessible-label.svg")));
        QVERIFY(svg.open(QIODevice::WriteOnly));
        svg.write("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'><rect width='16' height='16' fill='red'/></svg>");
        svg.close();
        QVERIFY(runLua(qsl("echo('%1', 'Map'); assert(setBackgroundImage('%1', [[%2]]))").arg(mLabelName, svg.fileName())));
        QCOMPARE(accessible()->role(), QAccessible::StaticText);
        listenTo(label());
        auto restore = qScopeGuard([this]() {
            stopListening();
        });
        QVERIFY(runLua(qsl("echo('%1', '')").arg(mLabelName)));
        QCOMPARE(accessible()->role(), QAccessible::Graphic);
        QVERIFY2(smEvents.contains(QAccessible::ActionChanged), "emptying the text over an SVG was not announced as a role change");
    }

    void test_aPressLeavesALinkUnderTheLabelAlone()
    {
        QVERIFY(runLua(qsl("echo('%1', '<a href=\"accessibleLinkHit = true\">' .. string.rep('W', 60) .. '</a>')").arg(mLabelName)));
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() accessibleLabelClicks = 1 end)").arg(mLabelName)));
        accessible()->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        QVERIFY(luaHolds(qsl("accessibleLabelClicks == 1")));
        QVERIFY2(luaHolds(qsl("accessibleLinkHit == nil")), "the press activated the link under the label's centre");
    }

    // A screen reader keeps what it was told about a control until it hears it changed
    void test_changesAreAnnounced()
    {
        listenTo(label());
        auto restore = qScopeGuard([this]() {
            stopListening();
        });

        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(mLabelName)));
        QVERIFY2(smEvents.contains(QAccessible::ActionChanged), "gaining a click callback was not announced");

        smEvents.clear();
        QVERIFY(runLua(qsl("setLabelAccessibleName('%1', 'Open the map')").arg(mLabelName)));
        QVERIFY2(smEvents.contains(QAccessible::NameChanged), "an authored name was not announced");

        smEvents.clear();
        QVERIFY(runLua(qsl("setLabelAccessibleName('%1', ''); echo('%1', ''); setLabelToolTip('%1', 'Map')").arg(mLabelName)));
        QVERIFY2(smEvents.contains(QAccessible::NameChanged), "a tooltip naming the label was not announced");

        QVERIFY(runLua(qsl("resetLabelToolTip('%1')").arg(mLabelName)));

        smEvents.clear();
        QVERIFY(runLua(qsl("enableClickthrough('%1')").arg(mLabelName)));
        QVERIFY2(smEvents.contains(QAccessible::ActionChanged), "turning click-through on was not announced");
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
        QVERIFY2(smEvents.contains(QAccessible::RoleChanged), "a button becoming text was not announced as a role change");
#endif

        // a screen reader may say the label again on hearing of a change that changed nothing
        smEvents.clear();
        QVERIFY(runLua(qsl("enableClickthrough('%1'); setLabelAccessibleRole('%1', ''); setLabelAccessibleName('%1', '')").arg(mLabelName)));
        QVERIFY2(smEvents.isEmpty(), "settings that did not change were announced");

        smEvents.clear();
        QImage image(16, 16, QImage::Format_RGB32);
        image.fill(Qt::blue);
        const QString path = mConfigDir.filePath(qsl("accessible-label-event.png"));
        QVERIFY(image.save(path));
        QVERIFY(runLua(qsl("disableClickthrough('%1'); setLabelClickCallback('%1', nil); echo('%1', 'Map')").arg(mLabelName)));
        smEvents.clear();
        QVERIFY(runLua(qsl("assert(setBackgroundImage('%1', [[%2]]))").arg(mLabelName, path)));
        QVERIFY2(smEvents.contains(QAccessible::NameChanged), "the text an image replaced was not announced");
        QVERIFY2(smEvents.contains(QAccessible::ActionChanged), "a label becoming a graphic was not announced");

        QVERIFY(runLua(qsl("resetBackgroundImage('%1'); setLabelToolTip('%1', 'Map')").arg(mLabelName)));
        smEvents.clear();
        QVERIFY(runLua(qsl("resetBackgroundImage('%1'); setLabelToolTip('%1', 'Map')").arg(mLabelName)));
        QVERIFY2(smEvents.isEmpty(), "removing an image that was not there, or repeating a tooltip, was announced");

        QVERIFY(runLua(qsl("resetLabelToolTip('%1'); setLabelAccessibleName('%1', 'Open the map')").arg(mLabelName)));
        smEvents.clear();
        QVERIFY(runLua(qsl("echo('%1', 'North')").arg(mLabelName)));
        QVERIFY2(!smEvents.contains(QAccessible::NameChanged), "new text was announced as a new name for a label a script named");

        QVERIFY(runLua(qsl("setLabelAccessibleName('%1', '')").arg(mLabelName)));
        smEvents.clear();
        QVERIFY(runLua(qsl("echo('%1', 'South')").arg(mLabelName)));
        QCOMPARE(smEvents.count(QAccessible::NameChanged), 1);
    }

    void test_aLabelThatDeletesItselfWhenPressedIsLeftAlone()
    {
        const QString doomed = qsl("accessibleLabelDoomed");
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), doomed, 10, 60, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() deleteLabel('%1') end); setLabelReleaseCallback('%1', function() accessibleLabelReleased = true end)").arg(doomed)));
        QPointer<TLabel> pDoomed = mpHost->mainConsoleView()->labelWidget(doomed);
        QVERIFY(pDoomed);
        QAccessible::queryAccessibleInterface(pDoomed)->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        QVERIFY2(luaHolds(qsl("accessibleLabelReleased == nil")), "the release callback ran on a label the click callback deleted");
        QTRY_VERIFY2(pDoomed.isNull(), "the label the callback deleted is still there");
    }

    // Last: a package's host section resets the profile's other settings to their defaults
    void test_aPackageChangesAccessibleLabelsOnlyWhenItSaysSo()
    {
        QVERIFY(runLua(qsl("setLabelClickCallback('%1', function() end)").arg(mLabelName)));
        const auto writePackage = [this](const QString& name, const QString& hostAttributes) {
            QFile file(mConfigDir.filePath(name + qsl(".xml")));
            if (!file.open(QIODevice::WriteOnly)) {
                return QString();
            }
            file.write(qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n<MudletPackage version=\"1.001\">"
                           "<HostPackage><Host %1></Host></HostPackage></MudletPackage>\n")
                               .arg(hostAttributes)
                               .toUtf8());
            return file.fileName();
        };
        const QString turnsOn = writePackage(qsl("accessibleLabelsOn"), qsl("accessibleLabels=\"yes\""));
        const QString saysNothing = writePackage(qsl("accessibleLabelsUnsaid"), QString());
        const QString turnsOff = writePackage(qsl("accessibleLabelsOff"), qsl("accessibleLabels=\"no\""));
        QVERIFY(!turnsOn.isEmpty() && !saysNothing.isEmpty() && !turnsOff.isEmpty());
        auto restore = qScopeGuard([this]() {
            runLua(qsl("uninstallPackage('accessibleLabelsOn'); uninstallPackage('accessibleLabelsUnsaid'); uninstallPackage('accessibleLabelsOff')"));
        });

        QVERIFY(runLua(qsl("assert(installPackage([[%1]]))").arg(turnsOn)));
        QCOMPARE(label()->focusPolicy(), Qt::TabFocus);
        QVERIFY(runLua(qsl("assert(installPackage([[%1]]))").arg(saysNothing)));
        QVERIFY2(mpHost->mAccessibleLabels, "a package from before the setting turned it off");
        QCOMPARE(label()->focusPolicy(), Qt::TabFocus);
        QVERIFY(runLua(qsl("assert(installPackage([[%1]]))").arg(turnsOff)));
        QVERIFY(!mpHost->mAccessibleLabels);
        QCOMPARE(label()->focusPolicy(), Qt::NoFocus);
    }
};

#include "LabelAccessibleInterfaceTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAccessibleInterfaceTest)
