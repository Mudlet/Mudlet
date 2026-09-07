/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * dlgRoomExits is the mapper's exits editor. Twelve normal directions each
 * carry a destination roomID, a stub flag, a "No route" lock, a weight and a
 * door type, and which of those controls are usable depends on the other ones -
 * a direction with a real destination cannot also be a stub, a direction with
 * neither cannot have a weight or a door. On top of that sits a table of
 * special exits with the same settings per row, and a Save that writes only
 * what the form still holds when it is pressed.
 *
 * All of that is widget state inside a QDialog. Lua can set the same room data
 * through setExit()/setExitStub()/addSpecialExit(), but nothing in the API opens
 * this dialog or can read which of its controls are enabled, so a busted spec
 * cannot reach any of it.
 *
 * Run with: ctest -R RoomExitsDialogTest -V
 */

#include <QAbstractItemDelegate>
#include <QCheckBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgRoomExits.h"
#include "mudlet.h"

#include "GroupedTest.h"

// ExitsTreeWidget keeps its column numbers in an enumeration only dlgRoomExits
// and its delegate are friends of, so mirror them here; the columnCount() check
// in openDialogOn() is the tripwire for them drifting apart.
namespace {
constexpr int colIndex_exitRoomId = 0;
constexpr int colIndex_exitStatus = 1;
constexpr int colIndex_lockExit = 2;
constexpr int colIndex_exitWeight = 3;
constexpr int colIndex_doorNone = 4;
constexpr int colIndex_doorOpen = 5;
constexpr int colIndex_doorClosed = 6;
constexpr int colIndex_doorLocked = 7;
constexpr int colIndex_command = 8;
constexpr int scmSpecialExitColumnCount = 9;
} // namespace

class RoomExitsDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    QPointer<dlgRoomExits> mpDialog;
    QPointer<QAbstractItemDelegate> mpRoomIdDelegate;
    QPointer<QAbstractItemDelegate> mpWeightDelegate;
    const QString mProfileName = qsl("RoomExitsDialog-Test");
    const QString mOtherAreaName = qsl("Far side");
    int mAreaId = 0;
    int mOtherAreaId = 0;
    static constexpr int scmSubjectRoom = 21;
    static constexpr int scmNearRoom = 22;
    static constexpr int scmFarAreaRoom = 23;
    static constexpr int scmLockedRoom = 24;
    static constexpr int scmSpareRoom = 25;
    static constexpr int scmNoSuchRoom = 9999;
    const QString mSpecialExitCommand = qsl("enter portal");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TRoom* room(const int id) const { return roomDB()->getRoom(id); }
    TRoom* subject() const { return room(scmSubjectRoom); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // The subject room gets one of each kind of thing the dialog has to render:
    // a plain in-area exit that carries a weight, a lock and a door; a stub; a
    // special exit; and directions left empty for the "nothing here yet" state.
    void buildMap()
    {
        map()->mapClear();
        mAreaId = roomDB()->addArea(qsl("Near side"));
        mOtherAreaId = roomDB()->addArea(mOtherAreaName);
        QVERIFY(mAreaId > 0);
        QVERIFY(mOtherAreaId > 0);

        int offset = 0;
        for (const int roomId : {scmSubjectRoom, scmNearRoom, scmFarAreaRoom, scmLockedRoom, scmSpareRoom}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, roomId == scmFarAreaRoom ? mOtherAreaId : mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, offset++, 0, 0));
        }
        room(scmNearRoom)->name = qsl("Next door");
        room(scmLockedRoom)->isLocked = true;

        TRoom* pRoom = subject();
        QVERIFY(pRoom->setExit(scmNearRoom, DIR_NORTH));
        pRoom->setExitWeight(qsl("n"), 7);
        pRoom->setDoor(qsl("n"), 2);
        pRoom->setExitLock(DIR_NORTH, true);
        pRoom->setExitStub(DIR_EAST, true);
        pRoom->setSpecialExit(scmNearRoom, mSpecialExitCommand);
        pRoom->setSpecialExitLock(mSpecialExitCommand, true);
        pRoom->setExitWeight(mSpecialExitCommand, 4);
        pRoom->setDoor(mSpecialExitCommand, 1);
    }

    // Mirrors of ExitsTreeWidget's private column enumeration are only safe while
    // the table still has the columns they name. A QCOMPARE only stands in a
    // function that returns nothing, which is why this is one rather than a line
    // in openDialogOn() below - and a Q_ASSERT there, which a build without
    // debug drops, would let the mirrors drift unnoticed.
    void verifySpecialExitColumnCount(dlgRoomExits* pDlg) { QCOMPARE(pDlg->specialExits->columnCount(), scmSpecialExitColumnCount); }

    dlgRoomExits* openDialogOn(const int roomId)
    {
        mpDialog = new dlgRoomExits(mpHost, roomId);
        verifySpecialExitColumnCount(mpDialog);
        // The dialog installs two item delegates it does not own (see the
        // specialExitDelegatesOutliveTheDialog case), so deleting the dialog is
        // not enough to get rid of them
        mpRoomIdDelegate = mpDialog->specialExits->itemDelegateForColumn(colIndex_exitRoomId);
        mpWeightDelegate = mpDialog->specialExits->itemDelegateForColumn(colIndex_exitWeight);
        return mpDialog;
    }

    QTreeWidgetItem* onlySpecialExitOf(dlgRoomExits* pDlg) const
    {
        if (pDlg->specialExits->topLevelItemCount() != 1) {
            return nullptr;
        }
        return pDlg->specialExits->topLevelItem(0);
    }

    // QLineEdit::clear() emits textChanged but not textEdited, and the dialog
    // only listens to the latter, so an emptied field has to be emptied the way
    // a player empties it
    static void clearByTyping(QLineEdit* pLineEdit)
    {
        while (!pLineEdit->text().isEmpty()) {
            QTest::keyClick(pLineEdit, Qt::Key_Backspace);
        }
    }

    // A change made straight to a tree item reaches the "is anything different?"
    // check when the item is clicked; "End editing" is the other control wired
    // to the same private slot and can be pressed from here.
    static void reappraiseSpecialExits(dlgRoomExits* pDlg, QTreeWidgetItem* pItem)
    {
        pDlg->slot_editSpecialExit(pItem, colIndex_lockExit);
        pDlg->button_endEditing->click();
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    // Runs after every case, a case QTest cut short at a failed QVERIFY
    // included, so nothing a failure walked away from is left for the leak
    // check to find
    void cleanup()
    {
        delete mpDialog.data();
        delete mpRoomIdDelegate.data();
        delete mpWeightDelegate.data();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void aStoredExitFillsItsWholeRowOfControls()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QCOMPARE(pDlg->roomID->text(), QString::number(scmSubjectRoom));
        QCOMPARE(pDlg->n->text(), QString::number(scmNearRoom));
        QVERIFY(pDlg->n->isEnabled());
        QCOMPARE(pDlg->weight_n->value(), 7);
        QVERIFY(pDlg->weight_n->isEnabled());
        QVERIFY2(pDlg->doortype_closed_n->isChecked(), "a stored door value of 2 is the closed door");
        QVERIFY(pDlg->noroute_n->isChecked());
        QVERIFY(pDlg->noroute_n->isEnabled());
        QVERIFY2(!pDlg->stub_n->isEnabled(), "a direction with a destination cannot also be a stub");
        QVERIFY(!pDlg->stub_n->isChecked());
        QCOMPARE(pDlg->getActionOnExit(pDlg->n), pDlg->mpAction_inAreaExit);
        QVERIFY2(!pDlg->button_save->isEnabled(), "Save was live before anything had been changed");
    }

    void aDirectionWithNothingInItOffersOnlyTheStub()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QVERIFY(pDlg->s->text().isEmpty());
        QVERIFY(pDlg->s->isEnabled());
        QVERIFY(pDlg->stub_s->isEnabled());
        QVERIFY(!pDlg->stub_s->isChecked());
        QVERIFY2(!pDlg->noroute_s->isEnabled(), "an exit that does not exist cannot be locked");
        QVERIFY2(!pDlg->weight_s->isEnabled(), "an exit that does not exist cannot carry a weight");
        QVERIFY(!pDlg->doortype_none_s->isEnabled());
        QVERIFY(!pDlg->doortype_locked_s->isEnabled());
        QVERIFY(pDlg->doortype_none_s->isChecked());
        QCOMPARE(pDlg->getActionOnExit(pDlg->s), pDlg->mpAction_noExit);
    }

    void aStoredStubTakesOverItsDirection()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QVERIFY(pDlg->stub_e->isChecked());
        QVERIFY2(!pDlg->e->isEnabled(), "a roomID cannot be typed over a stub exit");
        QVERIFY2(!pDlg->weight_e->isEnabled(), "a stub exit cannot carry a weight");
        QVERIFY2(pDlg->noroute_e->isEnabled(), "the Lua API can lock a stub, so the dialog has to be able to as well");
        QVERIFY2(pDlg->doortype_locked_e->isEnabled(), "a stub exit can have a door on it");
    }

    // The status icon on the roomID field is the only thing that tells the
    // player where an exit lands before they go looking
    void theExitStatusIconDistinguishesTheDestination()
    {
        buildMap();
        QVERIFY(subject()->setExit(scmFarAreaRoom, DIR_WEST));
        QVERIFY(subject()->setExit(scmLockedRoom, DIR_SOUTH));
        QVERIFY(subject()->setExit(scmSpareRoom, DIR_UP));
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QCOMPARE(pDlg->getActionOnExit(pDlg->w), pDlg->mpAction_otherAreaExit);
        QVERIFY2(pDlg->w->toolTip().contains(mOtherAreaName), qPrintable(qsl("an out-of-area exit did not name its area: %1").arg(pDlg->w->toolTip())));
        QCOMPARE(pDlg->getActionOnExit(pDlg->s), pDlg->mpAction_exitRoomLocked);
        QCOMPARE(pDlg->getActionOnExit(pDlg->up), pDlg->mpAction_inAreaExit);
    }

    // A room the exit points at may have been deleted since; initExit() has to
    // notice rather than dereference the missing room
    void anExitToARoomThatIsGoneIsShownAsEmpty()
    {
        buildMap();
        QVERIFY(subject()->setExit(scmNoSuchRoom, DIR_DOWN));

        auto* pDlg = openDialogOn(scmSubjectRoom);

        QVERIFY2(pDlg->down->text().isEmpty(), "an exit to a deleted room was still shown as a destination");
        QVERIFY(pDlg->stub_down->isEnabled());
        QCOMPARE(pDlg->getActionOnExit(pDlg->down), pDlg->mpAction_noExit);
    }

    void typingAKnownRoomIdOpensUpTheRestOfTheRow()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY(!pDlg->weight_s->isEnabled());

        QTest::keyClicks(pDlg->s, QString::number(scmNearRoom));

        QCOMPARE(pDlg->s->text(), QString::number(scmNearRoom));
        QVERIFY(pDlg->weight_s->isEnabled());
        QVERIFY(pDlg->noroute_s->isEnabled());
        QVERIFY(pDlg->doortype_locked_s->isEnabled());
        QVERIFY2(!pDlg->stub_s->isEnabled(), "the stub control stayed usable next to a real destination");
        QCOMPARE(pDlg->getActionOnExit(pDlg->s), pDlg->mpAction_inAreaExit);
        QVERIFY2(pDlg->button_save->isEnabled(), "typing a destination did not count as a change");
    }

    void typingARoomIdThatDoesNotExistLeavesTheRowShut()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QTest::keyClicks(pDlg->s, QString::number(scmNoSuchRoom));

        QCOMPARE(pDlg->getActionOnExit(pDlg->s), pDlg->mpAction_invalidExit);
        QVERIFY2(!pDlg->weight_s->isEnabled(), "a weight could be set on an exit that goes nowhere");
        QVERIFY2(!pDlg->noroute_s->isEnabled(), "an exit that goes nowhere could be locked");
        QVERIFY2(pDlg->stub_s->isEnabled(), "the stub control has to stay available while the roomID is unusable");
    }

    void checkingAStubClearsAndLocksTheRoomIdField()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTest::keyClicks(pDlg->s, QString::number(scmNoSuchRoom));

        pDlg->stub_s->setChecked(true);

        QVERIFY2(pDlg->s->text().isEmpty(), "the unusable roomID text survived the stub being set");
        QVERIFY(!pDlg->s->isEnabled());
        QVERIFY(pDlg->noroute_s->isEnabled());
        QVERIFY(pDlg->doortype_none_s->isEnabled());
        QVERIFY(pDlg->doortype_open_s->isEnabled());
        QVERIFY(pDlg->doortype_closed_s->isEnabled());
        QVERIFY(pDlg->doortype_locked_s->isEnabled());
        QVERIFY2(!pDlg->weight_s->isEnabled(), "a stub exit cannot carry a weight");
    }

    void clearingAStubShutsTheDirectionBackDown()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        pDlg->stub_s->setChecked(true);
        pDlg->noroute_s->setChecked(true);

        pDlg->stub_s->setChecked(false);

        QVERIFY(pDlg->s->isEnabled());
        QVERIFY2(!pDlg->noroute_s->isChecked(), "the lock survived the stub it belonged to");
        QVERIFY(!pDlg->noroute_s->isEnabled());
        QVERIFY(!pDlg->doortype_locked_s->isEnabled());
        QVERIFY(pDlg->doortype_none_s->isChecked());
    }

    /*
     * Known defect, kept as an expected failure so that fixing it is noticed:
     * slot_stub_nw_stateChanged() hands normalStubExitChanged() the north row's
     * doortype_locked_n where the northwest row's doortype_locked_nw was meant,
     * so ticking the northwest stub leaves its own "locked door" choice greyed
     * out and unticking it greys out north's instead. Correcting that one
     * argument turns both QVERIFYs below green.
     */
    void theNorthwestStubReachesIntoTheNorthRow()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY2(pDlg->doortype_locked_n->isEnabled(), "north has a real exit, so its locked-door choice should start available");
        QVERIFY(!pDlg->doortype_locked_nw->isEnabled());

        pDlg->stub_nw->setChecked(true);

        QEXPECT_FAIL("", "issue #10421: the northwest stub enables north's locked-door choice instead of its own", Continue);
        QVERIFY(pDlg->doortype_locked_nw->isEnabled());

        pDlg->stub_nw->setChecked(false);

        QEXPECT_FAIL("", "issue #10421: clearing the northwest stub disables north's locked-door choice", Continue);
        QVERIFY(pDlg->doortype_locked_n->isEnabled());
    }

    void savingWritesTheNormalExitBackToTheRoom()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        clearByTyping(pDlg->n);
        QTest::keyClicks(pDlg->n, QString::number(scmSpareRoom));
        pDlg->weight_n->setValue(13);
        pDlg->doortype_locked_n->setChecked(true);
        pDlg->noroute_n->setChecked(false);
        QVERIFY(pDlg->button_save->isEnabled());
        pDlg->button_save->click();

        QCOMPARE(subject()->getExit(DIR_NORTH), scmSpareRoom);
        QCOMPARE(subject()->getExitWeight(qsl("n")), 13);
        QCOMPARE(subject()->getDoor(qsl("n")), 3);
        QVERIFY2(!subject()->hasExitLock(DIR_NORTH), "clearing No route did not reach the room");
    }

    void savingANewlyCheckedStubStoresIt()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY(!subject()->hasExitStub(DIR_SOUTHWEST));

        pDlg->stub_sw->setChecked(true);
        QVERIFY(pDlg->button_save->isEnabled());
        pDlg->button_save->click();

        QVERIFY2(subject()->hasExitStub(DIR_SOUTHWEST), "a stub ticked in the dialog was not stored on the room");
    }

    // Emptying the roomID is how an exit is deleted from this dialog
    void savingAnEmptiedDirectionDeletesTheExit()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        clearByTyping(pDlg->n);
        QVERIFY(pDlg->button_save->isEnabled());
        pDlg->button_save->click();

        QVERIFY2(subject()->getExit(DIR_NORTH) <= 0, "clearing the roomID left the exit in place");
        // getExitWeight() falls back to the room's own weight, so the presence
        // of a per-exit one is the only thing worth asking about
        QVERIFY2(!subject()->hasExitWeight(qsl("n")), "the weight of the deleted exit outlived it");
    }

    /*
     * Known defect, kept as an expected failure so that fixing it is noticed:
     * normalExitEdited() greys the "no route" box out when the roomID is
     * emptied but, unlike the equivalent branch of normalStubExitChanged(),
     * leaves it ticked, and save() writes every one of those boxes through
     * unconditionally. The room is left claiming a speedwalk lock on a
     * direction it has no exit in, which Lua's hasExitLock() reports and which
     * silently applies to whatever exit is put there next by setExit().
     * Clearing the box alongside disabling it turns the QVERIFY below green.
     */
    void deletingAnExitLeavesItsSpeedwalkLockBehind()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY(pDlg->noroute_n->isChecked());

        clearByTyping(pDlg->n);
        QVERIFY(!pDlg->noroute_n->isEnabled());
        pDlg->button_save->click();

        QCOMPARE(subject()->getExit(DIR_NORTH), -1);
        QEXPECT_FAIL("", "issue #10422: the lock control is only greyed out, not cleared, so save() stores it on a direction with no exit", Continue);
        QVERIFY(!subject()->hasExitLock(DIR_NORTH));
    }

    void aStoredSpecialExitFillsItsRow()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);
        QCOMPARE(pItem->text(colIndex_command), mSpecialExitCommand);
        QCOMPARE(pItem->text(colIndex_exitRoomId), QString::number(scmNearRoom));
        QCOMPARE(pItem->checkState(colIndex_lockExit), Qt::Checked);
        QCOMPARE(pItem->data(colIndex_exitWeight, Qt::EditRole).toInt(), 4);
        QCOMPARE(pItem->checkState(colIndex_doorOpen), Qt::Checked);
        QCOMPARE(pItem->checkState(colIndex_doorNone), Qt::Unchecked);
        QVERIFY2(!pItem->icon(colIndex_exitStatus).isNull(), "a valid special exit was left without a status icon");
    }

    void addingASpecialExitStartsItAsAPlaceholderRow()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        pDlg->button_addSpecialExit->click();

        QCOMPARE(pDlg->specialExits->topLevelItemCount(), 2);
        QTreeWidgetItem* pItem = pDlg->specialExits->topLevelItem(1);
        QCOMPARE(pItem->text(colIndex_exitRoomId), pDlg->mSpecialExitRoomIdPlaceholder);
        QCOMPARE(pItem->text(colIndex_command), pDlg->mSpecialExitCommandPlaceholder);
        QCOMPARE(pItem->checkState(colIndex_doorNone), Qt::Checked);
        QCOMPARE(pItem->checkState(colIndex_lockExit), Qt::Unchecked);
        QVERIFY2(pItem->icon(colIndex_exitStatus).isNull(), "a row with no roomID in it was given a status icon");
    }

    // A new row is only half a special exit until it has both a command and a
    // destination, and neither the "is anything different?" check nor save()
    // may count one before then
    void savingLeavesAnUnfilledSpecialExitRowAlone()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);

        pDlg->button_addSpecialExit->click();
        QCOMPARE(pDlg->specialExits->topLevelItemCount(), 2);
        QTreeWidgetItem* pItem = pDlg->specialExits->topLevelItem(1);
        QVERIFY(pItem);
        reappraiseSpecialExits(pDlg, pItem);
        QVERIFY2(!pDlg->button_save->isEnabled(), "an untouched new row counted as a change worth saving");

        pItem->setText(colIndex_exitRoomId, QString::number(scmSpareRoom));
        reappraiseSpecialExits(pDlg, pItem);
        QVERIFY2(!pDlg->button_save->isEnabled(), "a new row with a destination but still no command counted as a change worth saving");

        pDlg->save();

        QCOMPARE(subject()->getSpecialExits().count(), 1);
        QVERIFY2(!subject()->getSpecialExits().contains(pDlg->mSpecialExitCommandPlaceholder), "the placeholder command was stored as a real special exit");
        QVERIFY(subject()->getSpecialExits().contains(mSpecialExitCommand));
    }

    void savingASpecialExitWithAnUnusableRoomIdDeletesIt()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);

        pItem->setText(colIndex_exitRoomId, QString::number(scmNoSuchRoom));
        reappraiseSpecialExits(pDlg, pItem);
        QVERIFY(pDlg->button_save->isEnabled());
        pDlg->button_save->click();

        QVERIFY2(!subject()->getSpecialExits().contains(mSpecialExitCommand), "a special exit pointing at a room that does not exist was kept");
        QCOMPARE(subject()->getDoor(mSpecialExitCommand), 0);
    }

    void savingWritesAnEditedSpecialExitBackToTheRoom()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);

        pItem->setText(colIndex_exitRoomId, QString::number(scmSpareRoom));
        pItem->setText(colIndex_exitWeight, qsl("9"));
        pItem->setCheckState(colIndex_lockExit, Qt::Unchecked);
        pDlg->slot_editSpecialExit(pItem, colIndex_doorClosed);
        pDlg->button_endEditing->click();
        QVERIFY(pDlg->button_save->isEnabled());
        pDlg->button_save->click();

        QCOMPARE(subject()->getSpecialExits().value(mSpecialExitCommand), scmSpareRoom);
        QCOMPARE(subject()->getExitWeight(mSpecialExitCommand), 9);
        QCOMPARE(subject()->getDoor(mSpecialExitCommand), 2);
        QVERIFY2(!subject()->hasSpecialExitLock(mSpecialExitCommand), "clearing the special exit's lock did not reach the room");
    }

    // The four door columns are checkboxes standing in for radio buttons, so
    // clicking one has to clear the other three itself
    void theSpecialExitDoorColumnsBehaveLikeRadioButtons()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);
        QCOMPARE(pItem->checkState(colIndex_doorOpen), Qt::Checked);

        pDlg->slot_editSpecialExit(pItem, colIndex_doorLocked);

        QCOMPARE(pItem->checkState(colIndex_doorLocked), Qt::Checked);
        QCOMPARE(pItem->checkState(colIndex_doorOpen), Qt::Unchecked);
        QCOMPARE(pItem->checkState(colIndex_doorClosed), Qt::Unchecked);
        QCOMPARE(pItem->checkState(colIndex_doorNone), Qt::Unchecked);
    }

    // Leaving the roomID cell empty has to put the greyed-out prompt back,
    // rather than leave a blank column the player cannot tell from a zero
    void endingAnEditRestoresTheRoomIdPrompt()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);

        pDlg->slot_editSpecialExit(pItem, colIndex_exitRoomId);
        QVERIFY2(!pDlg->button_addSpecialExit->isEnabled(), "a second exit could be added while one was being edited");
        QVERIFY(pDlg->button_endEditing->isEnabled());
        pItem->setText(colIndex_exitRoomId, QString());
        pDlg->slot_endEditSpecialExits();

        QCOMPARE(pItem->text(colIndex_exitRoomId), pDlg->mSpecialExitRoomIdPlaceholder);
        QVERIFY(pDlg->button_addSpecialExit->isEnabled());
        QVERIFY(!pDlg->button_endEditing->isEnabled());
    }

    void saveOnlyLightsUpOnceSomethingReallyDiffers()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY(!pDlg->button_save->isEnabled());
        QVERIFY(!pDlg->isWindowModified());

        pDlg->noroute_n->setChecked(false);
        QVERIFY2(pDlg->button_save->isEnabled(), "clearing a lock did not arm Save");
        QVERIFY(pDlg->isWindowModified());

        pDlg->noroute_n->setChecked(true);
        QVERIFY2(!pDlg->button_save->isEnabled(), "putting the lock back left Save armed with nothing to save");
        QVERIFY(!pDlg->isWindowModified());
    }

    void changingASpecialExitAlsoArmsSave()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QTreeWidgetItem* pItem = onlySpecialExitOf(pDlg);
        QVERIFY(pItem);

        pItem->setText(colIndex_exitRoomId, QString::number(scmSpareRoom));
        reappraiseSpecialExits(pDlg, pItem);

        QVERIFY2(pDlg->button_save->isEnabled(), "repointing a special exit did not arm Save");
    }

    // The dialog is opened from a room selection, and the room can be gone by
    // the time it is; Save must not write through the null it holds
    void aDialogForARoomThatDoesNotExistSavesNothing()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmNoSuchRoom);

        QCOMPARE(pDlg->specialExits->topLevelItemCount(), 0);
        map()->mMapGraphNeedsUpdate = false;
        pDlg->save();

        QVERIFY2(map()->mMapGraphNeedsUpdate, "save() on a dialog with no room did not even reach its first statement");
        QVERIFY2(subject()->getExit(DIR_NORTH) == scmNearRoom, "save() on a dialog with no room wrote over another room's exits");
    }

    /*
     * Known defect, kept as an expected failure so that fixing it is noticed:
     * dlgRoomExits' constructor hands specialExits two parentless delegates
     * through setItemDelegateForColumn(), which does not take ownership, so
     * both outlive the dialog with nothing left pointing at them. Every opening
     * of the exits editor leaks a RoomIdLineEditDelegate and a
     * WeightSpinBoxDelegate; parenting them to the dialog is the fix, and it
     * turns both QVERIFYs below green.
     */
    void specialExitDelegatesOutliveTheDialog()
    {
        buildMap();
        auto* pDlg = openDialogOn(scmSubjectRoom);
        QVERIFY(mpRoomIdDelegate);
        QVERIFY(mpWeightDelegate);

        delete pDlg;

        QEXPECT_FAIL("", "issue #10423: the roomID delegate is parentless, so deleting the dialog does not destroy it", Continue);
        QVERIFY(mpRoomIdDelegate.isNull());
        QEXPECT_FAIL("", "issue #10423: the weight delegate is parentless, so deleting the dialog does not destroy it", Continue);
        QVERIFY(mpWeightDelegate.isNull());
    }
};

#include "RoomExitsDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomExitsDialogTest)
