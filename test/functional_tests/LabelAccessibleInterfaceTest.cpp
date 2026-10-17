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
#include <QImage>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAccessibleLabel.h"
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

    // A mouse click passes through it to what is underneath, so a screen reader cannot press it either
    // Through the keys a player has: the command line keeps Tab for completion, so the way in is from the
    // main window in caret mode, where Tab carries on from the game text's links to the buttons
    void test_tabReachesTheButtonsFromTheMainWindowAndComesBack()
    {
        TTextEdit* pPane = mpHost->mainConsoleView()->mUpperPane;
        const QString second = qsl("accessibleLabelSecond");
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), second, 10, 110, 200, 40, true, false);
        QVERIFY2(created, qPrintable(createMessage));
        auto restore = qScopeGuard([this, second]() {
            if (mpHost->caretEnabled()) {
                mpHost->setCaretEnabled(false);
            }
            mpHost->deleteLabel(second);
        });
        QVERIFY(runLua(qsl("setConfig('accessibleLabels', true); setLabelClickCallback('%1', function() accessibleLabelClicks = (accessibleLabelClicks or 0) + 1 end); "
                           "setLabelClickCallback('%2', function() end)")
                               .arg(mLabelName, second)));
        mudlet::self()->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mudlet::self()));
        mpHost->setCaretEnabled(true);
        QTRY_COMPARE(QApplication::focusWidget(), pPane);

        // the buffer holds no links, so the first Tab goes straight to the buttons, in reading order
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(label()));
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Space);
        QVERIFY(luaHolds(qsl("accessibleLabelClicks == 1")));

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(mpHost->mainConsoleView()->labelWidget(second)));

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
        QTRY_VERIFY2(mpHost->caretEnabled(), "leaving the last button did not go back to the main window");
        QTRY_COMPARE(QApplication::focusWidget(), pPane);

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Backtab, Qt::ShiftModifier);
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(mpHost->mainConsoleView()->labelWidget(second)));
    }

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
};

#include "LabelAccessibleInterfaceTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelAccessibleInterfaceTest)
