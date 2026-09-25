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
 * A menu item's key sequence reaches its slot in one of two ways: through the
 * menu item that carries it when the menu bar is shown, or through a QShortcut
 * wired straight to the slot when it is hidden. Only one of the two may exist
 * at a time - a QShortcut merely forgotten rather than destroyed goes on
 * competing with the menu item for the same sequence, which is how the menu
 * shortcuts stopped working (PR #5723, issues #649 and #3985).
 *
 * Run with: ctest -R MenuShortcutTeardownTest -V
 */

#include <QAction>
#include <QShortcut>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "enums.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MenuShortcutTeardownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // Direct children only: that is where the shortcuts under test are put, and
    // a fixture that opened a profile would otherwise also count the ones its
    // console brings
    static int ownShortcutCount() { return mudlet::self()->findChildren<QShortcut*>(QString(), Qt::FindDirectChildrenOnly).count(); }

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // PR #5723: showing the menu bar again used to forget the shortcuts the
    // hidden menu bar had needed instead of deleting them, leaving them alive
    // and still listening.
    void test_theHiddenMenuBarShortcutsAreGoneOnceTheMenuBarIsBack()
    {
        mudlet::self()->setMenuBarVisibility(enums::visibleAlways);
        const int withMenuBar = ownShortcutCount();

        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        QVERIFY2(ownShortcutCount() > withMenuBar, "hiding the menu bar wired no shortcuts of its own, so there is nothing here to tear down");

        mudlet::self()->setMenuBarVisibility(enums::visibleAlways);

        QCOMPARE(ownShortcutCount(), withMenuBar);
        QVERIFY2(!mudlet::self()->dactionScriptEditor->shortcut().isEmpty(), "the menu item was left with no key sequence to be reached by");
    }
};

#include "MenuShortcutTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(MenuShortcutTeardownTest)
