/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * Deleting a group takes its children down with it, and each child's teardown
 * still reaches its parent through a T*: its unit's unregister call (for a
 * button, ActionUnit::unregisterAction() reads getParent()->mPackageName) and
 * Tree::popChild(). So the children must be gone while the parent is still a
 * whole T, not left to ~Tree(), by which point the parent's T part has been
 * destroyed (#11059). Neither ASan nor valgrind sees that, because the storage
 * has not been freed yet, so each case plants a child that records its
 * parent's dynamic type at the start of its own teardown.
 *
 * Run with: ctest -R TreeChildTeardownTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <typeinfo>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "AliasUnit.h"
#include "EditorDeleteItemCommand.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

// A child that records, as its destruction begins, whether its parent is still a whole ParentType.
// The parent is kept as a Tree<ParentType>* taken while it was alive, so reading its dynamic type
// later is well defined even when only its Tree part is left.
// clang-format off
#define TEARDOWN_PROBE(ProbeType, ParentType) \
    class ProbeType : public ParentType \
    { \
    public: \
        ProbeType(ParentType* parent, Host* host, bool* parentWhole) \
        : ParentType(parent, host), mpParentTree(parent), mpParentWhole(parentWhole) {} \
        ~ProbeType() override { *mpParentWhole = typeid(*mpParentTree) == typeid(ParentType); } \
    private: \
        Tree<ParentType>* mpParentTree; \
        bool* mpParentWhole; \
    };
// clang-format on

TEARDOWN_PROBE(ActionProbe, TAction)
TEARDOWN_PROBE(TriggerProbe, TTrigger)
TEARDOWN_PROBE(AliasProbe, TAlias)
TEARDOWN_PROBE(TimerProbe, TTimer)
TEARDOWN_PROBE(KeyProbe, TKey)
TEARDOWN_PROBE(ScriptProbe, TScript)

class TreeChildTeardownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "TreeChildTeardown-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    static constexpr const char* scmParentGone = "the child was destroyed after its parent had stopped being a whole item";

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a concurrent copy of this test
        // does not find the profile name already in use.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The editor's redo of a deletion deletes only the group, with mpHost nulled on
    // the group alone, so the child unregisters itself from inside the group's teardown
    void test_redoingAButtonGroupDeletion()
    {
        ActionUnit* unit = mpHost->getActionUnit();
        const auto rootCount = unit->getActionRootNodeList().size();
        auto* group = new TAction(qsl("tct-action-group"), mpHost);
        group->setIsFolder(true);
        group->setIsActive(true);
        QVERIFY(unit->registerAction(group));
        bool parentWhole = false;
        auto* child = new ActionProbe(group, mpHost, &parentWhole);
        child->setName(qsl("tct-action-child"));
        child->setIsActive(true);
        QVERIFY(unit->registerAction(child));
        unit->updateAllToolbars();
        const int groupID = group->getID();
        const int childID = child->getID();

        QList<EditorDeleteItemCommand::DeletedItemInfo> items;
        items.append({groupID, -1, 0, QString(), group->getName()});
        items.append({childID, groupID, 0, QString(), child->getName()});
        EditorDeleteItemCommand command(EditorViewType::cmActionView, items, mpHost);
        command.redo();

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getAction(groupID));
        QVERIFY(!unit->getAction(childID));
        QCOMPARE(unit->getActionRootNodeList().size(), rootCount);
    }

    void test_deletingAButtonGroup()
    {
        ActionUnit* unit = mpHost->getActionUnit();
        auto* group = new TAction(qsl("tct-action-group"), mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerAction(group));
        bool parentWhole = false;
        auto* child = new ActionProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerAction(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getAction(childID));
    }

    void test_deletingATriggerGroup()
    {
        TriggerUnit* unit = mpHost->getTriggerUnit();
        auto* group = new TTrigger(qsl("tct-trigger-group"), QStringList(), QList<int>(), false, mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerTrigger(group));
        bool parentWhole = false;
        auto* child = new TriggerProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerTrigger(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getTrigger(childID));
    }

    void test_deletingAnAliasGroup()
    {
        AliasUnit* unit = mpHost->getAliasUnit();
        auto* group = new TAlias(qsl("tct-alias-group"), mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerAlias(group));
        bool parentWhole = false;
        auto* child = new AliasProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerAlias(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getAlias(childID));
    }

    void test_deletingATimerGroup()
    {
        TimerUnit* unit = mpHost->getTimerUnit();
        auto* group = new TTimer(qsl("tct-timer-group"), QTime(0, 0, 1), mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerTimer(group));
        bool parentWhole = false;
        auto* child = new TimerProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerTimer(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getTimer(childID));
    }

    void test_deletingAKeyGroup()
    {
        KeyUnit* unit = mpHost->getKeyUnit();
        auto* group = new TKey(qsl("tct-key-group"), mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerKey(group));
        bool parentWhole = false;
        auto* child = new KeyProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerKey(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getKey(childID));
    }

    void test_deletingAScriptGroup()
    {
        ScriptUnit* unit = mpHost->getScriptUnit();
        auto* group = new TScript(qsl("tct-script-group"), mpHost);
        group->setIsFolder(true);
        QVERIFY(unit->registerScript(group));
        bool parentWhole = false;
        auto* child = new ScriptProbe(group, mpHost, &parentWhole);
        QVERIFY(unit->registerScript(child));
        const int childID = child->getID();

        delete group;

        QVERIFY2(parentWhole, scmParentGone);
        QVERIFY(!unit->getScript(childID));
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletApp::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "TreeChildTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(TreeChildTeardownTest)
