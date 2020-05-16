/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
 * Eventually these should be defined for whole application to force explicit
 * definition of all strings as:
 * EITHER: QStringLiteral("<string>") for internal non-user visable use not
 * subject to translation
 * OR: tr("<string>") for GUI or other user visible strings that need to be
 * handled by the translation system {or qApp->translate("<classname>",
 * "<string>") for classes NOT derived from qobject...
 */
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII

#include "dlgRoomExits.h"


#include "Host.h"
#include "TArea.h"
#include "TRoomDB.h"

dlgRoomExits::dlgRoomExits(Host* pH, QWidget* pW) : QDialog(pW), mpHost(pH), mpEditItem(nullptr), pR(), mRoomID(), mEditColumn()
{
    setupUi(this);
}

void dlgRoomExits::slot_endEditSpecialExits()
{
    button_endEditing->setDisabled(true);
    if (!button_addSpecialExit->isEnabled()) {
        button_addSpecialExit->setEnabled(true);
    }
    if (mpEditItem != nullptr && mEditColumn >= 0) {
        specialExits->closePersistentEditor(mpEditItem, mEditColumn);
        mpEditItem = nullptr;
        mEditColumn = -1;
    }
    specialExits->clearSelection();
}

void dlgRoomExits::slot_editSpecialExit(QTreeWidgetItem* pI, int column)
{
    /* column is now
       0 = exitRoomID
       1 = no route (locked)
       2 = exit weight
       3 = door type: none
       4 = door type: open
       5 = door type: closed
       6 = door type: locked
       7 = cmd
     */

    if (!button_endEditing->isEnabled()) {
        button_endEditing->setEnabled(true);
    }
    if (button_addSpecialExit->isEnabled()) {
        button_addSpecialExit->setEnabled(false);
    }

    if (mpEditItem != nullptr && (pI != mpEditItem || mEditColumn != column)) {
        // Thing that was clicked on is not the same as last thing that was clicked on
        // ... so clean up the old column
        TRoom* pExitToRoom = mpHost->mpMap->mpRoomDB->getRoom(mpEditItem->text(0).toInt());
        switch (mEditColumn) {
        case 0:
            if (mpEditItem->text(0).toInt() < 1) {
                mpEditItem->setText(0, tr("(room ID)", "Placeholder, if no room ID is set for an exit, yet. This string is used in 2 places, ensure they match!"));
            }
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            break;

        case 2:
            mpEditItem->setText(2, QString::number((mpEditItem->text(2).toInt() < 0) ? (-1 * mpEditItem->text(2).toInt()) : mpEditItem->text(2).toInt())); //Force result to be non-negative integer
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            break;

        case 3: // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
            mpEditItem->setCheckState(3, Qt::Checked);
            mpEditItem->setCheckState(4, Qt::Unchecked);
            mpEditItem->setCheckState(5, Qt::Unchecked);
            mpEditItem->setCheckState(6, Qt::Unchecked);
            break;

        case 4:
            mpEditItem->setCheckState(3, Qt::Unchecked);
            mpEditItem->setCheckState(4, Qt::Checked);
            mpEditItem->setCheckState(5, Qt::Unchecked);
            mpEditItem->setCheckState(6, Qt::Unchecked);
            break;

        case 5:
            mpEditItem->setCheckState(3, Qt::Unchecked);
            mpEditItem->setCheckState(4, Qt::Unchecked);
            mpEditItem->setCheckState(5, Qt::Checked);
            mpEditItem->setCheckState(6, Qt::Unchecked);
            break;

        case 6:
            mpEditItem->setCheckState(3, Qt::Unchecked);
            mpEditItem->setCheckState(4, Qt::Unchecked);
            mpEditItem->setCheckState(5, Qt::Unchecked);
            mpEditItem->setCheckState(6, Qt::Checked);
            break;

        case 7:
            if (!mpEditItem->text(7).trimmed().length()) {
                mpEditItem->setText(7, tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same"));
            }
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            //            qDebug()<<"Closed PE on item:"<<mpEditItem->text(7)<<"column:"<<mEditColumn;
            break;
        default:; //noop for other column (1)
        }

        if (pExitToRoom) {
            mpEditItem->setForeground(0, QColor(Qt::blue));
            if (!pExitToRoom->name.isEmpty()) {
                mpEditItem->setToolTip(0,
                                       QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                               .arg(tr(R"(Exit to "%1".)").arg(pExitToRoom->name),
                                                    tr("<b>Room</b> Weight of destination: %1.",
                                                       "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                            .arg(pExitToRoom->getWeight())));
            } else {
                mpEditItem->setToolTip(0,
                                       QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                               .arg(tr("Exit to unnamed room is valid"),
                                                    tr("<b>Room</b> Weight of destination: %1.",
                                                       "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                            .arg(pExitToRoom->getWeight())));
            }
        } else {
            mpEditItem->setForeground(0, QColor(Qt::red));
            mpEditItem->setToolTip(0,
                                   QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                           .arg(tr("Entered number is invalid, set the number of the room that this special exit leads to, will turn blue for "
                                                   "a valid number; if left like this, this exit will be deleted when &lt;i&gt;save&lt;/i&gt; is clicked.")));
        }

        mpEditItem = nullptr; //This will cause a new PE to be opened, it will also be zeroed on the first time this funciton is called
        mEditColumn = -1;
    }

    // Now process the new column that was selected:
    if (mpEditItem == nullptr) {
        if (column == 0 || column == 2 || column == 7) {
            //            qDebug()<<"Opened PE on item:"<<pI->text(7)<<"column:"<<column;
            specialExits->openPersistentEditor(pI, column);
            specialExits->editItem(pI, column);
        }
        mpEditItem = pI;
        mEditColumn = column;
    }

    //    qDebug()<<"A Special Exit is been edited, it has the command:" << pI->text(7) <<"and the editing is on column "<<column;
    switch (column) {
    case 2:
        pI->setText(2, QString::number((pI->text(2).toInt() < 0) ? (-1 * pI->text(2).toInt()) : pI->text(2).toInt())); //Force result to be non-negative
        break;

    case 3: // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
        pI->setCheckState(3, Qt::Checked);
        pI->setCheckState(4, Qt::Unchecked);
        pI->setCheckState(5, Qt::Unchecked);
        pI->setCheckState(6, Qt::Unchecked);
        break;

    case 4:
        pI->setCheckState(3, Qt::Unchecked);
        pI->setCheckState(4, Qt::Checked);
        pI->setCheckState(5, Qt::Unchecked);
        pI->setCheckState(6, Qt::Unchecked);
        break;

    case 5:
        pI->setCheckState(3, Qt::Unchecked);
        pI->setCheckState(4, Qt::Unchecked);
        pI->setCheckState(5, Qt::Checked);
        pI->setCheckState(6, Qt::Unchecked);
        break;

    case 6:
        pI->setCheckState(3, Qt::Unchecked);
        pI->setCheckState(4, Qt::Unchecked);
        pI->setCheckState(5, Qt::Unchecked);
        pI->setCheckState(6, Qt::Checked);
        break;

    default:; //noop for other columns?
    }
}

void dlgRoomExits::slot_addSpecialExit()
{
    auto pI = new QTreeWidgetItem(specialExits);
    pI->setText(0, tr("(room ID)", "Placeholder, if no room ID is set for an exit, yet. This string is used in 2 places, ensure they match!")); //Exit RoomID
    pI->setForeground(0, QColor(Qt::red));
    pI->setToolTip(0,
                   QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                           .arg(tr("Set the number of the room that this special exit leads to, will turn blue for a valid number; if left like "
                                   "this, this exit will be deleted when &lt;i&gt;save&lt;/i&gt; is clicked.")));
    pI->setTextAlignment(0, Qt::AlignRight);
    pI->setToolTip(1, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Prevent a route being created via this exit, equivalent to an infinite exit weight.")));
    pI->setCheckState(1, Qt::Unchecked); //Locked
    pI->setText(2, QStringLiteral("0")); //Exit Weight
    pI->setTextAlignment(2, Qt::AlignRight);
    pI->setToolTip(2,
                   QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                           .arg(tr("Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.")));
    pI->setCheckState(3, Qt::Checked); //Doortype: none
    pI->setToolTip(3, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("No door symbol is drawn on 2D Map for this exit (only functional choice currently).")));
    pI->setCheckState(4, Qt::Unchecked); //Doortype: open
    pI->setToolTip(4,
                   QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setCheckState(5, Qt::Unchecked); //Doortype: closed
    pI->setToolTip(
            5, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setCheckState(6, Qt::Unchecked); //Doortype: locked
    pI->setToolTip(6,
                   QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setText(7, tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same")); //Exit command
    pI->setTextAlignment(7, Qt::AlignLeft);
    specialExits->addTopLevelItem(pI);
}

void dlgRoomExits::save()
{
    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    if (!pR) {
        return;
    }

    QMultiMap<int, QString> oldSpecialExits = pR->getOtherMap();
    QMutableMapIterator<int, QString> exitIterator = oldSpecialExits;
    while (exitIterator.hasNext()) {
        exitIterator.next();
        if (exitIterator.value().length() > 1 && (exitIterator.value().startsWith(QStringLiteral("0")) || exitIterator.value().startsWith(QStringLiteral("1")))) {
            exitIterator.setValue(exitIterator.value().mid(1));
        }
    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<QString> originalExitCmdsList{oldSpecialExits.values()};
    QSet<QString> originalExitCmds{originalExitCmdsList.begin(), originalExitCmdsList.end()};
#else
    QSet<QString> originalExitCmds{oldSpecialExits.values().toSet()};
#endif

    //    pR->clearSpecialExits(); // Previous code could not change the destination
    // room of an existing special exit
    // so had to clear all and rebuild all of them
    for (int i = 0; i < specialExits->topLevelItemCount(); i++) {
        QTreeWidgetItem* pI = specialExits->topLevelItem(i);
        int key = pI->text(0).toInt();
        int weight = pI->text(2).toInt();
        int door = 0;
        if (pI->checkState(6) == Qt::Checked) {
            door = 3;
        } else if (pI->checkState(5) == Qt::Checked) {
            door = 2;
        } else if (pI->checkState(4) == Qt::Checked) {
            door = 1;
        } else if (pI->checkState(3) == Qt::Checked) {
            door = 0;
        }
        QString value = pI->text(7);
        if (value != tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same")
            && key != 0 && mpHost->mpMap->mpRoomDB->getRoom(key) != nullptr) {
            originalExitCmds.remove(value);
            if (pI->checkState(1) == Qt::Unchecked) {
                value = value.prepend(QStringLiteral("0"));
            } else {
                value = value.prepend(QStringLiteral("1"));
            }
            pR->setSpecialExit(key, value); // Now can overwrite an existing exit with a different destination
            if (pR->hasExitWeight(value.mid(1)) || weight > 0) {
                pR->setExitWeight(value.mid(1), weight);
            }
            if (pR->getDoor(value.mid(1)) || door > 0) {
                pR->setDoor(value.mid(1), door);
            }
        }
    }

    // Clean up after any deleted specialExits originally present but not now so
    for (auto value : originalExitCmds) {
        pR->customLinesArrow.remove(value);
        pR->customLinesColor.remove(value);
        pR->customLinesStyle.remove(value);
        pR->customLines.remove(value);
        pR->setDoor(value, 0);
        pR->setExitWeight(value, 0);
        pR->setSpecialExit(-1, value);
    }

    QString exitKey = QStringLiteral("nw");
    int dirCode = DIR_NORTHWEST;
    if (nw->isEnabled() && !nw->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != nullptr) {
        // There IS a valid exit on the dialogue in this direction
        if (originalExits.value(dirCode)->destination != nw->text().toInt()) {
            pR->setExit(nw->text().toInt(), dirCode); // Destination is different - so store it
        }
        if (pR->hasExitStub(dirCode)) { // And ensure that stub exit is cleared if set
            pR->setExitStub(dirCode, false);
        }
        if (weight_nw->value()) { // And store any weighing specifed
            pR->setExitWeight(exitKey, weight_nw->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else { // No valid exit on the dialogue
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode); // Destination has been deleted So ensure the value for no exit is stored
        }
        if (stub_nw->isChecked() != pR->hasExitStub(dirCode)) {
            // Does the stub exit setting differ from what is stored
            pR->setExitStub(dirCode, stub_nw->isChecked()); // So change stored idea to match
        }
        pR->setExitWeight(exitKey, 0); // And clear any weighting that was stored
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey); // And remove any custom line stuff, which uses opposite case keys - *sigh*
    }

    exitKey = QStringLiteral("n");
    dirCode = DIR_NORTH;
    if (n->isEnabled() && !n->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != n->text().toInt()) {
            pR->setExit(n->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_n->value()) {
            pR->setExitWeight(exitKey, weight_n->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_n->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_n->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("ne");
    dirCode = DIR_NORTHEAST;
    if (ne->isEnabled() && !ne->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != ne->text().toInt()) {
            pR->setExit(ne->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_ne->value()) {
            pR->setExitWeight(exitKey, weight_ne->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_ne->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_ne->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("up");
    dirCode = DIR_UP;
    if (up->isEnabled() && !up->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != up->text().toInt()) {
            pR->setExit(up->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_up->value()) {
            pR->setExitWeight(exitKey, weight_up->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_up->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_up->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("w");
    dirCode = DIR_WEST;
    if (w->isEnabled() && !w->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != w->text().toInt()) {
            pR->setExit(w->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_w->value()) {
            pR->setExitWeight(exitKey, weight_w->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_w->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_w->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("e");
    dirCode = DIR_EAST;
    if (e->isEnabled() && !e->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != e->text().toInt()) {
            pR->setExit(e->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_e->value()) {
            pR->setExitWeight(exitKey, weight_e->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_e->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_e->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("down");
    dirCode = DIR_DOWN;
    if (down->isEnabled() && !down->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != down->text().toInt()) {
            pR->setExit(down->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_down->value()) {
            pR->setExitWeight(exitKey, weight_down->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_down->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_down->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("sw");
    dirCode = DIR_SOUTHWEST;
    if (sw->isEnabled() && !sw->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != sw->text().toInt()) {
            pR->setExit(sw->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_sw->value()) {
            pR->setExitWeight(exitKey, weight_sw->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_sw->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_sw->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("s");
    dirCode = DIR_SOUTH;
    if (s->isEnabled() && !s->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != s->text().toInt()) {
            pR->setExit(s->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_s->value()) {
            pR->setExitWeight(exitKey, weight_s->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_s->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_s->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("se");
    dirCode = DIR_SOUTHEAST;
    if (se->isEnabled() && !se->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != se->text().toInt()) {
            pR->setExit(se->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_se->value()) {
            pR->setExitWeight(exitKey, weight_se->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_se->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_se->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("in");
    dirCode = DIR_IN;
    if (in->isEnabled() && !in->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != in->text().toInt()) {
            pR->setExit(in->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_in->value()) {
            pR->setExitWeight(exitKey, weight_in->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        if (originalExits.value(dirCode)->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_in->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_in->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    exitKey = QStringLiteral("out");
    dirCode = DIR_OUT;
    if (out->isEnabled() && !out->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != nullptr) {
        if (originalExits.value(dirCode)->destination != out->text().toInt()) {
            pR->setExit(out->text().toInt(), dirCode);
        }
        if (pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, false);
        }
        if (weight_out->value()) {
            pR->setExitWeight(exitKey, weight_out->value());
        } else {
            pR->setExitWeight(exitKey, 0);
        }
    } else {
        auto exit = originalExits.value(dirCode);
        if (exit && exit->destination > 0) {
            pR->setExit(-1, dirCode);
        }
        if (stub_out->isChecked() != pR->hasExitStub(dirCode)) {
            pR->setExitStub(dirCode, stub_out->isChecked());
        }
        pR->setExitWeight(exitKey, 0);
        pR->customLinesArrow.remove(exitKey);
        pR->customLinesColor.remove(exitKey);
        pR->customLinesStyle.remove(exitKey);
        pR->customLines.remove(exitKey);
    }

    pR->setExitLock(DIR_NORTHWEST, noroute_nw->isChecked());
    pR->setExitLock(DIR_NORTH, noroute_n->isChecked());
    pR->setExitLock(DIR_NORTHEAST, noroute_ne->isChecked());
    pR->setExitLock(DIR_UP, noroute_up->isChecked());
    pR->setExitLock(DIR_WEST, noroute_w->isChecked());
    pR->setExitLock(DIR_EAST, noroute_e->isChecked());
    pR->setExitLock(DIR_DOWN, noroute_down->isChecked());
    pR->setExitLock(DIR_SOUTHWEST, noroute_sw->isChecked());
    pR->setExitLock(DIR_SOUTH, noroute_s->isChecked());
    pR->setExitLock(DIR_SOUTHEAST, noroute_se->isChecked());
    pR->setExitLock(DIR_IN, noroute_in->isChecked());
    pR->setExitLock(DIR_OUT, noroute_out->isChecked());

    // return value from checkedId() is -1 for no radio button in group checked,
    //   and then more negative values starting from -2 for each button that was
    //   created without an explict Id, any attempt to set a different Id using
    //   setId() seems to fail for me :(
    if (doortype_nw->checkedId() < -1) {
        pR->setDoor(QStringLiteral("nw"), -2 - doortype_nw->checkedId());
    }

    if (doortype_n->checkedId() < -1) {
        pR->setDoor(QStringLiteral("n"), -2 - doortype_n->checkedId());
    }

    if (doortype_ne->checkedId() < -1) {
        pR->setDoor(QStringLiteral("ne"), -2 - doortype_ne->checkedId());
    }

    if (doortype_up->checkedId() < -1) {
        pR->setDoor(QStringLiteral("up"), -2 - doortype_up->checkedId());
    }

    if (doortype_w->checkedId() < -1) {
        pR->setDoor(QStringLiteral("w"), -2 - doortype_w->checkedId());
    }

    if (doortype_e->checkedId() < -1) {
        pR->setDoor(QStringLiteral("e"), -2 - doortype_e->checkedId());
    }

    if (doortype_down->checkedId() < -1) {
        pR->setDoor(QStringLiteral("down"), -2 - doortype_down->checkedId());
    }

    if (doortype_sw->checkedId() < -1) {
        pR->setDoor(QStringLiteral("sw"), -2 - doortype_sw->checkedId());
    }

    if (doortype_s->checkedId() < -1) {
        pR->setDoor(QStringLiteral("s"), -2 - doortype_s->checkedId());
    }

    if (doortype_se->checkedId() < -1) {
        pR->setDoor(QStringLiteral("se"), -2 - doortype_se->checkedId());
    }

    if (doortype_in->checkedId() < -1) {
        pR->setDoor(QStringLiteral("in"), -2 - doortype_in->checkedId());
    }

    if (doortype_out->checkedId() < -1) {
        pR->setDoor(QStringLiteral("out"), -2 - doortype_out->checkedId());
    }

    TArea* pA = mpHost->mpMap->mpRoomDB->getArea(pR->getArea());
    if (pA) {
        pA->determineAreaExitsOfRoom(pR->getId());
    }

    close();
}

// These slots are called as the text for the exitID is edited
void dlgRoomExits::slot_nw_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        // Valid exit roomID in place
        nw->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_nw->setChecked(false);
        stub_nw->setEnabled(false);
        noroute_nw->setEnabled(true);
        weight_nw->setEnabled(true);
        doortype_none_nw->setEnabled(true);
        doortype_open_nw->setEnabled(true);
        doortype_closed_nw->setEnabled(true);
        doortype_locked_nw->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (!text.isEmpty()) {
        // Something is entered but it does not yield a valid exit roomID
        // Enable stub exit control
        nw->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                               .arg(tr("Entered number is invalid, set the number of the room northwest of this one, will turn blue for a valid number.")));
        stub_nw->setEnabled(true);
        noroute_nw->setEnabled(false);
        weight_nw->setEnabled(false);
        doortype_none_nw->setEnabled(false);
        doortype_open_nw->setEnabled(false);
        doortype_closed_nw->setEnabled(false);
        doortype_locked_nw->setEnabled(false);
    } else {
        // Nothing is entered - so we can enable the stub exit control
        nw->setStyleSheet(QString());
        nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.")));
        stub_nw->setEnabled(true);
        noroute_nw->setEnabled(false);
        weight_nw->setEnabled(false);
        doortype_none_nw->setEnabled(false);
        doortype_open_nw->setEnabled(false);
        doortype_closed_nw->setEnabled(false);
        doortype_locked_nw->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_n_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        n->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        ;
        stub_n->setChecked(false);
        stub_n->setEnabled(false);
        noroute_n->setEnabled(true);
        weight_n->setEnabled(true);
        doortype_none_n->setEnabled(true);
        doortype_open_n->setEnabled(true);
        doortype_closed_n->setEnabled(true);
        doortype_locked_n->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            n->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        } else {
            n->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr("Exit to unnamed room is valid"),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        n->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        n->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room north of this one, will turn blue for a valid number.")));
        stub_n->setEnabled(true);
        noroute_n->setEnabled(false);
        weight_n->setEnabled(false);
        doortype_none_n->setEnabled(false);
        doortype_open_n->setEnabled(false);
        doortype_closed_n->setEnabled(false);
        doortype_locked_n->setEnabled(false);
    } else {
        n->setStyleSheet(QString());
        n->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room north of this one, will be blue for a valid number or red for invalid.")));
        stub_n->setEnabled(true);
        noroute_n->setEnabled(false);
        weight_n->setEnabled(false);
        doortype_none_n->setEnabled(false);
        doortype_open_n->setEnabled(false);
        doortype_closed_n->setEnabled(false);
        doortype_locked_n->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_ne_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        ne->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_ne->setChecked(false);
        stub_ne->setEnabled(false);
        noroute_ne->setEnabled(true);
        weight_ne->setEnabled(true);
        doortype_none_ne->setEnabled(true);
        doortype_open_ne->setEnabled(true);
        doortype_closed_ne->setEnabled(true);
        doortype_locked_ne->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        ne->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                               .arg(tr("Entered number is invalid, set the number of the room northeast of this one, will turn blue for a valid number.")));
        stub_ne->setEnabled(true);
        noroute_ne->setEnabled(false);
        weight_ne->setEnabled(false);
        doortype_none_ne->setEnabled(false);
        doortype_open_ne->setEnabled(false);
        doortype_closed_ne->setEnabled(false);
        doortype_locked_ne->setEnabled(false);
    } else {
        ne->setStyleSheet(QString());
        ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.")));
        stub_ne->setEnabled(true);
        noroute_ne->setEnabled(false);
        weight_ne->setEnabled(false);
        doortype_none_ne->setEnabled(false);
        doortype_open_ne->setEnabled(false);
        doortype_closed_ne->setEnabled(false);
        doortype_locked_ne->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_up_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        up->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_up->setChecked(false);
        stub_up->setEnabled(false);
        noroute_up->setEnabled(true);
        weight_up->setEnabled(true);
        doortype_none_up->setEnabled(true);
        doortype_open_up->setEnabled(true);
        doortype_closed_up->setEnabled(true);
        doortype_locked_up->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            up->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            up->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        up->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        up->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room up from this one, will turn blue for a valid number.")));
        stub_up->setEnabled(true);
        noroute_up->setEnabled(false);
        weight_up->setEnabled(false);
        doortype_none_up->setEnabled(false);
        doortype_open_up->setEnabled(false);
        doortype_closed_up->setEnabled(false);
        doortype_locked_up->setEnabled(false);
    } else {
        up->setStyleSheet(QString());
        up->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room up from this one, will be blue for a valid number or red for invalid.")));
        stub_up->setEnabled(true);
        noroute_up->setEnabled(false);
        weight_up->setEnabled(false);
        doortype_none_up->setEnabled(false);
        doortype_open_up->setEnabled(false);
        doortype_closed_up->setEnabled(false);
        doortype_locked_up->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_w_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        w->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_w->setChecked(false);
        stub_w->setEnabled(false);
        noroute_w->setEnabled(true);
        weight_w->setEnabled(true);
        doortype_none_w->setEnabled(true);
        doortype_open_w->setEnabled(true);
        doortype_closed_w->setEnabled(true);
        doortype_locked_w->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            w->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        } else {
            w->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr("Exit to unnamed room is valid"),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        w->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        w->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room west of this one, will turn blue for a valid number.")));
        stub_w->setEnabled(true);
        noroute_w->setEnabled(false);
        weight_w->setEnabled(false);
        doortype_none_w->setEnabled(false);
        doortype_open_w->setEnabled(false);
        doortype_closed_w->setEnabled(false);
        doortype_locked_w->setEnabled(false);
    } else {
        w->setStyleSheet(QString());
        w->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room west of this one, will be blue for a valid number or red for invalid.")));
        stub_w->setEnabled(true);
        noroute_w->setEnabled(false);
        weight_w->setEnabled(false);
        doortype_none_w->setEnabled(false);
        doortype_open_w->setEnabled(false);
        doortype_closed_w->setEnabled(false);
        doortype_locked_w->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_e_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        e->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_e->setChecked(false);
        stub_e->setEnabled(false);
        noroute_e->setEnabled(true);
        weight_e->setEnabled(true);
        doortype_none_e->setEnabled(true);
        doortype_open_e->setEnabled(true);
        doortype_closed_e->setEnabled(true);
        doortype_locked_e->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            e->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        } else {
            e->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr("Exit to unnamed room is valid"),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        e->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        e->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room east of this one, will turn blue for a valid number.")));
        stub_e->setEnabled(true);
        noroute_e->setEnabled(false);
        weight_e->setEnabled(false);
        doortype_none_e->setEnabled(false);
        doortype_open_e->setEnabled(false);
        doortype_closed_e->setEnabled(false);
        doortype_locked_e->setEnabled(false);
    } else {
        e->setStyleSheet(QString());
        e->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room east of this one, will be blue for a valid number or red for invalid.")));
        stub_e->setEnabled(true);
        noroute_e->setEnabled(false);
        weight_e->setEnabled(false);
        doortype_none_e->setEnabled(false);
        doortype_open_e->setEnabled(false);
        doortype_closed_e->setEnabled(false);
        doortype_locked_e->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_down_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        down->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_down->setChecked(false);
        stub_down->setEnabled(false);
        noroute_down->setEnabled(true);
        weight_down->setEnabled(true);
        doortype_none_down->setEnabled(true);
        doortype_open_down->setEnabled(true);
        doortype_closed_down->setEnabled(true);
        doortype_locked_down->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            down->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                     .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                          tr("<b>Room</b> Weight of destination: %1.",
                                             "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                  .arg(exitToRoom->getWeight())));
        } else {
            down->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                     .arg(tr("Exit to unnamed room is valid"),
                                          tr("<b>Room</b> Weight of destination: %1.",
                                             "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                  .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        down->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        down->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room down from this one, will turn blue for a valid number.")));
        stub_down->setEnabled(true);
        noroute_down->setEnabled(false);
        weight_down->setEnabled(false);
        doortype_none_down->setEnabled(false);
        doortype_open_down->setEnabled(false);
        doortype_closed_down->setEnabled(false);
        doortype_locked_down->setEnabled(false);
    } else {
        down->setStyleSheet(QString());
        down->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room down from this one, will be blue for a valid number or red for invalid.")));
        stub_down->setEnabled(true);
        noroute_down->setEnabled(false);
        weight_down->setEnabled(false);
        doortype_none_down->setEnabled(false);
        doortype_open_down->setEnabled(false);
        doortype_closed_down->setEnabled(false);
        doortype_locked_down->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_sw_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        sw->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_sw->setChecked(false);
        stub_sw->setEnabled(false);
        noroute_sw->setEnabled(true);
        weight_sw->setEnabled(true);
        doortype_none_sw->setEnabled(true);
        doortype_open_sw->setEnabled(true);
        doortype_closed_sw->setEnabled(true);
        doortype_locked_sw->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        sw->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                               .arg(tr("Entered number is invalid, set the number of the room southwest of this one, will turn blue for a valid number.")));
        stub_sw->setEnabled(true);
        noroute_sw->setEnabled(false);
        weight_sw->setEnabled(false);
        doortype_none_sw->setEnabled(false);
        doortype_open_sw->setEnabled(false);
        doortype_closed_sw->setEnabled(false);
        doortype_locked_sw->setEnabled(false);
    } else {
        sw->setStyleSheet(QString());
        sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.")));
        stub_sw->setEnabled(true);
        noroute_sw->setEnabled(false);
        weight_sw->setEnabled(false);
        doortype_none_sw->setEnabled(false);
        doortype_open_sw->setEnabled(false);
        doortype_closed_sw->setEnabled(false);
        doortype_locked_sw->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_s_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        s->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_s->setChecked(false);
        stub_s->setEnabled(false);
        noroute_s->setEnabled(true);
        weight_s->setEnabled(true);
        doortype_none_s->setEnabled(true);
        doortype_open_s->setEnabled(true);
        doortype_closed_s->setEnabled(true);
        doortype_locked_s->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            s->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        } else {
            s->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                  .arg(tr("Exit to unnamed room is valid"),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                               .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        s->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        s->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room south of this one, will turn blue for a valid number.")));
        stub_s->setEnabled(true);
        noroute_s->setEnabled(false);
        weight_s->setEnabled(false);
        doortype_none_s->setEnabled(false);
        doortype_open_s->setEnabled(false);
        doortype_closed_s->setEnabled(false);
        doortype_locked_s->setEnabled(false);
    } else {
        s->setStyleSheet(QString());
        s->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room south of this one, will be blue for a valid number or red for invalid.")));
        stub_s->setEnabled(true);
        noroute_s->setEnabled(false);
        weight_s->setEnabled(false);
        doortype_none_s->setEnabled(false);
        doortype_open_s->setEnabled(false);
        doortype_closed_s->setEnabled(false);
        doortype_locked_s->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_se_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        se->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_se->setChecked(false);
        stub_se->setEnabled(false);
        noroute_se->setEnabled(true);
        weight_se->setEnabled(true);
        doortype_none_se->setEnabled(true);
        doortype_open_se->setEnabled(true);
        doortype_closed_se->setEnabled(true);
        doortype_locked_se->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        se->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                               .arg(tr("Entered number is invalid, set the number of the room southeast of this one, will turn blue for a valid number.")));
        stub_se->setEnabled(true);
        noroute_se->setEnabled(false);
        weight_se->setEnabled(false);
        doortype_none_se->setEnabled(false);
        doortype_open_se->setEnabled(false);
        doortype_closed_se->setEnabled(false);
        doortype_locked_se->setEnabled(false);
    } else {
        se->setStyleSheet(QString());
        se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.")));
        stub_se->setEnabled(true);
        noroute_se->setEnabled(false);
        weight_se->setEnabled(false);
        doortype_none_se->setEnabled(false);
        doortype_open_se->setEnabled(false);
        doortype_closed_se->setEnabled(false);
        doortype_locked_se->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_in_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        in->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_in->setChecked(false);
        stub_in->setEnabled(false);
        noroute_in->setEnabled(true);
        weight_in->setEnabled(true);
        doortype_none_in->setEnabled(true);
        doortype_open_in->setEnabled(true);
        doortype_closed_in->setEnabled(true);
        doortype_locked_in->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            in->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        } else {
            in->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                   .arg(tr("Exit to unnamed room is valid"),
                                        tr("<b>Room</b> Weight of destination: %1.",
                                           "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        in->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        in->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room in from this one, will turn blue for a valid number.")));
        stub_in->setEnabled(true);
        noroute_in->setEnabled(false);
        weight_in->setEnabled(false);
        doortype_none_in->setEnabled(false);
        doortype_open_in->setEnabled(false);
        doortype_closed_in->setEnabled(false);
        doortype_locked_in->setEnabled(false);
    } else {
        in->setStyleSheet(QString());
        in->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room in from this one, will be blue for a valid number or red for invalid.")));
        stub_in->setEnabled(true);
        noroute_in->setEnabled(false);
        weight_in->setEnabled(false);
        doortype_none_in->setEnabled(false);
        doortype_open_in->setEnabled(false);
        doortype_closed_in->setEnabled(false);
        doortype_locked_in->setEnabled(false);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_out_textEdited(const QString& text)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if (exitToRoom) {
        out->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        stub_out->setChecked(false);
        stub_out->setEnabled(false);
        noroute_out->setEnabled(true);
        weight_out->setEnabled(true);
        doortype_none_out->setEnabled(true);
        doortype_open_out->setEnabled(true);
        doortype_closed_out->setEnabled(true);
        doortype_locked_out->setEnabled(true);
        if (exitToRoom->name.trimmed().length()) {
            out->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                    .arg(tr(R"(Exit to "%1".)").arg(exitToRoom->name),
                                         tr("<b>Room</b> Weight of destination: %1.",
                                            "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                 .arg(exitToRoom->getWeight())));
        } else {
            out->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                    .arg(tr("Exit to unnamed room is valid"),
                                         tr("<b>Room</b> Weight of destination: %1.",
                                            "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                 .arg(exitToRoom->getWeight())));
        }
    } else if (text.size() > 0) {
        out->setStyleSheet(QStringLiteral(".QLineEdit { color:red }"));
        out->setToolTip(
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Entered number is invalid, set the number of the room out from this one, will turn blue for a valid number.")));
        stub_out->setEnabled(true);
        noroute_out->setEnabled(false);
        weight_out->setEnabled(false);
        doortype_none_out->setEnabled(false);
        doortype_open_out->setEnabled(false);
        doortype_closed_out->setEnabled(false);
        doortype_locked_out->setEnabled(false);
    } else {
        out->setStyleSheet(QString());
        out->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room out from this one, will be blue for a valid number or red for invalid.")));
        stub_out->setEnabled(true);
        noroute_out->setEnabled(false);
        weight_out->setEnabled(false);
        doortype_none_out->setEnabled(false);
        doortype_open_out->setEnabled(false);
        doortype_closed_out->setEnabled(false);
        doortype_locked_out->setEnabled(false);
    }
    slot_checkModified();
}

// These slots are called as the stub exit checkboxes are clicked
void dlgRoomExits::slot_stub_nw_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != nullptr) {
            nw->setText(QString());
            nw->setStyleSheet(QString());
            weight_nw->setValue(0);        // Can't have a weight for a stub exit
            noroute_nw->setChecked(false); // nor a "lock"
        }
        noroute_nw->setEnabled(false); // Disable "lock" on this exit
        nw->setEnabled(false);         // Prevent entry of an exit roomID
        nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_nw->setEnabled(true);
        doortype_open_nw->setEnabled(true);
        doortype_closed_nw->setEnabled(true);
        doortype_locked_nw->setEnabled(true); // Permit a door to be set on a stub exit
        weight_nw->setEnabled(false);         // Prevent a weight to be set/changed on a stub
    } else {
        nw->setEnabled(true);
        nw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.")));
        //  noroute_nw->setEnabled(true); although this branch will enable the exit entry
        //  there will not be a valid one there yet so don't enable the noroute(lock) control here!
        doortype_none_nw->setEnabled(false);
        doortype_open_nw->setEnabled(false);
        doortype_closed_nw->setEnabled(false);
        doortype_locked_nw->setEnabled(false);
        doortype_none_nw->setChecked(true);
        //  similarly as there won't be a valid exit or a stub exit at theis point disable/reset the door type controls
        weight_nw->setEnabled(false);
        weight_nw->setValue(0); // Prevent a weight to be set/changed on a also
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_n_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != nullptr) {
            n->setText(QString());
            n->setStyleSheet(QString());
            weight_n->setValue(0);
            noroute_n->setChecked(false);
        }
        noroute_n->setEnabled(false);
        n->setEnabled(false);
        n->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_n->setEnabled(true);
        doortype_open_n->setEnabled(true);
        doortype_closed_n->setEnabled(true);
        doortype_locked_n->setEnabled(true);
        weight_n->setEnabled(false);
    } else {
        n->setEnabled(true);
        n->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room north of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_n->setEnabled(false);
        doortype_open_n->setEnabled(false);
        doortype_closed_n->setEnabled(false);
        doortype_locked_n->setEnabled(false);
        doortype_none_n->setChecked(true);
        weight_n->setEnabled(false);
        weight_n->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_ne_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != nullptr) {
            ne->setText(QString());
            ne->setStyleSheet(QString());
            weight_ne->setValue(0);
            noroute_ne->setChecked(false);
        }
        noroute_ne->setEnabled(false);
        ne->setEnabled(false);
        ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_ne->setEnabled(true);
        doortype_open_ne->setEnabled(true);
        doortype_closed_ne->setEnabled(true);
        doortype_locked_ne->setEnabled(true);
        weight_ne->setEnabled(false);
    } else {
        ne->setEnabled(true);
        ne->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_ne->setEnabled(false);
        doortype_open_ne->setEnabled(false);
        doortype_closed_ne->setEnabled(false);
        doortype_locked_ne->setEnabled(false);
        doortype_none_ne->setChecked(true);
        weight_ne->setEnabled(false);
        weight_ne->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_up_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != nullptr) {
            up->setText(QString());
            up->setStyleSheet(QString());
            weight_up->setValue(0);
            noroute_up->setChecked(false);
        }
        noroute_up->setEnabled(false);
        up->setEnabled(false);
        up->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_up->setEnabled(true);
        doortype_open_up->setEnabled(true);
        doortype_closed_up->setEnabled(true);
        doortype_locked_up->setEnabled(true);
        weight_up->setEnabled(false);
    } else {
        up->setEnabled(true);
        up->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room up from this one, will be blue for a valid number or red for invalid.")));
        doortype_none_up->setEnabled(false);
        doortype_open_up->setEnabled(false);
        doortype_closed_up->setEnabled(false);
        doortype_locked_up->setEnabled(false);
        doortype_none_up->setChecked(true);
        weight_up->setEnabled(false);
        weight_up->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_w_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != nullptr) {
            w->setText(QString());
            w->setStyleSheet(QString());
            weight_w->setValue(0);
            noroute_w->setChecked(false);
        }
        noroute_w->setEnabled(false);
        w->setEnabled(false);
        w->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_w->setEnabled(true);
        doortype_open_w->setEnabled(true);
        doortype_closed_w->setEnabled(true);
        doortype_locked_w->setEnabled(true);
        weight_w->setEnabled(false);
    } else {
        w->setEnabled(true);
        w->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room west of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_w->setEnabled(false);
        doortype_open_w->setEnabled(false);
        doortype_closed_w->setEnabled(false);
        doortype_locked_w->setEnabled(false);
        doortype_none_w->setChecked(true);
        weight_w->setEnabled(false);
        weight_w->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_e_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != nullptr) {
            e->setText(QString());
            e->setStyleSheet(QString());
            weight_e->setValue(0);
            noroute_e->setChecked(false);
        }
        noroute_e->setEnabled(false);
        e->setEnabled(false);
        e->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_e->setEnabled(true);
        doortype_open_e->setEnabled(true);
        doortype_closed_e->setEnabled(true);
        doortype_locked_e->setEnabled(true);
        weight_e->setEnabled(false);
    } else {
        e->setEnabled(true);
        e->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room east of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_e->setEnabled(false);
        doortype_open_e->setEnabled(false);
        doortype_closed_e->setEnabled(false);
        doortype_locked_e->setEnabled(false);
        doortype_none_e->setChecked(true);
        weight_e->setEnabled(false);
        weight_e->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_down_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != nullptr) {
            down->setText(QString());
            down->setStyleSheet(QString());
            weight_down->setValue(0);
            noroute_down->setChecked(false);
        }
        noroute_down->setEnabled(false);
        down->setEnabled(false);
        down->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_down->setEnabled(true);
        doortype_open_down->setEnabled(true);
        doortype_closed_down->setEnabled(true);
        doortype_locked_down->setEnabled(true);
        weight_down->setEnabled(false);
    } else {
        down->setEnabled(true);
        down->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room down from this one, will be blue for a valid number or red for invalid.")));
        doortype_none_down->setEnabled(false);
        doortype_open_down->setEnabled(false);
        doortype_closed_down->setEnabled(false);
        doortype_locked_down->setEnabled(false);
        doortype_none_down->setChecked(true);
        weight_down->setEnabled(false);
        weight_down->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_sw_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != nullptr) {
            sw->setText(QString());
            sw->setStyleSheet(QString());
            weight_sw->setValue(0);
            noroute_sw->setChecked(false);
        }
        noroute_sw->setEnabled(false);
        sw->setEnabled(false);
        sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_sw->setEnabled(true);
        doortype_open_sw->setEnabled(true);
        doortype_closed_sw->setEnabled(true);
        doortype_locked_sw->setEnabled(true);
        weight_sw->setEnabled(false);
    } else {
        sw->setEnabled(true);
        sw->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_sw->setEnabled(false);
        doortype_open_sw->setEnabled(false);
        doortype_closed_sw->setEnabled(false);
        doortype_locked_sw->setEnabled(false);
        doortype_none_sw->setChecked(true);
        weight_sw->setEnabled(false);
        weight_sw->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_s_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != nullptr) {
            s->setText(QString());
            s->setStyleSheet(QString());
            weight_s->setValue(0);
            noroute_s->setChecked(false);
        }
        noroute_s->setEnabled(false);
        s->setEnabled(false);
        s->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_s->setEnabled(true);
        doortype_open_s->setEnabled(true);
        doortype_closed_s->setEnabled(true);
        doortype_locked_s->setEnabled(true);
        weight_s->setEnabled(false);
    } else {
        s->setEnabled(true);
        s->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room south of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_s->setEnabled(false);
        doortype_open_s->setEnabled(false);
        doortype_closed_s->setEnabled(false);
        doortype_locked_s->setEnabled(false);
        doortype_none_s->setChecked(true);
        weight_s->setEnabled(false);
        weight_s->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_se_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != nullptr) {
            se->setText(QString());
            se->setStyleSheet(QString());
            weight_se->setValue(0);
            noroute_se->setChecked(false);
        }
        noroute_se->setEnabled(false);
        se->setEnabled(false);
        se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_se->setEnabled(true);
        doortype_open_se->setEnabled(true);
        doortype_closed_se->setEnabled(true);
        doortype_locked_se->setEnabled(true);
        weight_se->setEnabled(false);
    } else {
        se->setEnabled(true);
        se->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.")));
        doortype_none_se->setEnabled(false);
        doortype_open_se->setEnabled(false);
        doortype_closed_se->setEnabled(false);
        doortype_locked_se->setEnabled(false);
        doortype_none_se->setChecked(true);
        weight_se->setEnabled(false);
        weight_se->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_in_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != nullptr) {
            in->setText(QString());
            in->setStyleSheet(QString());
            weight_in->setValue(0);
            noroute_in->setChecked(false);
        }
        noroute_in->setEnabled(false);
        in->setEnabled(false);
        in->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_in->setEnabled(true);
        doortype_open_in->setEnabled(true);
        doortype_closed_in->setEnabled(true);
        doortype_locked_in->setEnabled(true);
        weight_in->setEnabled(false);
    } else {
        in->setEnabled(true);
        in->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room in from this one, will be blue for a valid number or red for invalid.")));
        doortype_none_in->setEnabled(false);
        doortype_open_in->setEnabled(false);
        doortype_closed_in->setEnabled(false);
        doortype_locked_in->setEnabled(false);
        doortype_none_in->setChecked(true);
        weight_in->setEnabled(false);
        weight_in->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::slot_stub_out_stateChanged(int state)
{
    if (state == Qt::Checked) {
        if (mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != nullptr) {
            out->setText(QString());
            out->setStyleSheet(QString());
            weight_out->setValue(0);
            noroute_out->setChecked(false);
        }
        noroute_out->setEnabled(false);
        out->setEnabled(false);
        out->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        doortype_none_out->setEnabled(true);
        doortype_open_out->setEnabled(true);
        doortype_closed_out->setEnabled(true);
        doortype_locked_out->setEnabled(true);
        weight_out->setEnabled(false);
    } else {
        out->setEnabled(true);
        out->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Set the number of the room out from this one, will be blue for a valid number or red for invalid.")));
        doortype_none_out->setEnabled(false);
        doortype_open_out->setEnabled(false);
        doortype_closed_out->setEnabled(false);
        doortype_locked_out->setEnabled(false);
        doortype_none_out->setChecked(true);
        weight_out->setEnabled(false);
        weight_out->setValue(0);
    }
    slot_checkModified();
}

void dlgRoomExits::initExit(int roomId,
                            int direction,
                            int exitId,
                            QLineEdit* exitLineEdit,
                            QCheckBox* noRoute,
                            QCheckBox* stub,
                            QRadioButton* none,
                            QRadioButton* open,
                            QRadioButton* closed,
                            QRadioButton* locked,
                            QSpinBox* weight)
{
    QString doorAndWeightText; // lowercase, initials for XY-plane, words for others
    QString exitText;          // lowercase, full words, no space
    switch (direction) {
        case DIR_NORTHWEST: doorAndWeightText = QStringLiteral("nw");   exitText = tr("northwest"); break;
        case DIR_NORTH    : doorAndWeightText = QStringLiteral("n");    exitText = tr("north");     break;
        case DIR_NORTHEAST: doorAndWeightText = QStringLiteral("ne");   exitText = tr("northeast"); break;
        case DIR_UP       : doorAndWeightText = QStringLiteral("up");   exitText = tr("up");        break;
        case DIR_WEST     : doorAndWeightText = QStringLiteral("w");    exitText = tr("west");      break;
        case DIR_EAST     : doorAndWeightText = QStringLiteral("e");    exitText = tr("east");      break;
        case DIR_DOWN     : doorAndWeightText = QStringLiteral("down"); exitText = tr("down");      break;
        case DIR_SOUTHWEST: doorAndWeightText = QStringLiteral("sw");   exitText = tr("southwest"); break;
        case DIR_SOUTH    : doorAndWeightText = QStringLiteral("s");    exitText = tr("south");     break;
        case DIR_SOUTHEAST: doorAndWeightText = QStringLiteral("se");   exitText = tr("southeast"); break;
        case DIR_IN       : doorAndWeightText = QStringLiteral("in");   exitText = tr("in");        break;
        case DIR_OUT      : doorAndWeightText = QStringLiteral("out");  exitText = tr("out");       break;
        default: Q_UNREACHABLE();
    }

    weight->setValue(pR->hasExitWeight(doorAndWeightText) ? pR->getExitWeight(doorAndWeightText) : 0);

    switch (pR->getDoor(doorAndWeightText)) {
    case 0:
        none->setChecked(true);
        break;
    case 1:
        open->setChecked(true);
        break;
    case 2:
        closed->setChecked(true);
        break;
    case 3:
        locked->setChecked(true);
        break;
    default:
        qWarning() << "dlgRoomExits::initExit(...) in room id(" << roomId << ") unexpected doors[" << doorAndWeightText << "] value:" << pR->getDoor(doorAndWeightText) << "found for room!";
    }

    TRoom* pExitR;
    if (exitId > 0) {
        pExitR = mpHost->mpMap->mpRoomDB->getRoom(exitId);
        if (!pExitR) {
            // Recover from a missing exit room - not doing this was causing seg. faults
            qWarning() << "dlgRoomExits::initExit(...): Warning: missing exit to" << exitId << "in direction" << exitText << ", resetting exit.";
            exitId = -1;
        }
    }

    if (exitId > 0 && pExitR) {                         //Does this exit point anywhere
        exitLineEdit->setText(QString::number(exitId)); //Put in the value
        exitLineEdit->setEnabled(true);                 //Enable it for editing
        exitLineEdit->setStyleSheet(QStringLiteral(".QLineEdit { color:blue }"));
        if (pExitR->name.trimmed().length()) {
            exitLineEdit->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                             .arg(tr(R"(Exit to "%1".)").arg(pExitR->name),
                                                  tr("<b>Room</b> Weight of destination: %1.",
                                                     "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                          .arg(pExitR->getWeight())));
        } else {
            exitLineEdit->setToolTip(QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                             .arg(tr("Exit to unnamed room is valid"),
                                                  tr("<b>Room</b> Weight of destination: %1.",
                                                     "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                          .arg(pExitR->getWeight())));
        }
        noRoute->setEnabled(true); //Enable speedwalk lock control
        none->setEnabled(true);    //Enable door type controls...
        open->setEnabled(true);
        closed->setEnabled(true);
        locked->setEnabled(true);                        //Already picked right one to check above here
        weight->setEnabled(true);                        //Enable exit weight control...
        stub->setEnabled(false);                         //Disable stub exit control, can't have one WITH an exit!
        stub->setChecked(false);                         //Ensure stub exit isn't set
        noRoute->setChecked(pR->hasExitLock(direction)); //Set/reset "locK" control as appropriate
    } else {                                             //No exit is set on initialisation
        exitLineEdit->setText(QString());                //Nothing to put in exitID box
        exitLineEdit->setStyleSheet(QString());
        noRoute->setEnabled(false); //Disable lock control, can't lock a non-existant exit..
        noRoute->setChecked(false); //.. and ensure there isn't one
        weight->setEnabled(false);  //Disable exit weight control...
        weight->setValue(0);        //And reset to default value (which will now cause the room's one to be used
        stub->setEnabled(true);     //Enable stub exit control
        if (pR->hasExitStub(direction)) {
            exitLineEdit->setEnabled(false); //There is a stub exit, so prevent exit number entry...
            exitLineEdit->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
            stub->setChecked(true);
            none->setEnabled(true); //Enable door type controls, can have a door on a stub exit..
            open->setEnabled(true);
            closed->setEnabled(true);
            locked->setEnabled(true);
        } else {
            exitLineEdit->setEnabled(true);
            exitLineEdit->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                             .arg(tr("Set the number of the room %1 of this one, will be blue for a valid number or red for invalid.").arg(exitText)));
            stub->setChecked(false);
            none->setEnabled(false); //Disable door type controls, can't lock a non-existant exit..
            open->setEnabled(false); //.. and ensure the "none" one is set if it ever gets enabled
            closed->setEnabled(false);
            locked->setEnabled(false);
            none->setChecked(true);
        }
    }
    originalExits[direction] = makeExitFromControls(direction);
}

void dlgRoomExits::init(int id)
{
    pR = mpHost->mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return;
    }

    roomID->setText(QString::number(id));
    roomWeight->setText(QString::number(pR->getWeight()));
    QString titleText;
    if (pR->name.trimmed().length()) {
        titleText = tr(R"(Exits for room: "%1" [*])").arg(pR->name);
    } else {
        titleText = tr("Exits for room Id: %1 [*]").arg(id);
    }

    this->setWindowTitle(titleText);

    // Because we are manipulating the settings for the exit we need to know
    // explicitly where the weight comes from, pR->getExitWeight() hides that
    // detail deliberately for normal usage
    initExit(id, DIR_NORTHWEST, pR->getExit(DIR_NORTHWEST), nw, noroute_nw, stub_nw, doortype_none_nw, doortype_open_nw, doortype_closed_nw, doortype_locked_nw, weight_nw);

    initExit(id, DIR_NORTH, pR->getExit(DIR_NORTH), n, noroute_n, stub_n, doortype_none_n, doortype_open_n, doortype_closed_n, doortype_locked_n, weight_n);

    initExit(id, DIR_NORTHEAST, pR->getExit(DIR_NORTHEAST), ne, noroute_ne, stub_ne, doortype_none_ne, doortype_open_ne, doortype_closed_ne, doortype_locked_ne, weight_ne);

    initExit(id, DIR_UP, pR->getExit(DIR_UP), up, noroute_up, stub_up, doortype_none_up, doortype_open_up, doortype_closed_up, doortype_locked_up, weight_up);

    initExit(id, DIR_WEST, pR->getExit(DIR_WEST), w, noroute_w, stub_w, doortype_none_w, doortype_open_w, doortype_closed_w, doortype_locked_w, weight_w);

    initExit(id, DIR_EAST, pR->getExit(DIR_EAST), e, noroute_e, stub_e, doortype_none_e, doortype_open_e, doortype_closed_e, doortype_locked_e, weight_e);

    initExit(id, DIR_DOWN, pR->getExit(DIR_DOWN), down, noroute_down, stub_down, doortype_none_down, doortype_open_down, doortype_closed_down, doortype_locked_down, weight_down);

    initExit(id, DIR_SOUTHWEST, pR->getExit(DIR_SOUTHWEST), sw, noroute_sw, stub_sw, doortype_none_sw, doortype_open_sw, doortype_closed_sw, doortype_locked_sw, weight_sw);

    initExit(id, DIR_SOUTH, pR->getExit(DIR_SOUTH), s, noroute_s, stub_s, doortype_none_s, doortype_open_s, doortype_closed_s, doortype_locked_s, weight_s);

    initExit(id, DIR_SOUTHEAST, pR->getExit(DIR_SOUTHEAST), se, noroute_se, stub_se, doortype_none_se, doortype_open_se, doortype_closed_se, doortype_locked_se, weight_se);

    initExit(id, DIR_IN, pR->getExit(DIR_IN), in, noroute_in, stub_in, doortype_none_in, doortype_open_in, doortype_closed_in, doortype_locked_in, weight_in);

    initExit(id, DIR_OUT, pR->getExit(DIR_OUT), out, noroute_out, stub_out, doortype_none_out, doortype_open_out, doortype_closed_out, doortype_locked_out, weight_out);

    QMapIterator<int, QString> it(pR->getOtherMap());
    while (it.hasNext()) {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if (dir.size() < 1) {
            continue;
        }
        if (dir.startsWith(QStringLiteral("0")) || dir.startsWith(QStringLiteral("1"))) {
            dir = dir.mid(1); // Not sure if this will be relevent here??
        }
        originalSpecialExits[dir] = new TExit();
        auto pI = new QTreeWidgetItem(specialExits);
        TRoom* pExitToRoom = mpHost->mpMap->mpRoomDB->getRoom(id_to);
        //0 was locked, now exit roomID
        pI->setText(0, QString::number(id_to));
        pI->setTextAlignment(0, Qt::AlignRight);
        if (pExitToRoom) {
            pI->setForeground(0, QColor(Qt::blue));
            if (!pExitToRoom->name.isEmpty()) {
                pI->setToolTip(0,
                               QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                       .arg(tr(R"(Exit to "%1".)").arg(pExitToRoom->name),
                                            tr("<b>Room</b> Weight of destination: %1.",
                                               "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                    .arg(pExitToRoom->getWeight())));
            } else {
                pI->setToolTip(0,
                               QStringLiteral("<html><head/><body><p>%1</p><p>%2</p></body></html>")
                                       .arg(tr("Exit to unnamed room is valid"),
                                            tr("<b>Room</b> Weight of destination: %1.",
                                               "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                                    .arg(pExitToRoom->getWeight())));
            }
        } else {
            pI->setForeground(0, QColor(Qt::red));
            pI->setToolTip(0,
                           QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                   .arg(tr("Room Id is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number.")));
        }
        originalSpecialExits.value(dir)->destination = id_to;
        //1 was roomID, now locked (or more properly "No route") - setCheckedState
        //automagically makes it a CheckBox!!!
        if (pR->hasSpecialExitLock(id_to, dir)) {
            pI->setCheckState(1, Qt::Checked);
            originalSpecialExits.value(dir)->hasNoRoute = true;
        } else {
            pI->setCheckState(1, Qt::Unchecked);
            originalSpecialExits.value(dir)->hasNoRoute = false;
        }
        pI->setToolTip(1, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Prevent a route being created via this exit, equivalent to an infinite exit weight.")));

        //2 was script, now exit weight - ideally want a spin box - but use a text edit for now
        if (pR->hasExitWeight(dir)) {
            pI->setText(2, QString::number(pR->getExitWeight(dir)));
        } else {
            pI->setText(2, QString::number(0));
        }
        pI->setTextAlignment(2, Qt::AlignRight);
        pI->setToolTip(2,
                       QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                               .arg(tr("Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.")));
        originalSpecialExits.value(dir)->weight = pI->text(2).toInt();


        //3-6 are new, now holds a buttongroup of 4, ideally QRadioButtons, to select a door type
        pI->setCheckState(3, Qt::Unchecked);
        pI->setTextAlignment(3, Qt::AlignCenter);
        pI->setToolTip(3, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("No door symbol is drawn on 2D Map for this exit (only functional choice currently).")));
        pI->setCheckState(4, Qt::Unchecked);
        pI->setTextAlignment(4, Qt::AlignCenter);
        pI->setToolTip(
                4, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setCheckState(5, Qt::Unchecked);
        pI->setTextAlignment(5, Qt::AlignCenter);
        pI->setToolTip(
                5,
                QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setTextAlignment(6, Qt::AlignCenter);
        pI->setToolTip(
                6, QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setCheckState(6, Qt::Unchecked);
        {
            int specialDoor = pR->getDoor(dir);
            switch (specialDoor) {
            case 0:
                pI->setCheckState(3, Qt::Checked);
                break;
            case 1:
                pI->setCheckState(4, Qt::Checked);
                break;
            case 2:
                pI->setCheckState(5, Qt::Checked);
                break;
            case 3:
                pI->setCheckState(6, Qt::Checked);
                break;
            default:
                qDebug() << "dlgRoomExits::init(" << id << ") unexpected (other exit) doors[" << dir << "] value:" << pR->doors[dir] << " found!";
            }
            originalSpecialExits.value(dir)->door = specialDoor;
        }

        //7 is new, but holds the script that was in 2
        pI->setText(7, dir);
        // Not relevant for special exits but better initialise it
        auto exit = originalSpecialExits.value(dir);
        if (exit) {
            exit->hasStub = false;
        }
    }
    mRoomID = id;
    button_save->setEnabled( false );
// We now do not connect up all these things until AFTER we have initialised
// things as some controls will issue unwanted signals upon setting values into
// them as we have above...
    // clang-format off
    connect(button_save,           &QAbstractButton::clicked,                      this, &dlgRoomExits::slot_endEditSpecialExits);
    connect(button_save,           &QAbstractButton::clicked,                      this, &dlgRoomExits::slot_checkModified);
    connect(button_save,           &QAbstractButton::clicked,                      this, &dlgRoomExits::save);
    connect(button_addSpecialExit, &QAbstractButton::clicked,                      this, &dlgRoomExits::slot_addSpecialExit);
    connect(specialExits,          &QTreeWidget::itemClicked,                      this, &dlgRoomExits::slot_editSpecialExit);
    connect(specialExits,          &QTreeWidget::itemClicked,                      this, &dlgRoomExits::slot_checkModified);
    connect(button_endEditing,     &QAbstractButton::clicked,                      this, &dlgRoomExits::slot_endEditSpecialExits);
    connect(button_endEditing,     &QAbstractButton::clicked,                      this, &dlgRoomExits::slot_checkModified);
    connect(nw,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_nw_textEdited);
    connect(n,                     &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_n_textEdited);
    connect(ne,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_ne_textEdited);
    connect(up,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_up_textEdited);
    connect(w,                     &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_w_textEdited);
    connect(e,                     &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_e_textEdited);
    connect(down,                  &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_down_textEdited);
    connect(sw,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_sw_textEdited);
    connect(s,                     &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_s_textEdited);
    connect(se,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_se_textEdited);
    connect(in,                    &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_in_textEdited);
    connect(out,                   &QLineEdit::textEdited,                         this, &dlgRoomExits::slot_out_textEdited);
    connect(stub_nw,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_nw_stateChanged);
    connect(stub_n,                &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_n_stateChanged);
    connect(stub_ne,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_ne_stateChanged);
    connect(stub_up,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_up_stateChanged);
    connect(stub_w,                &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_w_stateChanged);
    connect(stub_e,                &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_e_stateChanged);
    connect(stub_down,             &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_down_stateChanged);
    connect(stub_sw,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_sw_stateChanged);
    connect(stub_s,                &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_s_stateChanged);
    connect(stub_se,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_se_stateChanged);
    connect(stub_in,               &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_in_stateChanged);
    connect(stub_out,              &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_stub_out_stateChanged);
    connect(noroute_nw,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_n,             &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_ne,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_up,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_w,             &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_e,             &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_down,          &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_sw,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_s,             &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_se,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_in,            &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(noroute_out,           &QCheckBox::stateChanged,                       this, &dlgRoomExits::slot_checkModified);
    connect(weight_nw,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_n,              qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_ne,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_up,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_w,              qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_e,              qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_down,           qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_sw,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_s,              qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_se,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_in,             qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(weight_out,            qOverload<int>(&QSpinBox::valueChanged),        this, &dlgRoomExits::slot_checkModified);
    connect(doortype_nw,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_n,            qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_ne,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_up,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_w,            qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_e,            qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_down,         qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_sw,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_s,            qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_se,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_in,           qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_down,         qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    connect(doortype_out,          qOverload<int>(&QButtonGroup::buttonClicked),   this, &dlgRoomExits::slot_checkModified);
    // clang-format on
}

TExit* dlgRoomExits::makeExitFromControls(int direction)
{
    auto exit = new TExit();
    switch (direction) {
    case DIR_NORTHWEST:
        exit->destination = nw->text().toInt();
        exit->hasStub = stub_nw->isChecked();
        exit->hasNoRoute = noroute_nw->isChecked();
        exit->weight = weight_nw->value();
        exit->door = -2 - doortype_nw->checkedId();
        break;
    case DIR_NORTH:
        exit->destination = n->text().toInt();
        exit->hasStub = stub_n->isChecked();
        exit->hasNoRoute = noroute_n->isChecked();
        exit->weight = weight_n->value();
        exit->door = -2 - doortype_n->checkedId();
        break;
    case DIR_NORTHEAST:
        exit->destination = ne->text().toInt();
        exit->hasStub = stub_ne->isChecked();
        exit->hasNoRoute = noroute_ne->isChecked();
        exit->weight = weight_ne->value();
        exit->door = -2 - doortype_ne->checkedId();
        break;
    case DIR_UP:
        exit->destination = up->text().toInt();
        exit->hasStub = stub_up->isChecked();
        exit->hasNoRoute = noroute_up->isChecked();
        exit->weight = weight_up->value();
        exit->door = -2 - doortype_up->checkedId();
        break;
    case DIR_WEST:
        exit->destination = w->text().toInt();
        exit->hasStub = stub_w->isChecked();
        exit->hasNoRoute = noroute_w->isChecked();
        exit->weight = weight_w->value();
        exit->door = -2 - doortype_w->checkedId();
        break;
    case DIR_EAST:
        exit->destination = e->text().toInt();
        exit->hasStub = stub_e->isChecked();
        exit->hasNoRoute = noroute_e->isChecked();
        exit->weight = weight_e->value();
        exit->door = -2 - doortype_e->checkedId();
        break;
    case DIR_DOWN:
        exit->destination = down->text().toInt();
        exit->hasStub = stub_down->isChecked();
        exit->hasNoRoute = noroute_down->isChecked();
        exit->weight = weight_down->value();
        exit->door = -2 - doortype_down->checkedId();
        break;
    case DIR_SOUTHWEST:
        exit->destination = sw->text().toInt();
        exit->hasStub = stub_sw->isChecked();
        exit->hasNoRoute = noroute_sw->isChecked();
        exit->weight = weight_sw->value();
        exit->door = -2 - doortype_sw->checkedId();
        break;
    case DIR_SOUTH:
        exit->destination = s->text().toInt();
        exit->hasStub = stub_s->isChecked();
        exit->hasNoRoute = noroute_s->isChecked();
        exit->weight = weight_s->value();
        exit->door = -2 - doortype_s->checkedId();
        break;
    case DIR_SOUTHEAST:
        exit->destination = se->text().toInt();
        exit->hasStub = stub_se->isChecked();
        exit->hasNoRoute = noroute_se->isChecked();
        exit->weight = weight_se->value();
        exit->door = -2 - doortype_se->checkedId();
        break;
    case DIR_IN:
        exit->destination = in->text().toInt();
        exit->hasStub = stub_in->isChecked();
        exit->hasNoRoute = noroute_in->isChecked();
        exit->weight = weight_in->value();
        exit->door = -2 - doortype_in->checkedId();
        break;
    case DIR_OUT:
        exit->destination = out->text().toInt();
        exit->hasStub = stub_out->isChecked();
        exit->hasNoRoute = noroute_out->isChecked();
        exit->weight = weight_out->value();
        exit->door = -2 - doortype_out->checkedId();
        break;
    default:
        Q_UNREACHABLE();
    }

    return exit;
}

// Check and set modified marking in dialog title as soon as a change is detected
void dlgRoomExits::slot_checkModified()
{
    bool isModified = false;

    // Things to check:
    // exit stub / rooms
    // exit locks (noroute)
    // exit doors
    // exit weights

    TExit* originalExit = originalExits.value(DIR_NORTHWEST);
    TExit* currentExit = makeExitFromControls(DIR_NORTHWEST);

    if (originalExit && currentExit && *originalExit != *currentExit) {
        isModified = true;
    }
    delete currentExit;

    if (!isModified) {
        originalExit = originalExits.value(DIR_NORTH);
        currentExit = makeExitFromControls(DIR_NORTH);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_NORTHEAST);
        currentExit = makeExitFromControls(DIR_NORTHEAST);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_UP);
        currentExit = makeExitFromControls(DIR_UP);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_WEST);
        currentExit = makeExitFromControls(DIR_WEST);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_EAST);
        currentExit = makeExitFromControls(DIR_EAST);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_DOWN);
        currentExit = makeExitFromControls(DIR_DOWN);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_SOUTHWEST);
        currentExit = makeExitFromControls(DIR_SOUTHWEST);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_SOUTH);
        currentExit = makeExitFromControls(DIR_SOUTH);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_SOUTHEAST);
        currentExit = makeExitFromControls(DIR_SOUTHEAST);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_IN);
        currentExit = makeExitFromControls(DIR_IN);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    if (!isModified) {
        originalExit = originalExits.value(DIR_OUT);
        currentExit = makeExitFromControls(DIR_OUT);
        if (originalExit && currentExit && *originalExit != *currentExit) {
            isModified = true;
        }
        delete currentExit;
    }

    // Detecting actual changes in the special exits is hard because of the
    // potential presence of new exits which may not be yet valid and thus will
    // not actually alter things if "save" is hit.
    // At the same time existing special exits which now have a empty/zero
    // value in the first (0) field will be deleted if "save"ed...
    if (!isModified) {
        int originalCount = originalSpecialExits.count();
        int currentCount = 0;
        for (int i = 0; i < specialExits->topLevelItemCount(); i++) {
            QTreeWidgetItem* pI = specialExits->topLevelItem(i);
            /*            qDebug("dlgRoomExits::slot_checkModified() considering specialExit (item %i, pass 1) to:%i, command:%s",
 *                   i,
 *                   pI->text(0).toInt(),
 *                   qPrintable(pI->text(7)));
 */
            if (pI->text(7) == tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same") || pI->text(0).toInt() <= 0)
                continue; // Ignore new or to be deleted entries
            currentCount++;
        }
        if (originalCount != currentCount)
            isModified = true;
        else {
            if (originalCount) {
                QMap<QString, TExit*> foundMap = originalSpecialExits;
                // Now make a TExit value for each current (valid) specialExit
                // and search for it in the foundMap; remove matches and
                // if any non-matches or if any left in foundMap at end then
                // set isModified...
                for (int i = 0; i < specialExits->topLevelItemCount(); i++) {
                    QTreeWidgetItem* pI = specialExits->topLevelItem(i);
                    /*                    qDebug("dlgRoomExits::slot_checkModified() considering specialExit (item %i, pass 2) to:%i, command:%s",
 *                           i,
 *                           pI->text(0).toInt(),
 *                           qPrintable(pI->text(7)));
 */
                    if (pI->text(7) == tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same") || pI->text(0).toInt() <= 0)
                        continue; // Ignore new or to be deleted entries
                    QString currentCmd = pI->text(7);
                    TExit currentExit;
                    currentExit.destination = pI->text(0).toInt();
                    currentExit.hasNoRoute = pI->checkState(1) == Qt::Checked;
                    currentExit.door = pI->checkState(6) == Qt::Checked ? 3 : pI->checkState(5) == Qt::Checked ? 2 : pI->checkState(4) == Qt::Checked ? 1 : 0;
                    currentExit.weight = pI->text(2).toInt();
                    currentExit.hasStub = false;
                    auto exit = foundMap.value(currentCmd);
                    if (exit
                        && exit->destination == currentExit.destination
                        && exit->door        == currentExit.door
                        && exit->hasNoRoute  == currentExit.hasNoRoute
                        && exit->weight      == currentExit.weight      ) {
                        foundMap.remove(currentCmd);
                    } else {
                        isModified = true;
                        break;
                    }
                }
                if (foundMap.count())
                    isModified = true;
            }
        }
    }

    if (isWindowModified() != isModified) {
        // There has been a change in the "are there changes?" state
        setWindowModified(isModified);
        button_save->setEnabled(isModified);
#if defined(Q_OS_MACOS) && (QT_VERSION > 0x050903)
        // Fix: https://bugreports.qt.io/browse/QTBUG-68067 which seems to
        // effect macOs for Qt versions somewhere after Qt 5.9.3 and which we
        // do not have a fix version - yet!
        button_save->repaint();
#endif
    }
}
