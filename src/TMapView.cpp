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
#include "TRoom.h"
#include "TRoomDB.h"
#include "utils.h"
#if defined(INCLUDE_3DMAPPER)
#include "modern_glwidget.h"
#endif

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
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
#if defined(INCLUDE_3DMAPPER)
        if (showing3DView() && (areaId == -1 || areaId == getCurrentAreaId())) {
            mp3DView->update();
        }
#endif
    });
#if defined(INCLUDE_3DMAPPER)
    // The 3D view reads the mapper settings as it draws, so it needs only to draw again
    const auto repaint3DView = [this]() {
        if (showing3DView()) {
            mp3DView->update();
        }
    };
    connect(mpMap, &TMap::signal_mapRepaintRequested, this, repaint3DView);
    connect(mpMap, &TMap::signal_mapperSettingChanged, this, repaint3DView);
#endif
    connect(mpMap, &TMap::signal_symbolCachesStale, this, [this]() {
        mp2dMap->flushSymbolPixmapCache();
        mp2dMap->update();
    });
    connect(mpMap, &TMap::signal_mapperColoursChanged, this, [this]() {
        setPalette(QApplication::palette());
    });

    setFont(qApp->font());
    setPalette(QApplication::palette());
    update3DViewAvailability();
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

#if defined(INCLUDE_3DMAPPER)
    mp3DButton = new QToolButton(mpPanelWidget);
    mp3DButton->setMinimumSize(30, 20);
    mp3DButton->setMaximumHeight(20);
    //: Button in a secondary map window that shows its map in 3D
    mp3DButton->setText(tr("3D"));
    mp3DButton->setCheckable(true);
    panelLayout->addWidget(mp3DButton);
    connect(mp3DButton, &QAbstractButton::toggled, this, &TMapView::show3DView);
#endif

    mainLayout->addWidget(mpPanelWidget);

    connect(mpTogglePanelButton, &QAbstractButton::clicked, this, [this](bool checked) {
        mpPanelWidget->setVisible(checked);
    });
    connect(mpZUpButton, &QAbstractButton::clicked, this, [this]() {
#if defined(INCLUDE_3DMAPPER)
        if (showing3DView()) {
            mp3DView->slot_shiftZup();
            return;
        }
#endif
        mp2dMap->slot_shiftZup();
    });
    connect(mpZDownButton, &QAbstractButton::clicked, this, [this]() {
#if defined(INCLUDE_3DMAPPER)
        if (showing3DView()) {
            mp3DView->slot_shiftZdown();
            return;
        }
#endif
        mp2dMap->slot_shiftZdown();
    });
    connect(mpAreaComboBox, qOverload<int>(&QComboBox::activated), this, &TMapView::slot_switchArea);
}

void TMapView::updateAreaComboBox()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        mpAreaComboBox->clear();
        mpAreaComboBox->setEnabled(false);
        return;
    }

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

    // Re-select whichever entry names the area this view is actually
    // showing - not whatever text used to be selected, which goes stale
    // across a rename of that same area.
    const QString currentAreaName = mp2dMap ? areaNamesMap.value(mp2dMap->getAreaId()) : QString();
    if (!currentAreaName.isEmpty()) {
        const int index = mpAreaComboBox->findText(currentAreaName);
        if (index != -1) {
            mpAreaComboBox->setCurrentIndex(index);
        }
    }

    mpAreaComboBox->setEnabled(mpAreaComboBox->count() > 0);
}

void TMapView::switchToAnotherArea()
{
    updateAreaComboBox();
    if (mpAreaComboBox->count() > 0) {
        slot_switchArea(mpAreaComboBox->currentIndex());
    } else if (mp2dMap && mpMap) {
        mp2dMap->switchArea(mpMap->getDefaultAreaName());
    }
}

void TMapView::slot_switchArea(int index)
{
    if (!mpMap || !mpMap->mpRoomDB || !mp2dMap) {
        qWarning() << "TMapView::slot_switchArea() - cannot switch area: invalid state";
        return;
    }

    const QString areaName = mpAreaComboBox->itemText(index);
    // 0 is never a valid area id (the default area is -1, every other area
    // is >= 1), unlike -1, so it is safe to use as the "not found" sentinel
    // here - the default area's own name must still resolve to switching.
    constexpr int notFound = 0;
    const int areaId = mpMap->mpRoomDB->getAreaNamesMap().key(areaName, notFound);

    if (areaId != notFound) {
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
    const auto result = mp2dMap->centerview(roomId);
    if (result.first && showing3DView()) {
        sync3DViewFrom2D();
    }
    return result;
}

std::pair<bool, QString> TMapView::setZoom(qreal zoom)
{
    if (!mp2dMap) {
        return {false, qsl("mp2dMap is null for view %1").arg(mViewId)};
    }
    const auto result = mp2dMap->setMapZoom(zoom, getCurrentAreaId());
#if defined(INCLUDE_3DMAPPER)
    if (result.first && showing3DView()) {
        mp3DView->applyMapZoom(zoom, mp3DView->shownAreaId());
    }
#endif
    return result;
}

int TMapView::getCurrentAreaId() const
{
    if (!mp2dMap) {
        qWarning() << "TMapView::getCurrentAreaId() - mp2dMap is null for view" << mViewId;
        return -1;
    }
#if defined(INCLUDE_3DMAPPER)
    if (showing3DView()) {
        return mp3DView->shownAreaId();
    }
#endif
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
#if defined(INCLUDE_3DMAPPER)
    if (showing3DView()) {
        return qRound(mp3DView->viewCenter().z());
    }
#endif
    return mp2dMap->getZLevel();
}

ModernGLWidget* TMapView::get3DView() const
{
#if defined(INCLUDE_3DMAPPER)
    return mp3DView.data();
#else
    return nullptr;
#endif
}

bool TMapView::showing3DView() const
{
#if defined(INCLUDE_3DMAPPER)
    return mp3DView && !mp3DView->isHidden();
#else
    return false;
#endif
}

void TMapView::show3DView(const bool show)
{
#if defined(INCLUDE_3DMAPPER)
    if (show && (!mpHost || !mpMap || !mpHost->getUseModern3DMapper())) {
        update3DViewAvailability();
        return;
    }
    if (show == showing3DView()) {
        return;
    }
    if (show && !mp3DView) {
        auto* view = new ModernGLWidget(mpMap, mpHost, this);
        view->setObjectName(qsl("mapView3D_%1").arg(mViewId));
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        view->setSecondaryView(true);
        view->setCompanion2DMap(mp2dMap);
        view->hide();
        static_cast<QVBoxLayout*>(layout())->insertWidget(0, view);
        connect(view, &ModernGLWidget::editIn2DRequested, this, [this, view](const int roomId) {
            if (const TRoom* pRoom = mpMap ? mpMap->mpRoomDB->getRoom(roomId) : nullptr) {
                view->syncView(pRoom->getArea(), pRoom->x(), pRoom->y(), pRoom->z(), mp2dMap->getCenterRoomId());
            }
            if (mp2dMap->mMapViewOnly) {
                mp2dMap->slot_toggleMapViewOnly();
            }
            show3DView(false);
        });
        mp3DView = view;
        mp2dMap->setCompanion3DView(view);
    }

    if (show) {
        sync3DViewFrom2D();
    } else {
        sync2DViewFrom3D();
    }
    mp2dMap->setVisible(!show);
    mp3DView->setVisible(show);
    if (mp3DButton) {
        const QSignalBlocker blocker(mp3DButton);
        mp3DButton->setChecked(show);
    }
#else
    Q_UNUSED(show)
#endif
}

void TMapView::update3DViewAvailability()
{
#if defined(INCLUDE_3DMAPPER)
    const bool available = mpHost && mpHost->getUseModern3DMapper();
    if (!available && mp3DView) {
        show3DView(false);
        mp2dMap->setCompanion3DView(nullptr);
        // Later, as a script can get here from inside one of the view's own event handlers
        mp3DView->deleteLater();
        mp3DView = nullptr;
    }
    if (!mp3DButton) {
        return;
    }
    mp3DButton->setEnabled(available);
    if (available) {
        //: Tooltip for the button in a secondary map window that switches it between the 2D and 3D map
        mp3DButton->setToolTip(tr("Show this window's map in 3D"));
    } else {
        //: Tooltip for the disabled 3D button in a secondary map window; "Use the modern 3D mapper" is the checkbox on the Mapper page of Preferences, so translate it the same way
        mp3DButton->setToolTip(tr("Map windows show the map in 3D with the modern 3D mapper only; turn on \"Use the modern 3D mapper\" in Preferences, Mapper to switch to it"));
    }
    const QSignalBlocker blocker(mp3DButton);
    mp3DButton->setChecked(showing3DView());
#endif
}

void TMapView::sync3DViewFrom2D()
{
#if defined(INCLUDE_3DMAPPER)
    if (!mp3DView || !mpMap || !mpMap->mpRoomDB->getArea(mp2dMap->getAreaId())) {
        return;
    }
    // The 2D map's y-axis points down the screen
    mp3DView->syncView(mp2dMap->getAreaId(), qRound(mp2dMap->mMapCenterX), qRound(-mp2dMap->mMapCenterY), mp2dMap->mMapCenterZ, mp2dMap->getCenterRoomId());
#endif
}

void TMapView::sync2DViewFrom3D()
{
#if defined(INCLUDE_3DMAPPER)
    if (!mp3DView || !mpMap) {
        return;
    }
    const int areaId = mp3DView->shownAreaId();
    if (!mpMap->mpRoomDB->getArea(areaId)) {
        return;
    }
    // Read first: switchArea() recenters the 3D view as well
    const QVector3D center = mp3DView->viewCenter();
    if (areaId != mp2dMap->getAreaId()) {
        mp2dMap->switchArea(areaId);
        updateAreaComboBox();
    }
    mp2dMap->mMapCenterX = center.x();
    mp2dMap->mMapCenterY = -center.y();
    mp2dMap->mMapCenterZ = qRound(center.z());
    mp2dMap->mShiftMode = true;
    mp2dMap->update();
#endif
}
