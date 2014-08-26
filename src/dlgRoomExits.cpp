/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "dlgRoomExits.h"


#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "Host.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QDebug>
#include <QStringBuilder>
#include "post_guard.h"

dlgRoomExits::dlgRoomExits( Host * pH, QWidget * pW ): QDialog( pW ), mpHost( pH ), mpEditItem( 0 )
{
    setupUi(this);
}

void dlgRoomExits::slot_endEditSpecialExits()
{
    button_endEditing->setDisabled(true);
    if ( ! button_addSpecialExit->isEnabled() )
        button_addSpecialExit->setEnabled(true);
    if ( mpEditItem !=0 && mEditColumn >=0 ) {
        specialExits->closePersistentEditor( mpEditItem, mEditColumn );
        mpEditItem = 0;
        mEditColumn = -1;
    }
    specialExits->clearSelection();
}

void dlgRoomExits::slot_editSpecialExit(QTreeWidgetItem * pI, int column )
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

    if ( ! button_endEditing->isEnabled() )
        button_endEditing->setEnabled(true);
    if ( button_addSpecialExit->isEnabled() )
        button_addSpecialExit->setEnabled(false);

    if ( mpEditItem != 0 && ( pI != mpEditItem || mEditColumn != column ) ) {
        // Thing that was clicked on is not the same as last thing that was clicked on
        // ... so clean up the old column
        switch ( mEditColumn ) {
        case 0:
            if( mpEditItem->text(0).toInt() < 1 || mpHost->mpMap->mpRoomDB->getRoom( mpEditItem->text(0).toInt() ) == 0 )
                mpEditItem->setText(0, "<room ID>");
            specialExits->closePersistentEditor( mpEditItem, mEditColumn );
            break;

        case 2:
            mpEditItem->setText(2, QString::number( (mpEditItem->text(2).toInt() < 0) ? (-1 * mpEditItem->text(2).toInt()) : mpEditItem->text(2).toInt()) ); //Force result to be non-negative integer
            specialExits->closePersistentEditor( mpEditItem, mEditColumn );
            break;

        case 3:  // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
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
            if( ! mpEditItem->text(7).trimmed().length() )
                mpEditItem->setText(7, "<command or Lua script>");
            specialExits->closePersistentEditor( mpEditItem, mEditColumn );
            //            qDebug()<<"Closed PE on item:"<<mpEditItem->text(7)<<"column:"<<mEditColumn;
            break;
        default:
            ; //noop for other column (1)
        }
        mpEditItem = 0;   //This will cause a new PE to be opened, it will also be zeroed on the first time this funciton is called
        mEditColumn = -1;
    }

    // Now process the new column that was selected:
    if ( mpEditItem == 0 ) {
        if ( column == 0 || column == 2 || column == 7 ) {
            //            qDebug()<<"Opened PE on item:"<<pI->text(7)<<"column:"<<column;
            specialExits->openPersistentEditor(pI, column);
            specialExits->editItem(pI, column);
        }
        mpEditItem = pI;
        mEditColumn = column;
    }

    //    qDebug()<<"A Special Exit is been edited, it has the command:" << pI->text(7) <<"and the editing is on column "<<column;
    switch ( column ) {
    case 2:
        pI->setText(2, QString::number( (pI->text(2).toInt() < 0) ? (-1 * pI->text(2).toInt()) : pI->text(2).toInt()) ); //Force result to be non-negative
        break;

    case 3:  // Enforce exclusive Radio Button type behaviour on the checkboxes in these four columns
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

    default:
        ; //noop for other columns?
    }
}

void dlgRoomExits::slot_addSpecialExit()
{
    QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
    pI->setText(0, "<room ID>"); //Exit RoomID
    pI->setTextAlignment(0, Qt::AlignRight );
    pI->setCheckState(1, Qt::Unchecked); //Locked
    pI->setText(2, "0");  //Exit Weight
    pI->setTextAlignment(2, Qt::AlignRight );
    pI->setCheckState(3, Qt::Checked);   //Doortype: none
    pI->setCheckState(4, Qt::Unchecked); //Doortype: open
    pI->setCheckState(5, Qt::Unchecked); //Doortype: closed
    pI->setCheckState(6, Qt::Unchecked); //Doortype: locked
    pI->setText(7, "<command or Lua script>"); //Exit command
    pI->setTextAlignment(7, Qt::AlignLeft );
    specialExits->addTopLevelItem(pI);
}

void dlgRoomExits::save()
{
    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    if ( !pR )
        return;

    QMultiMap<int, QString> oldSpecialExits = pR->getOtherMap();
    QMutableMapIterator<int, QString> exitIterator = oldSpecialExits;
    while (exitIterator.hasNext()) {
        exitIterator.next();
        if ( exitIterator.value().length() > 1 && ( exitIterator.value().startsWith("0") || exitIterator.value().startsWith("1") ) )
            exitIterator.setValue( exitIterator.value().mid(1) );
    }
    QSet<QString> originalExitCmds = oldSpecialExits.values().toSet();

//    pR->clearSpecialExits(); // Previous code could not change the destination
                               // room of an existing special exit
                               // so had to clear all and rebuild all of them
    for ( int i=0; i<specialExits->topLevelItemCount(); i++ ) {
        QTreeWidgetItem * pI = specialExits->topLevelItem(i);
        int    key = pI->text(0).toInt();
        int weight = pI->text(2).toInt();
        int   door = 0;
        if ( pI->checkState(6) == Qt::Checked )
            door = 3;
        else if ( pI->checkState(5) == Qt::Checked )
            door = 2;
        else if ( pI->checkState(4) == Qt::Checked )
            door = 1;
        else if ( pI->checkState(3) == Qt::Checked )
            door = 0;
        QString value = pI->text(7);
        if ( value != "<command or Lua script>" && key != 0 && mpHost->mpMap->mpRoomDB->getRoom(key) !=0 ) {
            originalExitCmds.remove( value );
            if ( pI->checkState(1) == Qt::Unchecked )
                value = value.prepend( '0' );
            else
                value = value.prepend( '1' );
            pR->setSpecialExit( key, value ); // Now can overwrite an existing exit with a different destination
            if ( pR->hasExitWeight(value.mid(1))  || weight > 0 )
                pR->setExitWeight(value.mid(1), weight);
            if ( pR->getDoor(value.mid(1)) || door > 0 )
                pR->setDoor(value.mid(1), door);
        }
    }

    // Clean up after any deleted specialExits originally present but not now so
    foreach (const QString &value, originalExitCmds) {
        pR->customLinesArrow.remove( value );
        pR->customLinesColor.remove( value );
        pR->customLinesStyle.remove( value );
        pR->customLines.remove( value );
        pR->setDoor( value, 0 );
        pR->setExitWeight( value, 0 );
        pR->setSpecialExit( -1, value );
    }

    if (nw->isEnabled() && nw->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != 0 ) {
        // There IS a valid exit on the dialogue in this direction
        pR->setExit( nw->text().toInt(), DIR_NORTHWEST ); // So store it
        if (pR->hasExitStub(DIR_NORTHWEST))   // And ensure that stub exit is cleared if set
            pR->setExitStub(DIR_NORTHWEST, false);
        if (weight_nw->value())  // And store any weighing specifed
            pR->setExitWeight( "nw", weight_nw->value());
        else
            pR->setExitWeight( "nw", 0);
    } else { // No valid exit on the dialogue
        pR->setExit( -1, DIR_NORTHWEST ); // So ensure the value for no exit is stored
        if (stub_nw->isChecked() != pR->hasExitStub(DIR_NORTHWEST))
            // Does the stub exit setting differ from what is stored
            pR->setExitStub(DIR_NORTHWEST, stub_nw->isChecked()); // So change stored idea to match
        pR->setExitWeight( "nw", 0); // And clear any weighting that was stored
        pR->customLinesArrow.remove( "NW" );
        pR->customLinesColor.remove( "NW" );
        pR->customLinesStyle.remove( "NW" );
        pR->customLines.remove( "NW" ); // And remove any custom line stuff, which uses opposite case keys - *sigh*
    }

    if (n->isEnabled() && n->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != 0 ) {
        pR->setExit( n->text().toInt(), DIR_NORTH );
        if (pR->hasExitStub(DIR_NORTH))
            pR->setExitStub(DIR_NORTH, false);
        if (weight_n->value())
            pR->setExitWeight( "n", weight_n->value());
        else
            pR->setExitWeight( "n", 0);
    } else {
        pR->setExit( -1, DIR_NORTH );
        if (stub_n->isChecked() != pR->hasExitStub(DIR_NORTH))
            pR->setExitStub(DIR_NORTH, stub_n->isChecked());
        pR->setExitWeight( "n", 0);
        pR->customLinesArrow.remove( "N" );
        pR->customLinesColor.remove( "N" );
        pR->customLinesStyle.remove( "N" );
        pR->customLines.remove( "N" );
    }

    if (ne->isEnabled() && ne->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != 0 ) {
        pR->setExit( ne->text().toInt(), DIR_NORTHEAST );
        if (pR->hasExitStub(DIR_NORTHEAST))
            pR->setExitStub(DIR_NORTHEAST, false);
        if (weight_ne->value())
            pR->setExitWeight( "ne", weight_ne->value());
        else
            pR->setExitWeight( "ne", 0);
    } else {
        pR->setExit( -1, DIR_NORTHEAST );
        if (stub_ne->isChecked() != pR->hasExitStub(DIR_NORTHEAST))
            pR->setExitStub(DIR_NORTHEAST, stub_ne->isChecked());
        pR->setExitWeight( "ne", 0);
        pR->customLinesArrow.remove( "NE" );
        pR->customLinesColor.remove( "NE" );
        pR->customLinesStyle.remove( "NE" );
        pR->customLines.remove( "NE" );
    }

    if (up->isEnabled() && up->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != 0 ) {
        pR->setExit( up->text().toInt(), DIR_UP );
        if (pR->hasExitStub(DIR_UP))
            pR->setExitStub(DIR_UP, false);
        if (weight_up->value())
            pR->setExitWeight( "up", weight_up->value());
        else
            pR->setExitWeight( "up", 0);
    } else {
        pR->setExit( -1, DIR_UP );
        if (stub_up->isChecked() != pR->hasExitStub(DIR_UP))
            pR->setExitStub(DIR_UP, stub_up->isChecked());
        pR->setExitWeight( "up", 0);
        pR->customLinesArrow.remove( "UP" );
        pR->customLinesColor.remove( "UP" );
        pR->customLinesStyle.remove( "UP" );
        pR->customLines.remove( "UP" );
    }

    if (w->isEnabled() && w->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != 0 ) {
        pR->setExit( w->text().toInt(), DIR_WEST );
        if (pR->hasExitStub(DIR_WEST))
            pR->setExitStub(DIR_WEST, false);
        if (weight_w->value())
            pR->setExitWeight( "w", weight_w->value());
        else
            pR->setExitWeight( "w", 0);
    } else {
        pR->setExit( -1, DIR_WEST );
        if (stub_w->isChecked() != pR->hasExitStub(DIR_WEST))
            pR->setExitStub(DIR_WEST, stub_w->isChecked());
        pR->setExitWeight( "w", 0);
        pR->customLinesArrow.remove( "W" );
        pR->customLinesColor.remove( "W" );
        pR->customLinesStyle.remove( "W" );
        pR->customLines.remove( "W" );
    }

    if (e->isEnabled() && e->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != 0 ) {
        pR->setExit( e->text().toInt(), DIR_EAST );
        if (pR->hasExitStub(DIR_EAST))
            pR->setExitStub(DIR_EAST, false);
        if (weight_e->value())
            pR->setExitWeight( "e", weight_e->value());
        else
            pR->setExitWeight( "e", 0);
    } else {
        pR->setExit( -1, DIR_EAST );
        if (stub_e->isChecked() != pR->hasExitStub(DIR_EAST))
            pR->setExitStub(DIR_EAST, stub_e->isChecked());
        pR->setExitWeight( "e", 0);
        pR->customLinesArrow.remove( "E" );
        pR->customLinesColor.remove( "E" );
        pR->customLinesStyle.remove( "E" );
        pR->customLines.remove( "E" );
    }

    if (down->isEnabled() && down->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != 0 ) {
        pR->setExit( down->text().toInt(), DIR_DOWN );
        if (pR->hasExitStub(DIR_DOWN))
            pR->setExitStub(DIR_DOWN, false);
        if (weight_down->value())
            pR->setExitWeight( "down", weight_down->value());
        else
            pR->setExitWeight( "down", 0);
    } else {
        pR->setExit( -1, DIR_DOWN );
        if (stub_down->isChecked() != pR->hasExitStub(DIR_DOWN))
            pR->setExitStub(DIR_DOWN, stub_down->isChecked());
        pR->setExitWeight( "down", 0);
        pR->customLinesArrow.remove( "DOWN" );
        pR->customLinesColor.remove( "DOWN" );
        pR->customLinesStyle.remove( "DOWN" );
        pR->customLines.remove( "DOWN" );
    }

    if (sw->isEnabled() && sw->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != 0 ) {
        pR->setExit( sw->text().toInt(), DIR_SOUTHWEST );
        if (pR->hasExitStub(DIR_SOUTHWEST))
            pR->setExitStub(DIR_SOUTHWEST, false);
        if (weight_sw->value())
            pR->setExitWeight( "sw", weight_sw->value());
        else
            pR->setExitWeight( "sw", 0);
    } else {
        pR->setExit( -1, DIR_SOUTHWEST );
        if (stub_sw->isChecked() != pR->hasExitStub(DIR_SOUTHWEST))
            pR->setExitStub(DIR_SOUTHWEST, stub_sw->isChecked());
        pR->setExitWeight( "sw", 0);
        pR->customLinesArrow.remove( "SW" );
        pR->customLinesColor.remove( "SW" );
        pR->customLinesStyle.remove( "SW" );
        pR->customLines.remove( "SW" );
    }

    if (s->isEnabled() && s->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != 0 ) {
        pR->setExit( s->text().toInt(), DIR_SOUTH );
        if (pR->hasExitStub(DIR_SOUTH))
            pR->setExitStub(DIR_SOUTH, false);
        if (weight_s->value())
            pR->setExitWeight( "s", weight_s->value());
        else
            pR->setExitWeight( "s", 0);
    } else {
        pR->setExit( -1, DIR_SOUTH );
        if (stub_s->isChecked() != pR->hasExitStub(DIR_SOUTH))
            pR->setExitStub(DIR_SOUTH, stub_s->isChecked());
        pR->setExitWeight( "s", 0);
        pR->customLinesArrow.remove( "S" );
        pR->customLinesColor.remove( "S" );
        pR->customLinesStyle.remove( "S" );
        pR->customLines.remove( "S" );
    }

    if (se->isEnabled() && se->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != 0 ) {
        pR->setExit( se->text().toInt(), DIR_SOUTHEAST );
        if (pR->hasExitStub(DIR_SOUTHEAST))
            pR->setExitStub(DIR_SOUTHEAST, false);
        if (weight_se->value())
            pR->setExitWeight( "se", weight_se->value());
        else
            pR->setExitWeight( "se", 0);
    } else {
        pR->setExit( -1, DIR_SOUTHEAST );
        if (stub_se->isChecked() != pR->hasExitStub(DIR_SOUTHEAST))
            pR->setExitStub(DIR_SOUTHEAST, stub_se->isChecked());
        pR->setExitWeight( "se", 0);
        pR->customLinesArrow.remove( "SE" );
        pR->customLinesColor.remove( "SE" );
        pR->customLinesStyle.remove( "SE" );
        pR->customLines.remove( "SE" );
    }

    if (in->isEnabled() && in->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != 0 ) {
        pR->setExit( in->text().toInt(), DIR_IN );
        if (pR->hasExitStub(DIR_IN))
            pR->setExitStub(DIR_IN, false);
        if (weight_in->value())
            pR->setExitWeight( "in", weight_in->value());
        else
            pR->setExitWeight( "in", 0);
    } else {
        pR->setExit( -1, DIR_IN );
        if (stub_in->isChecked() != pR->hasExitStub(DIR_IN))
            pR->setExitStub(DIR_IN, stub_in->isChecked());
        pR->setExitWeight( "in", 0);
        pR->customLinesArrow.remove( "IN" );
        pR->customLinesColor.remove( "IN" );
        pR->customLinesStyle.remove( "IN" );
        pR->customLines.remove( "IN" );
    }

    if (out->isEnabled() && out->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != 0 ) {
        pR->setExit( out->text().toInt(), DIR_OUT );
        if (pR->hasExitStub(DIR_OUT))
            pR->setExitStub(DIR_OUT, false);
        if (weight_out->value())
            pR->setExitWeight( "out", weight_out->value());
        else
            pR->setExitWeight( "out", 0);
    } else {
        pR->setExit( -1, DIR_OUT );
        if (stub_out->isChecked() != pR->hasExitStub(DIR_OUT))
            pR->setExitStub(DIR_OUT, stub_out->isChecked());
        pR->setExitWeight( "out", 0);
        pR->customLinesArrow.remove( "OUT" );
        pR->customLinesColor.remove( "OUT" );
        pR->customLinesStyle.remove( "OUT" );
        pR->customLines.remove( "OUT" );
    }

    pR->setExitLock(DIR_NORTHWEST, noroute_nw->isChecked());
    pR->setExitLock(DIR_NORTH,     noroute_n->isChecked());
    pR->setExitLock(DIR_NORTHEAST, noroute_ne->isChecked());
    pR->setExitLock(DIR_UP,        noroute_up->isChecked());
    pR->setExitLock(DIR_WEST,      noroute_w->isChecked());
    pR->setExitLock(DIR_EAST,      noroute_e->isChecked());
    pR->setExitLock(DIR_DOWN,      noroute_down->isChecked());
    pR->setExitLock(DIR_SOUTHWEST, noroute_sw->isChecked());
    pR->setExitLock(DIR_SOUTH,     noroute_s->isChecked());
    pR->setExitLock(DIR_SOUTHEAST, noroute_se->isChecked());
    pR->setExitLock(DIR_IN,        noroute_in->isChecked());
    pR->setExitLock(DIR_OUT,       noroute_out->isChecked());

    // return value from checkedId() is -1 for no radio button in group checked,
    //   and then more negative values starting from -2 for each button that was
    //   created without an explict Id, any attempt to set a different Id using
    //   setId() seems to fail for me :(
    if (doortype_nw->checkedId()<-1)
        pR->setDoor( "nw", -2-doortype_nw->checkedId());

    if (doortype_n->checkedId()<-1)
        pR->setDoor( "n", -2-doortype_n->checkedId());

    if (doortype_ne->checkedId()<-1)
        pR->setDoor( "ne", -2-doortype_ne->checkedId());

    if (doortype_up->checkedId()<-1)
        pR->setDoor( "up", -2-doortype_up->checkedId());

    if (doortype_w->checkedId()<-1)
        pR->setDoor( "w", -2-doortype_w->checkedId());

    if (doortype_e->checkedId()<-1)
        pR->setDoor( "e", -2-doortype_e->checkedId());

    if (doortype_down->checkedId()<-1)
        pR->setDoor( "down", -2-doortype_down->checkedId());

    if (doortype_sw->checkedId()<-1)
        pR->setDoor( "sw", -2-doortype_sw->checkedId());

    if (doortype_s->checkedId()<-1)
        pR->setDoor( "s", -2-doortype_s->checkedId());

    if (doortype_se->checkedId()<-1)
        pR->setDoor( "se", -2-doortype_se->checkedId());

    if (doortype_in->checkedId()<-1)
        pR->setDoor( "in", -2-doortype_in->checkedId());

    if (doortype_out->checkedId()<-1)
        pR->setDoor( "out", -2-doortype_out->checkedId());

    TArea * pA = mpHost->mpMap->mpRoomDB->getArea( pR->getArea() );
    if( pA )
        pA->fast_ausgaengeBestimmen( pR->getId() );

    close();
}


// These slots are called as the text for the exitID is edited
void dlgRoomExits::slot_nw_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        // Valid exit roomID in place
        nw->setStyleSheet("color:blue");
        stub_nw->setChecked(false);
        stub_nw->setEnabled(false);
        noroute_nw->setEnabled(true);
        weight_nw->setEnabled(true);
        doortype_none_nw->setEnabled(true);
        doortype_open_nw->setEnabled(true);
        doortype_closed_nw->setEnabled(true);
        doortype_locked_nw->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            nw->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            nw->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0 ) {
        // Something is entered but it does not yield a valid exit roomID
        // Enable stub exit control
        nw->setStyleSheet("color:red");
        nw->setToolTip("Entered number is invalid, set the number of the room northwest of this one, will turn blue for a valid number.");
        stub_nw->setEnabled(true);
        noroute_nw->setEnabled(false);
        weight_nw->setEnabled(false);
        doortype_none_nw->setEnabled(false);
        doortype_open_nw->setEnabled(false);
        doortype_closed_nw->setEnabled(false);
        doortype_locked_nw->setEnabled(false);
    } else {
        // Nothing is entered - so we can enable the stub exit control
        nw->setStyleSheet("");
        nw->setToolTip("Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_n_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        n->setStyleSheet("color:blue");;
        stub_n->setChecked(false);
        stub_n->setEnabled(false);
        noroute_n->setEnabled(true);
        weight_n->setEnabled(true);
        doortype_none_n->setEnabled(true);
        doortype_open_n->setEnabled(true);
        doortype_closed_n->setEnabled(true);
        doortype_locked_n->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            n->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            n->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        n->setStyleSheet("color:red");
        n->setToolTip("Entered number is invalid, set the number of the room north of this one, will turn blue for a valid number.");
        stub_n->setEnabled(true);
        noroute_n->setEnabled(false);
        weight_n->setEnabled(false);
        doortype_none_n->setEnabled(false);
        doortype_open_n->setEnabled(false);
        doortype_closed_n->setEnabled(false);
        doortype_locked_n->setEnabled(false);
    } else {
        n->setStyleSheet("");
        n->setToolTip("Set the number of the room north of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_ne_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        ne->setStyleSheet("color:blue");
        stub_ne->setChecked(false);
        stub_ne->setEnabled(false);
        noroute_ne->setEnabled(true);
        weight_ne->setEnabled(true);
        doortype_none_ne->setEnabled(true);
        doortype_open_ne->setEnabled(true);
        doortype_closed_ne->setEnabled(true);
        doortype_locked_ne->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            ne->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            ne->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        ne->setStyleSheet("color:red");
        ne->setToolTip("Entered number is invalid, set the number of the room northeast of this one, will turn blue for a valid number.");
        stub_ne->setEnabled(true);
        noroute_ne->setEnabled(false);
        weight_ne->setEnabled(false);
        doortype_none_ne->setEnabled(false);
        doortype_open_ne->setEnabled(false);
        doortype_closed_ne->setEnabled(false);
        doortype_locked_ne->setEnabled(false);
    } else {
        ne->setStyleSheet("");
        ne->setToolTip("Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_up_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        up->setStyleSheet("color:blue");
        stub_up->setChecked(false);
        stub_up->setEnabled(false);
        noroute_up->setEnabled(true);
        weight_up->setEnabled(true);
        doortype_none_up->setEnabled(true);
        doortype_open_up->setEnabled(true);
        doortype_closed_up->setEnabled(true);
        doortype_locked_up->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            up->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            up->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        up->setStyleSheet("color:red");
        up->setToolTip("Entered number is invalid, set the number of the room up from this one, will turn blue for a valid number.");
        stub_up->setEnabled(true);
        noroute_up->setEnabled(false);
        weight_up->setEnabled(false);
        doortype_none_up->setEnabled(false);
        doortype_open_up->setEnabled(false);
        doortype_closed_up->setEnabled(false);
        doortype_locked_up->setEnabled(false);
    } else {
        up->setStyleSheet("");
        up->setToolTip("Set the number of the room up from this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_w_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        w->setStyleSheet("color:blue");
        stub_w->setChecked(false);
        stub_w->setEnabled(false);
        noroute_w->setEnabled(true);
        weight_w->setEnabled(true);
        doortype_none_w->setEnabled(true);
        doortype_open_w->setEnabled(true);
        doortype_closed_w->setEnabled(true);
        doortype_locked_w->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            w->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            w->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        w->setStyleSheet("color:red");
        w->setToolTip("Entered number is invalid, set the number of the room west of this one, will turn blue for a valid number.");
        stub_w->setEnabled(true);
        noroute_w->setEnabled(false);
        weight_w->setEnabled(false);
        doortype_none_w->setEnabled(false);
        doortype_open_w->setEnabled(false);
        doortype_closed_w->setEnabled(false);
        doortype_locked_w->setEnabled(false);
    } else {
        w->setStyleSheet("");
        w->setToolTip("Set the number of the room west of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_e_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        e->setStyleSheet("color:blue");
        stub_e->setChecked(false);
        stub_e->setEnabled(false);
        noroute_e->setEnabled(true);
        weight_e->setEnabled(true);
        doortype_none_e->setEnabled(true);
        doortype_open_e->setEnabled(true);
        doortype_closed_e->setEnabled(true);
        doortype_locked_e->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            e->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            e->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        e->setStyleSheet("color:red");
        e->setToolTip("Entered number is invalid, set the number of the room east of this one, will turn blue for a valid number.");
        stub_e->setEnabled(true);
        noroute_e->setEnabled(false);
        weight_e->setEnabled(false);
        doortype_none_e->setEnabled(false);
        doortype_open_e->setEnabled(false);
        doortype_closed_e->setEnabled(false);
        doortype_locked_e->setEnabled(false);
    } else {
        e->setStyleSheet("");
        e->setToolTip("Set the number of the room east of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_down_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        down->setStyleSheet("color:blue");
        stub_down->setChecked(false);
        stub_down->setEnabled(false);
        noroute_down->setEnabled(true);
        weight_down->setEnabled(true);
        doortype_none_down->setEnabled(true);
        doortype_open_down->setEnabled(true);
        doortype_closed_down->setEnabled(true);
        doortype_locked_down->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            down->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            down->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        down->setStyleSheet("color:red");
        down->setToolTip("Entered number is invalid, set the number of the room down from this one, will turn blue for a valid number.");
        stub_down->setEnabled(true);
        noroute_down->setEnabled(false);
        weight_down->setEnabled(false);
        doortype_none_down->setEnabled(false);
        doortype_open_down->setEnabled(false);
        doortype_closed_down->setEnabled(false);
        doortype_locked_down->setEnabled(false);
    } else {
        down->setStyleSheet("");
        down->setToolTip("Set the number of the room down from this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_sw_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        sw->setStyleSheet("color:blue");
        stub_sw->setChecked(false);
        stub_sw->setEnabled(false);
        noroute_sw->setEnabled(true);
        weight_sw->setEnabled(true);
        doortype_none_sw->setEnabled(true);
        doortype_open_sw->setEnabled(true);
        doortype_closed_sw->setEnabled(true);
        doortype_locked_sw->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            sw->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            sw->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        sw->setStyleSheet("color:red");
        sw->setToolTip("Entered number is invalid, set the number of the room southwest of this one, will turn blue for a valid number.");
        stub_sw->setEnabled(true);
        noroute_sw->setEnabled(false);
        weight_sw->setEnabled(false);
        doortype_none_sw->setEnabled(false);
        doortype_open_sw->setEnabled(false);
        doortype_closed_sw->setEnabled(false);
        doortype_locked_sw->setEnabled(false);
    } else {
        sw->setStyleSheet("");
        sw->setToolTip("Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_s_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        s->setStyleSheet("color:blue");
        stub_s->setChecked(false);
        stub_s->setEnabled(false);
        noroute_s->setEnabled(true);
        weight_s->setEnabled(true);
        doortype_none_s->setEnabled(true);
        doortype_open_s->setEnabled(true);
        doortype_closed_s->setEnabled(true);
        doortype_locked_s->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            s->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            s->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        s->setStyleSheet("color:red");
        s->setToolTip("Entered number is invalid, set the number of the room south of this one, will turn blue for a valid number.");
        stub_s->setEnabled(true);
        noroute_s->setEnabled(false);
        weight_s->setEnabled(false);
        doortype_none_s->setEnabled(false);
        doortype_open_s->setEnabled(false);
        doortype_closed_s->setEnabled(false);
        doortype_locked_s->setEnabled(false);
    } else {
        s->setStyleSheet("");
        s->setToolTip("Set the number of the room south of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_se_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        se->setStyleSheet("color:blue");
        stub_se->setChecked(false);
        stub_se->setEnabled(false);
        noroute_se->setEnabled(true);
        weight_se->setEnabled(true);
        doortype_none_se->setEnabled(true);
        doortype_open_se->setEnabled(true);
        doortype_closed_se->setEnabled(true);
        doortype_locked_se->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            se->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            se->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        se->setStyleSheet("color:red");
        se->setToolTip("Entered number is invalid, set the number of the room southeast of this one, will turn blue for a valid number.");
        stub_se->setEnabled(true);
        noroute_se->setEnabled(false);
        weight_se->setEnabled(false);
        doortype_none_se->setEnabled(false);
        doortype_open_se->setEnabled(false);
        doortype_closed_se->setEnabled(false);
        doortype_locked_se->setEnabled(false);
    } else {
        se->setStyleSheet("");
        se->setToolTip("Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_in_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        in->setStyleSheet("color:blue");
        stub_in->setChecked(false);
        stub_in->setEnabled(false);
        noroute_in->setEnabled(true);
        weight_in->setEnabled(true);
        doortype_none_in->setEnabled(true);
        doortype_open_in->setEnabled(true);
        doortype_closed_in->setEnabled(true);
        doortype_locked_in->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            in->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            in->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        in->setStyleSheet("color:red");
        in->setToolTip("Entered number is invalid, set the number of the room in from this one, will turn blue for a valid number.");
        stub_in->setEnabled(true);
        noroute_in->setEnabled(false);
        weight_in->setEnabled(false);
        doortype_none_in->setEnabled(false);
        doortype_open_in->setEnabled(false);
        doortype_closed_in->setEnabled(false);
        doortype_locked_in->setEnabled(false);
    } else {
        in->setStyleSheet("");
        in->setToolTip("Set the number of the room in from this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::slot_out_textEdited(const QString &text)
{
    TRoom * exitToRoom = mpHost->mpMap->mpRoomDB->getRoom(text.toInt());

    if ( exitToRoom != 0 ) {
        out->setStyleSheet("color:blue");
        stub_out->setChecked(false);
        stub_out->setEnabled(false);
        noroute_out->setEnabled(true);
        weight_out->setEnabled(true);
        doortype_none_out->setEnabled(true);
        doortype_open_out->setEnabled(true);
        doortype_closed_out->setEnabled(true);
        doortype_locked_out->setEnabled(true);
        if( exitToRoom->name.trimmed().length() )
            out->setToolTip(QString("Exit to \"" % exitToRoom->name % "\""));
        else
            out->setToolTip(QString("Exit to unnamed room is valid"));
    } else if ( text.size() > 0) {
        out->setStyleSheet("color:red");
        out->setToolTip("Entered number is invalid, set the number of the room out from this one, will turn blue for a valid number.");
        stub_out->setEnabled(true);
        noroute_out->setEnabled(false);
        weight_out->setEnabled(false);
        doortype_none_out->setEnabled(false);
        doortype_open_out->setEnabled(false);
        doortype_closed_out->setEnabled(false);
        doortype_locked_out->setEnabled(false);
    } else {
        out->setStyleSheet("");
        out->setToolTip("Set the number of the room out from this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != 0 ) {
            nw->setText("");
            nw->setStyleSheet("");
            weight_nw->setValue(0); // Can't have a weight for a stub exit
            noroute_nw->setChecked(false); // nor a "lock"
        }
        noroute_nw->setEnabled(false); // Disable "lock" on this exit
        nw->setEnabled(false); // Prevent entry of an exit roomID
        nw->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_nw->setEnabled(true);
        doortype_open_nw->setEnabled(true);
        doortype_closed_nw->setEnabled(true);
        doortype_locked_nw->setEnabled(true); // Permit a door to be set on a stub exit
        weight_nw->setEnabled(false); // Prevent a weight to be set/changed on a stub
    } else {
        nw->setEnabled(true);
        nw->setToolTip("Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != 0 ) {
            n->setText("");
            n->setStyleSheet("");
            weight_n->setValue(0);
            noroute_n->setChecked(false);
        }
        noroute_n->setEnabled(false);
        n->setEnabled(false);
        n->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_n->setEnabled(true);
        doortype_open_n->setEnabled(true);
        doortype_closed_n->setEnabled(true);
        doortype_locked_n->setEnabled(true);
        weight_n->setEnabled(false);
    } else {
        n->setEnabled(true);
        n->setToolTip("Set the number of the room north of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != 0 ) {
            ne->setText("");
            ne->setStyleSheet("");
            weight_ne->setValue(0);
            noroute_ne->setChecked(false);
        }
        noroute_ne->setEnabled(false);
        ne->setEnabled(false);
        ne->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_ne->setEnabled(true);
        doortype_open_ne->setEnabled(true);
        doortype_closed_ne->setEnabled(true);
        doortype_locked_ne->setEnabled(true);
        weight_ne->setEnabled(false);
    } else {
        ne->setEnabled(true);
        ne->setToolTip("Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != 0 ) {
            up->setText("");
            up->setStyleSheet("");
            weight_up->setValue(0);
            noroute_up->setChecked(false);
        }
        noroute_up->setEnabled(false);
        up->setEnabled(false);
        up->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_up->setEnabled(true);
        doortype_open_up->setEnabled(true);
        doortype_closed_up->setEnabled(true);
        doortype_locked_up->setEnabled(true);
        weight_up->setEnabled(false);
    } else {
        up->setEnabled(true);
        up->setToolTip("Set the number of the room up from this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != 0 ) {
            w->setText("");
            w->setStyleSheet("");
            weight_w->setValue(0);
            noroute_w->setChecked(false);
        }
        noroute_w->setEnabled(false);
        w->setEnabled(false);
        w->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_w->setEnabled(true);
        doortype_open_w->setEnabled(true);
        doortype_closed_w->setEnabled(true);
        doortype_locked_w->setEnabled(true);
        weight_w->setEnabled(false);
    } else {
        w->setEnabled(true);
        w->setToolTip("Set the number of the room west of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != 0 ) {
            e->setText("");
            e->setStyleSheet("");
            weight_e->setValue(0);
            noroute_e->setChecked(false);
        }
        noroute_e->setEnabled(false);
        e->setEnabled(false);
        e->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_e->setEnabled(true);
        doortype_open_e->setEnabled(true);
        doortype_closed_e->setEnabled(true);
        doortype_locked_e->setEnabled(true);
        weight_e->setEnabled(false);
    } else {
        e->setEnabled(true);
        e->setToolTip("Set the number of the room east of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != 0 ) {
            down->setText("");
            down->setStyleSheet("");
            weight_down->setValue(0);
            noroute_down->setChecked(false);
        }
        noroute_down->setEnabled(false);
        down->setEnabled(false);
        down->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_down->setEnabled(true);
        doortype_open_down->setEnabled(true);
        doortype_closed_down->setEnabled(true);
        doortype_locked_down->setEnabled(true);
        weight_down->setEnabled(false);
    } else {
        down->setEnabled(true);
        down->setToolTip("Set the number of the room down from this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != 0 ) {
            sw->setText("");
            sw->setStyleSheet("");
            weight_sw->setValue(0);
            noroute_sw->setChecked(false);
        }
        noroute_sw->setEnabled(false);
        sw->setEnabled(false);
        sw->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_sw->setEnabled(true);
        doortype_open_sw->setEnabled(true);
        doortype_closed_sw->setEnabled(true);
        doortype_locked_sw->setEnabled(true);
        weight_sw->setEnabled(false);
    } else {
        sw->setEnabled(true);
        sw->setToolTip("Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != 0 ) {
            s->setText("");
            s->setStyleSheet("");
            weight_s->setValue(0);
            noroute_s->setChecked(false);
        }
        noroute_s->setEnabled(false);
        s->setEnabled(false);
        s->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_s->setEnabled(true);
        doortype_open_s->setEnabled(true);
        doortype_closed_s->setEnabled(true);
        doortype_locked_s->setEnabled(true);
        weight_s->setEnabled(false);
    } else {
        s->setEnabled(true);
        s->setToolTip("Set the number of the room south of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != 0 ) {
            se->setText("");
            se->setStyleSheet("");
            weight_se->setValue(0);
            noroute_se->setChecked(false);
        }
        noroute_se->setEnabled(false);
        se->setEnabled(false);
        se->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_se->setEnabled(true);
        doortype_open_se->setEnabled(true);
        doortype_closed_se->setEnabled(true);
        doortype_locked_se->setEnabled(true);
        weight_se->setEnabled(false);
    } else {
        se->setEnabled(true);
        se->setToolTip("Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != 0 ) {
            in->setText("");
            in->setStyleSheet("");
            weight_in->setValue(0);
            noroute_in->setChecked(false);
        }
        noroute_in->setEnabled(false);
        in->setEnabled(false);
        in->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_in->setEnabled(true);
        doortype_open_in->setEnabled(true);
        doortype_closed_in->setEnabled(true);
        doortype_locked_in->setEnabled(true);
        weight_in->setEnabled(false);
    } else {
        in->setEnabled(true);
        in->setToolTip("Set the number of the room in from this one, will be blue for a valid number or red for invalid.");
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
    if ( state==Qt::Checked ) {
        if ( mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != 0 ) {
            out->setText("");
            out->setStyleSheet("");
            weight_out->setValue(0);
            noroute_out->setChecked(false);
        }
        noroute_out->setEnabled(false);
        out->setEnabled(false);
        out->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
        doortype_none_out->setEnabled(true);
        doortype_open_out->setEnabled(true);
        doortype_closed_out->setEnabled(true);
        doortype_locked_out->setEnabled(true);
        weight_out->setEnabled(false);
    } else {
        out->setEnabled(true);
        out->setToolTip("Set the number of the room out from this one, will be blue for a valid number or red for invalid.");
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

void dlgRoomExits::initExit( int roomId, int direction, int exitId, QLineEdit * exitLineEdit,
                             QCheckBox * noRoute, QCheckBox * stub,
                             QRadioButton * none, QRadioButton * open,
                             QRadioButton * closed, QRadioButton * locked,
                             QSpinBox * weight) {

    QString doorText;   // lowercase, initials
    QString weightText; // lowercase, full words, no space
    switch( direction ) {
        case DIR_NORTHWEST: doorText =   "nw"; weightText = "northwest"; break;
        case DIR_NORTH    : doorText =    "n"; weightText =     "north"; break;
        case DIR_NORTHEAST: doorText =   "ne"; weightText = "northeast"; break;
        case DIR_UP       : doorText =   "up"; weightText =        "up"; break;
        case DIR_WEST     : doorText =    "w"; weightText =      "west"; break;
        case DIR_EAST     : doorText =    "e"; weightText =      "east"; break;
        case DIR_DOWN     : doorText = "down"; weightText =      "down"; break;
        case DIR_SOUTHWEST: doorText =   "sw"; weightText = "southwest"; break;
        case DIR_SOUTH    : doorText =    "s"; weightText =     "south"; break;
        case DIR_SOUTHEAST: doorText =   "se"; weightText = "southeast"; break;
        case DIR_IN       : doorText =   "in"; weightText =        "in"; break;
        case DIR_OUT      : doorText =  "out"; weightText =       "out"; break;
        default: Q_UNREACHABLE();
    }

    weight->setValue( pR->hasExitWeight( weightText ) ? pR->getExitWeight( weightText ) : 0 );

    switch( pR->getDoor( doorText ) ) {
    case 0:   none->setChecked(true); break;
    case 1:   open->setChecked(true); break;
    case 2: closed->setChecked(true); break;
    case 3: locked->setChecked(true); break;
    default:
        qWarning("dlgRoomExits::initExit roomId(%i) unexpected doors[\"%s\"] value:%i found for room!",
                 roomId, qPrintable( doorText ), pR->getDoor( doorText ) );
    }

    if ( exitId > 0 ) { //Does this exit point anywhere
        exitLineEdit->setText(QString::number( exitId ));  //Put in the value
        exitLineEdit->setEnabled(true);     //Enable it for editing
        exitLineEdit->setStyleSheet("color:blue");
        if( mpHost->mpMap->mpRoomDB->getRoom( exitId )->name.trimmed().length() )
            exitLineEdit->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom( exitId )->name % "\""));
        else
            exitLineEdit->setToolTip(QString("Exit to unnamed room is valid"));
        noRoute->setEnabled(true);    //Enable speedwalk lock control
        none->setEnabled(true);   //Enable door type controls...
        open->setEnabled(true);
        closed->setEnabled(true);
        locked->setEnabled(true); //Already picked right one to check above here
        weight->setEnabled(true);   //Enable exit weight control...
        stub->setEnabled(false);  //Disable stub exit control, can't have one WITH an exit!
        stub->setChecked(false);  //Ensure stub exit isn't set
        noRoute->setChecked( pR->hasExitLock( direction ) );  //Set/reset "locK" control as appropriate
    } else {  //No exit is set on initialisation
        exitLineEdit->setText("");    //Nothing to put in exitID box
        exitLineEdit->setStyleSheet("");
        noRoute->setEnabled(false);   //Disable lock control, can't lock a non-existant exit..
        noRoute->setChecked(false);   //.. and ensure there isn't one
        weight->setEnabled(false);   //Disable exit weight control...
        weight->setValue(0);   //And reset to default value (which will now cause the room's one to be used
        stub->setEnabled(true);  //Enable stub exit control
        if ( pR->hasExitStub( direction ) ) {
            exitLineEdit->setEnabled(false); //There is a stub exit, so prevent exit number entry...
            exitLineEdit->setToolTip("Clear the stub exit for this exit to enter an exit roomID.");
            stub->setChecked(true);
            none->setEnabled(true);   //Enable door type controls, can have a door on a stub exit..
            open->setEnabled(true);
            closed->setEnabled(true);
            locked->setEnabled(true);
        } else {
            exitLineEdit->setEnabled(true);
            exitLineEdit->setToolTip("Set the number of the room " % weightText % " of this one, will be blue for a valid number or red for invalid.");
            stub->setChecked(false);
            none->setEnabled(false);   //Disable door type controls, can't lock a non-existant exit..
            open->setEnabled(false);   //.. and ensure the "none" one is set if it ever gets enabled
            closed->setEnabled(false);
            locked->setEnabled(false);
            none->setChecked(true);
        }
    }
    originalExits[ direction ] = makeExitFromControls( direction );
}

void dlgRoomExits::init( int id ) {
    pR = mpHost->mpMap->mpRoomDB->getRoom( id );
    if ( !pR )
        return;

    roomID->setText(QString::number(id));
    roomWeight->setText(QString::number(pR->getWeight()));
    QString titleText;
    if( pR->name.trimmed().length() )
        titleText = QString("Exits for room: \"%1\" [*]").arg(pR->name);
    else
        titleText = QString("Exits for room Id: %1 [*]").arg(id);

    this->setWindowTitle(titleText);

    // Because we are manipulating the settings for the exit we need to know
    // explicitly where the weight comes from, pR->getExitWeight() hides that
    // detail deliberately for normal usage
    initExit( id, DIR_NORTHWEST, pR->getExit(DIR_NORTHWEST), nw, noroute_nw, stub_nw,
              doortype_none_nw, doortype_open_nw, doortype_closed_nw,
              doortype_locked_nw, weight_nw);

    initExit( id, DIR_NORTH, pR->getExit(DIR_NORTH), n, noroute_n, stub_n,
              doortype_none_n, doortype_open_n, doortype_closed_n,
              doortype_locked_n, weight_n);

    initExit( id, DIR_NORTHEAST, pR->getExit(DIR_NORTHEAST), ne, noroute_ne, stub_ne,
              doortype_none_ne, doortype_open_ne, doortype_closed_ne,
              doortype_locked_ne, weight_ne);

    initExit( id, DIR_UP, pR->getExit(DIR_UP), up, noroute_up, stub_up,
              doortype_none_up, doortype_open_up, doortype_closed_up,
              doortype_locked_up, weight_up);

    initExit( id, DIR_WEST, pR->getExit(DIR_WEST), w, noroute_w, stub_w,
              doortype_none_w, doortype_open_w, doortype_closed_w,
              doortype_locked_w, weight_w);

    initExit( id, DIR_EAST, pR->getExit(DIR_EAST), e, noroute_e, stub_e,
              doortype_none_e, doortype_open_e, doortype_closed_e,
              doortype_locked_e, weight_e);

    initExit( id, DIR_DOWN, pR->getExit(DIR_DOWN), down, noroute_down, stub_down,
              doortype_none_down, doortype_open_down, doortype_closed_down,
              doortype_locked_down, weight_down);

    initExit( id, DIR_SOUTHWEST, pR->getExit(DIR_SOUTHWEST), sw, noroute_sw, stub_sw,
              doortype_none_sw, doortype_open_sw, doortype_closed_sw,
              doortype_locked_sw, weight_sw);

    initExit( id, DIR_SOUTH, pR->getExit(DIR_SOUTH), s, noroute_s, stub_s,
              doortype_none_s, doortype_open_s, doortype_closed_s,
              doortype_locked_s, weight_s);

    initExit( id, DIR_SOUTHEAST, pR->getExit(DIR_SOUTHEAST), se, noroute_se, stub_se,
              doortype_none_se, doortype_open_se, doortype_closed_se,
              doortype_locked_se, weight_se);

    initExit( id, DIR_IN, pR->getExit(DIR_IN), in, noroute_in, stub_in,
              doortype_none_in, doortype_open_in, doortype_closed_in,
              doortype_locked_in, weight_in);

    initExit( id, DIR_OUT, pR->getExit(DIR_OUT), out, noroute_out, stub_out,
              doortype_none_out, doortype_open_out, doortype_closed_out,
              doortype_locked_out, weight_out);

    QMapIterator<int, QString> it(pR->getOtherMap());
    while ( it.hasNext() ) {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if ( dir.size() < 1 )
            continue;
        if ( dir.startsWith('0') || dir.startsWith('1') )
            dir = dir.mid(1);  // Not sure if this will be relevent here??

        originalSpecialExits[dir] = new TExit();
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        //0 was locked, now exit roomID
        pI->setText( 0, QString::number(id_to) );
        pI->setTextAlignment( 0, Qt::AlignRight );
        originalSpecialExits.value( dir )->destination = id_to;
        //1 was roomID, now locked (or more properly "No route") - setCheckedState
        //automagically makes it a CheckBox!!!
        if ( pR->hasSpecialExitLock( id_to, dir ) ) {
            pI->setCheckState( 1, Qt::Checked );
            originalSpecialExits.value( dir )->hasNoRoute = true;
        } else {
            pI->setCheckState( 1, Qt::Unchecked );
            originalSpecialExits.value( dir )->hasNoRoute = false;
        }

        //2 was script, now exit weight - ideally want a spin box - but use a text edit for now
        if ( pR->hasExitWeight( dir ) )
            pI->setText( 2, QString::number(pR->getExitWeight(dir)) );
        else
            pI->setText( 2, QString::number(0) );
        pI->setTextAlignment( 2, Qt::AlignRight );
        originalSpecialExits.value( dir )->weight = pI->text(2).toInt();

        //3-6 are new, now holds a buttongroup of 4, ideally QRadioButtons, to select a door type
        pI->setCheckState( 3, Qt::Unchecked );
        pI->setCheckState( 4, Qt::Unchecked );
        pI->setCheckState( 5, Qt::Unchecked );
        pI->setCheckState( 6, Qt::Unchecked );
        {
            int specialDoor = pR->getDoor( dir );
            switch ( specialDoor ) {
            case 0:
                pI->setCheckState( 3, Qt::Checked );
                break;
            case 1:
                pI->setCheckState( 4, Qt::Checked );
                break;
            case 2:
                pI->setCheckState( 5, Qt::Checked );
                break;
            case 3:
                pI->setCheckState( 6, Qt::Checked );
                break;
            default:
                qDebug()<<"dlgRoomExits::init() unexpected (other exit) doors["<<dir<<"] value:"<<pR->doors[dir]<<" found for roomID["<<id<<"]!";
            }
            originalSpecialExits.value( dir )->door = specialDoor;
        }

        //7 is new, but holds the script that was in 2
        pI->setText( 7, dir );
        // Not relevent for special exits but better initialise it
        originalSpecialExits.value( dir )->hasStub = false;
    }
    mRoomID = id;
    button_save->setEnabled( false );
// We now do not connect up all these things until AFTER we have initialised
// things as some controls will issue unwanted signals upon setting values into
// them as we have above...
    connect( button_save,          SIGNAL(clicked()),                            this, SLOT(slot_endEditSpecialExits()));
    connect( button_save,          SIGNAL(clicked()),                            this, SLOT(slot_checkModified()));
    connect( button_save,          SIGNAL(clicked()),                            this, SLOT(save()));
    connect( button_addSpecialExit,SIGNAL(clicked()),                            this, SLOT(slot_addSpecialExit()));
    connect( specialExits,         SIGNAL(itemClicked( QTreeWidgetItem *, int)), this, SLOT(slot_editSpecialExit(QTreeWidgetItem *, int)));
    connect( specialExits,         SIGNAL(itemClicked( QTreeWidgetItem *, int)), this, SLOT(slot_checkModified()));
    connect( button_endEditing,    SIGNAL(clicked()),                            this, SLOT(slot_endEditSpecialExits()));
    connect( button_endEditing,    SIGNAL(clicked()),                            this, SLOT(slot_checkModified()));
    connect( nw,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_nw_textEdited(const QString &)));
    connect( n,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_n_textEdited(const QString &)));
    connect( ne,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_ne_textEdited(const QString &)));
    connect( up,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_up_textEdited(const QString &)));
    connect( w,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_w_textEdited(const QString &)));
    connect( e,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_e_textEdited(const QString &)));
    connect( down,                 SIGNAL(textEdited(const QString &)),          this, SLOT(slot_down_textEdited(const QString &)));
    connect( sw,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_sw_textEdited(const QString &)));
    connect( s,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_s_textEdited(const QString &)));
    connect( se,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_se_textEdited(const QString &)));
    connect( in,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_in_textEdited(const QString &)));
    connect( out,                  SIGNAL(textEdited(const QString &)),          this, SLOT(slot_out_textEdited(const QString &)));
    connect( stub_nw,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_nw_stateChanged(int)));
    connect( stub_n,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_n_stateChanged(int)));
    connect( stub_ne,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_ne_stateChanged(int)));
    connect( stub_up,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_up_stateChanged(int)));
    connect( stub_w,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_w_stateChanged(int)));
    connect( stub_e,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_e_stateChanged(int)));
    connect( stub_down,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_down_stateChanged(int)));
    connect( stub_sw,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_sw_stateChanged(int)));
    connect( stub_s,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_s_stateChanged(int)));
    connect( stub_se,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_se_stateChanged(int)));
    connect( stub_in,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_in_stateChanged(int)));
    connect( stub_out,             SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_out_stateChanged(int)));
    connect( noroute_nw,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_n,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_ne,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_up,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_w,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_e,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_down,         SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_sw,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_s,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_se,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_in,           SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( noroute_out,          SIGNAL(stateChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_nw,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_n,             SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_ne,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_up,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_w,             SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_e,             SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_down,          SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_sw,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_s,             SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_se,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_in,            SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( weight_out,           SIGNAL(valueChanged(int)),                    this, SLOT(slot_checkModified()));
    connect( doortype_nw,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_n,           SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_ne,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_up,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_w,           SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_e,           SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_down,        SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_sw,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_s,           SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_se,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_in,          SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
    connect( doortype_down,        SIGNAL(buttonClicked(int)),                   this, SLOT(slot_checkModified()));
}

TExit * dlgRoomExits::makeExitFromControls( int direction ) {
    TExit * exit = new TExit();
    switch( direction ) {
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

    TExit * originalExit = originalExits.value(DIR_NORTHWEST);
    TExit * currentExit = makeExitFromControls(DIR_NORTHWEST);

    if( * originalExit != * currentExit )
        isModified = true;
    delete currentExit;

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_NORTH);
        currentExit = makeExitFromControls(DIR_NORTH);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_NORTHEAST);
        currentExit = makeExitFromControls(DIR_NORTHEAST);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_UP);
        currentExit = makeExitFromControls(DIR_UP);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_WEST);
        currentExit = makeExitFromControls(DIR_WEST);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_EAST);
        currentExit = makeExitFromControls(DIR_EAST);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_DOWN);
        currentExit = makeExitFromControls(DIR_DOWN);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_SOUTHWEST);
        currentExit = makeExitFromControls(DIR_SOUTHWEST);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_SOUTH);
        currentExit = makeExitFromControls(DIR_SOUTH);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_SOUTHEAST);
        currentExit = makeExitFromControls(DIR_SOUTHEAST);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_IN);
        currentExit = makeExitFromControls(DIR_IN);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    if( ! isModified ) {
        originalExit = originalExits.value(DIR_OUT);
        currentExit = makeExitFromControls(DIR_OUT);
        if( * originalExit != * currentExit )
            isModified = true;
        delete currentExit;
    }

    // Detecting actual changes in the special exits is hard because of the
    // potential presence of new exits which may not be yet valid and thus will
    // not actually alter things if "save" is hit.
    // At the same time existing special exits which now have a empty/zero
    // value in the first (0) field will be deleted if "save"ed...
    if( ! isModified ) {
        int originalCount = originalSpecialExits.count();
        int currentCount = 0;
        for ( int i=0; i<specialExits->topLevelItemCount(); i++ ) {
            QTreeWidgetItem * pI = specialExits->topLevelItem(i);
/*            qDebug("dlgRoomExits::slot_checkModified() considering specialExit (item %i, pass 1) to:%i, command:%s",
 *                   i,
 *                   pI->text(0).toInt(),
 *                   qPrintable(pI->text(7)));
 */
            if( pI->text(7) == "<command or Lua script>"
                || pI->text(0).toInt() <= 0 )
                continue; // Ignore new or to be deleted entries
            currentCount++;
        }
        if( originalCount != currentCount )
            isModified = true;
        else {
            if( originalCount ) {
                QMap<QString, TExit *> foundMap = originalSpecialExits;
                // Now make a TExit value for each current (valid) specialExit
                // and search for it in the foundMap; remove matches and
                // if any non-matches or if any left in foundMap at end then
                // set isModified...
                for ( int i=0; i<specialExits->topLevelItemCount(); i++ ) {
                    QTreeWidgetItem * pI = specialExits->topLevelItem(i);
/*                    qDebug("dlgRoomExits::slot_checkModified() considering specialExit (item %i, pass 2) to:%i, command:%s",
 *                           i,
 *                           pI->text(0).toInt(),
 *                           qPrintable(pI->text(7)));
 */
                    if( pI->text(7) == "<command or Lua script>"
                        || pI->text(0).toInt() <= 0 )
                        continue; // Ignore new or to be deleted entries
                    QString currentCmd = pI->text(7);
                    TExit currentExit;
                    currentExit.destination = pI->text(0).toInt();
                    currentExit.hasNoRoute = pI->checkState(1)==Qt::Checked;
                    currentExit.door = pI->checkState(6) == Qt::Checked
                                       ? 3 : pI->checkState(5) == Qt::Checked
                                         ? 2 : pI->checkState(4) == Qt::Checked
                                           ? 1 : 0 ;
                    currentExit.weight = pI->text(2).toInt();
                    currentExit.hasStub = false;
                    if( foundMap.contains( currentCmd )
                        && foundMap.value( currentCmd )->destination == currentExit.destination
                        && foundMap.value( currentCmd )->door        == currentExit.door
                        && foundMap.value( currentCmd )->hasNoRoute  == currentExit.hasNoRoute
                        && foundMap.value( currentCmd )->weight      == currentExit.weight      )
                        foundMap.remove( currentCmd );
                    else {
                        isModified = true;
                        break;
                    }
                }
                if( foundMap.count() )
                    isModified = true;
            }
        }
    }
    setWindowModified( isModified );
    button_save->setEnabled( isModified );
}
