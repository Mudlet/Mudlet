/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
#include <QDebug>
#include <QStringBuilder>
#include "Host.h"
#include "TMap.h"
#include "TRoom.h"
#include "dlgRoomExits.h"


dlgRoomExits::dlgRoomExits( Host * pH, QWidget * pW )
    : QDialog( pW )
    , mpHost( pH )
    , mpEditItem( 0 )
{
    setupUi(this);
    connect( button_save,          SIGNAL(pressed()),                            this, SLOT(slot_endEditSpecialExits()));
    connect( button_save,          SIGNAL(pressed()),                            this, SLOT(save()));
    connect( button_addSpecialExit,SIGNAL(pressed()),                            this, SLOT(slot_addSpecialExit()));
    connect( specialExits,         SIGNAL(itemClicked( QTreeWidgetItem *, int)), this, SLOT(slot_editSpecialExit(QTreeWidgetItem *, int)));
    connect( button_endEditing,    SIGNAL(pressed()),                            this, SLOT(slot_endEditSpecialExits()));
    connect( n,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_n_textEdited(const QString &)));
    connect( ne,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_ne_textEdited(const QString &)));
    connect( nw,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_nw_textEdited(const QString &)));
    connect( e,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_e_textEdited(const QString &)));
    connect( w,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_w_textEdited(const QString &)));
    connect( s,                    SIGNAL(textEdited(const QString &)),          this, SLOT(slot_s_textEdited(const QString &)));
    connect( se,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_se_textEdited(const QString &)));
    connect( sw,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_sw_textEdited(const QString &)));
    connect( up,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_up_textEdited(const QString &)));
    connect( down,                 SIGNAL(textEdited(const QString &)),          this, SLOT(slot_down_textEdited(const QString &)));
    connect( in,                   SIGNAL(textEdited(const QString &)),          this, SLOT(slot_in_textEdited(const QString &)));
    connect( out,                  SIGNAL(textEdited(const QString &)),          this, SLOT(slot_out_textEdited(const QString &)));
    connect( stub_n,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_n_stateChanged(int)));
    connect( stub_ne,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_ne_stateChanged(int)));
    connect( stub_nw,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_nw_stateChanged(int)));
    connect( stub_e,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_e_stateChanged(int)));
    connect( stub_w,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_w_stateChanged(int)));
    connect( stub_s,               SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_s_stateChanged(int)));
    connect( stub_se,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_se_stateChanged(int)));
    connect( stub_sw,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_sw_stateChanged(int)));
    connect( stub_up,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_up_stateChanged(int)));
    connect( stub_down,            SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_down_stateChanged(int)));
    connect( stub_in,              SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_in_stateChanged(int)));
    connect( stub_out,             SIGNAL(stateChanged(int)),                    this, SLOT(slot_stub_out_stateChanged(int)));
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
        case 2:
            mpEditItem->setText(2, QString::number( (mpEditItem->text(2).toInt() < 0) ? (-1 * mpEditItem->text(2).toInt()) : mpEditItem->text(2).toInt()) ); //Force result to be non-negative integer
            // run on into next case
        case 0:
        case 7:
            specialExits->closePersistentEditor( mpEditItem, mEditColumn );
            //            qDebug()<<"Closed PE on item:"<<mpEditItem->text(7)<<"column:"<<mEditColumn;
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

        default:
            ; //noop for other columns?
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
    specialExits->addTopLevelItem(pI);
}

// include of TRoom.h moved from here

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

    if (nw->isEnabled() && nw->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(nw->text().toInt()) != 0 )
    { // There IS a valid exit on the dialogue in this direction
        pR->setExit( nw->text().toInt(), DIR_NORTHWEST ); // So store it
        if (pR->hasExitStub(DIR_NORTHWEST))   // And ensure that stub exit is cleared if set
            pR->setExitStub(DIR_NORTHWEST, false);
        if (weight_nw->value())  // And store any weighing specifed
            pR->setExitWeight( "nw", weight_nw->value());
        else
            pR->setExitWeight( "nw", 0);
    }
    else
    { // No valid exit on the dialogue
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

    if (n->isEnabled() && n->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(n->text().toInt()) != 0 )
    {
        pR->setExit( n->text().toInt(), DIR_NORTH );
        if (pR->hasExitStub(DIR_NORTH))
            pR->setExitStub(DIR_NORTH, false);
        if (weight_n->value())
            pR->setExitWeight( "n", weight_n->value());
        else
            pR->setExitWeight( "n", 0);
    }
    else
    {
        pR->setExit( -1, DIR_NORTH );
        if (stub_n->isChecked() != pR->hasExitStub(DIR_NORTH))
            pR->setExitStub(DIR_NORTH, stub_n->isChecked());
        pR->setExitWeight( "n", 0);
        pR->customLinesArrow.remove( "N" );
        pR->customLinesColor.remove( "N" );
        pR->customLinesStyle.remove( "N" );
        pR->customLines.remove( "N" );
    }

    if (ne->isEnabled() && ne->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(ne->text().toInt()) != 0 )
    {
        pR->setExit( ne->text().toInt(), DIR_NORTHEAST );
        if (pR->hasExitStub(DIR_NORTHEAST))
            pR->setExitStub(DIR_NORTHEAST, false);
        if (weight_ne->value())
            pR->setExitWeight( "ne", weight_ne->value());
        else
            pR->setExitWeight( "ne", 0);
    }
    else
    {
        pR->setExit( -1, DIR_NORTHEAST );
        if (stub_ne->isChecked() != pR->hasExitStub(DIR_NORTHEAST))
            pR->setExitStub(DIR_NORTHEAST, stub_ne->isChecked());
        pR->setExitWeight( "ne", 0);
        pR->customLinesArrow.remove( "NE" );
        pR->customLinesColor.remove( "NE" );
        pR->customLinesStyle.remove( "NE" );
        pR->customLines.remove( "NE" );
    }

    if (up->isEnabled() && up->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(up->text().toInt()) != 0 )
    {
        pR->setExit( up->text().toInt(), DIR_UP );
        if (pR->hasExitStub(DIR_UP))
            pR->setExitStub(DIR_UP, false);
        if (weight_up->value())
            pR->setExitWeight( "up", weight_up->value());
        else
            pR->setExitWeight( "up", 0);
    }
    else
    {
        pR->setExit( -1, DIR_UP );
        if (stub_up->isChecked() != pR->hasExitStub(DIR_UP))
            pR->setExitStub(DIR_UP, stub_up->isChecked());
        pR->setExitWeight( "up", 0);
        pR->customLinesArrow.remove( "UP" );
        pR->customLinesColor.remove( "UP" );
        pR->customLinesStyle.remove( "UP" );
        pR->customLines.remove( "UP" );
    }

    if (w->isEnabled() && w->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(w->text().toInt()) != 0 )
    {
        pR->setExit( w->text().toInt(), DIR_WEST );
        if (pR->hasExitStub(DIR_WEST))
            pR->setExitStub(DIR_WEST, false);
        if (weight_w->value())
            pR->setExitWeight( "w", weight_w->value());
        else
            pR->setExitWeight( "w", 0);
    }
    else
    {
        pR->setExit( -1, DIR_WEST );
        if (stub_w->isChecked() != pR->hasExitStub(DIR_WEST))
            pR->setExitStub(DIR_WEST, stub_w->isChecked());
        pR->setExitWeight( "w", 0);
        pR->customLinesArrow.remove( "W" );
        pR->customLinesColor.remove( "W" );
        pR->customLinesStyle.remove( "W" );
        pR->customLines.remove( "W" );
    }

    if (e->isEnabled() && e->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(e->text().toInt()) != 0 )
    {
        pR->setExit( e->text().toInt(), DIR_EAST );
        if (pR->hasExitStub(DIR_EAST))
            pR->setExitStub(DIR_EAST, false);
        if (weight_e->value())
            pR->setExitWeight( "e", weight_e->value());
        else
            pR->setExitWeight( "e", 0);
    }
    else
    {
        pR->setExit( -1, DIR_EAST );
        if (stub_e->isChecked() != pR->hasExitStub(DIR_EAST))
            pR->setExitStub(DIR_EAST, stub_e->isChecked());
        pR->setExitWeight( "e", 0);
        pR->customLinesArrow.remove( "E" );
        pR->customLinesColor.remove( "E" );
        pR->customLinesStyle.remove( "E" );
        pR->customLines.remove( "E" );
    }

    if (down->isEnabled() && down->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(down->text().toInt()) != 0 )
    {
        pR->setExit( down->text().toInt(), DIR_DOWN );
        if (pR->hasExitStub(DIR_DOWN))
            pR->setExitStub(DIR_DOWN, false);
        if (weight_down->value())
            pR->setExitWeight( "down", weight_down->value());
        else
            pR->setExitWeight( "down", 0);
    }
    else
    {
        pR->setExit( -1, DIR_DOWN );
        if (stub_down->isChecked() != pR->hasExitStub(DIR_DOWN))
            pR->setExitStub(DIR_DOWN, stub_down->isChecked());
        pR->setExitWeight( "down", 0);
        pR->customLinesArrow.remove( "DOWN" );
        pR->customLinesColor.remove( "DOWN" );
        pR->customLinesStyle.remove( "DOWN" );
        pR->customLines.remove( "DOWN" );
    }

    if (sw->isEnabled() && sw->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(sw->text().toInt()) != 0 )
    {
        pR->setExit( sw->text().toInt(), DIR_SOUTHWEST );
        if (pR->hasExitStub(DIR_SOUTHWEST))
            pR->setExitStub(DIR_SOUTHWEST, false);
        if (weight_sw->value())
            pR->setExitWeight( "sw", weight_sw->value());
        else
            pR->setExitWeight( "sw", 0);
    }
    else
    {
        pR->setExit( -1, DIR_SOUTHWEST );
        if (stub_sw->isChecked() != pR->hasExitStub(DIR_SOUTHWEST))
            pR->setExitStub(DIR_SOUTHWEST, stub_sw->isChecked());
        pR->setExitWeight( "sw", 0);
        pR->customLinesArrow.remove( "SW" );
        pR->customLinesColor.remove( "SW" );
        pR->customLinesStyle.remove( "SW" );
        pR->customLines.remove( "SW" );
    }

    if (s->isEnabled() && s->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(s->text().toInt()) != 0 )
    {
        pR->setExit( s->text().toInt(), DIR_SOUTH );
        if (pR->hasExitStub(DIR_SOUTH))
            pR->setExitStub(DIR_SOUTH, false);
        if (weight_s->value())
            pR->setExitWeight( "s", weight_s->value());
        else
            pR->setExitWeight( "s", 0);
    }
    else
    {
        pR->setExit( -1, DIR_SOUTH );
        if (stub_s->isChecked() != pR->hasExitStub(DIR_SOUTH))
            pR->setExitStub(DIR_SOUTH, stub_s->isChecked());
        pR->setExitWeight( "s", 0);
        pR->customLinesArrow.remove( "S" );
        pR->customLinesColor.remove( "S" );
        pR->customLinesStyle.remove( "S" );
        pR->customLines.remove( "S" );
    }

    if (se->isEnabled() && se->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(se->text().toInt()) != 0 )
    {
        pR->setExit( se->text().toInt(), DIR_SOUTHEAST );
        if (pR->hasExitStub(DIR_SOUTHEAST))
            pR->setExitStub(DIR_SOUTHEAST, false);
        if (weight_se->value())
            pR->setExitWeight( "se", weight_se->value());
        else
            pR->setExitWeight( "se", 0);
        pR->customLinesArrow.remove( "SE" );
        pR->customLinesColor.remove( "SE" );
        pR->customLinesStyle.remove( "SE" );
        pR->customLines.remove( "SE" );
    }
    else
    {
        pR->setExit( -1, DIR_SOUTHEAST );
        if (stub_se->isChecked() != pR->hasExitStub(DIR_SOUTHEAST))
            pR->setExitStub(DIR_SOUTHEAST, stub_se->isChecked());
        pR->setExitWeight( "se", 0);
    }

    if (in->isEnabled() && in->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(in->text().toInt()) != 0 )
    {
        pR->setExit( in->text().toInt(), DIR_IN );
        if (pR->hasExitStub(DIR_IN))
            pR->setExitStub(DIR_IN, false);
        if (weight_in->value())
            pR->setExitWeight( "in", weight_in->value());
        else
            pR->setExitWeight( "in", 0);
    }
    else
    {
        pR->setExit( -1, DIR_IN );
        if (stub_in->isChecked() != pR->hasExitStub(DIR_IN))
            pR->setExitStub(DIR_IN, stub_in->isChecked());
        pR->setExitWeight( "in", 0);
        pR->customLinesArrow.remove( "IN" );
        pR->customLinesColor.remove( "IN" );
        pR->customLinesStyle.remove( "IN" );
        pR->customLines.remove( "IN" );
    }

    if (out->isEnabled() && out->text().size() > 0 && mpHost->mpMap->mpRoomDB->getRoom(out->text().toInt()) != 0 )
    {
        pR->setExit( out->text().toInt(), DIR_OUT );
        if (pR->hasExitStub(DIR_OUT))
            pR->setExitStub(DIR_OUT, false);
        if (weight_out->value())
            pR->setExitWeight( "out", weight_out->value());
        else
            pR->setExitWeight( "out", 0);
    }
    else
    {
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
    {
        pR->setDoor( "nw", -2-doortype_nw->checkedId());
    }

    if (doortype_n->checkedId()<-1)
    {
        pR->setDoor( "n", -2-doortype_n->checkedId());
    }

    if (doortype_ne->checkedId()<-1)
    {
        pR->setDoor( "ne", -2-doortype_ne->checkedId());
    }

    if (doortype_up->checkedId()<-1)
    {
        pR->setDoor( "up", -2-doortype_up->checkedId());
    }

    if (doortype_w->checkedId()<-1)
    {
        pR->setDoor( "w", -2-doortype_w->checkedId());
    }

    if (doortype_e->checkedId()<-1)
    {
        pR->setDoor( "e", -2-doortype_e->checkedId());
    }

    if (doortype_down->checkedId()<-1)
    {
        pR->setDoor( "down", -2-doortype_down->checkedId());
    }

    if (doortype_sw->checkedId()<-1)
    {
        pR->setDoor( "sw", -2-doortype_sw->checkedId());
    }

    if (doortype_s->checkedId()<-1)
    {
        pR->setDoor( "s", -2-doortype_s->checkedId());
    }

    if (doortype_se->checkedId()<-1)
    {
        pR->setDoor( "se", -2-doortype_se->checkedId());
    }

    if (doortype_in->checkedId()<-1)
    {
        pR->setDoor( "in", -2-doortype_in->checkedId());
    }

    if (doortype_out->checkedId()<-1)
    {
        pR->setDoor( "in", -2-doortype_out->checkedId());
    }

    TArea * pA = mpHost->mpMap->mpRoomDB->getArea( pR->getArea() );
    if( pA )
    {
        pA->fast_ausgaengeBestimmen( pR->getId() );
    }
    close();
}


// These slots are called as the text for the exitID is edited
void dlgRoomExits::slot_nw_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        nw->setToolTip(toolTipText);
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
    frame_nw->setToolTip(toolTipText);
}

void dlgRoomExits::slot_n_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        n->setToolTip(toolTipText);
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
    frame_n->setToolTip(toolTipText);
}

void dlgRoomExits::slot_ne_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        ne->setToolTip(toolTipText);
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
    frame_ne->setToolTip(toolTipText);
}

void dlgRoomExits::slot_up_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        up->setToolTip(toolTipText);
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
    frame_up->setToolTip(toolTipText);
}

void dlgRoomExits::slot_w_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        w->setToolTip(toolTipText);
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
    frame_w->setToolTip(toolTipText);
}

void dlgRoomExits::slot_e_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\" [*]");
        e->setToolTip(toolTipText);
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
    frame_e->setToolTip(toolTipText);
}

void dlgRoomExits::slot_down_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        down->setToolTip(toolTipText);
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
    frame_down->setToolTip(toolTipText);
}

void dlgRoomExits::slot_sw_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        sw->setToolTip(toolTipText);
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
    frame_sw->setToolTip(toolTipText);
}

void dlgRoomExits::slot_s_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        s->setToolTip(toolTipText);
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
    frame_s->setToolTip(toolTipText);
}

void dlgRoomExits::slot_se_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        se->setToolTip(toolTipText);
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
    frame_se->setToolTip(toolTipText);
}

void dlgRoomExits::slot_in_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        in->setToolTip(toolTipText);
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
    frame_in->setToolTip(toolTipText);
}

void dlgRoomExits::slot_out_textEdited(const QString &text)
{
    QString toolTipText = "";
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
        toolTipText = "Exits to \"";
        toolTipText.append(exitToRoom->name);
        toolTipText.append("\"");
        out->setToolTip(toolTipText);
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
    frame_out->setToolTip(toolTipText);
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
        //  noroute_ne->setEnabled(true); although this branch will enable the exit entry
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
}

void dlgRoomExits::init( int id )
{
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
    if (pR->hasExitWeight("nw"))
        weight_nw->setValue(pR->getExitWeight("nw"));
    else
        weight_nw->setValue(0);

    if (pR->hasExitWeight("n"))
        weight_n->setValue(pR->getExitWeight("n"));
    else
        weight_n->setValue(0);

    if (pR->hasExitWeight("ne"))
        weight_ne->setValue(pR->getExitWeight("ne"));
    else
        weight_ne->setValue(0);

    if (pR->hasExitWeight("up"))
        weight_up->setValue(pR->getExitWeight("up"));
    else
        weight_up->setValue(0);

    if (pR->hasExitWeight("w"))
        weight_w->setValue(pR->getExitWeight("w"));
    else
        weight_w->setValue(0);

    if (pR->hasExitWeight("e"))
        weight_e->setValue(pR->getExitWeight("e"));
    else
        weight_e->setValue(0);

    if (pR->hasExitWeight("sw"))
        weight_sw->setValue(pR->getExitWeight("sw"));
    else
        weight_sw->setValue(0);

    if (pR->hasExitWeight("s"))
        weight_s->setValue(pR->getExitWeight("s"));
    else
        weight_s->setValue(0);

    if (pR->hasExitWeight("se"))
        weight_se->setValue(pR->getExitWeight("se"));
    else
        weight_se->setValue(0);

    if (pR->hasExitWeight("in"))
        weight_in->setValue(pR->getExitWeight("in"));
    else
        weight_in->setValue(0);

    if (pR->hasExitWeight("out"))
        weight_out->setValue(pR->getExitWeight("out"));
    else
        weight_out->setValue(0);

    QStringList keys = pR->doors.keys();
    for ( int i=0; i<keys.size(); i++ ) {
        // qDebug()<<"dlgRoomExits::init() doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<"found for roomID["<<id<<"]!";
        if (keys.value(i).compare("nw")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_nw->setChecked(true); break;
            case 1: doortype_open_nw->setChecked(true); break;
            case 2: doortype_closed_nw->setChecked(true); break;
            case 3: doortype_locked_nw->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("n")==0) {
            switch(pR->doors[keys[i]])
            {
            case 0: doortype_none_n->setChecked(true); break;
            case 1: doortype_open_n->setChecked(true); break;
            case 2: doortype_closed_n->setChecked(true); break;
            case 3: doortype_locked_n->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("ne")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_ne->setChecked(true); break;
            case 1: doortype_open_ne->setChecked(true); break;
            case 2: doortype_closed_ne->setChecked(true); break;
            case 3: doortype_locked_ne->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("up")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_up->setChecked(true); break;
            case 1: doortype_open_up->setChecked(true); break;
            case 2: doortype_closed_up->setChecked(true); break;
            case 3: doortype_locked_up->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("w")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_w->setChecked(true); break;
            case 1: doortype_open_w->setChecked(true); break;
            case 2: doortype_closed_w->setChecked(true); break;
            case 3: doortype_locked_w->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("e")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_e->setChecked(true); break;
            case 1: doortype_open_e->setChecked(true); break;
            case 2: doortype_closed_e->setChecked(true); break;
            case 3: doortype_locked_e->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("down")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_down->setChecked(true); break;
            case 1: doortype_open_down->setChecked(true); break;
            case 2: doortype_closed_down->setChecked(true); break;
            case 3: doortype_locked_down->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("sw")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_sw->setChecked(true); break;
            case 1: doortype_open_sw->setChecked(true); break;
            case 2: doortype_closed_sw->setChecked(true); break;
            case 3: doortype_locked_sw->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("s")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_s->setChecked(true); break;
            case 1: doortype_open_s->setChecked(true); break;
            case 2: doortype_closed_s->setChecked(true); break;
            case 3: doortype_locked_s->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("se")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_se->setChecked(true); break;
            case 1: doortype_open_se->setChecked(true); break;
            case 2: doortype_closed_se->setChecked(true); break;
            case 3: doortype_locked_se->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("in")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_in->setChecked(true); break;
            case 1: doortype_open_in->setChecked(true); break;
            case 2: doortype_closed_in->setChecked(true); break;
            case 3: doortype_locked_in->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
        if (keys.value(i).compare("out")==0) {
            switch(pR->doors[keys[i]]) {
            case 0: doortype_none_out->setChecked(true); break;
            case 1: doortype_open_out->setChecked(true); break;
            case 2: doortype_closed_out->setChecked(true); break;
            case 3: doortype_locked_out->setChecked(true); break;
            default:
                qWarning()<<"dlgRoomExits::init() unexpected doors["<<keys[i]<<"] value:"<<pR->doors[keys[i]]<<" found for roomID["<<id<<"]!";
            }
        }
    }

    if ( pR->getNorthwest() > 0 ) { //Does this exit point anywhere
        nw->setText(QString::number(pR->getNorthwest()));  //Put in the value
        nw->setEnabled(true);     //Enable it for editing
        nw->setStyleSheet("color:blue");
        nw->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getNorthwest())->name % "\""));
        noroute_nw->setEnabled(true);    //Enable speedwalk lock control
        doortype_none_nw->setEnabled(true);   //Enable door type controls...
        doortype_open_nw->setEnabled(true);
        doortype_closed_nw->setEnabled(true);
        doortype_locked_nw->setEnabled(true); //Already picked right one to check above here
        weight_nw->setEnabled(true);   //Enable exit weight control...
        stub_nw->setEnabled(false);  //Disable stub exit control, can't have one WITH an exit!
        stub_nw->setChecked(false);  //Ensure stub exit isn't set
        if ( pR->hasExitLock(DIR_NORTHWEST) ) {  //Is exit locked for speedwalks?
            noroute_nw->setChecked(true);
        } else {
            noroute_nw->setChecked(false);
        }
    } else {  //No exit is set on initialisation
        nw->setText("");    //Nothing to put in exitID box
        nw->setStyleSheet("");
        nw->setToolTip("Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.");
        noroute_nw->setEnabled(false);   //Disable lock control, can't lock a non-existant exit..
        noroute_nw->setChecked(false);   //.. and ensure there isn't one
        weight_nw->setEnabled(false);   //Disable exit weight control...
        weight_nw->setValue(0);   //And reset to default value (which will now cause the room's one to be used
        stub_nw->setEnabled(true);  //Enable stub exit control
        if ( pR->hasExitStub(DIR_NORTHWEST) ) {
            nw->setEnabled(false); //There is a stub exit, so prevent exit number entry...
            stub_nw->setChecked(true);
            doortype_none_nw->setEnabled(true);   //Enable door type controls, can have a door on a stub exit..
            doortype_open_nw->setEnabled(true);
            doortype_closed_nw->setEnabled(true);
            doortype_locked_nw->setEnabled(true);
        } else {
            nw->setEnabled(true);
            stub_nw->setChecked(false);
            doortype_none_nw->setEnabled(false);   //Disable door type controls, can't lock a non-existant exit..
            doortype_open_nw->setEnabled(false);   //.. and ensure the "none" one is set if it ever gets enabled
            doortype_closed_nw->setEnabled(false);
            doortype_locked_nw->setEnabled(false);
            doortype_none_nw->setChecked(true);
        }
    }

    if ( pR->getNorth() > 0 ) {
        n->setText(QString::number(pR->getNorth()));
        n->setEnabled(true);
        n->setStyleSheet("color:blue");
        n->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getNorth())->name % "\""));
        noroute_n->setEnabled(true);
        doortype_none_n->setEnabled(true);
        doortype_open_n->setEnabled(true);
        doortype_closed_n->setEnabled(true);
        doortype_locked_n->setEnabled(true);
        weight_n->setEnabled(true);
        stub_n->setEnabled(false);
        stub_n->setChecked(false);
        if ( pR->hasExitLock(DIR_NORTH) ) {
            noroute_n->setChecked(true);
        } else {
            noroute_n->setChecked(false);
        }
    } else {
        n->setText("");
        n->setStyleSheet("");
        n->setToolTip("Set the number of the room north of this one, will be blue for a valid number or red for invalid.");
        noroute_n->setEnabled(false);
        noroute_n->setChecked(false);
        weight_n->setEnabled(false);
        weight_n->setValue(0);
        stub_n->setEnabled(true);
        if ( pR->hasExitStub(DIR_NORTH) ) {
            n->setEnabled(false);
            stub_n->setChecked(true);
            doortype_none_n->setEnabled(true);
            doortype_open_n->setEnabled(true);
            doortype_closed_n->setEnabled(true);
            doortype_locked_n->setEnabled(true);
        } else {
            n->setEnabled(true);
            stub_n->setChecked(false);
            doortype_none_n->setEnabled(false);
            doortype_open_n->setEnabled(false);
            doortype_closed_n->setEnabled(false);
            doortype_locked_n->setEnabled(false);
            doortype_none_n->setChecked(true);
        }
    }

    if ( pR->getNortheast() > 0 ) {
        ne->setText(QString::number(pR->getNortheast()));
        ne->setEnabled(true);
        ne->setStyleSheet("color:blue");
        ne->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getNortheast())->name % "\""));
        noroute_ne->setEnabled(true);
        doortype_none_ne->setEnabled(true);
        doortype_open_ne->setEnabled(true);
        doortype_closed_ne->setEnabled(true);
        doortype_locked_ne->setEnabled(true);
        weight_ne->setEnabled(true);
        stub_ne->setEnabled(false);
        stub_ne->setChecked(false);
        if ( pR->hasExitLock(DIR_NORTHEAST) ) {
            noroute_ne->setChecked(true);
        } else {
            noroute_ne->setChecked(false);
        }
    } else {
        ne->setText("");
        ne->setStyleSheet("");
        ne->setToolTip("Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.");
        noroute_ne->setEnabled(false);
        noroute_ne->setChecked(false);
        weight_ne->setEnabled(false);
        weight_ne->setValue(0);
        stub_ne->setEnabled(true);
        if ( pR->hasExitStub(DIR_NORTHEAST) ) {
            ne->setEnabled(false);
            stub_ne->setChecked(true);
            doortype_none_ne->setEnabled(true);
            doortype_open_ne->setEnabled(true);
            doortype_closed_ne->setEnabled(true);
            doortype_locked_ne->setEnabled(true);
        } else {
            ne->setEnabled(true);
            stub_ne->setChecked(false);
            doortype_none_ne->setEnabled(false);
            doortype_open_ne->setEnabled(false);
            doortype_closed_ne->setEnabled(false);
            doortype_locked_ne->setEnabled(false);
            doortype_none_ne->setChecked(true);
        }
    }

    if ( pR->getUp() > 0 ) {
        up->setText(QString::number(pR->getUp()));
        up->setEnabled(true);
        up->setStyleSheet("color:blue");
        up->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getUp())->name % "\""));
        noroute_up->setEnabled(true);
        doortype_none_up->setEnabled(true);
        doortype_open_up->setEnabled(true);
        doortype_closed_up->setEnabled(true);
        doortype_locked_up->setEnabled(true);
        weight_up->setEnabled(true);
        stub_up->setEnabled(false);
        stub_up->setChecked(false);
        if ( pR->hasExitLock(DIR_UP) ) {
            noroute_up->setChecked(true);
        } else {
            noroute_up->setChecked(false);
        }
    } else {
        up->setText("");
        up->setStyleSheet("");
        up->setToolTip("Set the number of the room up from this one, will be blue for a valid number or red for invalid.");
        noroute_up->setEnabled(false);
        noroute_up->setChecked(false);
        weight_up->setEnabled(false);
        weight_up->setValue(0);
        stub_up->setEnabled(true);
        if ( pR->hasExitStub(DIR_UP) ) {
            up->setEnabled(false);
            stub_up->setChecked(true);
            doortype_none_up->setEnabled(true);
            doortype_open_up->setEnabled(true);
            doortype_closed_up->setEnabled(true);
            doortype_locked_up->setEnabled(true);
        } else {
            up->setEnabled(true);
            stub_up->setChecked(false);
            doortype_none_up->setEnabled(false);
            doortype_open_up->setEnabled(false);
            doortype_closed_up->setEnabled(false);
            doortype_locked_up->setEnabled(false);
            doortype_none_up->setChecked(true);
        }
    }

    if ( pR->getWest() > 0 ) {
        w->setText(QString::number(pR->getWest()));
        w->setEnabled(true);
        w->setStyleSheet("color:blue");
        w->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getWest())->name % "\""));
        noroute_w->setEnabled(true);
        doortype_none_w->setEnabled(true);
        doortype_open_w->setEnabled(true);
        doortype_closed_w->setEnabled(true);
        doortype_locked_w->setEnabled(true);
        weight_w->setEnabled(true);
        stub_w->setEnabled(false);
        stub_w->setChecked(false);
        if ( pR->hasExitLock(DIR_WEST) )
            noroute_w->setChecked(true);
        else
            noroute_w->setChecked(false);
    } else {
        w->setText("");
        w->setStyleSheet("");
        w->setToolTip("Set the number of the room west of this one, will be blue for a valid number or red for invalid.");
        noroute_w->setEnabled(false);
        noroute_w->setChecked(false);
        weight_w->setEnabled(false);
        weight_w->setValue(0);
        stub_w->setEnabled(true);
        if ( pR->hasExitStub(DIR_WEST) ) {
            w->setEnabled(false);
            stub_w->setChecked(true);
            doortype_none_w->setEnabled(true);
            doortype_open_w->setEnabled(true);
            doortype_closed_w->setEnabled(true);
            doortype_locked_w->setEnabled(true);
        } else {
            w->setEnabled(true);
            stub_w->setChecked(false);
            doortype_none_w->setEnabled(false);
            doortype_open_w->setEnabled(false);
            doortype_closed_w->setEnabled(false);
            doortype_locked_w->setEnabled(false);
            doortype_none_w->setChecked(true);
        }
    }

    if ( pR->getEast() > 0 ) {
        e->setText(QString::number(pR->getEast()));
        e->setEnabled(true);
        e->setStyleSheet("color:blue");
        e->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getEast())->name % "\""));
        noroute_e->setEnabled(true);
        doortype_none_e->setEnabled(true);
        doortype_open_e->setEnabled(true);
        doortype_closed_e->setEnabled(true);
        doortype_locked_e->setEnabled(true);
        weight_e->setEnabled(true);
        stub_e->setEnabled(false);
        stub_e->setChecked(false);
        if ( pR->hasExitLock(DIR_EAST) )
            noroute_e->setChecked(true);
        else
            noroute_e->setChecked(false);
    } else {
        e->setText("");
        e->setStyleSheet("");
        e->setToolTip("Set the number of the room east of this one, will be blue for a valid number or red for invalid.");
        noroute_e->setEnabled(false);
        noroute_e->setChecked(false);
        weight_e->setEnabled(false);
        weight_e->setValue(0);
        stub_e->setEnabled(true);
        if ( pR->hasExitStub(DIR_EAST) ) {
            e->setEnabled(false);
            stub_e->setChecked(true);
            doortype_none_e->setEnabled(true);
            doortype_open_e->setEnabled(true);
            doortype_closed_e->setEnabled(true);
            doortype_locked_e->setEnabled(true);
        } else {
            e->setEnabled(true);
            stub_e->setChecked(false);
            doortype_none_e->setEnabled(false);
            doortype_open_e->setEnabled(false);
            doortype_closed_e->setEnabled(false);
            doortype_locked_e->setEnabled(false);
            doortype_none_e->setChecked(true);
        }
    }

    if ( pR->getDown() > 0 ) {
        down->setText(QString::number(pR->getDown()));
        down->setEnabled(true);
        down->setStyleSheet("color:blue");
        down->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getDown())->name % "\""));
        noroute_down->setEnabled(true);
        doortype_none_down->setEnabled(true);
        doortype_open_down->setEnabled(true);
        doortype_closed_down->setEnabled(true);
        doortype_locked_down->setEnabled(true);
        weight_down->setEnabled(true);
        stub_down->setEnabled(false);
        stub_down->setChecked(false);
        if ( pR->hasExitLock(DIR_DOWN) )
            noroute_down->setChecked(true);
        else
            noroute_down->setChecked(false);
    } else {
        down->setText("");
        down->setStyleSheet("");
        down->setToolTip("Set the number of the room down from this one, will be blue for a valid number or red for invalid.");
        noroute_down->setEnabled(false);
        noroute_down->setChecked(false);
        weight_down->setEnabled(false);
        weight_down->setValue(0);
        stub_down->setEnabled(true);
        if ( pR->hasExitStub(DIR_DOWN) ) {
            down->setEnabled(false);
            stub_down->setChecked(true);
            doortype_none_down->setEnabled(true);
            doortype_open_down->setEnabled(true);
            doortype_closed_down->setEnabled(true);
            doortype_locked_down->setEnabled(true);
        } else {
            down->setEnabled(true);
            stub_down->setChecked(false);
            doortype_none_down->setEnabled(false);
            doortype_open_down->setEnabled(false);
            doortype_closed_down->setEnabled(false);
            doortype_locked_down->setEnabled(false);
            doortype_none_down->setChecked(true);
        }
    }

    if ( pR->getSouthwest() > 0 ) {
        sw->setText(QString::number(pR->getSouthwest()));
        sw->setEnabled(true);
        sw->setStyleSheet("color:blue");
        sw->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getSouthwest())->name % "\""));
        noroute_sw->setEnabled(true);
        doortype_none_sw->setEnabled(true);
        doortype_open_sw->setEnabled(true);
        doortype_closed_sw->setEnabled(true);
        doortype_locked_sw->setEnabled(true);
        weight_sw->setEnabled(true);
        stub_sw->setEnabled(false);
        stub_sw->setChecked(false);
        if ( pR->hasExitLock(DIR_SOUTHWEST) )
            noroute_sw->setChecked(true);
        else
            noroute_sw->setChecked(false);
    } else {
        sw->setText("");
        sw->setStyleSheet("");
        sw->setToolTip("Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.");
        noroute_sw->setEnabled(false);
        noroute_sw->setChecked(false);
        weight_sw->setEnabled(false);
        weight_sw->setValue(0);
        stub_sw->setEnabled(true);
        if( pR->hasExitStub(DIR_SOUTHWEST) ) {
            sw->setEnabled(false);
            stub_sw->setChecked(true);
            doortype_none_sw->setEnabled(true);
            doortype_open_sw->setEnabled(true);
            doortype_closed_sw->setEnabled(true);
            doortype_locked_sw->setEnabled(true);
        } else {
            sw->setEnabled(true);
            stub_sw->setChecked(false);
            doortype_none_sw->setEnabled(false);
            doortype_open_sw->setEnabled(false);
            doortype_closed_sw->setEnabled(false);
            doortype_locked_sw->setEnabled(false);
            doortype_none_sw->setChecked(true);
        }
    }

    if ( pR->getSouth() > 0 ) {
        s->setText(QString::number(pR->getSouth()));
        s->setEnabled(true);
        s->setStyleSheet("color:blue");
        s->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getSouth())->name % "\""));
        noroute_s->setEnabled(true);
        doortype_none_s->setEnabled(true);
        doortype_open_s->setEnabled(true);
        doortype_closed_s->setEnabled(true);
        doortype_locked_s->setEnabled(true);
        weight_s->setEnabled(true);
        stub_s->setEnabled(false);
        stub_s->setChecked(false);
        if ( pR->hasExitLock(DIR_SOUTH) )
            noroute_s->setChecked(true);
        else
            noroute_s->setChecked(false);
    } else {
        s->setText("");
        s->setStyleSheet("");
        s->setToolTip("Set the number of the room south of this one, will be blue for a valid number or red for invalid.");
        noroute_s->setEnabled(false);
        noroute_s->setChecked(false);
        weight_s->setEnabled(false);
        weight_s->setValue(0);
        stub_s->setEnabled(true);
        if ( pR->hasExitStub(DIR_SOUTH) ) {
            s->setEnabled(false);
            stub_s->setChecked(true);
            doortype_none_s->setEnabled(true);
            doortype_open_s->setEnabled(true);
            doortype_closed_s->setEnabled(true);
            doortype_locked_s->setEnabled(true);
        } else {
            s->setEnabled(true);
            stub_s->setChecked(false);
            doortype_none_s->setEnabled(false);
            doortype_open_s->setEnabled(false);
            doortype_closed_s->setEnabled(false);
            doortype_locked_s->setEnabled(false);
            doortype_none_s->setChecked(true);
        }
    }

    if ( pR->getSoutheast() > 0 ) {
        se->setText(QString::number(pR->getSoutheast()));
        se->setEnabled(true);
        se->setStyleSheet("color:blue");
        se->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getSoutheast())->name % "\""));
        noroute_se->setEnabled(true);
        doortype_none_se->setEnabled(true);
        doortype_open_se->setEnabled(true);
        doortype_closed_se->setEnabled(true);
        doortype_locked_se->setEnabled(true);
        weight_se->setEnabled(true);
        stub_se->setEnabled(false);
        stub_se->setChecked(false);
        if ( pR->hasExitLock(DIR_SOUTHEAST) )
            noroute_se->setChecked(true);
        else
            noroute_se->setChecked(false);
    } else {
        se->setText("");
        se->setStyleSheet("");
        se->setToolTip("Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.");
        noroute_se->setEnabled(false);
        noroute_se->setChecked(false);
        weight_se->setEnabled(false);
        weight_se->setValue(0);
        stub_se->setEnabled(true);
        if ( pR->hasExitStub(DIR_SOUTHEAST) ) {
            se->setEnabled(false);
            stub_se->setChecked(true);
            doortype_none_se->setEnabled(true);
            doortype_open_se->setEnabled(true);
            doortype_closed_se->setEnabled(true);
            doortype_locked_se->setEnabled(true);
        } else {
            se->setEnabled(true);
            stub_se->setChecked(false);
            doortype_none_se->setEnabled(false);
            doortype_open_se->setEnabled(false);
            doortype_closed_se->setEnabled(false);
            doortype_locked_se->setEnabled(false);
            doortype_none_se->setChecked(true);
        }
    }

    if ( pR->getIn() > 0 ) {
        in->setText(QString::number(pR->getIn()));
        in->setEnabled(true);
        in->setStyleSheet("color:blue");
        in->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getIn())->name % "\""));
        noroute_in->setEnabled(true);
        doortype_none_in->setEnabled(true);
        doortype_open_in->setEnabled(true);
        doortype_closed_in->setEnabled(true);
        doortype_locked_in->setEnabled(true);
        weight_in->setEnabled(true);
        stub_in->setEnabled(false);
        stub_in->setChecked(false);
        if ( pR->hasExitLock(DIR_IN) ) {
            noroute_in->setChecked(true);
        } else {
            noroute_in->setChecked(false);
        }
    } else {
        in->setText("");
        in->setStyleSheet("");
        in->setToolTip("Set the number of the room in from this one, will be blue for a valid number or red for invalid.");
        noroute_in->setEnabled(false);
        noroute_in->setChecked(false);
        weight_in->setEnabled(false);
        weight_in->setValue(0);
        stub_in->setEnabled(true);
        if ( pR->hasExitStub(DIR_IN) ) {
            in->setEnabled(false);
            stub_in->setChecked(true);
            doortype_none_in->setEnabled(true);
            doortype_open_in->setEnabled(true);
            doortype_closed_in->setEnabled(true);
            doortype_locked_in->setEnabled(true);
        } else {
            in->setEnabled(true);
            stub_in->setChecked(false);
            doortype_none_in->setEnabled(false);
            doortype_open_in->setEnabled(false);
            doortype_closed_in->setEnabled(false);
            doortype_locked_in->setEnabled(false);
            doortype_none_in->setChecked(true);
        }
    }

    if ( pR->getOut() > 0 ) {
        out->setText(QString::number(pR->getOut()));
        out->setEnabled(true);
        out->setStyleSheet("color:blue");
        out->setToolTip(QString("Exit to \"" % mpHost->mpMap->mpRoomDB->getRoom(pR->getOut())->name % "\""));
        noroute_out->setEnabled(true);
        doortype_none_out->setEnabled(true);
        doortype_open_out->setEnabled(true);
        doortype_closed_out->setEnabled(true);
        doortype_locked_out->setEnabled(true);
        weight_out->setEnabled(true);
        stub_out->setEnabled(false);
        stub_out->setChecked(false);
        if ( pR->hasExitLock(DIR_OUT) ) {
            noroute_out->setChecked(true);
        } else {
            noroute_out->setChecked(false);
        }
    } else {
        out->setText("");
        out->setStyleSheet("");
        out->setToolTip("Set the number of the room out from this one, will be blue for a valid number or red for invalid.");
        noroute_out->setEnabled(false);
        noroute_out->setChecked(false);
        weight_out->setEnabled(false);
        weight_out->setValue(0);
        stub_out->setEnabled(true);
        if ( pR->hasExitStub(DIR_OUT) ) {
            out->setEnabled(false);
            stub_out->setChecked(true);
            doortype_none_out->setEnabled(true);
            doortype_open_out->setEnabled(true);
            doortype_closed_out->setEnabled(true);
            doortype_locked_out->setEnabled(true);
        } else {
            out->setEnabled(true);
            stub_out->setChecked(false);
            doortype_none_out->setEnabled(false);
            doortype_open_out->setEnabled(false);
            doortype_closed_out->setEnabled(false);
            doortype_locked_out->setEnabled(false);
            doortype_none_out->setChecked(true);
        }
    }

    QMapIterator<int, QString> it(pR->getOtherMap());
    while ( it.hasNext() ) {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if ( dir.size() < 1 )
            continue;
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        //0 was locked, now exit roomID
        pI->setText( 0, QString::number(id_to) );
        pI->setTextAlignment( 0, Qt::AlignRight );
        //1 was roomID, now locked (or more properly "No route") - setCheckedState
        //automagically makes it a CheckBox!!!
        if ( pR->hasSpecialExitLock( id_to, dir ) ) {
            pI->setCheckState( 1, Qt::Checked );
        } else {
            pI->setCheckState( 1, Qt::Unchecked );
        }

        if ( dir.startsWith('0') || dir.startsWith('1') )
            dir = dir.mid(1);  // Not sure if this will be relevent here??

        //2 was script, now exit weight - ideally want a spin box - but use a text edit for now
        if ( pR->hasExitWeight( dir ) )
            pI->setText( 2, QString::number(pR->getExitWeight(dir)) );
        else
            pI->setText( 2, QString::number(0) );
        pI->setTextAlignment( 2, Qt::AlignRight );

        //3-6 are new, now holds a buttongroup of 4, ideally QRadioButtons, to select a door type
        if ( pR->doors.contains(dir) ) {
            switch (pR->doors.value(dir)) {
            case 0:
                pI->setCheckState( 3, Qt::Checked );
                pI->setCheckState( 4, Qt::Unchecked );
                pI->setCheckState( 5, Qt::Unchecked );
                pI->setCheckState( 6, Qt::Unchecked );
                break;
            case 1:
                pI->setCheckState( 3, Qt::Unchecked );
                pI->setCheckState( 4, Qt::Checked );
                pI->setCheckState( 5, Qt::Unchecked );
                pI->setCheckState( 6, Qt::Unchecked );
                break;
            case 2:
                pI->setCheckState( 3, Qt::Unchecked );
                pI->setCheckState( 4, Qt::Unchecked );
                pI->setCheckState( 5, Qt::Checked );
                pI->setCheckState( 6, Qt::Unchecked );
                break;
            case 3:
                pI->setCheckState( 3, Qt::Unchecked );
                pI->setCheckState( 4, Qt::Unchecked );
                pI->setCheckState( 5, Qt::Unchecked );
                pI->setCheckState( 6, Qt::Checked );
                break;
            default:
                qDebug()<<"dlgRoomExits::init() unexpected (other exit) doors["<<dir<<"] value:"<<pR->doors[dir]<<" found for roomID["<<id<<"]!";
            }
        } else {
            pI->setCheckState( 3, Qt::Checked );
            pI->setCheckState( 4, Qt::Unchecked );
            pI->setCheckState( 5, Qt::Unchecked );
            pI->setCheckState( 6, Qt::Unchecked );
        }

        //7 is new, but holds the script that was in 2
        pI->setText( 7, dir );
    }
}


