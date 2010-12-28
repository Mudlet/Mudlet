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

#include "dlgMapper.h"
#include "TDebug.h"
#include "Host.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>

dlgMapper::dlgMapper( QWidget * parent, Host * pH, TMap * pM )
: QWidget( parent )
, mpMap( pM )
, mpHost( pH )
{
    setupUi(this);

    glWidget->mpMap = pM;
    searchList->setSelectionMode( QAbstractItemView::SingleSelection );
    connect(roomID, SIGNAL(returnPressed()), this, SLOT(goRoom()));
    connect(ortho, SIGNAL(pressed()), glWidget, SLOT(fullView()));
    connect(singleLevel, SIGNAL(pressed()), glWidget, SLOT(singleView()));
    connect(increaseTop, SIGNAL(pressed()), glWidget, SLOT(increaseTop()));
    connect(increaseBottom, SIGNAL(pressed()), glWidget, SLOT(increaseBottom()));
    connect(reduceTop, SIGNAL(pressed()), glWidget, SLOT(reduceTop()));
    connect(reduceBottom, SIGNAL(pressed()), glWidget, SLOT(reduceBottom()));
    connect(searchList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(choseRoom(QListWidgetItem*)));

    connect(defaultView, SIGNAL(pressed()), glWidget, SLOT(defaultView()));
    connect(sideView, SIGNAL(pressed()), glWidget, SLOT(sideView()));
    connect(topView, SIGNAL(pressed()), glWidget, SLOT(topView()));
    connect(togglePanel, SIGNAL(pressed()), this, SLOT(slot_togglePanel()));

    connect(scale, SIGNAL(valueChanged(int)), glWidget, SLOT(setScale(int)));
    connect(xRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setXRotation(int)));
    connect(yRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setYRotation(int)));
    connect(zRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setZRotation(int)));

    mpDownloader = new QNetworkAccessManager( this );
    connect(mpDownloader, SIGNAL(finished(QNetworkReply*)),this, SLOT(replyFinished(QNetworkReply*)));
}

void dlgMapper::slot_togglePanel()
{
    qDebug()<<"TOGGLE";
    panel->setVisible(!panel->isVisible());
}

void dlgMapper::downloadMap()
{
    QString url = mpHost->mUrl;
    url.prepend("http://www.");
    url.append("/maps/map.xml");
    qDebug()<<"DOWNLOADING:"<<url;
    QNetworkReply * reply = mpDownloader->get( QNetworkRequest( QUrl( url ) ) );
    mpProgressDialog = new QProgressDialog("downloading map ...", "Abort", 0, 4000000, this);
    connect(reply, SIGNAL(downloadProgress( qint64, qint64 )), this, SLOT(setDownloadProgress(qint64,qint64)));
    mpProgressDialog->show();
}

void dlgMapper::setDownloadProgress( qint64 got, qint64 tot )
{
    mpProgressDialog->setRange(0, static_cast<int>(tot) );
    mpProgressDialog->setValue(static_cast<int>(got));
}

#include "XMLimport.h"

void dlgMapper::replyFinished( QNetworkReply * reply )
{
    qDebug()<<"download complete!";
    mpProgressDialog->close();

    QString name = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName()+"/map.xml";
    QFile file(name);
    file.open( QFile::WriteOnly );
    file.write( reply->readAll() );
    file.flush();
    file.close();

    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Import Map Package:"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(name)
                             .arg(file.errorString()));
        return;
    }

    XMLimport reader( mpHost );
    reader.importPackage( & file );

    mpHost->mpMap->init( mpHost );
    glWidget->updateGL();
}

void dlgMapper::choseRoom(QListWidgetItem * pT )
{
    QString txt = pT->text();

    QMapIterator<int, TRoom *> it( mpMap->rooms );
    while( it.hasNext() )
    {
        it.next();
        int i = it.key();
        if( mpMap->rooms[i]->name == txt )
        {
            qDebug()<<"found room id="<<i;
            mpMap->mTargetID = i;
            if( ! mpMap->findPath( mpMap->mRoomId, i ) )
            {
                QMessageBox msgBox;
                msgBox.setText("Cannot find a path to this room using regular exits.#2\n");
                msgBox.exec();
            }
            else
                mpMap->mpHost->startSpeedWalk();
            break;
        }
    }
}

void dlgMapper::goRoom()
{
    QString txt = roomID->text();
    searchList->clear();
    int id = txt.toInt();

    if( id != 0 && mpMap->rooms.contains( id ) )
    {
        mpMap->mTargetID = id;
        if( mpMap->findPath(0,0) )
        {
            qDebug()<<"glwidget: starting speedwalk path length="<<mpMap->mPathList.size();
            mpMap->mpHost->startSpeedWalk();
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Cannot find a path to this room using regular exits.#1\n");
            msgBox.exec();
        }
    }
    else
    {
        QMapIterator<int, TRoom *> it( mpMap->rooms );
        while( it.hasNext() )
        {
            it.next();
            int i = it.key();
            if( mpMap->rooms[i]->name.contains( txt, Qt::CaseInsensitive ) )
            {
                qDebug()<<"inserting match:"<<i;
                searchList->addItem( mpMap->rooms[i]->name );
            }
        }
    }
}
