/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "TMapView.h"

#include "Host.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "utils.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

TMapView::TMapView(int viewId, Host* pHost, TMap* pMap, QWidget* parent)
: QWidget(parent)
, mViewId(viewId)
, mpHost(pHost)
, mpMap(pMap)
{
    Q_ASSERT(pHost);
    Q_ASSERT(pMap);

    setupUi();

    mp2dMap->mpMap = pMap;
    mp2dMap->mpHost = pHost;
    mp2dMap->setSecondaryView(true);
    mp2dMap->setPlayerRoomStyle(pMap->mPlayerRoomStyle);

    updateAreaComboBox();

    if (mpHost) {
        mp2dMap->init();
    }

    connect(mpMap, &TMap::signal_areaChanged, this, [this](int areaId) {
        if (areaId == -1 || areaId == mp2dMap->getAreaId()) {
            mp2dMap->update();
        }
    });

    setFont(qApp->font());
    setPalette(QApplication::palette());
}

TMapView::~TMapView() = default;

void TMapView::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mp2dMap = new T2DMap(this);
    mp2dMap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(mp2dMap);

    mpTogglePanelButton = new QToolButton(this);
    mpTogglePanelButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpTogglePanelButton->setMaximumHeight(15);
    mpTogglePanelButton->setText(qsl("^"));
    mpTogglePanelButton->setCheckable(true);
    mpTogglePanelButton->setChecked(true);
    mainLayout->addWidget(mpTogglePanelButton);

    mpPanelWidget = new QWidget(this);
    mpPanelWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpPanelWidget->setAutoFillBackground(true);
    auto* panelLayout = new QHBoxLayout(mpPanelWidget);
    panelLayout->setSpacing(1);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    mpZUpButton = new QToolButton(mpPanelWidget);
    mpZUpButton->setMinimumSize(30, 20);
    mpZUpButton->setMaximumSize(30, 20);
    mpZUpButton->setFont(QFont(QString(), -1, QFont::Bold));
    mpZUpButton->setText(qsl("+"));
    //: Tooltip for z-level up button in secondary map view
    mpZUpButton->setToolTip(tr("Go up one z-level"));
    panelLayout->addWidget(mpZUpButton);

    mpZDownButton = new QToolButton(mpPanelWidget);
    mpZDownButton->setMinimumSize(30, 20);
    mpZDownButton->setMaximumSize(30, 20);
    mpZDownButton->setFont(QFont(QString(), -1, QFont::Bold));
    mpZDownButton->setText(qsl("-"));
    //: Tooltip for z-level down button in secondary map view
    mpZDownButton->setToolTip(tr("Go down one z-level"));
    panelLayout->addWidget(mpZDownButton);

    //: Label for area selection combobox in secondary map view
    auto* areaLabel = new QLabel(tr("Area:"), mpPanelWidget);
    areaLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    areaLabel->setMaximumHeight(20);
    panelLayout->addWidget(areaLabel);

    mpAreaComboBox = new QComboBox(mpPanelWidget);
    mpAreaComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpAreaComboBox->setMaximumHeight(20);
    mpAreaComboBox->setInsertPolicy(QComboBox::InsertAlphabetically);
    panelLayout->addWidget(mpAreaComboBox);

    mainLayout->addWidget(mpPanelWidget);

    connect(mpTogglePanelButton, &QAbstractButton::clicked, this, [this](bool checked) {
        mpPanelWidget->setVisible(checked);
    });
    connect(mpZUpButton, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZup);
    connect(mpZDownButton, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZdown);
    connect(mpAreaComboBox, qOverload<int>(&QComboBox::activated), this, &TMapView::slot_switchArea);
}

void TMapView::updateAreaComboBox()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        mpAreaComboBox->clear();
        mpAreaComboBox->setEnabled(false);
        return;
    }

    const QString oldValue = mpAreaComboBox->currentText();
    const auto& areaNamesMap = mpMap->mpRoomDB->getAreaNamesMap();

    QMap<QString, QString> areaNames;
    for (const auto& [areaId, areaName] : areaNamesMap.asKeyValueRange()) {
        if (areaId == -1 && !mpMap->getDefaultAreaShown()) {
            continue;
        }
        areaNames.insert(areaName.toLower(), areaName);
    }

    mpAreaComboBox->clear();
    for (const auto& [sortKey, areaName] : areaNames.asKeyValueRange()) {
        Q_UNUSED(sortKey)
        mpAreaComboBox->addItem(areaName);
    }

    if (!oldValue.isEmpty()) {
        const int index = mpAreaComboBox->findText(oldValue);
        if (index != -1) {
            mpAreaComboBox->setCurrentIndex(index);
        }
    }

    mpAreaComboBox->setEnabled(mpAreaComboBox->count() > 0);
}

void TMapView::slot_switchArea(int index)
{
    if (!mpMap || !mpMap->mpRoomDB || !mp2dMap) {
        qWarning() << "TMapView::slot_switchArea() - cannot switch area: invalid state";
        return;
    }

    const QString areaName = mpAreaComboBox->itemText(index);
    const int areaId = mpMap->mpRoomDB->getAreaNamesMap().key(areaName, -1);

    if (areaId != -1) {
        mp2dMap->switchArea(areaId);
    } else {
        qWarning() << "TMapView::slot_switchArea() - area" << areaName << "not found in area names map";
    }
}

void TMapView::setArea(int areaId)
{
    if (!mpMap || !mpMap->mpRoomDB || !mp2dMap) {
        qWarning() << "TMapView::setArea() - cannot set area" << areaId << ": invalid state";
        return;
    }

    const QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
    if (areaName.isEmpty()) {
        qWarning() << "TMapView::setArea() - area" << areaId << "not found";
        return;
    }

    const int index = mpAreaComboBox->findText(areaName);
    if (index != -1) {
        mpAreaComboBox->setCurrentIndex(index);
    } else {
        qWarning() << "TMapView::setArea() - area" << areaName << "not found in combo box for view" << mViewId;
    }

    mp2dMap->switchArea(areaId);
}

std::pair<bool, QString> TMapView::centerOnRoom(int roomId)
{
    if (!mp2dMap) {
        return {false, qsl("mp2dMap is null for view %1").arg(mViewId)};
    }
    return mp2dMap->centerview(roomId);
}

std::pair<bool, QString> TMapView::setZoom(qreal zoom)
{
    if (!mp2dMap) {
        return {false, qsl("mp2dMap is null for view %1").arg(mViewId)};
    }
    return mp2dMap->setMapZoom(zoom, getCurrentAreaId());
}

int TMapView::getCurrentAreaId() const
{
    if (!mp2dMap) {
        qWarning() << "TMapView::getCurrentAreaId() - mp2dMap is null for view" << mViewId;
        return -1;
    }
    return mp2dMap->getAreaId();
}

int TMapView::getCenteredRoomId() const
{
    if (!mp2dMap) {
        qWarning() << "TMapView::getCenteredRoomId() - mp2dMap is null for view" << mViewId;
        return 0;
    }
    return mp2dMap->getCenterRoomId();
}

qreal TMapView::getZoom() const
{
    if (!mp2dMap) {
        qWarning() << "TMapView::getZoom() - mp2dMap is null for view" << mViewId;
        return 0.0;
    }
    return mp2dMap->getZoom();
}

int TMapView::getZLevel() const
{
    if (!mp2dMap) {
        qWarning() << "TMapView::getZLevel() - mp2dMap is null for view" << mViewId;
        return 0;
    }
    return mp2dMap->getZLevel();
}
