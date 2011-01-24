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
, QDialog( pW )
{
    setupUi(this);
    connect( saveButton, SIGNAL(pressed()), this, SLOT(save()));
}


void dlgRoomExits::save()
{
    qDebug()<<"exits saved clicked";
    if( !mpHost->mpMap->rooms.contains(mRoomID) ) return;

    mpHost->mpMap->rooms[mRoomID]->other.clear();

    for( int i=0; i<specialExits->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem * pI = specialExits->topLevelItem(i);
        int key = pI->text(0).toInt();
        QString value = pI->text(1);
        mpHost->mpMap->rooms[mRoomID]->other.insertMulti( key, value );
    }
qDebug()<<"#####trace###4958---1";
    mpHost->mpMap->rooms[mRoomID]->north = n->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->south = s->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->west = w->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->east = e->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->northeast = ne->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->northwest = nw->text().toInt();
qDebug()<<"#####trace###4958---2";
    mpHost->mpMap->rooms[mRoomID]->southeast = se->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->southwest = sw->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->up = up->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->down = down->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->in = in->text().toInt();
    mpHost->mpMap->rooms[mRoomID]->out = out->text().toInt();
qDebug()<<"#####trace###4958---3";
    if( ln->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTH, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTH, false);
qDebug()<<"#####trace###4958---1";
    if( lne->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHEAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHEAST, false);
    if( lnw->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHWEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHWEST, false);
    if( lw->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_WEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_WEST, false);
    if( le->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_EAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_EAST, false);
    if( ls->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTH, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTH, false);
    if( lse->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHEAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHEAST, false);
    if( lsw->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHWEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHWEST, false);
    if( lup->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_UP, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_UP, false);
    if( ldown->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_DOWN, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_DOWN, false);
    if( lin->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_IN, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_IN, false);
    if( lout->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_OUT, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_OUT, false);
qDebug()<<"#####trace###4958---1";
    accept();
}

void dlgRoomExits::init( int id )
{
    if( !mpHost->mpMap->rooms.contains(id) ) return;

    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTH) )
        ln->setDisabled(true);
    else
        ln->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTHEAST) )
        lne->setDisabled(true);
    else
        lne->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTHWEST) )
        lnw->setDisabled(true);
    else
        lnw->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTH) )
        ls->setDisabled(true);
    else
        ls->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTHEAST) )
        lse->setDisabled(true);
    else
        lse->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTHWEST) )
        lsw->setDisabled(true);
    else
        lsw->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_WEST) )
        lw->setDisabled(true);
    else
        lw->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_EAST) )
        le->setDisabled(true);
    else
        le->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_UP) )
        lup->setDisabled(true);
    else
        lup->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_DOWN) )
        ldown->setDisabled(true);
    else
        ldown->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_IN) )
        lin->setDisabled(true);
    else
        lin->setDisabled(false);
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_OUT) )
        lout->setDisabled(true);
    else
        lout->setDisabled(false);

    n->setText(QString::number(mpHost->mpMap->rooms[id]->north));
    s->setText(QString::number(mpHost->mpMap->rooms[id]->south));
    w->setText(QString::number(mpHost->mpMap->rooms[id]->west));
    e->setText(QString::number(mpHost->mpMap->rooms[id]->east));
    nw->setText(QString::number(mpHost->mpMap->rooms[id]->northwest));
    ne->setText(QString::number(mpHost->mpMap->rooms[id]->northeast));
    sw->setText(QString::number(mpHost->mpMap->rooms[id]->southwest));
    se->setText(QString::number(mpHost->mpMap->rooms[id]->southeast));
    up->setText(QString::number(mpHost->mpMap->rooms[id]->up));
    down->setText(QString::number(mpHost->mpMap->rooms[id]->down));
    in->setText(QString::number(mpHost->mpMap->rooms[id]->in));
    out->setText(QString::number(mpHost->mpMap->rooms[id]->out));

    QMapIterator<int, QString> it(mpHost->mpMap->rooms[id]->other);
    while( it.hasNext() )
    {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        pI->setText( 0, QString::number(id_to) );
        pI->setText( 1, dir );
    }
    mRoomID = id;
}



