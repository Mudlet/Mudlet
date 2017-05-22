/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "Host.h"
#include "TConsole.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include "post_guard.h"


dlgMapper::dlgMapper( QWidget * parent, Host * pH, TMap * pM )
: QWidget( parent )
, mpMap( pM )
, mpHost( pH )
, mShowDefaultArea( true )
{
    setupUi(this);

    glWidget->mpMap = pM;
    mp2dMap->mpMap = pM;
    mp2dMap->mpHost = pH;
    QMapIterator<int, QString> it(mpMap->mpRoomDB->getAreaNamesMap());
    //sort them alphabetically (case sensitive)
    QMap<QString, QString> areaNames;
    while (it.hasNext()) {
        it.next();
        QString name = it.value();
        areaNames.insert(name.toLower(), name);
    }
    //areaNames.sort();
    QMapIterator<QString, QString> areaIt(areaNames);
    while (areaIt.hasNext()) {
        areaIt.next();
        showArea->addItem(areaIt.value());
    }
    bubbles->setChecked(mpHost->mBubbleMode);
    mp2dMap->mBubbleMode = mpHost->mBubbleMode;
    d3buttons->setVisible(false);
    roomSize->setValue(mpHost->mRoomSize * 10);
    lineSize->setValue(mpHost->mLineSize);
    showInfo->setChecked(mpHost->mShowInfo);
    mp2dMap->mShowInfo = mpHost->mShowInfo;

    showRoomIDs->setChecked(mpHost->mShowRoomID);
    mp2dMap->mShowRoomID = mpHost->mShowRoomID;

    panel->setVisible(mpHost->mShowPanel);
    //searchList->setSelectionMode( QAbstractItemView::SingleSelection );
    //connect(roomID, SIGNAL(returnPressed()), this, SLOT(goRoom()));
    connect(bubbles, SIGNAL(clicked()), this, SLOT(slot_bubbles()));
    connect(showInfo, SIGNAL(clicked()), this, SLOT(slot_info()));
    connect(ortho, SIGNAL(pressed()), glWidget, SLOT(fullView()));
    connect(singleLevel, SIGNAL(pressed()), glWidget, SLOT(singleView()));
    connect(increaseTop, SIGNAL(pressed()), glWidget, SLOT(increaseTop()));
    connect(increaseBottom, SIGNAL(pressed()), glWidget, SLOT(increaseBottom()));
    connect(reduceTop, SIGNAL(pressed()), glWidget, SLOT(reduceTop()));
    connect(reduceBottom, SIGNAL(pressed()), glWidget, SLOT(reduceBottom()));
    //connect(searchList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(choseRoom(QListWidgetItem*)));
    connect(shiftZup, SIGNAL(pressed()), mp2dMap, SLOT(shiftZup()));
    connect(shiftZdown, SIGNAL(pressed()), mp2dMap, SLOT(shiftZdown()));
    connect(shiftLeft, SIGNAL(pressed()), mp2dMap, SLOT(shiftLeft()));
    connect(shiftRight, SIGNAL(pressed()), mp2dMap, SLOT(shiftRight()));
    connect(shiftUp, SIGNAL(pressed()), mp2dMap, SLOT(shiftUp()));
    connect(shiftDown, SIGNAL(pressed()), mp2dMap, SLOT(shiftDown()));

    connect(shiftZup, SIGNAL(pressed()), glWidget, SLOT(shiftZup()));
    connect(shiftZdown, SIGNAL(pressed()), glWidget, SLOT(shiftZdown()));
    connect(shiftLeft, SIGNAL(pressed()), glWidget, SLOT(shiftLeft()));
    connect(shiftRight, SIGNAL(pressed()), glWidget, SLOT(shiftRight()));
    connect(shiftUp, SIGNAL(pressed()), glWidget, SLOT(shiftUp()));
    connect(shiftDown, SIGNAL(pressed()), glWidget, SLOT(shiftDown()));
    connect(showInfo, SIGNAL(clicked()), glWidget, SLOT(showInfo()));
    connect(showArea, SIGNAL(activated(QString)), mp2dMap, SLOT(slot_switchArea(QString)));
    connect(defaultView, SIGNAL(pressed()), glWidget, SLOT(defaultView()));
    connect(dim2, SIGNAL(pressed()), this, SLOT(show2dView()));
    connect(sideView, SIGNAL(pressed()), glWidget, SLOT(sideView()));
    connect(topView, SIGNAL(pressed()), glWidget, SLOT(topView()));
    connect(togglePanel, SIGNAL(pressed()), this, SLOT(slot_togglePanel()));
    connect(lineSize, SIGNAL(valueChanged(int)), this, SLOT(slot_lineSize(int)));
    connect(roomSize, SIGNAL(valueChanged(int)), this, SLOT(slot_roomSize(int)));
    connect(scale, SIGNAL(valueChanged(int)), glWidget, SLOT(setScale(int)));
    connect(xRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setXRotation(int)));
    connect(yRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setYRotation(int)));
    connect(zRot, SIGNAL(valueChanged(int)), glWidget, SLOT(setZRotation(int)));
    connect(showRoomIDs, SIGNAL(stateChanged(int)), this, SLOT(slot_toggleShowRoomIDs(int)));
    QFont mapperFont = QFont(mpHost->mDisplayFont.family());
    if (mpHost->mNoAntiAlias) {
        mapperFont.setStyleStrategy(QFont::NoAntialias);
    } else {
        mapperFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality));
    }
    setFont(mapperFont);
    // Explicitly set the font otherwise it changes between the Application and
    // the default System one as the mapper is docked and undocked!
    mp2dMap->mFontHeight = QFontMetrics(mpHost->mDisplayFont).height();
    glWidget->hide();
    mpMap->customEnvColors[257] = mpHost->mRed_2;
    mpMap->customEnvColors[258] = mpHost->mGreen_2;
    mpMap->customEnvColors[259] = mpHost->mYellow_2;
    mpMap->customEnvColors[260] = mpHost->mBlue_2;
    mpMap->customEnvColors[261] = mpHost->mMagenta_2;
    mpMap->customEnvColors[262] = mpHost->mCyan_2;
    mpMap->customEnvColors[263] = mpHost->mWhite_2;
    mpMap->customEnvColors[264] = mpHost->mBlack_2;
    mpMap->customEnvColors[265] = mpHost->mLightRed_2;
    mpMap->customEnvColors[266] = mpHost->mLightGreen_2;
    mpMap->customEnvColors[267] = mpHost->mLightYellow_2;
    mpMap->customEnvColors[268] = mpHost->mLightBlue_2;
    mpMap->customEnvColors[269] = mpHost->mLightMagenta_2;
    mpMap->customEnvColors[270] = mpHost->mLightCyan_2;
    mpMap->customEnvColors[271] = mpHost->mLightWhite_2;
    mpMap->customEnvColors[272] = mpHost->mLightBlack_2;
// Not needed as already explicitly called, IMHO - Slysven
//    qDebug()<<"dlgMapper constructor -> call T2DMap::init()";
//    mp2dMap->init();
}

void dlgMapper::updateAreaComboBox()
{
    QString oldValue = showArea->currentText(); // Remember where we were
    QMapIterator<int, QString> itAreaNamesA(mpMap->mpRoomDB->getAreaNamesMap());
    //insert sort them alphabetically (case INsensitive)
    QMap<QString, QString> _areaNames;
    while (itAreaNamesA.hasNext()) {
        itAreaNamesA.next();
        if (itAreaNamesA.key() == -1 && !mShowDefaultArea) {
            continue; // Skip the default area from the listing if so directed
        }

        uint deduplicate = 0;
        QString _name;
        do {
            _name = QStringLiteral("%1+%2").arg(itAreaNamesA.value().toLower(), QString::number(++deduplicate));
            // Use a different suffix separator to one that area names
            // deduplication uses ('_') - makes debugging easier?
        } while (_areaNames.contains(_name));
        _areaNames.insert(_name, itAreaNamesA.value());
    }

    showArea->clear();
    QMapIterator<QString, QString> itAreaNamesB(_areaNames);
    while (itAreaNamesB.hasNext()) {
        itAreaNamesB.next();
        showArea->addItem(itAreaNamesB.value());
    }
    showArea->setCurrentText(oldValue); // Try and reset to previous value
}

void dlgMapper::slot_toggleShowRoomIDs(int s)
{
    if (s == Qt::Checked)
        mp2dMap->mShowRoomID = true;
    else
        mp2dMap->mShowRoomID = false;
    mp2dMap->mpHost->mShowRoomID = mp2dMap->mShowRoomID;
    mp2dMap->update();
}

void dlgMapper::slot_toggleStrongHighlight(int v)
{
    mpHost->mMapStrongHighlight = v == Qt::Checked ? true : false;
    mp2dMap->update();
}

void dlgMapper::slot_togglePanel()
{
    panel->setVisible(!panel->isVisible());
    mpHost->mShowPanel = panel->isVisible();
}

void dlgMapper::show2dView()
{
    glWidget->setVisible(!glWidget->isVisible());
    mp2dMap->setVisible(!mp2dMap->isVisible());
    if (glWidget->isVisible()) {
        d3buttons->setVisible(true);
    } else {
        d3buttons->setVisible(false);
    }
}

void dlgMapper::choseRoom(QListWidgetItem* pT)
{
    QString txt = pT->text();

    QHashIterator<int, TRoom*> it(mpMap->mpRoomDB->getRoomMap());
    while (it.hasNext()) {
        it.next();
        int i = it.key();
        TRoom* pR = mpMap->mpRoomDB->getRoom(i);
        if (!pR) {
            continue;
        }
        if (pR->name == txt) {
            qDebug() << "found room id=" << i;
            mpMap->mTargetID = i;
            if (!mpMap->findPath(mpMap->mRoomIdHash.value(mpMap->mpHost->getName()), i)) {
                QString msg = "Cannot find a path to this room.\n";
                mpHost->mpConsole->printSystemMessage(msg);
            } else {
                mpMap->mpHost->startSpeedWalk();
            }
            break;
        }
    }
    mpHost->mpConsole->setFocus();
}

void dlgMapper::goRoom()
{
//    QString txt = roomID->text();
//    searchList->clear();
//    int id = txt.toInt();

//    if( id != 0 && mpMap->rooms.contains( id ) )
//    {
//        mpMap->mTargetID = id;
//        if( mpMap->findPath(0,0) )
//        {
//            qDebug()<<"glwidget: starting speedwalk path length="<<mpMap->mPathList.size();
//            mpMap->mpHost->startSpeedWalk();
//        }
//        else
//        {
//            QString msg = "Cannot find a path to this room.\n";
//            mpHost->mpConsole->printSystemMessage(msg);
//        }
//    }
//    else
//    {
//        QMapIterator<int, TRoom *> it( mpMap->rooms );
//        while( it.hasNext() )
//        {
//            it.next();
//            int i = it.key();
//            if( mpMap->rooms[i]->name.contains( txt, Qt::CaseInsensitive ) )
//            {
//                qDebug()<<"inserting match:"<<i;
//                searchList->addItem( mpMap->rooms[i]->name );
//            }
//        }
//    }
//    mpHost->mpConsole->setFocus();
}


void dlgMapper::slot_roomSize(int d)
{
    float s = (float)d / 10.0;
    mp2dMap->setRoomSize(s);
    mp2dMap->update();
}

void dlgMapper::slot_lineSize(int d)
{
    mp2dMap->setExitSize(d);
    mp2dMap->update();
}

void dlgMapper::slot_bubbles()
{
    mp2dMap->mBubbleMode = bubbles->isChecked();
    mp2dMap->mpHost->mBubbleMode = mp2dMap->mBubbleMode;
    mp2dMap->update();
}

void dlgMapper::slot_info()
{
    mp2dMap->mShowInfo = showInfo->isChecked();
    mp2dMap->mpHost->mShowInfo = mp2dMap->mShowInfo;
    mp2dMap->update();
}

void dlgMapper::setDefaultAreaShown(bool state)
{
    if (mShowDefaultArea != state) {
        mShowDefaultArea = state;
        updateAreaComboBox();
    }
}

void dlgMapper::resetAreaComboBoxToPlayerRoomArea()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    TRoom* pR = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(pHost->getName()));
    if (pR) {
        int playerRoomArea = pR->getArea();
        TArea* pA = mpMap->mpRoomDB->getArea(playerRoomArea);
        if (pA) {
            QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(playerRoomArea);
            if (!areaName.isEmpty()) {
                showArea->setCurrentText(areaName);
            } else {
                qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room area name not valid.";
            }
        } else {
            qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room area valid.";
        }
    } else {
        qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room not valid.";
    }
}
