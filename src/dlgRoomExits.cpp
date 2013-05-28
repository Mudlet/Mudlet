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
#include "Host.h"
#include "TMap.h"
#include "dlgRoomExits.h"


dlgRoomExits::dlgRoomExits( Host * pH, QWidget * pW )
: mpHost( pH )
, mpEditItem( 0 )
, QDialog( pW )
{
    setupUi(this);
    connect( saveButton, SIGNAL(pressed()), this, SLOT(save()));
    connect( addSpecialExit,SIGNAL(pressed()), this, SLOT(slot_addSpecialExit()));
    connect( specialExits,SIGNAL(itemClicked ( QTreeWidgetItem *, int)), this, SLOT(slot_editItem(QTreeWidgetItem*,int)));
}

void dlgRoomExits::slot_editItem(QTreeWidgetItem * pI, int column )
{
    if( column == 0 || !pI )
    {
        if( mpEditItem ) specialExits->closePersistentEditor( mpEditItem, mEditColumn );
        mpEditItem = 0;
        mEditColumn = 0;
        return;
    }
    if( mpEditItem != 0 )
    {
        specialExits->closePersistentEditor( mpEditItem, mEditColumn );
        mpEditItem = 0;
    }
    mpEditItem = pI;
    mEditColumn = column;
    specialExits->openPersistentEditor(pI, column);
    specialExits->editItem(pI, column);
}

void dlgRoomExits::slot_addSpecialExit()
{
    QStringList sL;
    QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);//
    pI->setCheckState( 0, Qt::Unchecked );
    pI->setText(1, "<room ID>");
    pI->setText(2, "<command or Lua script>");
    specialExits->addTopLevelItem(pI);

}

#include "TRoom.h"

void dlgRoomExits::save()
{
    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    TRoom * pR = mpHost->mpMap->mpRoomDB->getRoom(mRoomID);
    if( !pR ) return;
    pR->clearSpecialExits();
    for( int i=0; i<specialExits->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem * pI = specialExits->topLevelItem(i);
        int key = pI->text(1).toInt();
        QString value = pI->text(2);
        //qDebug()<<"key="<<key<<" value=<"<<value<<">";
        if( value != "<command or Lua script>" && key != 0 )
        {
            if( pI->checkState( 0 ) == Qt::Unchecked )
                value = value.prepend( '0' );
            else
                value = value.prepend( '1' );
            //qDebug()<<"setting: key="<<key<<" val="<<value;
            pR->addSpecialExit( key, value );
        }
    }

    int id = pR->getId();
    mpHost->mpMap->setExit(id, n->text().toInt(), DIR_NORTH );
    mpHost->mpMap->setExit(id, nw->text().toInt(), DIR_NORTHWEST );
    mpHost->mpMap->setExit(id, ne->text().toInt(), DIR_NORTHEAST );
    mpHost->mpMap->setExit(id, s->text().toInt(), DIR_SOUTH );
    mpHost->mpMap->setExit(id, sw->text().toInt(), DIR_SOUTHWEST );
    mpHost->mpMap->setExit(id, se->text().toInt(), DIR_SOUTHEAST );
    mpHost->mpMap->setExit(id, e->text().toInt(), DIR_EAST );
    mpHost->mpMap->setExit(id, w->text().toInt(), DIR_WEST );
    mpHost->mpMap->setExit(id, up->text().toInt(), DIR_UP );
    mpHost->mpMap->setExit(id, down->text().toInt(), DIR_DOWN );
    mpHost->mpMap->setExit(id, in->text().toInt(), DIR_IN );
    mpHost->mpMap->setExit(id, out->text().toInt(), DIR_OUT );

    if( !n->isEnabled() )
        pR->setExitLock(DIR_NORTH, true);
    else
        pR->setExitLock(DIR_NORTH, false);
    if( !ne->isEnabled() )
        pR->setExitLock(DIR_NORTHEAST, true);
    else
        pR->setExitLock(DIR_NORTHEAST, false);
    if( !nw->isEnabled() )
        pR->setExitLock(DIR_NORTHWEST, true);
    else
        pR->setExitLock(DIR_NORTHWEST, false);
    if( !w->isEnabled() )
        pR->setExitLock(DIR_WEST, true);
    else
        pR->setExitLock(DIR_WEST, false);
    if( !e->isEnabled() )
        pR->setExitLock(DIR_EAST, true);
    else
        pR->setExitLock(DIR_EAST, false);
    if( !s->isEnabled() )
        pR->setExitLock(DIR_SOUTH, true);
    else
        pR->setExitLock(DIR_SOUTH, false);
    if( !se->isEnabled() )
        pR->setExitLock(DIR_SOUTHEAST, true);
    else
        pR->setExitLock(DIR_SOUTHEAST, false);
    if( !sw->isEnabled() )
        pR->setExitLock(DIR_SOUTHWEST, true);
    else
        pR->setExitLock(DIR_SOUTHWEST, false);
    if( !up->isEnabled() )
        pR->setExitLock(DIR_UP, true);
    else
        pR->setExitLock(DIR_UP, false);
    if( !down->isEnabled() )
        pR->setExitLock(DIR_DOWN, true);
    else
        pR->setExitLock(DIR_DOWN, false);
    if( !in->isEnabled() )
        pR->setExitLock(DIR_IN, true);
    else
        pR->setExitLock(DIR_IN, false);
    if( !out->isEnabled() )
        pR->setExitLock(DIR_OUT, true);
    else
        pR->setExitLock(DIR_OUT, false);

    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    close();
}

void dlgRoomExits::init( int id )
{
    TRoom * pR = mpHost->mpMap->mpRoomDB->getRoom( id );
    if( !pR ) return;

    if( pR->hasExitLock(DIR_NORTH) )
    {
        n->setDisabled(true);
        ln->setChecked(true);
    }
    else
    {
        n->setDisabled(false);
        ln->setChecked(false);
    }
    if( pR->hasExitLock(DIR_NORTHEAST) )
    {
        ne->setDisabled(true);
        lne->setChecked(true);
    }
    else
    {
        ne->setDisabled(false);
        le->setChecked(false);
    }
    if( pR->hasExitLock(DIR_NORTHWEST) )
    {
        nw->setDisabled(true);
        lnw->setChecked(true);
    }
    else
    {
        nw->setDisabled(false);
        lnw->setChecked(false);
    }
    if( pR->hasExitLock(DIR_SOUTH) )
    {
        s->setDisabled(true);
        ls->setChecked(true);
    }
    else
    {
        s->setDisabled(false);
        ls->setChecked(false);
    }
    if( pR->hasExitLock(DIR_SOUTHEAST) )
    {
        se->setDisabled(true);
        lse->setChecked(true);
    }
    else
    {
        se->setDisabled(false);
        lse->setChecked(false);
    }
    if( pR->hasExitLock(DIR_SOUTHWEST) )
    {
        sw->setDisabled(true);
        lsw->setChecked(true);
    }
    else
    {
        sw->setDisabled(false);
        lsw->setChecked(false);
    }
    if( pR->hasExitLock(DIR_WEST) )
    {
        w->setDisabled(true);
        lw->setChecked(true);
    }
    else
    {
        w->setDisabled(false);
        lw->setChecked(false);
    }
    if( pR->hasExitLock(DIR_EAST) )
    {
        e->setDisabled(true);
        le->setChecked(true);
    }
    else
    {
        e->setDisabled(false);
        le->setChecked(false);
    }
    if( pR->hasExitLock(DIR_UP) )
    {
        up->setDisabled(true);
        lup->setChecked(true);
    }
    else
    {
        up->setDisabled(false);
        lup->setChecked(false);
    }
    if( pR->hasExitLock(DIR_DOWN) )
    {
        down->setDisabled(true);
        ldown->setChecked(true);
    }
    else
    {
        down->setDisabled(false);
        ldown->setChecked(false);
    }
    if( pR->hasExitLock(DIR_IN) )
    {
        in->setDisabled(true);
        lin->setChecked(true);
    }
    else
    {
        in->setDisabled(false);
        lin->setChecked(false);
    }
    if( pR->hasExitLock(DIR_OUT) )
    {
        out->setDisabled(true);
        lout->setChecked(true);
    }
    else
    {
        out->setDisabled(false);
        lout->setChecked(false);
    }

    if( pR->getNorth() > 0 )
        n->setText(QString::number(pR->getNorth()));
    if( pR->getSouth() > 0 )
        s->setText(QString::number(pR->getSouth()));
    if( pR->getWest() > 0 )
        w->setText(QString::number(pR->getWest()));
    if( pR->getEast() > 0 )
        e->setText(QString::number(pR->getEast()));
    if( pR->getNorthwest() > 0 )
        nw->setText(QString::number(pR->getNorthwest()));
    if( pR->getNortheast() > 0 )
        ne->setText(QString::number(pR->getNortheast()));
    if( pR->getSouthwest() > 0 )
        sw->setText(QString::number(pR->getSouthwest()));
    if( pR->getSoutheast() > 0 )
        se->setText(QString::number(pR->getSoutheast()));
    if( pR->getUp() > 0 )
        up->setText(QString::number(pR->getUp()));
    if( pR->getDown() > 0 )
        down->setText(QString::number(pR->getDown()));
    if( pR->getIn() > 0 )
        in->setText(QString::number(pR->getIn()));
    if( pR->getOut() > 0 )
        out->setText(QString::number(pR->getOut()));

    QMapIterator<int, QString> it(pR->getOtherMap());
    while( it.hasNext() )
    {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if( dir.size() < 1 )
            continue;
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        if( pR->hasSpecialExitLock( id_to, dir ) )
            pI->setCheckState( 0, Qt::Checked );
        else
            pI->setCheckState( 0, Qt::Unchecked );
        pI->setText( 1, QString::number(id_to) );
        qDebug()<<"dir="<<dir;
        if( dir.startsWith('0') || dir.startsWith('1') ) dir = dir.mid(1);

        pI->setText( 2, dir );
    }
    mRoomID = id;
}


