/***************************************************************************
 *   Copyright (C) 2026 by Jesús Pavón Abián - galorasd@gmail.com          *
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
 * Functional tests for the keyboard traversal of the preferences' shortcuts
 * page.
 *
 * The editors on that page are built only once a profile is loaded, so the .ui
 * file cannot list them among its tab stops and Qt appends them to the end of
 * the dialog's focus chain. That put the page's 'reset to defaults' button -
 * which the .ui file does chain, right after the security page's proxy fields -
 * ahead of the editors it resets, so tabbing through the page reached the
 * button first and only then jumped to the editors. Sighted users can ignore an
 * odd focus order because the page tells them where they are; a screen reader
 * user only has that order.
 *
 * Nothing about it is visible on screen, which is exactly why it needs a test:
 * the layout keeps looking right however the focus chain ends up.
 *
 * Run with: ctest -R ShortcutsTabOrderTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include <QLineEdit>
#include <QScopeGuard>
#include <QTabBar>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TKeySequenceEdit.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForShortcutsTabOrderTest();

class ShortcutsTabOrderTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ShortcutsTabOrder-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // Opens the preferences on the shortcuts page. The dialog is owned by the
    // host, so each test deletes what it gets back.
    dlgProfilePreferences* openPreferencesOnShortcutsTab()
    {
        mudlet::self()->showOptionsDialog(qsl("tab_shortcuts"), mpHost);
        QTest::qWait(100ms);
        return mpHost->mpDlgProfilePreferences.data();
    }

    // The editors in the order they were built, which is the order they were
    // put into the grid and so the order they are read in. findChildren()
    // reports the children of a parent in the order they were added to it, and
    // addWidget() reparents them all to the same frame.
    QList<TKeySequenceEdit*> editorsInVisualOrder(dlgProfilePreferences* preferences) { return preferences->findChildren<TKeySequenceEdit*>(); }

    // Presses Tab (or Shift+Tab) on whatever holds the focus and returns where
    // it went. The editors capture keystrokes, so this also covers them letting
    // the traversal keys through rather than recording them as a binding.
    QWidget* tabFrom(QWidget* focused, bool forwards = true)
    {
        if (!focused) {
            return nullptr;
        }
        if (forwards) {
            QTest::keyClick(focused, Qt::Key_Tab);
        } else {
            QTest::keyClick(focused, Qt::Key_Backtab, Qt::ShiftModifier);
        }
        QCoreApplication::processEvents();
        return QApplication::focusWidget();
    }

    // Names the focused widget for failure messages: the editors focus an inner
    // line edit that carries the action's name for screen readers, which says
    // far more than the class name alone.
    QString describe(QWidget* widget)
    {
        if (!widget) {
            return qsl("nothing");
        }
        const QString name = widget->accessibleName().isEmpty() ? widget->objectName() : widget->accessibleName();
        return qsl("%1(%2)").arg(QString::fromUtf8(widget->metaObject()->className()), name);
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForShortcutsTabOrderTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mProfileName);
        delete mudlet::self();
    }

    // Tab moves from each editor to the one next to it, and the button that
    // resets them all comes after the last of them - the order they are laid
    // out in, and the order they are announced in.
    void test_tabbingFollowsTheOrderTheEditorsAreLaidOutIn()
    {
        auto* preferences = openPreferencesOnShortcutsTab();
        QVERIFY2(preferences, "Preferences dialog was not created");
        auto cleanUp = qScopeGuard([preferences]() {
            delete preferences;
        });

        const auto editors = editorsInVisualOrder(preferences);
        QVERIFY2(editors.size() > 1, "The shortcuts page has fewer than two editors - this test needs an order to check");

        editors.first()->setFocus();
        QCoreApplication::processEvents();
        QVERIFY2(editors.first()->hasFocus(), "The first shortcut editor would not take the focus");

        for (int i = 1; i < editors.size(); ++i) {
            QWidget* reached = tabFrom(QApplication::focusWidget());
            QVERIFY2(editors.at(i)->hasFocus(),
                     qPrintable(qsl("Tabbing out of shortcut editor %1 reached %2 instead of editor %3").arg(QString::number(i - 1), describe(reached), QString::number(i))));
        }

        QWidget* afterTheEditors = tabFrom(QApplication::focusWidget());
        QVERIFY2(preferences->toolButton_resetMainWindowShortcuts->hasFocus(),
                 qPrintable(qsl("Tabbing out of the last shortcut editor reached %1 rather than the reset button below them").arg(describe(afterTheEditors))));
    }

    // The same chain walked backwards. Shift+Tab arrives as Backtab with the
    // Shift modifier still set, which the editors used to record as a binding
    // instead of moving the focus (#8873).
    void test_shiftTabbingWalksBackUpTheSameChain()
    {
        auto* preferences = openPreferencesOnShortcutsTab();
        QVERIFY2(preferences, "Preferences dialog was not created");
        auto cleanUp = qScopeGuard([preferences]() {
            delete preferences;
        });

        const auto editors = editorsInVisualOrder(preferences);
        QVERIFY2(editors.size() > 1, "The shortcuts page has fewer than two editors - this test needs an order to check");

        preferences->toolButton_resetMainWindowShortcuts->setFocus();
        QCoreApplication::processEvents();
        QVERIFY2(preferences->toolButton_resetMainWindowShortcuts->hasFocus(), "The reset button would not take the focus");

        for (int i = editors.size() - 1; i >= 0; --i) {
            QWidget* reached = tabFrom(QApplication::focusWidget(), false);
            QVERIFY2(editors.at(i)->hasFocus(), qPrintable(qsl("Shift+Tab reached %1 instead of shortcut editor %2").arg(describe(reached), QString::number(i))));
        }
    }

    // The regression itself: whatever else the page's focus chain picks up on
    // the way, arriving from the tab bar has to reach the editors before the
    // button that resets them.
    void test_theResetButtonIsNotReachedBeforeTheEditors()
    {
        auto* preferences = openPreferencesOnShortcutsTab();
        QVERIFY2(preferences, "Preferences dialog was not created");
        auto cleanUp = qScopeGuard([preferences]() {
            delete preferences;
        });

        const auto editors = editorsInVisualOrder(preferences);
        QVERIFY2(!editors.isEmpty(), "The shortcuts page has no editors at all");

        auto* tabBar = preferences->tabWidget->findChild<QTabBar*>();
        QVERIFY2(tabBar, "The preferences' tab bar was not found");
        tabBar->setFocus();
        QCoreApplication::processEvents();
        QVERIFY2(tabBar->hasFocus(), "The tab bar would not take the focus");

        // A cap rather than a wait for the chain to wrap: a broken chain that
        // never reaches either widget has to end the walk by itself
        const int maximumSteps = editors.size() + 20;
        int stepsToFirstEditor = -1;
        int stepsToResetButton = -1;
        QStringList visited;
        for (int step = 1; step <= maximumSteps; ++step) {
            QWidget* reached = tabFrom(QApplication::focusWidget());
            visited.append(describe(reached));
            if (stepsToFirstEditor < 0 && editors.first()->hasFocus()) {
                stepsToFirstEditor = step;
            }
            if (stepsToResetButton < 0 && preferences->toolButton_resetMainWindowShortcuts->hasFocus()) {
                stepsToResetButton = step;
            }
            if (stepsToFirstEditor > 0 && stepsToResetButton > 0) {
                break;
            }
        }

        QVERIFY2(stepsToFirstEditor > 0, qPrintable(qsl("Tabbing from the tab bar never reached a shortcut editor; it visited %1").arg(visited.join(qsl(", ")))));
        QVERIFY2(stepsToResetButton > 0, qPrintable(qsl("Tabbing from the tab bar never reached the reset button; it visited %1").arg(visited.join(qsl(", ")))));
        QVERIFY2(stepsToFirstEditor < stepsToResetButton,
                 qPrintable(qsl("The reset button is reached after %1 tabs, before the first editor at %2 - the page is announced back to front")
                                    .arg(QString::number(stepsToResetButton), QString::number(stepsToFirstEditor))));
    }
};

void initializeQRCResourcesForShortcutsTabOrderTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "ShortcutsTabOrderTest.moc"
QTEST_MAIN(ShortcutsTabOrderTest)
