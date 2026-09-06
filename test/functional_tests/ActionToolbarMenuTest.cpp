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

#include <QMenu>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAction.h"
#include "TEasyButtonBar.h"
#include "TFlipButton.h"
#include "TToolBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

// A button group nested inside another button group is the only thing that
// reaches TEasyButtonBar::fillMenu() and TToolBar::addActionToMenu(), and no
// fixture in the tree built one, so the drop-down menus those two make were
// unexercised. The two bars disagree about what a menu holds, and both quirks
// are pinned here rather than tidied: the button bar takes each entry's icon
// from the parent group and leaves inactive children out, while the floating
// toolbar adds the group itself as an intermediate entry and keeps inactive
// children.
class ActionToolbarMenuTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ActionToolbarMenu";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    inline static const QString mIconPath = qsl(":/icons/mudlet_main_32px.png");

    TAction* makeAction(Host* host, TAction* parent, const QString& name, const bool folder)
    {
        auto* action = parent ? new TAction(parent, host) : new TAction(name, host);
        action->setName(name);
        action->setIsFolder(folder);
        action->setIsActive(true);
        host->getActionUnit()->registerAction(action);
        return action;
    }

    // The shape every case here needs: a master folder standing in for an
    // installed package, a toolbar group under it, a button group under that
    // (which is what makes a menu at all), and in the group a plain entry
    // beside a further group holding one more. The location decides which bar
    // builds it - 0 is the button bar docked in the profile window, 4 the
    // floating toolbar.
    TAction* buildNestedGroup(Host* host, const int location)
    {
        auto* master = makeAction(host, nullptr, qsl("menuTestPackage"), true);
        master->mPackageName = qsl("menuTestPackage");
        master->mModuleMasterFolder = true;
        // regenerateToolBars() reads the location off the root before it ever
        // looks at the toolbar under it, so both have to carry it
        master->mLocation = location;

        auto* toolbar = makeAction(host, master, qsl("menuTestToolbar"), true);
        toolbar->mLocation = location;
        toolbar->mToolbarLastDockArea = Qt::LeftDockWidgetArea;

        auto* group = makeAction(host, toolbar, qsl("menuTestGroup"), true);
        group->setIcon(mIconPath);

        makeAction(host, group, qsl("menuTestEntry"), false);

        auto* subGroup = makeAction(host, group, qsl("menuTestSubGroup"), true);
        makeAction(host, subGroup, qsl("menuTestSubEntry"), false);

        host->mInstalledPackages << qsl("menuTestPackage");
        return group;
    }

    // Searched from the main window rather than the console, because a
    // location-4 toolbar is a dock on the window while a location-0 bar sits in
    // the console. TFlipButton has no Q_OBJECT, so findChildren<> it as its
    // QPushButton base and downcast.
    static TFlipButton* findButtonWidget(const TAction* action)
    {
        for (auto* pushButton : mudlet::self()->findChildren<QPushButton*>()) {
            auto* pB = dynamic_cast<TFlipButton*>(pushButton);
            if (pB && pB->mpTAction == action) {
                return pB;
            }
        }
        return nullptr;
    }

    // findItems() answers with ids, not the actions themselves.
    static TAction* actionNamed(Host* host, const QString& name)
    {
        const auto ids = host->getActionUnit()->findItems(name);
        return ids.size() == 1 ? host->getActionUnit()->getAction(ids.front()) : nullptr;
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // ActionSelfRemovalTest).
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto* host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static QStringList entryNames(const QMenu* menu)
    {
        QStringList names;
        for (const auto* action : menu->actions()) {
            names << action->text();
        }
        return names;
    }

    static QAction* entryNamed(const QMenu* menu, const QString& name)
    {
        for (auto* action : menu->actions()) {
            if (action->text() == name) {
                return action;
            }
        }
        return nullptr;
    }

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_aButtonBarGroupGetsAMenuOfItsChildren()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* group = buildNestedGroup(host, 0);
        host->getActionUnit()->updateAllToolbars();

        auto* button = findButtonWidget(group);
        QVERIFY2(button, "the nested group should have a button on the bar, so this test has something to read");
        auto* menu = button->menu();
        QVERIFY2(menu, "a group inside a toolbar should carry a drop-down menu");

        QCOMPARE(entryNames(menu), QStringList({qsl("menuTestEntry"), qsl("menuTestSubGroup")}));

        auto* subEntry = entryNamed(menu, qsl("menuTestSubGroup"));
        QVERIFY(subEntry);
        auto* subMenu = subEntry->menu();
        QVERIFY2(subMenu, "a group inside a menu should carry a sub-menu of its own");
        QCOMPARE(entryNames(subMenu), QStringList({qsl("menuTestSubEntry")}));
    }

    // The entry is named after the child but wears the parent group's icon.
    // Long-standing behaviour rather than a decision worth defending, but a
    // package that leans on it should not change appearance without notice.
    void test_aButtonBarMenuEntryTakesItsIconFromTheGroup()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* group = buildNestedGroup(host, 0);
        QVERIFY2(!QIcon(mIconPath).isNull(), "the icon resource this case reads did not load, so it could not tell the two apart");
        auto* entryAction = actionNamed(host, qsl("menuTestEntry"));
        QVERIFY(entryAction);
        QVERIFY2(entryAction->getIcon().isEmpty(), "the entry must have no icon of its own, or this case cannot tell whose icon it got");

        host->getActionUnit()->updateAllToolbars();

        auto* menu = findButtonWidget(group)->menu();
        QVERIFY(menu);
        auto* entry = entryNamed(menu, qsl("menuTestEntry"));
        QVERIFY(entry);
        QVERIFY2(!entry->icon().isNull(), "the entry has no icon of its own, so an icon here can only have come from its group");
    }

    void test_anInactiveChildIsLeftOutOfTheButtonBarMenu()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* group = buildNestedGroup(host, 0);
        auto* entryAction = actionNamed(host, qsl("menuTestEntry"));
        QVERIFY(entryAction);
        entryAction->setIsActive(false);

        host->getActionUnit()->updateAllToolbars();

        auto* menu = findButtonWidget(group)->menu();
        QVERIFY(menu);
        QCOMPARE(entryNames(menu), QStringList({qsl("menuTestSubGroup")}));
    }

    // The floating toolbar builds a different menu from the same tree: the
    // group is added as an entry of its own and its children hang off that,
    // where the button bar puts the children straight in.
    void test_aFloatingToolbarGroupGetsAnIntermediateEntry()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* group = buildNestedGroup(host, 4);
        host->getActionUnit()->updateAllToolbars();

        auto* button = findButtonWidget(group);
        QVERIFY2(button, "the nested group should have a button on the floating toolbar");
        auto* menu = button->menu();
        QVERIFY2(menu, "a group on a floating toolbar should carry a drop-down menu");

        QCOMPARE(entryNames(menu), QStringList({qsl("menuTestGroup")}));

        auto* intermediate = entryNamed(menu, qsl("menuTestGroup"));
        QVERIFY(intermediate);
        auto* inner = intermediate->menu();
        QVERIFY2(inner, "the intermediate entry is what the group's children hang off");
        QCOMPARE(entryNames(inner), QStringList({qsl("menuTestEntry"), qsl("menuTestSubGroup")}));

        auto* subEntry = entryNamed(inner, qsl("menuTestSubGroup"));
        QVERIFY(subEntry);
        QVERIFY2(subEntry->menu(), "the nesting should keep going for a group inside a group");
        QCOMPARE(entryNames(subEntry->menu()), QStringList({qsl("menuTestSubEntry")}));
    }

    // The two bars disagree here, and the disagreement is the point: an entry
    // hidden from the docked bar still shows on the floating one.
    void test_anInactiveChildStaysInTheFloatingToolbarMenu()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        auto* group = buildNestedGroup(host, 4);
        auto* entryAction = actionNamed(host, qsl("menuTestEntry"));
        QVERIFY(entryAction);
        entryAction->setIsActive(false);

        host->getActionUnit()->updateAllToolbars();

        auto* menu = findButtonWidget(group)->menu();
        QVERIFY(menu);
        auto* inner = entryNamed(menu, qsl("menuTestGroup"))->menu();
        QVERIFY(inner);
        QCOMPARE(entryNames(inner), QStringList({qsl("menuTestEntry"), qsl("menuTestSubGroup")}));
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }
};

#include "ActionToolbarMenuTest.moc"
MUDLET_GROUPED_TEST_MAIN(ActionToolbarMenuTest)
