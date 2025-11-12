/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016, 2019-2020, 2022 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
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
#include "TRoomDB.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"

#include <QElapsedTimer>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QProgressDialog>

using namespace std::chrono_literals;

dlgMapper::dlgMapper( QWidget * parent, Host * pH, TMap * pM )
: QWidget(parent)
, mpMap(pM)
, mpHost(pH)
{
    setupUi(this);

#if defined(INCLUDE_3DMAPPER)
    QSurfaceFormat fmt;
#ifndef NDEBUG
    fmt.setOption(QSurfaceFormat::DebugContext);
#endif
    fmt.setSamples(10);
    QSurfaceFormat::setDefaultFormat(fmt);
#endif
    mp2dMap->mpMap = pM;
    mp2dMap->mpHost = pH;
    // Have to do this here rather than in the T2DMap constructor because that
    // classes mpMap pointer is not initialised in its constructor.
    // Set up default player room markings:
    mp2dMap->setPlayerRoomStyle(mpMap->mPlayerRoomStyle);
    QMapIterator<int, QString> it(mpMap->mpRoomDB->getAreaNamesMap());
    //sort them alphabetically (case sensitive)
    QMap<QString, QString> areaNames;
    while (it.hasNext()) {
        it.next();
        const QString name = it.value();
        areaNames.insert(name.toLower(), name);
    }
    //areaNames.sort();
    QMapIterator<QString, QString> areaIt(areaNames);
    while (areaIt.hasNext()) {
        areaIt.next();
        comboBox_showArea->addItem(areaIt.value());
    }
    slot_toggleRoundRooms(mpHost->mBubbleMode);
    widget_3DControls->setVisible(false);
    widget_playerIconControls->setVisible(false);
    mp2dMap->mShowRoomID = mpHost->mShowRoomID;


    widget_panel->setVisible(mpHost->mShowPanel);
    connect(toolButton_shiftZup, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZup);
    connect(toolButton_shiftZdown, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZdown);
    connect(toolButton_shiftLeft, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftLeft);
    connect(toolButton_shiftRight, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftRight);
    connect(toolButton_shiftUp, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftUp);
    connect(toolButton_shiftDown, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftDown);
    connect(toolButton_mapperMenu, &QToolButton::clicked, this, &dlgMapper::slot_setupMapperMenu);
    connect(toolButton_togglePanel, &QAbstractButton::clicked, this, &dlgMapper::slot_togglePanel);
    connect(comboBox_showArea, qOverload<int>(&QComboBox::activated), this, &dlgMapper::slot_switchArea);
#if defined(INCLUDE_3DMAPPER)
    mIs3DMode = mpHost->mShow3DView;
    if (mpHost->mShow3DView) {
        // Defer 3D view initialization until widget is fully constructed and added to parent
        QTimer::singleShot(0ms, this, [this]() {
            slot_toggle3DView(true);
        });
    }
#endif

    // Explicitly set the font otherwise it changes between the Application and
    // the default System one as the mapper is docked and undocked!
    // Previous code set this to the main console one (before it was read from
    // the game save file, apparently) so it was actually the initial value
    // specified in the Host class. Revising the code to make it use the
    // selected font for the main console meant that all the controls at the
    // bottom took on that font which was glaringly inconsistant with the
    // overall GUI appearance - instead now it adopts the "default" application
    // font:
    setFont(qApp->font());
    mpMap->restore16ColorSet();

    if (mpHost) {
        qDebug() << "dlgMapper::dlgMapper(...) INFO constructor called, mpMap->mProfileName: " << mpMap->mProfileName;
        mp2dMap->init();
    } else {
        qDebug() << "dlgMapper::dlgMapper(...) INFO constructor called, mpHost is null";
    }
    //stops inheritance of palette from mpConsole->mpMainFrame
    setPalette(QApplication::palette());

    connect(mpMap->mMapInfoContributorManager, &MapInfoContributorManager::signal_contributorsUpdated, this, &dlgMapper::slot_updateInfoContributors);
    slot_updateInfoContributors();

}

int dlgMapper::getCurrentShownAreaIndex()
{
    return comboBox_showArea->currentIndex();
}

void dlgMapper::updateAreaComboBox()
{
    if (!mpMap) {
        // We do not have a valid TMap instance pointer so doing anything now
        // is pointless - just leave the widget empty and disabled
        comboBox_showArea->clear();
        comboBox_showArea->setEnabled(false);
        return;
    }

    const QString oldValue = comboBox_showArea->currentText(); // Remember where we were
    QMapIterator<int, QString> itAreaNamesA(mpMap->mpRoomDB->getAreaNamesMap());
    //insert sort them alphabetically (case INsensitive)
    QMap<QString, QString> areaNames;
    while (itAreaNamesA.hasNext()) {
        itAreaNamesA.next();
        if (itAreaNamesA.key() == -1 && !mpMap->getDefaultAreaShown()) {
            continue; // Skip the default area from the listing if so directed
        }

        uint deduplicate = 0;
        QString name;
        do {
            name = qsl("%1+%2").arg(itAreaNamesA.value().toLower(), QString::number(++deduplicate));
            // Use a different suffix separator to one that area names
            // deduplication uses ('_') - makes debugging easier?
        } while (areaNames.contains(name));
        areaNames.insert(name, itAreaNamesA.value());
    }

    comboBox_showArea->clear();

    if (areaNames.isEmpty() || (mpMap && areaNames.count() == 1 && (*areaNames.constBegin() == mpMap->getDefaultAreaName()) && !mpMap->getDefaultAreaShown())) {
        // IF there are no area names to show - should be impossible as there
        // should always be the "Default Area" one
        // OR there is only one sorted name
        //    AND it is the "Default Area"
        //    AND we are not supposed to show it
        // THEN
        //    We do not have ANYTHING to go in the QComboBox - so leave the
        // control empty and disabled:
        comboBox_showArea->setEnabled(false);
        return;
    }

    if (areaNames.count() == ((areaNames.contains(mpMap->getDefaultAreaName()) && !mpMap->getDefaultAreaShown()) ? 2 : 1)) {
        // IF we have exactly 2 (if we are NOT showing the default area AND the names include it)
        //         OR exactly 1 otherwise
        // THEN
        //    We only have one item to show - so show it but disable the control
        comboBox_showArea->setEnabled(false);
    } else {
        comboBox_showArea->setEnabled(true);
    }

    QMapIterator<QString, QString> itAreaNamesB(areaNames);
    while (itAreaNamesB.hasNext()) {
        itAreaNamesB.next();
        comboBox_showArea->addItem(itAreaNamesB.value());
    }
    comboBox_showArea->setCurrentText(oldValue); // Try and reset to previous value
}

void dlgMapper::slot_toggleShowRoomIDs(int toggle)
{
    mp2dMap->mShowRoomID = (toggle == Qt::Checked);
    mp2dMap->mpHost->mShowRoomID = mp2dMap->mShowRoomID;
    mp2dMap->update();
}

void dlgMapper::slot_toggleShowRoomNames(int toggle)
{
    mpMap->setRoomNamesShown(toggle == Qt::Checked);
    mp2dMap->update();
}

void dlgMapper::slot_toggleStrongHighlight(int toggle)
{
    mpHost->mMapStrongHighlight = (toggle == Qt::Checked);
    mp2dMap->update();
}

void dlgMapper::slot_togglePanel()
{
    dlgMapper::slot_setMapperPanelVisible(!widget_panel->isVisible());
}

void dlgMapper::slot_setMapperPanelVisible(bool panelVisible)
{
    widget_panel->setVisible(panelVisible);
    mpHost->mShowPanel = panelVisible;
}

void dlgMapper::slot_toggle3DView(const bool is3DMode)
{
#if defined(INCLUDE_3DMAPPER)
    mIs3DMode = is3DMode;
    if (glWidget) {
        glWidget->update();
    } else {
        glWidget = GLWidgetFactory::createGLWidget(mpMap, mpHost, this);
        glWidget->setObjectName("glWidget");

        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy);
        verticalLayout_mapper->insertWidget(0, glWidget);
        mpMap->mpM = glWidget;
        connect(pushButton_ortho, SIGNAL(clicked()), glWidget, SLOT(slot_showAllLevels()));
        connect(pushButton_singleLevel, SIGNAL(clicked()), glWidget, SLOT(slot_singleLevelView()));
        connect(pushButton_increaseTop, SIGNAL(clicked()), glWidget, SLOT(slot_showMoreUpperLevels()));
        connect(pushButton_increaseBottom, SIGNAL(clicked()), glWidget, SLOT(slot_showMoreLowerLevels()));
        connect(pushButton_reduceTop, SIGNAL(clicked()), glWidget, SLOT(slot_showLessUpperLevels()));
        connect(pushButton_reduceBottom, SIGNAL(clicked()), glWidget, SLOT(slot_showLessLowerLevels()));
        connect(toolButton_shiftZup, SIGNAL(clicked()), glWidget, SLOT(slot_shiftZup()));
        connect(toolButton_shiftZdown, SIGNAL(clicked()), glWidget, SLOT(slot_shiftZdown()));
        connect(toolButton_shiftLeft, SIGNAL(clicked()), glWidget, SLOT(slot_shiftLeft()));
        connect(toolButton_shiftRight, SIGNAL(clicked()), glWidget, SLOT(slot_shiftRight()));
        connect(toolButton_shiftUp, SIGNAL(clicked()), glWidget, SLOT(slot_shiftUp()));
        connect(toolButton_shiftDown, SIGNAL(clicked()), glWidget, SLOT(slot_shiftDown()));
        connect(pushButton_defaultView, SIGNAL(clicked()), glWidget, SLOT(slot_defaultView()));
        connect(pushButton_sideView, SIGNAL(clicked()), glWidget, SLOT(slot_sideView()));
        connect(pushButton_topView, SIGNAL(clicked()), glWidget, SLOT(slot_topView()));
        connect(slider_scale, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setScale(int)));
        connect(slider_xRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionX(int)));
        connect(slider_yRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionY(int)));
        connect(slider_zRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionZ(int)));
        
        // Player icon adjustment controls
        connect(slider_playerIconHeight, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconHeight(int)));
        connect(slider_playerIconRotX, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationX(int)));
        connect(slider_playerIconRotY, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationY(int)));
        connect(slider_playerIconRotZ, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationZ(int)));
        connect(slider_playerIconScale, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconScale(int)));
        connect(pushButton_resetPlayerIcon, SIGNAL(clicked()), glWidget, SLOT(slot_resetPlayerIcon()));
        
        // Connect reset signal from glWidget back to sliders (cast to ModernGLWidget*)
        if (ModernGLWidget* modernWidget = qobject_cast<ModernGLWidget*>(glWidget)) {
            connect(modernWidget, &ModernGLWidget::resetPlayerIconSliders, this, [this](int height, int rotX, int rotY, int rotZ, int scale) {
                slider_playerIconHeight->setValue(height);
                slider_playerIconRotX->setValue(rotX);
                slider_playerIconRotY->setValue(rotY);
                slider_playerIconRotZ->setValue(rotZ);
                slider_playerIconScale->setValue(scale);
            });
        }
    }


    mp2dMap->setVisible(!is3DMode);
    glWidget->setVisible(is3DMode);
    if (glWidget->isVisible()) {
        widget_3DControls->setVisible(true);
        widget_playerIconControls->setVisible(mpHost && mpHost->experimentEnabled("experiment.3d-player-icon")
#ifdef DEBUG_PLAYER_ICON_CONTROLS
                                               && true
#else
                                               && false
#endif
                                               );
    } else {
        // workaround for buttons reloading oddly
        QTimer::singleShot(100ms, this, [this]() {
            widget_3DControls->setVisible(false);
            widget_playerIconControls->setVisible(false);
        });
    }
    mpHost->mShow3DView = is3DMode;
#else
    Q_UNUSED(is3DMode)
    mp2dMap->setVisible(true);
    widget_3DControls->setVisible(false);
    widget_playerIconControls->setVisible(false);
#endif
}

void dlgMapper::slot_roomSize(int size)
{
    const float floatSize = static_cast<float>(size / 10.0);
    mp2dMap->setRoomSize(floatSize);
    mp2dMap->update();
}

void dlgMapper::slot_exitSize(int size)
{
    mp2dMap->setExitSize(size);
    mp2dMap->update();
}


void dlgMapper::slot_setShowRoomIds(bool showRoomIds)
{
    dlgMapper::slot_toggleShowRoomIDs(showRoomIds ? Qt::Checked : Qt::Unchecked);
}

void dlgMapper::slot_toggleRoundRooms(const bool state)
{
    if (mp2dMap->mpHost->mBubbleMode != state) {
        mp2dMap->mpHost->mBubbleMode = state;
    }
    if (mp2dMap->mBubbleMode != state) {
        mp2dMap->mBubbleMode = state;
        mp2dMap->update();
    }
}

void dlgMapper::resetAreaComboBoxToPlayerRoomArea()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    TRoom* pR = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
    if (pR) {
        const int playerRoomArea = pR->getArea();
        TArea* pA = mpMap->mpRoomDB->getArea(playerRoomArea);
        if (pA) {
            const QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(playerRoomArea);
            if (!areaName.isEmpty()) {
                comboBox_showArea->setCurrentText(areaName);
            } else {
                qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room area name not valid.";
            }
        } else {
            qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room area not valid.";
        }
    } else {
        qDebug() << "dlgResetAreaComboBoxTolayerRoomArea() warning: player room not valid.";
    }
}

void dlgMapper::slot_switchArea(const int index)
{
    const QString areaName{comboBox_showArea->itemText(index)};
    mp2dMap->switchArea(areaName);
}

void dlgMapper::slot_updateInfoContributors()
{
    updateInfoMenu();
    mp2dMap->update();
}

// Is the mapper contained inside a floating/dockable QDockWidget?
bool dlgMapper::isFloatAndDockable() const
{
    // The class name should be a const char* - no QString wrapper is needed:
    if (parentWidget() && parentWidget()->inherits("QDockWidget")) {
        return true;
    }
    return false;
}

void dlgMapper::setFont(const QFont& newFont)
{
    QWidget::setFont(newFont);
    mp2dMap->setFont(newFont);
    mp2dMap->mFontHeight = mp2dMap->fontMetrics().height();
}

void dlgMapper::recreate3DWidget()
{
#if defined(INCLUDE_3DMAPPER)
    if (!glWidget) {
        return;
    }

    if (GLWidgetFactory::isCorrectWidgetType(glWidget, mpHost.data())) {
        return;
    }

    bool was3DMode = glWidget->isVisible();

    glWidget->setParent(nullptr);
    glWidget->deleteLater();
    glWidget = nullptr;
    mpMap->mpM = nullptr;

    glWidget = GLWidgetFactory::createGLWidget(mpMap, mpHost.data(), this);
    glWidget->setObjectName("glWidget");

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
    glWidget->setSizePolicy(sizePolicy);
    verticalLayout_mapper->insertWidget(0, glWidget);
    mpMap->mpM = glWidget;

    connect(pushButton_ortho, SIGNAL(clicked()), glWidget, SLOT(slot_showAllLevels()));
    connect(pushButton_singleLevel, SIGNAL(clicked()), glWidget, SLOT(slot_singleLevelView()));
    connect(pushButton_increaseTop, SIGNAL(clicked()), glWidget, SLOT(slot_showMoreUpperLevels()));
    connect(pushButton_increaseBottom, SIGNAL(clicked()), glWidget, SLOT(slot_showMoreLowerLevels()));
    connect(pushButton_reduceTop, SIGNAL(clicked()), glWidget, SLOT(slot_showLessUpperLevels()));
    connect(pushButton_reduceBottom, SIGNAL(clicked()), glWidget, SLOT(slot_showLessLowerLevels()));
    connect(toolButton_shiftZup, SIGNAL(clicked()), glWidget, SLOT(slot_shiftZup()));
    connect(toolButton_shiftZdown, SIGNAL(clicked()), glWidget, SLOT(slot_shiftZdown()));
    connect(toolButton_shiftLeft, SIGNAL(clicked()), glWidget, SLOT(slot_shiftLeft()));
    connect(toolButton_shiftRight, SIGNAL(clicked()), glWidget, SLOT(slot_shiftRight()));
    connect(toolButton_shiftUp, SIGNAL(clicked()), glWidget, SLOT(slot_shiftUp()));
    connect(toolButton_shiftDown, SIGNAL(clicked()), glWidget, SLOT(slot_shiftDown()));
    connect(pushButton_defaultView, SIGNAL(clicked()), glWidget, SLOT(slot_defaultView()));
    connect(pushButton_sideView, SIGNAL(clicked()), glWidget, SLOT(slot_sideView()));
    connect(pushButton_topView, SIGNAL(clicked()), glWidget, SLOT(slot_topView()));
    connect(slider_scale, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setScale(int)));
    connect(slider_xRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionX(int)));
    connect(slider_yRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionY(int)));
    connect(slider_zRot, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setCameraPositionZ(int)));

    // Player icon adjustment controls
    connect(slider_playerIconHeight, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconHeight(int)));
    connect(slider_playerIconRotX, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationX(int)));
    connect(slider_playerIconRotY, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationY(int)));
    connect(slider_playerIconRotZ, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconRotationZ(int)));
    connect(slider_playerIconScale, SIGNAL(valueChanged(int)), glWidget, SLOT(slot_setPlayerIconScale(int)));
    connect(pushButton_resetPlayerIcon, SIGNAL(clicked()), glWidget, SLOT(slot_resetPlayerIcon()));

    // Connect reset signal from glWidget back to sliders (cast to ModernGLWidget*)
    if (ModernGLWidget* modernWidget = qobject_cast<ModernGLWidget*>(glWidget)) {
        connect(modernWidget, &ModernGLWidget::resetPlayerIconSliders, this, [this](int height, int rotX, int rotY, int rotZ, int scale) {
            slider_playerIconHeight->setValue(height);
            slider_playerIconRotX->setValue(rotX);
            slider_playerIconRotY->setValue(rotY);
            slider_playerIconRotZ->setValue(rotZ);
            slider_playerIconScale->setValue(scale);
        });
    }

    glWidget->setVisible(was3DMode);
#endif
}

void dlgMapper::paintMapInfo(const QElapsedTimer& renderTimer, QPainter& painter, Host* pHost, TMap* pMap,
                            int roomID, int displayAreaId, int selectionSize, QColor& infoColor,
                            int xOffset, int yOffset, int widgetWidth, int fontHeight)
{
    if (!pMap || !pMap->mMapInfoContributorManager || !pHost) {
        return;
    }

    QList<QString> contributorList = pMap->mMapInfoContributorManager->getContributorKeys();
    QSet<QString> const contributorKeys{contributorList.begin(), contributorList.end()};
    if (!contributorKeys.intersects(pHost->mMapInfoContributors)) {
        return;
    }

    TRoom* pRoom = pMap->mpRoomDB->getRoom(roomID);
    if (!pRoom) {
        return;
    }

    const int initialYOffset = yOffset;

    painter.save();
    painter.setFont(pHost->getDisplayFont());

    for (const auto& key : pMap->mMapInfoContributorManager->getContributorKeys()) {
        if (pHost->mMapInfoContributors.contains(key)) {
            auto properties = pMap->mMapInfoContributorManager->getContributor(key)(roomID, selectionSize, pRoom->getArea(), displayAreaId, infoColor);
            if (!properties.color.isValid()) {
                properties.color = infoColor;
            }
            yOffset += paintMapInfoContributor(painter, xOffset, yOffset, properties, pHost->mMapInfoBg, fontHeight, widgetWidth);
        }
    }

#ifdef QT_DEBUG
    yOffset += paintMapInfoContributor(painter,
                         xOffset,
                         yOffset,
                         {false,
                          false,
                          QObject::tr("render time: %1S")
                                  .arg(renderTimer.nsecsElapsed() * 1.0e-9, 0, 'f', 3),
                          infoColor},
                         pHost->mMapInfoBg,
                         fontHeight,
                         widgetWidth);
#else
    Q_UNUSED(renderTimer)
#endif

    painter.restore();

    if (yOffset > initialYOffset) {
        painter.fillRect(xOffset, initialYOffset - 10, widgetWidth - 10 - xOffset, 10, pHost->mMapInfoBg);
    }
}

int dlgMapper::paintMapInfoContributor(QPainter& painter, int xOffset, int yOffset,
                                      const MapInfoProperties& properties, QColor bgColor, int fontHeight,
                                      int widgetWidth)
{
    if (properties.text.isEmpty()) {
        return 0;
    }

    painter.save();
    auto infoText = properties.text.trimmed();
    auto font = painter.font();
    font.setBold(properties.isBold);
    font.setItalic(properties.isItalic);
    painter.setFont(font);
    const int infoHeight = fontHeight;
    QRect testRect;
    QRect mapInfoRect = QRect(xOffset, yOffset, widgetWidth - 10 - xOffset, infoHeight);
    testRect = painter.boundingRect(mapInfoRect.left() + 10, mapInfoRect.top(), mapInfoRect.width() - 20, mapInfoRect.height() - 20,
                                   Qt::Alignment(Qt::AlignTop | Qt::AlignLeft) | Qt::TextFlag(Qt::TextWordWrap | Qt::TextIncludeTrailingSpaces),
                                   infoText);
    mapInfoRect.setHeight(testRect.height() + 10);
    painter.fillRect(mapInfoRect, bgColor);
    painter.setPen(properties.color);
    painter.drawText(mapInfoRect.left() + 10, mapInfoRect.top(), mapInfoRect.width() - 20, mapInfoRect.height() - 10,
                    Qt::Alignment(Qt::AlignTop | Qt::AlignLeft) | Qt::TextFlag(Qt::TextWordWrap | Qt::TextIncludeTrailingSpaces),
                    infoText);
    painter.restore();
    return mapInfoRect.height();
}

void dlgMapper::slot_setupMapperMenu()
{
    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto* upperLowerLevelsAction = new QAction(tr("Draw rooms on upper and lower levels"), this);
    upperLowerLevelsAction->setCheckable(true);
    upperLowerLevelsAction->setChecked(mudlet::self()->mDrawUpperLowerLevels);
    upperLowerLevelsAction->setToolTip(tr("When enabled, rooms on floors above and below the current level will be drawn with a lighter color to show the map layout context."));

    connect(upperLowerLevelsAction, &QAction::toggled, this, &dlgMapper::slot_toggleUpperLowerLevels);
    menu->addAction(upperLowerLevelsAction);

    auto* roundRoomsAction = new QAction(tr("Round rooms"), this);
    roundRoomsAction->setCheckable(true);
    roundRoomsAction->setChecked(mpHost->mBubbleMode);
    roundRoomsAction->setToolTip(tr("When enabled, rooms will be drawn with round corners instead of square corners."));

    connect(roundRoomsAction, &QAction::toggled, this, &dlgMapper::slot_toggleRoundRooms);
    menu->addAction(roundRoomsAction);

    auto* showRoomIdsAction = new QAction(tr("Show room IDs"), this);
    showRoomIdsAction->setCheckable(true);
    showRoomIdsAction->setChecked(mpHost->mShowRoomID);
    showRoomIdsAction->setToolTip(tr("When enabled, room IDs will be displayed on the map."));

    connect(showRoomIdsAction, &QAction::toggled, this, &dlgMapper::slot_toggleShowRoomIDsFromMenu);
    menu->addAction(showRoomIdsAction);

#if defined(INCLUDE_3DMAPPER)
    auto* show3DMapAction = new QAction(tr("Show map in 3D"), this);
    show3DMapAction->setCheckable(true);
    show3DMapAction->setChecked(mIs3DMode);
    show3DMapAction->setToolTip(tr("When enabled, the map will be displayed in 3D mode."));
    connect(show3DMapAction, &QAction::toggled, this, &dlgMapper::slot_toggle3DView);
    menu->addAction(show3DMapAction);
#endif

    // Add separator and Info submenu
    menu->addSeparator();
    mpInfoMenu = menu->addMenu(tr("Info overlays"));
    updateInfoMenu();

    menu->exec(toolButton_mapperMenu->mapToGlobal(toolButton_mapperMenu->rect().bottomLeft()));
}

void dlgMapper::slot_toggleUpperLowerLevels(bool enabled)
{
    mudlet::self()->mDrawUpperLowerLevels = enabled;
    if (mp2dMap) {
        mp2dMap->update();
    }
}

void dlgMapper::slot_toggleShowRoomIDsFromMenu(bool enabled)
{
    mp2dMap->mShowRoomID = enabled;
    mp2dMap->mpHost->mShowRoomID = enabled;
    mp2dMap->update();
}

void dlgMapper::updateInfoMenu()
{
    if (!mpInfoMenu) {
        return;
    }

    mpInfoMenu->clear();

    //: Don't show the map overlay, 'none' meaning no map overlay styled are enabled
    auto* clearAction = new QAction(tr("None"), mpInfoMenu);
    mpInfoMenu->addAction(clearAction);
    connect(clearAction, &QAction::triggered, this, [=, this]() {
        for (auto action : mpInfoMenu->actions()) {
            action->setChecked(false);
        }
    });

    for (const auto& name : mpMap->mMapInfoContributorManager->getContributorKeys()) {
        auto* action = new QAction(name, mpInfoMenu);
        action->setCheckable(true);
        action->setChecked(mpHost->mMapInfoContributors.contains(name));
        connect(action, &QAction::toggled, this, [=, this](bool isToggled) {
            if (isToggled) {
                mpHost->mMapInfoContributors.insert(name);
            } else {
                mpHost->mMapInfoContributors.remove(name);
            }
            mp2dMap->update();
        });
        mpInfoMenu->addAction(action);
    }
}
