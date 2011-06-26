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
    connect( addSpecialExit,SIGNAL(pressed()), this, SLOT(slot_addSpecialExit()));
    connect( specialExits,SIGNAL(itemClicked ( QTreeWidgetItem *, int)), this, SLOT(slot_editItem(QTreeWidgetItem*,int)));
}

void dlgRoomExits::slot_editItem(QTreeWidgetItem * pI, int column )
{
    if( column == 0 ) return;
    if( mpEditItem != 0 )
        specialExits->closePersistentEditor( mpEditItem, mEditColumn );
    mpEditItem = pI;
    mEditColumn = column;
    specialExits->openPersistentEditor(pI, column);
}

void dlgRoomExits::slot_addSpecialExit()
{
    QStringList sL;
    QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);//
    pI->setText(1, "<room ID>");
    pI->setText(2, "<command or Lua script>");
    specialExits->addTopLevelItem(pI);

}

void dlgRoomExits::save()
{
    if( !mpHost->mpMap->rooms.contains(mRoomID) ) return;

    mpHost->mpMap->mMapGraphNeedsUpdate = true;

    mpHost->mpMap->rooms[mRoomID]->other.clear();

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
            mpHost->mpMap->rooms[mRoomID]->other.insertMulti( key, value );
        }
    }
    if( n->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->north = n->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->north = -1;
    if( s->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->south = s->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->south = -1;
    if( w->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->west = w->text().toInt();
    else mpHost->mpMap->rooms[mRoomID]->west = -1;
    if( e->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->east = e->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->east = -1;
    if( ne->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->northeast = ne->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->northeast = -1;
    if( nw->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->northwest = nw->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->northwest = -1;
    if( se->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->southeast = se->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->southeast = -1;
    if( sw->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->southwest = sw->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->southwest = -1;
    if( up->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->up = up->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->up = -1;
    if( down->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->down = down->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->down = -1;
    if( in->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->in = in->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->in = -1;
    if( out->text().size() > 0 )
        mpHost->mpMap->rooms[mRoomID]->out = out->text().toInt();
    else
        mpHost->mpMap->rooms[mRoomID]->out = -1;

    if( !n->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTH, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTH, false);
    if( !ne->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHEAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHEAST, false);
    if( !nw->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHWEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_NORTHWEST, false);
    if( !w->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_WEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_WEST, false);
    if( !e->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_EAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_EAST, false);
    if( !s->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTH, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTH, false);
    if( !se->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHEAST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHEAST, false);
    if( !sw->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHWEST, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_SOUTHWEST, false);
    if( !up->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_UP, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_UP, false);
    if( !down->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_DOWN, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_DOWN, false);
    if( !in->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_IN, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_IN, false);
    if( !out->isEnabled() )
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_OUT, true);
    else
        mpHost->mpMap->rooms[mRoomID]->setExitLock(DIR_OUT, false);

    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    close();
}

void dlgRoomExits::init( int id )
{
    if( !mpHost->mpMap->rooms.contains(id) ) return;

    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTH) )
    {
        n->setDisabled(true);
        ln->setChecked(true);
    }
    else
    {
        n->setDisabled(false);
        ln->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTHEAST) )
    {
        ne->setDisabled(true);
        lne->setChecked(true);
    }
    else
    {
        ne->setDisabled(false);
        le->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_NORTHWEST) )
    {
        nw->setDisabled(true);
        lnw->setChecked(true);
    }
    else
    {
        nw->setDisabled(false);
        lnw->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTH) )
    {
        s->setDisabled(true);
        ls->setChecked(true);
    }
    else
    {
        s->setDisabled(false);
        ls->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTHEAST) )
    {
        se->setDisabled(true);
        lse->setChecked(true);
    }
    else
    {
        se->setDisabled(false);
        lse->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_SOUTHWEST) )
    {
        sw->setDisabled(true);
        lsw->setChecked(true);
    }
    else
    {
        sw->setDisabled(false);
        lsw->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_WEST) )
    {
        w->setDisabled(true);
        lw->setChecked(true);
    }
    else
    {
        w->setDisabled(false);
        lw->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_EAST) )
    {
        e->setDisabled(true);
        le->setChecked(true);
    }
    else
    {
        e->setDisabled(false);
        le->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_UP) )
    {
        up->setDisabled(true);
        lup->setChecked(true);
    }
    else
    {
        up->setDisabled(false);
        lup->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_DOWN) )
    {
        down->setDisabled(true);
        ldown->setChecked(true);
    }
    else
    {
        down->setDisabled(false);
        ldown->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_IN) )
    {
        in->setDisabled(true);
        lin->setChecked(true);
    }
    else
    {
        in->setDisabled(false);
        lin->setChecked(false);
    }
    if( mpHost->mpMap->rooms[id]->hasExitLock(DIR_OUT) )
    {
        out->setDisabled(true);
        lout->setChecked(true);
    }
    else
    {
        out->setDisabled(false);
        lout->setChecked(false);
    }

    if( mpHost->mpMap->rooms[id]->north > 0 )
        n->setText(QString::number(mpHost->mpMap->rooms[id]->north));
    if( mpHost->mpMap->rooms[id]->south > 0 )
        s->setText(QString::number(mpHost->mpMap->rooms[id]->south));
    if( mpHost->mpMap->rooms[id]->west > 0 )
        w->setText(QString::number(mpHost->mpMap->rooms[id]->west));
    if( mpHost->mpMap->rooms[id]->east > 0 )
        e->setText(QString::number(mpHost->mpMap->rooms[id]->east));
    if( mpHost->mpMap->rooms[id]->northwest > 0 )
        nw->setText(QString::number(mpHost->mpMap->rooms[id]->northwest));
    if( mpHost->mpMap->rooms[id]->northeast > 0 )
        ne->setText(QString::number(mpHost->mpMap->rooms[id]->northeast));
    if( mpHost->mpMap->rooms[id]->southwest > 0 )
        sw->setText(QString::number(mpHost->mpMap->rooms[id]->southwest));
    if( mpHost->mpMap->rooms[id]->southeast > 0 )
        se->setText(QString::number(mpHost->mpMap->rooms[id]->southeast));
    if( mpHost->mpMap->rooms[id]->up > 0 )
        up->setText(QString::number(mpHost->mpMap->rooms[id]->up));
    if( mpHost->mpMap->rooms[id]->down > 0 )
        down->setText(QString::number(mpHost->mpMap->rooms[id]->down));
    if( mpHost->mpMap->rooms[id]->in > 0 )
        in->setText(QString::number(mpHost->mpMap->rooms[id]->in));
    if( mpHost->mpMap->rooms[id]->out > 0 )
        out->setText(QString::number(mpHost->mpMap->rooms[id]->out));

    QMapIterator<int, QString> it(mpHost->mpMap->rooms[id]->other);
    while( it.hasNext() )
    {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if( dir.size() < 1 )
            continue;
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        if( mpHost->mpMap->rooms[id]->hasSpecialExitLock( id_to, dir ) )
            pI->setCheckState( 0, Qt::Checked );
        else
            pI->setCheckState( 0, Qt::Unchecked );
        pI->setText( 1, QString::number(id_to) );
        if( dir.startsWith('0') || dir.startsWith('1') ) dir = dir.mid(1);

        pI->setText( 2, dir );
    }
    mRoomID = id;
}



