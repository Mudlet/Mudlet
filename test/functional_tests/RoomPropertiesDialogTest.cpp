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
 * dlgRoomProperties is what the 2D mapper's "Properties..." menu entry opens on
 * a room selection. It has to fold a whole selection into one form - a single
 * shared value gets a plain entry field, a mixed one gets a dropdown of what is
 * in use or a partially-checked box - and then report on OK only the fields the
 * player actually decided about, so that the ones left showing "(Multiple
 * values...)" leave every room alone.
 *
 * None of that is reachable from Lua: the dialog is a QDialog with no scripting
 * entry point, and its decisions live in widget state and in the arguments of
 * signal_save_symbol rather than in the map.
 *
 * Run with: ctest -R RoomPropertiesDialogTest -V
 */

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgRoomProperties.h"
#include "mudlet.h"

#include "GroupedTest.h"

// What the dialog shows in place of a value the selection does not agree on.
// dlgRoomProperties keeps it as a private tr() string, and no translator is
// installed in a test run, so the source string is what appears.
static const QString scmMultipleValues{qsl("(Multiple values...)")};

// Everything signal_save_symbol carries, so a case can assert on the decision
// the dialog reached rather than on a room the mapper has yet to write to.
struct EmittedProperties
{
    int emitCount = 0;
    bool changeName = false;
    QString newName;
    bool changeRoomColor = false;
    int roomColorNumber = 0;
    bool changeSymbol = false;
    QString newSymbol;
    bool changeSymbolColor = false;
    QColor newSymbolColor;
    bool changeWeight = false;
    int newWeight = 0;
    bool changeLockStatus = false;
    std::optional<bool> newLockStatus;
    bool changeHiddenStatus = false;
    std::optional<bool> newHiddenStatus;
    bool changeBorderColor = false;
    QColor newBorderColor;
    bool changeBorderThickness = false;
    int newBorderThickness = 0;
};

class RoomPropertiesDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("RoomPropertiesDialog-Test");
    int mAreaId = 0;
    QPointer<dlgRoomProperties> mpDialog;
    EmittedProperties mEmitted;
    static constexpr int scmFirstRoom = 11;
    static constexpr int scmSecondRoom = 12;
    static constexpr int scmThirdRoom = 13;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TRoom* room(const int id) const { return roomDB()->getRoom(id); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildMap()
    {
        map()->mapClear();
        mAreaId = roomDB()->addArea(qsl("Properties area"));
        QVERIFY(mAreaId > 0);
        int offset = 0;
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, offset++, 0, 0));
        }
        mEmitted = EmittedProperties();
    }

    // The scan T2DMap::slot_setRoomProperties()'s caller does before it opens
    // the dialog: the dialog is handed counts, not rooms, so how a selection is
    // summarised is part of what it is being asked to display.
    dlgRoomProperties* openDialogOn(const QList<int>& roomIds)
    {
        QHash<QString, int> usedNames;
        QHash<int, int> usedColors;
        QHash<QString, int> usedSymbols;
        QHash<int, int> usedWeights;
        QHash<bool, int> usedLockStatus;
        int hiddenRoomCount = 0;
        QSet<TRoom*> rooms;

        for (const int roomId : roomIds) {
            TRoom* pRoom = room(roomId);
            if (!pRoom) {
                continue;
            }
            rooms.insert(pRoom);
            if (!pRoom->name.isEmpty()) {
                ++usedNames[pRoom->name];
            }
            ++usedColors[pRoom->environment];
            ++usedSymbols[pRoom->mSymbol];
            if (pRoom->getWeight() > 0) {
                ++usedWeights[pRoom->getWeight()];
            }
            ++usedLockStatus[pRoom->isLocked];
            if (pRoom->isHidden()) {
                ++hiddenRoomCount;
            }
        }

        auto* pDlg = new dlgRoomProperties(mpHost);
        connect(pDlg,
                &dlgRoomProperties::signal_save_symbol,
                this,
                [this](bool changeName,
                       QString newName,
                       bool changeRoomColor,
                       int roomColorNumber,
                       bool changeSymbol,
                       QString newSymbol,
                       bool changeSymbolColor,
                       QColor newSymbolColor,
                       bool changeWeight,
                       int newWeight,
                       bool changeLockStatus,
                       std::optional<bool> newLockStatus,
                       bool changeHiddenStatus,
                       std::optional<bool> newHiddenStatus,
                       bool changeBorderColor,
                       QColor newBorderColor,
                       bool changeBorderThickness,
                       int newBorderThickness,
                       QSet<TRoom*>) {
                    ++mEmitted.emitCount;
                    mEmitted.changeName = changeName;
                    mEmitted.newName = newName;
                    mEmitted.changeRoomColor = changeRoomColor;
                    mEmitted.roomColorNumber = roomColorNumber;
                    mEmitted.changeSymbol = changeSymbol;
                    mEmitted.newSymbol = newSymbol;
                    mEmitted.changeSymbolColor = changeSymbolColor;
                    mEmitted.newSymbolColor = newSymbolColor;
                    mEmitted.changeWeight = changeWeight;
                    mEmitted.newWeight = newWeight;
                    mEmitted.changeLockStatus = changeLockStatus;
                    mEmitted.newLockStatus = newLockStatus;
                    mEmitted.changeHiddenStatus = changeHiddenStatus;
                    mEmitted.newHiddenStatus = newHiddenStatus;
                    mEmitted.changeBorderColor = changeBorderColor;
                    mEmitted.newBorderColor = newBorderColor;
                    mEmitted.changeBorderThickness = changeBorderThickness;
                    mEmitted.newBorderThickness = newBorderThickness;
                });
        pDlg->init(usedNames, usedColors, usedSymbols, usedWeights, usedLockStatus, hiddenRoomCount, rooms);
        mpDialog = pDlg;
        return pDlg;
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
    // check to find. The dialog carries WA_DeleteOnClose, so accept()/reject()
    // may already have queued its deletion; run that before deciding there is
    // one left to do.
    void cleanup()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        delete mpDialog.data();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void oneRoomPrefillsItsOwnValues()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->name = qsl("The Armoury");
        pRoom->mSymbol = qsl("!");
        pRoom->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->lineEdit_name->text(), qsl("The Armoury"));
        QCOMPARE(pDlg->lineEdit_roomSymbol->text(), qsl("!"));
        QVERIFY2(pDlg->comboBox_roomSymbol->isHidden(), "the symbol dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->spinBox_weight->value(), 5);
        QVERIFY2(pDlg->comboBox_weight->isHidden(), "the weight dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_hidden->isTristate());
    }

    void aSelectionThatDisagreesShowsPlaceholdersAndTristates()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        QCOMPARE(pDlg->lineEdit_name->text(), scmMultipleValues);
        QVERIFY2(pDlg->lineEdit_roomSymbol->isHidden(), "the plain symbol field cannot show two different symbols");
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(0), scmMultipleValues);
        QVERIFY2(pDlg->spinBox_weight->isHidden(), "the plain weight field cannot show two different weights");
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(0), scmMultipleValues);
        QVERIFY(pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::PartiallyChecked);
        QVERIFY(pDlg->checkBox_hidden->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::PartiallyChecked);
    }

    // The whole point of the placeholders: a field left showing one has to come
    // back as "no decision", or OK would overwrite every room in the selection
    // with the placeholder text.
    void okOnAnUntouchedMixedSelectionChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY2(!mEmitted.changeName, "the name placeholder was taken for a new name");
        QVERIFY2(!mEmitted.changeSymbol, "the symbol placeholder was taken for a new symbol");
        QVERIFY2(!mEmitted.changeWeight, "the weight placeholder was taken for a new weight");
        QVERIFY2(!mEmitted.changeLockStatus, "a partially checked lock box was taken for a decision");
        QVERIFY2(!mEmitted.changeHiddenStatus, "a partially checked hidden box was taken for a decision");
        QVERIFY2(!mEmitted.changeRoomColor, "the room colour was reported as changed without the colour selector being used");
        QVERIFY2(!mEmitted.changeSymbolColor, "the symbol colour was reported as changed without the colour selector being used");
    }

    void okReportsTheEditedNameSymbolWeightAndCheckboxes()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmFirstRoom)->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Old Armoury"));
        pDlg->lineEdit_roomSymbol->setText(qsl("A"));
        pDlg->spinBox_weight->setValue(12);
        pDlg->checkBox_locked->setChecked(true);
        pDlg->checkBox_hidden->setChecked(true);
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY(mEmitted.changeName);
        QCOMPARE(mEmitted.newName, qsl("Old Armoury"));
        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("A"));
        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 12);
        QVERIFY(mEmitted.changeLockStatus);
        QCOMPARE(mEmitted.newLockStatus, std::optional<bool>(true));
        QVERIFY(mEmitted.changeHiddenStatus);
        QCOMPARE(mEmitted.newHiddenStatus, std::optional<bool>(true));
    }

    // Cancel has to emit nothing at all, whatever was typed into the form
    void cancelReportsNothing()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Never Applied"));
        pDlg->reject();

        QCOMPARE(mEmitted.emitCount, 0);
    }

    // The dropdown entries carry a usage count for the player's benefit; the
    // count has to be stripped back off before the value is handed on.
    void theWeightDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);
        room(scmThirdRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(1), qsl("7 (count: 2)"));
        QCOMPARE(pDlg->comboBox_weight->itemText(2), qsl("3 (count: 1)"));

        pDlg->comboBox_weight->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 7);
    }

    void theSymbolDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmThirdRoom)->mSymbol = qsl("?");

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(1), qsl("? (count: 2)"));
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(2), qsl("! (count: 1)"));

        pDlg->comboBox_roomSymbol->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("?"));
    }

    // The weight dropdown is editable, so a player can type anything into it
    void anUnreadableTypedWeightIsNotAppliedToTheSelection()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(pDlg->comboBox_weight->lineEdit());
        pDlg->comboBox_weight->setCurrentText(qsl("heavy"));
        pDlg->accept();

        QVERIFY2(!mEmitted.changeWeight, "text that names no number was still applied as a room weight");
    }

    // The border controls preview onto the rooms themselves as they are moved,
    // so that the map behind the dialog shows the result; Cancel has to put the
    // rooms back the way it found them.
    void theBorderPreviewIsLiveAndCancelUndoesIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(pDlg->spinBox_borderThickness->value(), 2);

        pDlg->spinBox_borderThickness->setValue(6);
        QCOMPARE(pRoom->mBorderThickness, 6);
        pDlg->pushButton_resetBorderColor->click();
        QVERIFY2(!pRoom->mBorderColor.isValid(), "resetting the border colour did not reach the room");

        pDlg->reject();

        QCOMPARE(pRoom->mBorderThickness, 2);
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));
    }

    void okKeepsThePreviewedBorderAndReportsIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->spinBox_borderThickness->setValue(6);
        pDlg->accept();

        QCOMPARE(pRoom->mBorderThickness, 6);
        QVERIFY(mEmitted.changeBorderThickness);
        QCOMPARE(mEmitted.newBorderThickness, 6);
        QVERIFY2(!mEmitted.changeBorderColor, "the border colour was reported as changed by a thickness edit");
    }

    // Every room in the selection previews, not just the one the dialog read its
    // starting values from
    void theBorderPreviewReachesEveryRoomInTheSelection()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        int previewCount = 0;
        connect(pDlg, &dlgRoomProperties::signal_preview_border, this, [&previewCount](QSet<TRoom*>) {
            ++previewCount;
        });

        pDlg->spinBox_borderThickness->setValue(4);

        QCOMPARE(previewCount, 1);
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 4);
        }

        pDlg->reject();
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 0);
        }
    }

    // Clearing the symbol is how a symbol is removed, and an empty field is a
    // decision rather than a "leave alone"
    void clearingTheSymbolFieldIsReportedAsAChange()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->clear();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QVERIFY(mEmitted.newSymbol.isEmpty());
    }

    void resettingTheSymbolColourIsReportedAsAnInvalidColour()
    {
        buildMap();
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->pushButton_resetSymbolColor->click();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbolColor);
        QVERIFY2(!mEmitted.newSymbolColor.isValid(), "a reset symbol colour has to be an invalid QColor so the default is used");
    }
};

#include "RoomPropertiesDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomPropertiesDialogTest)
