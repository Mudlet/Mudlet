/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2021 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

// A couple of templates for tooltip HTML formatting so that we do not have
// 65/30 copies of the same QString s in the read-only segment of the code:
const QString singleParagraph{QStringLiteral("<p>%1</p>")};
const QString doubleParagraph{QStringLiteral("<p>%1</p><p>%2</p>")};

// Abstract all the indexes into the special exits treewidget so that we can
// tweak them and change all of them correctly:
const int colIndex_exitRoomId = 0;
const int colIndex_exitStatus = 1;
const int colIndex_lockExit = 2;
const int colIndex_exitWeight = 3;
const int colIndex_doorNone = 4;
const int colIndex_doorOpen = 5;
const int colIndex_doorClosed = 6;
const int colIndex_doorLocked = 7;
const int colIndex_command = 8;

WeightSpinBoxDelegate::WeightSpinBoxDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QWidget* WeightSpinBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /* option */, const QModelIndex& /* index */) const
{
    auto* pEditor = new QSpinBox(parent);
    pEditor->setFrame(false);
    pEditor->setMinimum(0);
    pEditor->setMaximum(9999);

    return pEditor;
}

void WeightSpinBoxDelegate::setEditorData(QWidget* pEditor, const QModelIndex& index) const
{
    auto value = index.model()->data(index, Qt::EditRole).toInt();
    auto* pSpinBox = static_cast<QSpinBox*>(pEditor);
    pSpinBox->setValue(value);
}

void WeightSpinBoxDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* pSpinBox = static_cast<QSpinBox*>(pEditor);
    pSpinBox->interpretText();
    auto value = pSpinBox->value();
    model->setData(index, value, Qt::EditRole);
}

void WeightSpinBoxDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& /* index */) const
{
    pEditor->setGeometry(option.rect);
}

RoomIdLineEditDelegate::RoomIdLineEditDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QWidget* RoomIdLineEditDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /* option */, const QModelIndex& index) const
{
    // Work our way up through the ancestors to get to the one that is the
    // dlgRoomExit pointer:
    auto* pSpecialExits = qobject_cast<ExitsTreeWidget*>(parent->parent());
    // This should end up pointing to the item that contains the roomIdLineEdit
    // that is being edited - we need it so as to be able to modify other
    // elements of the same item. We cannot access them via index.sibling(...)
    // as that returns const values which cannot thus be modified (and that is
    // to prevent changes which might modify index itself) - basically be
    // careful not to modify the roomID element that is contained in what mpItem
    // points to!
    if (pSpecialExits) {
        auto* pGroupBox_specialExits = qobject_cast<QGroupBox*>(pSpecialExits->parent());
        if (pGroupBox_specialExits) {
            mpDlgRoomExits = qobject_cast<dlgRoomExits*>(pGroupBox_specialExits->parent());
        }
        mpItem = pSpecialExits->topLevelItem(index.row());
    }
    mpEditor = new QLineEdit(parent);
    mpEditor->setFrame(false);
    if (mpDlgRoomExits) {
        if (!mpHost) {
            mpHost = mpDlgRoomExits->getHost();
        }
        if (!mAreaID) {
            mAreaID = mpDlgRoomExits->getAreaID();
        }
        if (mpHost) {
            // Need to set the status icon on the QLineEdit on the opening of
            // the editor otherwise it will not be shown until the user edits it:
            auto text = index.model()->data(index, Qt::EditRole).toString();
            TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());
            if (exitToRoom) {
                // Valid exit roomID in place:
                int exitAreaID = exitToRoom->getArea();
                bool outOfAreaExit = (exitAreaID && exitAreaID != mAreaID);
                QString exitAreaName;
                const QString roomIdToolTipText{mpDlgRoomExits->generateToolTip(exitToRoom->name, exitAreaName, outOfAreaExit, exitToRoom->getWeight())};
                mpDlgRoomExits->setActionOnExit(mpEditor, outOfAreaExit ? mpDlgRoomExits->mpAction_otherAreaExit : mpDlgRoomExits->mpAction_inAreaExit);
                // Set the tooltip for the QLineEdit:
                mpEditor->setToolTip(roomIdToolTipText);
                if (mpItem) {
                    auto commandModelIndex = index.sibling(index.row(), colIndex_command);
                    if (commandModelIndex.isValid() && commandModelIndex.model()->data(commandModelIndex, Qt::EditRole).toString() == mpDlgRoomExits->mSpecialExitCommandPlaceholder) {
                        const QString commandToolTipText = singleParagraph.arg(tr("No command or Lua script entered, if left like this, this exit will be deleted when <tt>save</tt> is clicked."));
                        mpItem->setToolTip(colIndex_exitStatus, commandToolTipText);
                        mpItem->setToolTip(colIndex_command, commandToolTipText);
                    } else {
                        mpItem->setToolTip(colIndex_exitStatus, roomIdToolTipText);
                        mpItem->setToolTip(colIndex_command, QString());
                    }
                }
            } else if (!text.isEmpty()) {
                // Something is entered but it does not yield a valid exit roomID:
                mpDlgRoomExits->setActionOnExit(mpEditor, mpDlgRoomExits->mpAction_invalidExit);
                mpEditor->setToolTip(singleParagraph.arg(tr("Entered number is invalid, set the number of the room that this special exit goes to.")));
            } else {
                // Nothing is entered:
                mpDlgRoomExits->setActionOnExit(mpEditor, mpDlgRoomExits->mpAction_noExit);
                mpEditor->setToolTip(singleParagraph.arg(tr("Set the number of the room that this special exit goes to.")));
            }
        }
    }

    connect(mpEditor, &QLineEdit::textEdited, this, &RoomIdLineEditDelegate::slot_specialRoomExitEdited);
    return mpEditor;
}

void RoomIdLineEditDelegate::setEditorData(QWidget* pEditor, const QModelIndex& index) const
{
    auto value = index.model()->data(index, Qt::EditRole).toString();
    auto* pLineEdit = static_cast<QLineEdit*>(pEditor);
    pLineEdit->setText(value);
}

void RoomIdLineEditDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* pLineEdit = static_cast<QLineEdit*>(pEditor);
    auto value = pLineEdit->text();
    model->setData(index, value, Qt::EditRole);
}

void RoomIdLineEditDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& /* index */) const
{
    pEditor->setGeometry(option.rect);
}

void RoomIdLineEditDelegate::slot_specialRoomExitEdited(const QString& text) const
{
    qDebug().nospace().noquote() << "RoomIdLineEditDelegate::slot_specialRoomExitEdited(\"" << text << "\") INFO - called with given text.";
    // We do not set an icon in the status column whilst we are editing the
    // exit room ID.

    if (!mpHost || !mpItem || !mAreaID) {
        return;
    }
    // The following code is a variation of that of
    // dlgRoomExits::setIconAndToolTipsOnSpecialExit(...) :
    TRoom* pExitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());
    QString roomIdToolTipText;
    if (pExitToRoom) {
        // A valid exit roomID number:
        int exitAreaID = pExitToRoom->getArea();
        bool outOfAreaExit = (exitAreaID && exitAreaID != mAreaID);
        QString exitAreaName;
        if (outOfAreaExit) {
            exitAreaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(exitAreaID);
        }

        // This is the toolTip text for the room ID number column (and the
        // status icon - but only for the latter if there is a valid command as
        // well):
        roomIdToolTipText = mpDlgRoomExits->generateToolTip(pExitToRoom->name, exitAreaName, outOfAreaExit, pExitToRoom->getWeight());
        if (mpItem->text(colIndex_command) == mpDlgRoomExits->mSpecialExitCommandPlaceholder) {
            const QString commandToolTipText{singleParagraph.arg(tr("No command or Lua script entered, if left like this, this exit will be deleted when <tt>save</tt> is clicked."))};
            mpItem->setToolTip(colIndex_exitStatus, commandToolTipText);
            mpItem->setToolTip(colIndex_command, commandToolTipText);
        } else {
            mpItem->setToolTip(colIndex_exitStatus, roomIdToolTipText);
            mpItem->setToolTip(colIndex_command, QString());
        }
        mpDlgRoomExits->setActionOnExit(mpEditor, outOfAreaExit ? mpDlgRoomExits->mpAction_otherAreaExit : mpDlgRoomExits->mpAction_inAreaExit);
    } else if (text.toInt() > 0) {
        // A number but not valid
        mpDlgRoomExits->setActionOnExit(mpEditor, mpDlgRoomExits->mpAction_invalidExit);
        roomIdToolTipText = singleParagraph.arg(tr("Entered number is invalid, set the number of the room that this special exit leads to; "
                                                   "if left like this, this exit will be deleted when <tt>save</tt> is clicked."));
        mpItem->setToolTip(colIndex_exitStatus, roomIdToolTipText);
    } else if (!text.isEmpty()) {
        // Something else that isn't a positive number
        mpDlgRoomExits->setActionOnExit(mpEditor, mpDlgRoomExits->mpAction_invalidExit);
        roomIdToolTipText = singleParagraph.arg(tr("A positive room ID of the room that this special exit leads to is expected here; "
                                                   "if left like this, this exit will be deleted when <tt>save</tt> is clicked."));
        mpItem->setToolTip(colIndex_exitStatus, roomIdToolTipText);
    } else {
        // Nothing is entered:
        mpDlgRoomExits->setActionOnExit(mpEditor, mpDlgRoomExits->mpAction_noExit);
        roomIdToolTipText = singleParagraph.arg(tr("Set the number of the room that this special exit goes to."));
        mpItem->setToolTip(colIndex_exitStatus, roomIdToolTipText);
    }
    mpEditor->setToolTip(roomIdToolTipText);
}

dlgRoomExits::dlgRoomExits(Host* pH, const int roomNumber, QWidget* pW)
: QDialog(pW)
, mpHost(pH)
, mRoomID(roomNumber)
{
    setupUi(this);

    mIcon_invalidExit.addFile(QStringLiteral(":/icons/dialog-error.png"), QSize(24, 24));
    mIcon_inAreaExit.addFile(QStringLiteral(":/icons/dialog-ok-apply.png"), QSize(24, 24));
    mIcon_otherAreaExit.addFile(QStringLiteral(":/icons/dialog-warning.png"), QSize(24, 24));

    mpAction_noExit = new QAction(this);
    mpAction_noExit->setText(QString());

    mpAction_invalidExit = new QAction(this);
    mpAction_invalidExit->setText(QString());
    mpAction_invalidExit->setToolTip(QString());
    mpAction_invalidExit->setIcon(mIcon_invalidExit);

    mpAction_inAreaExit = new QAction(this);
    mpAction_inAreaExit->setText(QString());
    mpAction_inAreaExit->setToolTip(QString());
    mpAction_inAreaExit->setIcon(mIcon_inAreaExit);

    mpAction_otherAreaExit = new QAction(this);
    mpAction_otherAreaExit->setText(QString());
    mpAction_otherAreaExit->setToolTip(QString());
    mpAction_otherAreaExit->setIcon(mIcon_otherAreaExit);

    mAllExitActionsSet << mpAction_noExit << mpAction_invalidExit << mpAction_inAreaExit << mpAction_otherAreaExit;
    mSpecialExitRoomIdPlaceholder = tr("(room ID)", "Placeholder, if no room ID is set for an exit, yet. This string is used programmatically in several places but is now defined only once so no need to check for multiple copies of it! 8-)");
    mSpecialExitCommandPlaceholder = tr("(command or Lua script)", "Placeholder, if a special exit has no code given, yet. This string is used programmatically in several places but is now defined only once so no need to check for multiple copies of it! 8-)");

    init();

    // Can't really use this now as the delegate for the exit weight is much
    // larger than the text shown normally when there isn't a "persistant
    // editor" active:
    // specialExits->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    specialExits->setItemDelegateForColumn(colIndex_exitRoomId, new RoomIdLineEditDelegate);
    specialExits->setItemDelegateForColumn(colIndex_exitWeight, new WeightSpinBoxDelegate);
}

dlgRoomExits::~dlgRoomExits()
{
    delete mpAction_otherAreaExit;
    delete mpAction_inAreaExit;
    delete mpAction_invalidExit;
    delete mpAction_noExit;
}

void dlgRoomExits::slot_endEditSpecialExits()
{
    button_endEditing->setDisabled(true);
    if (!button_addSpecialExit->isEnabled()) {
        button_addSpecialExit->setEnabled(true);
    }
    if (mpEditItem != nullptr && mEditColumn >= 0) {
        specialExits->closePersistentEditor(mpEditItem, mEditColumn);
        setIconAndToolTipsOnSpecialExit(mpEditItem, true);
        mpEditItem = nullptr;
        mEditColumn = -1;
    }
    specialExits->clearSelection();
}

void dlgRoomExits::slot_editSpecialExit(QTreeWidgetItem* pI, int column)
{
    if (!button_endEditing->isEnabled()) {
        button_endEditing->setEnabled(true);
    }
    if (button_addSpecialExit->isEnabled()) {
        button_addSpecialExit->setEnabled(false);
    }

    if (mpEditItem != nullptr && (pI != mpEditItem || mEditColumn != column)) {
        // Thing that was clicked on is not the same as last thing that was clicked on
        // ... so clean up the old column
        switch (mEditColumn) {
        case colIndex_exitRoomId:
            if (mpEditItem->text(colIndex_exitRoomId).toInt() < 1) {
                mpEditItem->setText(colIndex_exitRoomId, mSpecialExitRoomIdPlaceholder);
            }
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            setIconAndToolTipsOnSpecialExit(mpEditItem, true);
            break;

        case colIndex_exitWeight:
            mpEditItem->setData(colIndex_exitWeight, Qt::EditRole,
                                abs(mpEditItem->data(colIndex_exitWeight, Qt::EditRole).toInt()));
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            break;

        case colIndex_doorNone: // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
            mpEditItem->setCheckState(colIndex_doorNone, Qt::Checked);
            mpEditItem->setCheckState(colIndex_doorOpen, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorClosed, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorLocked, Qt::Unchecked);
            break;

        case colIndex_doorOpen:
            mpEditItem->setCheckState(colIndex_doorNone, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorOpen, Qt::Checked);
            mpEditItem->setCheckState(colIndex_doorClosed, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorLocked, Qt::Unchecked);
            break;

        case colIndex_doorClosed:
            mpEditItem->setCheckState(colIndex_doorNone, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorOpen, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorClosed, Qt::Checked);
            mpEditItem->setCheckState(colIndex_doorLocked, Qt::Unchecked);
            break;

        case colIndex_doorLocked:
            mpEditItem->setCheckState(colIndex_doorNone, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorOpen, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorClosed, Qt::Unchecked);
            mpEditItem->setCheckState(colIndex_doorLocked, Qt::Checked);
            break;

        case colIndex_command:
            if (!mpEditItem->text(colIndex_command).trimmed().length()) {
                mpEditItem->setText(colIndex_command, mSpecialExitCommandPlaceholder);
            }
            specialExits->closePersistentEditor(mpEditItem, mEditColumn);
            //            qDebug() << "Closed PE on item:" << mpEditItem->text(colIndex_command) << "column:" << mEditColumn;
            break;
        default:
            {} //noop for other column (colIndex_exitStatus & colIndex_lockExit)
        }

        setIconAndToolTipsOnSpecialExit(mpEditItem, true);

        mpEditItem = nullptr; //This will cause a new PE to be opened, it will also be zeroed on the first time this function is called
        mEditColumn = -1;
    }

    // Now process the new column that was selected:
    if (mpEditItem == nullptr) {
        if (column == colIndex_exitRoomId || column == colIndex_exitWeight || column == colIndex_command) {
            //            qDebug() << "Opened PE on item:" << pI->text(colIndex_command) << "column:" << column;
            specialExits->openPersistentEditor(pI, column);
            specialExits->editItem(pI, column);
            if (column == colIndex_exitRoomId) {
                setIconAndToolTipsOnSpecialExit(pI, false);
                // Hide the Edit Status icon (as the status will be replicated
                // and adjusted within the Exit Room ID column's QLineEdit)
                // whilst the value is being edited:
                // pI->setIcon(colIndex_exitStatus, QIcon());
            }
        }
        mpEditItem = pI;
        mEditColumn = column;
    }

    //    qDebug() << "A Special Exit is been edited, it has the command:" << pI->text(colIndex_command) << "and the editing is on column " << column;
    switch (column) {
    case colIndex_exitWeight:
        pI->setData(colIndex_exitWeight, Qt::EditRole, abs(pI->data(colIndex_exitWeight, Qt::EditRole).toInt())); //Force result to be non-negative
        break;

    case colIndex_doorNone: // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
        pI->setCheckState(colIndex_doorNone, Qt::Checked);
        pI->setCheckState(colIndex_doorOpen, Qt::Unchecked);
        pI->setCheckState(colIndex_doorClosed, Qt::Unchecked);
        pI->setCheckState(colIndex_doorLocked, Qt::Unchecked);
        break;

    case colIndex_doorOpen:
        pI->setCheckState(colIndex_doorNone, Qt::Unchecked);
        pI->setCheckState(colIndex_doorOpen, Qt::Checked);
        pI->setCheckState(colIndex_doorClosed, Qt::Unchecked);
        pI->setCheckState(colIndex_doorLocked, Qt::Unchecked);
        break;

    case colIndex_doorClosed:
        pI->setCheckState(colIndex_doorNone, Qt::Unchecked);
        pI->setCheckState(colIndex_doorOpen, Qt::Unchecked);
        pI->setCheckState(colIndex_doorClosed, Qt::Checked);
        pI->setCheckState(colIndex_doorLocked, Qt::Unchecked);
        break;

    case colIndex_doorLocked:
        pI->setCheckState(colIndex_doorNone, Qt::Unchecked);
        pI->setCheckState(colIndex_doorOpen, Qt::Unchecked);
        pI->setCheckState(colIndex_doorClosed, Qt::Unchecked);
        pI->setCheckState(colIndex_doorLocked, Qt::Checked);
        break;

    default:; //noop for other columns?
    }
}

void dlgRoomExits::slot_addSpecialExit()
{
    auto pI = new QTreeWidgetItem(specialExits);
    pI->setIcon(colIndex_exitStatus, QIcon());
    pI->setText(colIndex_exitRoomId, mSpecialExitRoomIdPlaceholder); //Exit RoomID
    pI->setToolTip(colIndex_exitRoomId, singleParagraph.arg(tr("Set the number of the room that this special exit leads to; if left like "
                                                               "this, this exit will be deleted when <tt>save</tt> is clicked.")));
    pI->setTextAlignment(colIndex_exitRoomId, Qt::AlignRight);
    pI->setToolTip(colIndex_lockExit, singleParagraph.arg(tr("Prevent a route being created via this exit, equivalent to an infinite exit weight.")));
    pI->setCheckState(colIndex_lockExit, Qt::Unchecked); //Locked
    pI->setData(colIndex_exitWeight, Qt::EditRole, 0); //Exit Weight
    // pI->setTextAlignment(colIndex_exitWeight, Qt::AlignRight);
    pI->setToolTip(colIndex_exitWeight, singleParagraph.arg(tr("Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.")));
    pI->setCheckState(colIndex_doorNone, Qt::Checked); //Doortype: none
    pI->setToolTip(colIndex_doorNone, singleParagraph.arg(tr("No door symbol is drawn on 2D Map for this exit (only functional choice currently).")));
    pI->setCheckState(colIndex_doorOpen, Qt::Unchecked); //Doortype: open
    pI->setToolTip(colIndex_doorOpen, singleParagraph.arg(tr("Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setCheckState(colIndex_doorClosed, Qt::Unchecked); //Doortype: closed
    pI->setToolTip(colIndex_doorClosed, singleParagraph.arg(tr("Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setCheckState(colIndex_doorLocked, Qt::Unchecked); //Doortype: locked
    pI->setToolTip(colIndex_doorLocked, singleParagraph.arg(tr("Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
    pI->setText(colIndex_command, mSpecialExitCommandPlaceholder); //Exit command
    pI->setTextAlignment(colIndex_command, Qt::AlignLeft);
    specialExits->addTopLevelItem(pI);
}

void dlgRoomExits::save()
{
    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    if (!pR) {
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<QString> originalExitCmdsList{pR->getSpecialExits().keys()};
    QSet<QString> originalExitCmds{originalExitCmdsList.begin(), originalExitCmdsList.end()};
#else
    QSet<QString> originalExitCmds = pR->getSpecialExits().keys().toSet();
#endif

    for (int i = 0; i < specialExits->topLevelItemCount(); ++i) {
        QTreeWidgetItem* pI = specialExits->topLevelItem(i);
        int value = pI->text(colIndex_exitRoomId).toInt();
        int weight = pI->data(colIndex_exitWeight, Qt::EditRole).toInt();
        int door = 0;
        bool locked = false;
        if (pI->checkState(colIndex_doorLocked) == Qt::Checked) {
            door = 3;
        } else if (pI->checkState(colIndex_doorClosed) == Qt::Checked) {
            door = 2;
        } else if (pI->checkState(colIndex_doorOpen) == Qt::Checked) {
            door = 1;
        } else {
            door = 0;
        }
        QString key = pI->text(colIndex_command);
        if (key != mSpecialExitCommandPlaceholder
            && value != 0 && mpHost->mpMap->mpRoomDB->getRoom(value) != nullptr) {
            originalExitCmds.remove(key);
            locked = (pI->checkState(colIndex_lockExit) != Qt::Unchecked);
            pR->setSpecialExit(value, key); // Now can overwrite an existing exit with a different destination
            pR->setSpecialExitLock(key, locked);
            if (pR->hasExitWeight(key) || weight > 0) {
                pR->setExitWeight(key, weight);
            }
            if (pR->getDoor(key) || door > 0) {
                pR->setDoor(key, door);
            }
        }
    }

    // Clean up after any deleted specialExits originally present but not now so
    for (auto& value : originalExitCmds) {
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
    auto pExit = originalExits.value(dirCode);
    if (nw->isEnabled() && !nw->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != nullptr) {
        // There IS a valid exit on the dialogue in this direction
        if (pExit && pExit->destination != nw->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
            pR->setExit(-1, dirCode); // Destination has been deleted so ensure the value for no exit is stored
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
    pExit = originalExits.value(dirCode);
    if (n->isEnabled() && !n->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != n->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (ne->isEnabled() && !ne->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != ne->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (up->isEnabled() && !up->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != up->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (w->isEnabled() && !w->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != w->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (e->isEnabled() && !e->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != e->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (down->isEnabled() && !down->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != down->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (sw->isEnabled() && !sw->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != sw->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (s->isEnabled() && !s->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != s->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (se->isEnabled() && !se->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != se->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (in->isEnabled() && !in->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != in->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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
    pExit = originalExits.value(dirCode);
    if (out->isEnabled() && !out->text().isEmpty() && mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != nullptr) {
        if (pExit && pExit->destination != out->text().toInt()) {
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
        if (pExit && pExit->destination > 0) {
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

void dlgRoomExits::setIconAndToolTipsOnSpecialExit(QTreeWidgetItem* pSpecialExit, const bool showIconOnExitStatus)
{
    TRoom* pExitToRoom = mpHost->mpMap->mpRoomDB->getRoom(pSpecialExit->text(colIndex_exitRoomId).toInt());
    if (pExitToRoom) {
        // A valid exit roomID number:
        int exitAreaID = pExitToRoom->getArea();
        bool outOfAreaExit = (exitAreaID && exitAreaID != mAreaID);
        QString exitAreaName;
        if (outOfAreaExit) {
            exitAreaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(exitAreaID);
        }

        // This is the toolTip text for the room ID number column (and the
        // status icon - but only for the latter if there is a valid command as
        // well):
        const QString roomIdToolTipText{generateToolTip(pExitToRoom->name, exitAreaName, outOfAreaExit, pExitToRoom->getWeight())};
        if (pSpecialExit->text(colIndex_command) == mSpecialExitCommandPlaceholder) {
            const QString commandToolTipText{singleParagraph.arg(tr("No command or Lua script entered, if left like this, this exit will be deleted when <tt>save</tt> is clicked."))};
            pSpecialExit->setIcon(colIndex_exitStatus, showIconOnExitStatus ? mIcon_invalidExit : QIcon());
            pSpecialExit->setToolTip(colIndex_exitStatus, commandToolTipText);
            pSpecialExit->setToolTip(colIndex_command, commandToolTipText);
        } else {
            pSpecialExit->setIcon(colIndex_exitStatus, showIconOnExitStatus ? (outOfAreaExit ? mIcon_otherAreaExit : mIcon_inAreaExit) : QIcon());
            pSpecialExit->setToolTip(colIndex_exitStatus, roomIdToolTipText);
            pSpecialExit->setToolTip(colIndex_command, QString());
        }
        pSpecialExit->setToolTip(colIndex_exitRoomId, roomIdToolTipText);

    } else if (pSpecialExit->text(colIndex_exitRoomId).toInt() > 0) {
        // A number but not valid
        pSpecialExit->setIcon(colIndex_exitStatus, showIconOnExitStatus ? mIcon_invalidExit : QIcon());
        pSpecialExit->setToolTip(colIndex_exitRoomId, singleParagraph.arg(tr("Entered number is invalid, set the number of the room that this special exit leads to; "
                                                                           "if left like this, this exit will be deleted when <tt>save</tt> is clicked.")));
    } else if (!pSpecialExit->text(colIndex_exitRoomId).isEmpty()) {
        // Something else that isn't a positive number
        pSpecialExit->setIcon(colIndex_exitStatus, showIconOnExitStatus ? mIcon_invalidExit : QIcon());
        pSpecialExit->setToolTip(colIndex_exitRoomId, singleParagraph.arg(tr("A positive room ID of the room that this special exit leads to is expected here; "
                                                                             "if left like this, this exit will be deleted when <tt>save</tt> is clicked.")));
    }
}

void dlgRoomExits::setActionOnExit(QLineEdit* pExitLineEdit, QAction* pWantedAction) const
{
    auto pActions = pExitLineEdit->actions();
    // In fact there should only be one action but this code is flexible enough
    // to deal with there being other unrealated ones also present:
    bool found = false;
    for (int index = 0, total = pActions.count(); index < total; ++index) {
        auto pAction = pActions[index];
        if (pAction && mAllExitActionsSet.contains(pAction)) {
            // This is one of the four we are looking for.
            if (pAction != pWantedAction) {
                // but it isn't the one we want - so remove it from the QLineEdit
                pExitLineEdit->removeAction(pAction);
            } else {
                // it is already there:
                found = true;
            }
        }
    }
    if (!found) {
        // It wasn't there so add it - so that the position is similar for the
        // special exit form put it to the right of the QLineEdit text:
        pExitLineEdit->addAction(pWantedAction, QLineEdit::TrailingPosition);
    }
}

// Check for and return the (first one of the) QAction's used for the status
// icon on the ExitRoomID - this is actually only used for the special exits:
QAction* dlgRoomExits::getActionOnExit(QLineEdit* pExitLineEdit) const
{
    auto pActions = pExitLineEdit->actions();
    for (int index = 0, total = pActions.count(); index < total; ++index) {
        auto pAction = pActions[index];
        if (pAction && mAllExitActionsSet.contains(pAction)) {
            // This is one of the four we are looking for.
            return pAction;
        }
    }
    // Handle the not found case:
    return nullptr;
}

/* static */ QString dlgRoomExits::generateToolTip(const QString& exitRoomName, const QString& exitAreaName, const bool outOfAreaExit, const int exitRoomWeight)
{
    if (exitRoomName.trimmed().length()) {
        if (outOfAreaExit) {
            return doubleParagraph.arg(tr("Exit to \"%1\" in area: \"%2\".")
                                           .arg(exitRoomName, exitAreaName),
                                       tr("<b>Room</b> Weight of destination: %1.",
                                          // Intentional comment to separate arguments
                                          "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                           .arg(exitRoomWeight));
        }
        return doubleParagraph.arg(tr("Exit to \"%1\".")
                                       .arg(exitRoomName),
                                   tr("<b>Room</b> Weight of destination: %1.",
                                      // Intentional comment to separate arguments
                                      "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                       .arg(exitRoomWeight));
    }

    if (outOfAreaExit) {
        return doubleParagraph.arg(tr("Exit to unnamed room in area: \"%1\", is valid.")
                                       .arg(exitAreaName),
                                   tr("<b>Room</b> Weight of destination: %1.",
                                      // Intentional comment to separate arguments
                                      "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                       .arg(exitRoomWeight));
    }

    return doubleParagraph.arg(tr("Exit to unnamed room is valid."),
                               tr("<b>Room</b> Weight of destination: %1.",
                                  // Intentional comment to separate arguments
                                  "Bold HTML tags are used to emphasis that the value is destination room's weight whether overridden by a non-zero exit weight here or not.")
                                     .arg(exitRoomWeight));
}

void dlgRoomExits::normalExitEdited(const QString& roomExitIdText, QLineEdit* pExit, QCheckBox* pNoRoute, QCheckBox* pStub, QSpinBox* pWeight, QRadioButton* pDoorType_none, QRadioButton* pDoorType_open, QRadioButton* pDoorType_closed, QRadioButton* pDoorType_locked, const QString& invalidExitToolTipText, const QString& noExitToolTipText)
{
    TRoom* exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(roomExitIdText.toInt());
    if (exitToRoom) {
        int exitAreaID = exitToRoom->getArea();
        bool outOfAreaExit = (exitAreaID && exitAreaID != mAreaID);
        QString exitAreaName;
        if (outOfAreaExit) {
            exitAreaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(exitAreaID);
        }
        // Valid exit roomID in place
        pStub->setChecked(false);
        pStub->setEnabled(false);
        pNoRoute->setEnabled(true);
        pWeight->setEnabled(true);
        pDoorType_none->setEnabled(true);
        pDoorType_open->setEnabled(true);
        pDoorType_closed->setEnabled(true);
        pDoorType_locked->setEnabled(true);
        setActionOnExit(pExit, outOfAreaExit ? mpAction_otherAreaExit : mpAction_inAreaExit);
        pExit->setToolTip(generateToolTip(exitToRoom->name, exitAreaName, outOfAreaExit, exitToRoom->getWeight()));
    } else if (!roomExitIdText.isEmpty()) {
        // Something is entered but it does not yield a valid exit roomID
        // Enable stub exit control
        setActionOnExit(pExit, mpAction_invalidExit);
        pExit->setToolTip(invalidExitToolTipText);
        pStub->setEnabled(true);
        pNoRoute->setEnabled(false);
        pWeight->setEnabled(false);
        pDoorType_none->setEnabled(false);
        pDoorType_open->setEnabled(false);
        pDoorType_closed->setEnabled(false);
        pDoorType_locked->setEnabled(false);
    } else {
        // Nothing is entered - so we can enable the stub exit control
        setActionOnExit(pExit, mpAction_noExit);
        pExit->setToolTip(noExitToolTipText);
        pStub->setEnabled(true);
        pNoRoute->setEnabled(false);
        pWeight->setEnabled(false);
        pDoorType_none->setEnabled(false);
        pDoorType_open->setEnabled(false);
        pDoorType_closed->setEnabled(false);
        pDoorType_locked->setEnabled(false);
    }
}

void dlgRoomExits::normalStubExitChanged(const int state, QLineEdit* pExit, QCheckBox* pNoRoute, QSpinBox* pWeight,
                                         QRadioButton* pDoorType_none, QRadioButton* pDoorType_open, QRadioButton* pDoorType_closed, QRadioButton* pDoorType_locked, const QString& noExitToolTipText)
{
    if (state == Qt::Checked) {
        if (!pExit->text().isEmpty()) {
            // There might be some text that does not evaluate to a valid Room
            // Id still in that field - so clear it:
            pExit->setText(QString());
            setActionOnExit(pExit, mpAction_noExit);
            pWeight->setValue(0);        // Can't have a weight for a stub pExit
            pNoRoute->setChecked(false); // nor a "lock"
        }
        pNoRoute->setEnabled(false); // Disable "lock" on this exit
        pExit->setEnabled(false);         // Prevent entry of an exit roomID
        pExit->setToolTip(singleParagraph.arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
        pDoorType_none->setEnabled(true);
        pDoorType_open->setEnabled(true);
        pDoorType_closed->setEnabled(true);
        pDoorType_locked->setEnabled(true); // Permit a door to be set on a stub exit
        pWeight->setEnabled(false);         // Prevent a weight to be set/changed on a stub
    } else {
        pExit->setEnabled(true);
        setActionOnExit(pExit, mpAction_noExit);
        pExit->setToolTip(noExitToolTipText);
        //  noroute_nw->setEnabled(true); although this branch will enable the exit entry
        //  there will not be a valid one there yet so don't enable the noroute(lock) control here!
        pDoorType_none->setEnabled(false);
        pDoorType_open->setEnabled(false);
        pDoorType_closed->setEnabled(false);
        pDoorType_locked->setEnabled(false);
        pDoorType_none->setChecked(true);
        //  similarly as there won't be a valid exit or a stub exit at theis point disable/reset the door type controls
        pWeight->setEnabled(false);
        pWeight->setValue(0); // Prevent a weight to be set/changed on a also
    }
}

// These slots are called as the text for the exitID is edited
void dlgRoomExits::slot_nw_textEdited(const QString& text)
{
    normalExitEdited(text, nw, noroute_nw, stub_nw, weight_nw,
                     doortype_none_nw, doortype_open_nw, doortype_closed_nw, doortype_locked_nw,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room northwest of this one.")),
                     singleParagraph.arg(tr("Set the number of the room northwest of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_n_textEdited(const QString& text)
{
    normalExitEdited(text, n, noroute_n, stub_n, weight_n,
                     doortype_none_n, doortype_open_n, doortype_closed_n, doortype_locked_n,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room north of this one.")),
                     singleParagraph.arg(tr("Set the number of the room north of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_ne_textEdited(const QString& text)
{
    normalExitEdited(text, ne, noroute_ne, stub_ne, weight_ne,
                     doortype_none_ne, doortype_open_ne, doortype_closed_ne, doortype_locked_ne,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room northeast of this one.")),
                     singleParagraph.arg(tr("Set the number of the room northeast of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_up_textEdited(const QString& text)
{
    normalExitEdited(text, up, noroute_up, stub_up, weight_up,
                     doortype_none_up, doortype_open_up, doortype_closed_up, doortype_locked_up,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room up from this one.")),
                     singleParagraph.arg(tr("Set the number of the room up from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_w_textEdited(const QString& text)
{
    normalExitEdited(text, w, noroute_w, stub_w, weight_w,
                     doortype_none_w, doortype_open_w, doortype_closed_w, doortype_locked_w,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room west of this one.")),
                     singleParagraph.arg(tr("Set the number of the room west of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_e_textEdited(const QString& text)
{
    normalExitEdited(text, e, noroute_e, stub_e, weight_e,
                     doortype_none_e, doortype_open_e, doortype_closed_e, doortype_locked_e,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room east of this one.")),
                     singleParagraph.arg(tr("Set the number of the room east of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_down_textEdited(const QString& text)
{
    normalExitEdited(text, down, noroute_down, stub_down, weight_down,
                     doortype_none_down, doortype_open_down, doortype_closed_down, doortype_locked_down,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room down from this one.")),
                     singleParagraph.arg(tr("Set the number of the room down from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_sw_textEdited(const QString& text)
{
    normalExitEdited(text, sw, noroute_sw, stub_sw, weight_sw,
                     doortype_none_sw, doortype_open_sw, doortype_closed_sw, doortype_locked_sw,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room southwest of this one.")),
                     singleParagraph.arg(tr("Set the number of the room southwest of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_s_textEdited(const QString& text)
{
    normalExitEdited(text, s, noroute_s, stub_s, weight_s,
                     doortype_none_s, doortype_open_s, doortype_closed_s, doortype_locked_s,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room south of this one.")),
                     singleParagraph.arg(tr("Set the number of the room south of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_se_textEdited(const QString& text)
{
    normalExitEdited(text, se, noroute_se, stub_se, weight_se,
                     doortype_none_se, doortype_open_se, doortype_closed_se, doortype_locked_se,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room southeast of this one.")),
                     singleParagraph.arg(tr("Set the number of the room southeast of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_in_textEdited(const QString& text)
{
    normalExitEdited(text, in, noroute_in, stub_in, weight_in,
                     doortype_none_in, doortype_open_in, doortype_closed_in, doortype_locked_in,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room in from this one.")),
                     singleParagraph.arg(tr("Set the number of the room in from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_out_textEdited(const QString& text)
{
    normalExitEdited(text, out, noroute_out, stub_out, weight_out,
                     doortype_none_out, doortype_open_out, doortype_closed_out, doortype_locked_out,
                     singleParagraph.arg(tr("Entered number is invalid, set the number of the room out from this one.")),
                     singleParagraph.arg(tr("Set the number of the room out from this one.")));
    slot_checkModified();
}

// These slots are called as the stub exit checkboxes are clicked
void dlgRoomExits::slot_stub_nw_stateChanged(int state)
{
    normalStubExitChanged(state, nw, noroute_nw, weight_nw,
                          doortype_none_nw, doortype_open_nw, doortype_closed_nw, doortype_locked_n,
                          singleParagraph.arg(tr("Set the number of the room northwest of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_n_stateChanged(int state)
{
    normalStubExitChanged(state, n, noroute_n, weight_n,
                          doortype_none_n, doortype_open_n, doortype_closed_n, doortype_locked_n,
                          singleParagraph.arg(tr("Set the number of the room north of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_ne_stateChanged(int state)
{
    normalStubExitChanged(state, ne, noroute_ne, weight_ne,
                          doortype_none_ne, doortype_open_ne, doortype_closed_ne, doortype_locked_ne,
                          singleParagraph.arg(tr("Set the number of the room northeast of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_up_stateChanged(int state)
{
    normalStubExitChanged(state, up, noroute_up, weight_up,
                          doortype_none_up, doortype_open_up, doortype_closed_up, doortype_locked_up,
                          singleParagraph.arg(tr("Set the number of the room up from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_w_stateChanged(int state)
{
    normalStubExitChanged(state, w, noroute_w, weight_w,
                          doortype_none_w, doortype_open_w, doortype_closed_w, doortype_locked_w,
                          singleParagraph.arg(tr("Set the number of the room west of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_e_stateChanged(int state)
{
    normalStubExitChanged(state, e, noroute_e, weight_e,
                          doortype_none_e, doortype_open_e, doortype_closed_e, doortype_locked_e,
                          singleParagraph.arg(tr("Set the number of the room east of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_down_stateChanged(int state)
{
    normalStubExitChanged(state, down, noroute_down, weight_down,
                          doortype_none_down, doortype_open_down, doortype_closed_down, doortype_locked_down,
                          singleParagraph.arg(tr("Set the number of the room down from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_sw_stateChanged(int state)
{
    normalStubExitChanged(state, sw, noroute_sw, weight_sw,
                          doortype_none_sw, doortype_open_sw, doortype_closed_sw, doortype_locked_sw,
                          singleParagraph.arg(tr("Set the number of the room southwest of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_s_stateChanged(int state)
{
    normalStubExitChanged(state, s, noroute_s, weight_s,
                          doortype_none_s, doortype_open_s, doortype_closed_s, doortype_locked_s,
                          singleParagraph.arg(tr("Set the number of the room south of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_se_stateChanged(int state)
{
    normalStubExitChanged(state, se, noroute_se, weight_se,
                          doortype_none_se, doortype_open_se, doortype_closed_se, doortype_locked_se,
                          singleParagraph.arg(tr("Set the number of the room southeast of this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_in_stateChanged(int state)
{
    normalStubExitChanged(state, in, noroute_in, weight_in,
                          doortype_none_in, doortype_open_in, doortype_closed_in, doortype_locked_in,
                          singleParagraph.arg(tr("Set the number of the room in from this one.")));
    slot_checkModified();
}

void dlgRoomExits::slot_stub_out_stateChanged(int state)
{
    normalStubExitChanged(state, out, noroute_out, weight_out,
                          doortype_none_out, doortype_open_out, doortype_closed_out, doortype_locked_out,
                          singleParagraph.arg(tr("Set the number of the room out from this one.")));
    slot_checkModified();
}

void dlgRoomExits::initExit(int direction,
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
        qWarning() << "dlgRoomExits::initExit(...) in room id(" << mRoomID << ") unexpected doors[" << doorAndWeightText << "] value:" << pR->getDoor(doorAndWeightText) << "found for room!";
    }

    TRoom* pExitR = nullptr;
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
        int exitAreaID = pExitR->getArea();
        bool outOfAreaExit = (exitAreaID && exitAreaID != mAreaID);
        QString exitAreaName;
        if (outOfAreaExit) {
            exitAreaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(exitAreaID);
        }
        setActionOnExit(exitLineEdit, outOfAreaExit ? mpAction_otherAreaExit : mpAction_inAreaExit);
        exitLineEdit->setToolTip(generateToolTip(pExitR->name, exitAreaName, outOfAreaExit, pExitR->getWeight()));
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
        setActionOnExit(exitLineEdit, mpAction_noExit);
        noRoute->setEnabled(false); //Disable lock control, can't lock a non-existant exit..
        noRoute->setChecked(false); //.. and ensure there isn't one
        weight->setEnabled(false);  //Disable exit weight control...
        weight->setValue(0);        //And reset to default value (which will now cause the room's one to be used
        stub->setEnabled(true);     //Enable stub exit control
        if (pR->hasExitStub(direction)) {
            exitLineEdit->setEnabled(false); //There is a stub exit, so prevent exit number entry...
            exitLineEdit->setToolTip(singleParagraph.arg(tr("Clear the stub exit for this exit to enter an exit roomID.")));
            stub->setChecked(true);
            none->setEnabled(true); //Enable door type controls, can have a door on a stub exit..
            open->setEnabled(true);
            closed->setEnabled(true);
            locked->setEnabled(true);
        } else {
            exitLineEdit->setEnabled(true);
            exitLineEdit->setToolTip(singleParagraph.arg(tr("Set the number of the room %1 of this one.").arg(exitText)));
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

void dlgRoomExits::init()
{
    pR = mpHost->mpMap->mpRoomDB->getRoom(mRoomID);
    if (!pR) {
        return;
    }

    mAreaID = pR->getArea();
    roomID->setText(QString::number(mRoomID));
    roomWeight->setText(QString::number(pR->getWeight()));
    QString titleText;
    if (pR->name.trimmed().length()) {
        titleText = tr(R"(Exits for room: "%1" [*])").arg(pR->name);
    } else {
        titleText = tr("Exits for room Id: %1 [*]").arg(mRoomID);
    }

    this->setWindowTitle(titleText);

    // Because we are manipulating the settings for the exit we need to know
    // explicitly where the weight comes from, pR->getExitWeight() hides that
    // detail deliberately for normal usage
    initExit(DIR_NORTHWEST, pR->getExit(DIR_NORTHWEST), nw, noroute_nw, stub_nw, doortype_none_nw, doortype_open_nw, doortype_closed_nw, doortype_locked_nw, weight_nw);

    initExit(DIR_NORTH, pR->getExit(DIR_NORTH), n, noroute_n, stub_n, doortype_none_n, doortype_open_n, doortype_closed_n, doortype_locked_n, weight_n);

    initExit(DIR_NORTHEAST, pR->getExit(DIR_NORTHEAST), ne, noroute_ne, stub_ne, doortype_none_ne, doortype_open_ne, doortype_closed_ne, doortype_locked_ne, weight_ne);

    initExit(DIR_UP, pR->getExit(DIR_UP), up, noroute_up, stub_up, doortype_none_up, doortype_open_up, doortype_closed_up, doortype_locked_up, weight_up);

    initExit(DIR_WEST, pR->getExit(DIR_WEST), w, noroute_w, stub_w, doortype_none_w, doortype_open_w, doortype_closed_w, doortype_locked_w, weight_w);

    initExit(DIR_EAST, pR->getExit(DIR_EAST), e, noroute_e, stub_e, doortype_none_e, doortype_open_e, doortype_closed_e, doortype_locked_e, weight_e);

    initExit(DIR_DOWN, pR->getExit(DIR_DOWN), down, noroute_down, stub_down, doortype_none_down, doortype_open_down, doortype_closed_down, doortype_locked_down, weight_down);

    initExit(DIR_SOUTHWEST, pR->getExit(DIR_SOUTHWEST), sw, noroute_sw, stub_sw, doortype_none_sw, doortype_open_sw, doortype_closed_sw, doortype_locked_sw, weight_sw);

    initExit(DIR_SOUTH, pR->getExit(DIR_SOUTH), s, noroute_s, stub_s, doortype_none_s, doortype_open_s, doortype_closed_s, doortype_locked_s, weight_s);

    initExit(DIR_SOUTHEAST, pR->getExit(DIR_SOUTHEAST), se, noroute_se, stub_se, doortype_none_se, doortype_open_se, doortype_closed_se, doortype_locked_se, weight_se);

    initExit(DIR_IN, pR->getExit(DIR_IN), in, noroute_in, stub_in, doortype_none_in, doortype_open_in, doortype_closed_in, doortype_locked_in, weight_in);

    initExit(DIR_OUT, pR->getExit(DIR_OUT), out, noroute_out, stub_out, doortype_none_out, doortype_open_out, doortype_closed_out, doortype_locked_out, weight_out);

    QMapIterator<QString, int> it(pR->getSpecialExits());
    while (it.hasNext()) {
        it.next();
        int id_to = it.value();
        QString dir = it.key();
        auto pSpecialExit = new TExit();
        // It should be impossible for this not to be valid:
        Q_ASSERT_X(pSpecialExit, "dlgRoomExits::init(...)", "failed to generate a new TExit");
        auto pI = new QTreeWidgetItem(specialExits);

        // exit status
        pI->setTextAlignment(colIndex_exitStatus, Qt::AlignRight);
        pI->setIcon(colIndex_exitStatus, QIcon());

        // exit roomID
        pI->setText(colIndex_exitRoomId, QString::number(id_to));
        pI->setTextAlignment(colIndex_exitRoomId, Qt::AlignRight);

        pSpecialExit->destination = id_to;
        // locked (or more properly "No route") - setCheckedState
        // automagically makes it a CheckBox!!!
        if (pR->hasSpecialExitLock(dir)) {
            pI->setCheckState(colIndex_lockExit, Qt::Checked);
            pSpecialExit->hasNoRoute = true;
        } else {
            pI->setCheckState(colIndex_lockExit, Qt::Unchecked);
            pSpecialExit->hasNoRoute = false;
        }
        pI->setToolTip(colIndex_lockExit, singleParagraph.arg(tr("Prevent a route being created via this exit, equivalent to an infinite exit weight.")));

        pI->setData(colIndex_exitWeight, Qt::EditRole, pR->hasExitWeight(dir) ? pR->getExitWeight(dir) : 0);
        // pI->setTextAlignment(colIndex_exitWeight, Qt::AlignRight);
        pI->setToolTip(colIndex_exitWeight, singleParagraph.arg(tr("Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.")));
        pSpecialExit->weight = pI->data(colIndex_exitWeight, Qt::EditRole).toInt();

        // a buttongroup of 4, ideally QRadioButtons, to select a door type
        pI->setCheckState(colIndex_doorNone, Qt::Unchecked);
        pI->setTextAlignment(colIndex_doorNone, Qt::AlignCenter);
        pI->setToolTip(colIndex_doorNone, singleParagraph.arg(tr("No door symbol is drawn on 2D Map for this exit (only functional choice currently).")));
        pI->setCheckState(colIndex_doorOpen, Qt::Unchecked);
        pI->setTextAlignment(colIndex_doorOpen, Qt::AlignCenter);
        pI->setToolTip(colIndex_doorOpen, singleParagraph.arg(tr("Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setCheckState(colIndex_doorClosed, Qt::Unchecked);
        pI->setTextAlignment(colIndex_doorClosed, Qt::AlignCenter);
        pI->setToolTip(colIndex_doorClosed, singleParagraph.arg(tr("Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setTextAlignment(colIndex_doorLocked, Qt::AlignCenter);
        pI->setToolTip(colIndex_doorLocked, singleParagraph.arg(tr("Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).")));
        pI->setCheckState(colIndex_doorLocked, Qt::Unchecked);
        {
            int specialDoor = pR->getDoor(dir);
            switch (specialDoor) {
            case 0:
                pI->setCheckState(colIndex_doorNone, Qt::Checked);
                break;
            case 1:
                pI->setCheckState(colIndex_doorOpen, Qt::Checked);
                break;
            case 2:
                pI->setCheckState(colIndex_doorClosed, Qt::Checked);
                break;
            case 3:
                pI->setCheckState(colIndex_doorLocked, Qt::Checked);
                break;
            default:
                qWarning().nospace().noquote() << "dlgRoomExits::init(" << mRoomID << ") WARNING - unexpected (special exit) doors[" << dir << "] value:" << pR->doors[dir] << " found!";
            }
            pSpecialExit->door = specialDoor;
            originalSpecialExits[dir] = pSpecialExit;
        }

        // the script/name
        pI->setText(colIndex_command, dir);
        // Not relevant for special exits but better initialise it
        pSpecialExit->hasStub = false;

        setIconAndToolTipsOnSpecialExit(pI, true);
    }

    button_save->setEnabled(false);
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
    connect(specialExits,          &QTreeWidget::itemChanged,                      this, &dlgRoomExits::slot_itemChanged);
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
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
// The qOverload<int>(&QButtonGroup::buttonClicked) was deprecated in Qt 5.15 and replaced by QButtonGroup::idClicked
    connect(doortype_nw,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_n,            &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_ne,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_up,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_w,            &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_e,            &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_down,         &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_sw,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_s,            &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_se,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_in,           &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_down,         &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
    connect(doortype_out,          &QButtonGroup::idClicked,                       this, &dlgRoomExits::slot_checkModified);
#else
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
#endif
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

    TExit* pOriginalExit = originalExits.value(DIR_NORTHWEST);
    TExit* pCurrentExit = makeExitFromControls(DIR_NORTHWEST);

    if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
        isModified = true;
    }
    delete pCurrentExit;

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_NORTH);
        pCurrentExit = makeExitFromControls(DIR_NORTH);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_NORTHEAST);
        pCurrentExit = makeExitFromControls(DIR_NORTHEAST);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_UP);
        pCurrentExit = makeExitFromControls(DIR_UP);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_WEST);
        pCurrentExit = makeExitFromControls(DIR_WEST);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_EAST);
        pCurrentExit = makeExitFromControls(DIR_EAST);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_DOWN);
        pCurrentExit = makeExitFromControls(DIR_DOWN);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_SOUTHWEST);
        pCurrentExit = makeExitFromControls(DIR_SOUTHWEST);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_SOUTH);
        pCurrentExit = makeExitFromControls(DIR_SOUTH);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_SOUTHEAST);
        pCurrentExit = makeExitFromControls(DIR_SOUTHEAST);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_IN);
        pCurrentExit = makeExitFromControls(DIR_IN);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
    }

    if (!isModified) {
        pOriginalExit = originalExits.value(DIR_OUT);
        pCurrentExit = makeExitFromControls(DIR_OUT);
        if (pOriginalExit && pCurrentExit && *pOriginalExit != *pCurrentExit) {
            isModified = true;
        }
        delete pCurrentExit;
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
             *                   pI->text(colIndex_exitRoomId).toInt(),
             *                   qPrintable(pI->text(colIndex_command)));
             */
            if (pI->text(colIndex_command) == mSpecialExitCommandPlaceholder
                || pI->text(colIndex_exitRoomId).toInt() <= 0) {
                continue;
            } // Ignore new or to be deleted entries
            currentCount++;
        }
        if (originalCount != currentCount) {
            isModified = true;
        } else {
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
                     *                           pI->text(colIndex_exitRoomId).toInt(),
                     *                           qPrintable(pI->text(colIndex_command)));
                     */
                    if (pI->text(colIndex_command) == mSpecialExitCommandPlaceholder
                        || pI->text(colIndex_exitRoomId).toInt() <= 0) {
                        continue; // Ignore new or to be deleted entries
                    }
                    QString currentCmd = pI->text(colIndex_command);
                    TExit currentExit;
                    currentExit.destination = pI->text(colIndex_exitRoomId).toInt();
                    currentExit.hasNoRoute = pI->checkState(colIndex_lockExit) == Qt::Checked;
                    currentExit.door = pI->checkState(colIndex_doorLocked) == Qt::Checked
                                               ? 3 : pI->checkState(colIndex_doorClosed) == Qt::Checked
                                                         ? 2 : pI->checkState(colIndex_doorOpen) == Qt::Checked ? 1 : 0;
                    currentExit.weight = pI->data(colIndex_exitWeight, Qt::EditRole).toInt();
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
                if (foundMap.count()) {
                    isModified = true;
                }
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

// By changed this means "the currently selected row or column number in the
// special exits has changed":
void dlgRoomExits::slot_itemChanged(QTreeWidgetItem* pI, int column)
{
    if (column != colIndex_exitRoomId && column != colIndex_command) {
        return;
    }

    if (column == colIndex_command) {
        setIconAndToolTipsOnSpecialExit(pI, true);
    }
}
