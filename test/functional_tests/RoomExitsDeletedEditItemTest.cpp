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
 * The room exits dialog remembers which special exit is being edited, and the
 * Delete key removes whichever special exits are selected - the one being
 * edited included. Ending the edit, saving, and clicking another exit all go
 * through that item, so the dialog has to forget it before the tree deletes it.
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "dlgRoomExits.h"
#include "exitstreewidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

class RoomExitsDeletedEditItemTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("RoomExitsDeletedEditItem-Test");

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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);

        TMap* map = mpHost->mpMap.data();
        const int areaId = map->mpRoomDB->addArea(qsl("Exits Area"));
        QVERIFY(areaId > 0);
        QVERIFY(map->addRoom(1) && map->setRoomArea(1, areaId));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        }
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_deletingTheSpecialExitBeingEditedForgetsIt()
    {
        dlgRoomExits dialog(mpHost, 1);
        dialog.show();
        dialog.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        dialog.slot_addSpecialExit();
        dialog.slot_addSpecialExit();
        dialog.slot_addSpecialExit();
        QCOMPARE(dialog.specialExits->topLevelItemCount(), 3);

        QTreeWidgetItem* edited = dialog.specialExits->topLevelItem(1);
        dialog.slot_editSpecialExit(edited, ExitsTreeWidget::colIndex_command);
        QCOMPARE(dialog.mpEditItem, edited);

        // Delete acts on the selection, not on the item being edited, and takes
        // the selected exits one at a time, so the edited one moves up a row
        // before its turn
        dialog.specialExits->topLevelItem(0)->setSelected(true);
        edited->setSelected(true);
        dialog.specialExits->setFocus();
        QTRY_VERIFY2(dialog.specialExits->hasFocus(), "the exits tree did not take the focus, so the Delete key would be ignored");
        QTest::keyClick(dialog.specialExits, Qt::Key_Delete);
        QCOMPARE(dialog.specialExits->topLevelItemCount(), 1);

        QVERIFY2(!dialog.mpEditItem, "the dialog still points at the special exit that was just deleted");
        QCOMPARE(dialog.mEditColumn, -1);
        QVERIFY2(dialog.button_addSpecialExit->isEnabled(), "the dialog is still in editing mode with nothing being edited");
        QVERIFY(!dialog.button_endEditing->isEnabled());

        // The paths that go through mpEditItem must still work, and editing must
        // carry on with the remaining exit
        dialog.slot_endEditSpecialExits();
        dialog.slot_editSpecialExit(dialog.specialExits->topLevelItem(0), ExitsTreeWidget::colIndex_command);
        dialog.slot_endEditSpecialExits();
        QVERIFY(!dialog.mpEditItem);
    }
};

#include "RoomExitsDeletedEditItemTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomExitsDeletedEditItemTest)
