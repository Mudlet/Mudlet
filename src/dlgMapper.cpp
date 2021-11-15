/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016, 2019-2020 by Stephen Lyons                   *
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

#include "pre_guard.h"
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include "post_guard.h"

using namespace std::chrono_literals;

dlgMapper::dlgMapper( QWidget * parent, Host * pH, TMap * pM )
: QWidget(parent)
, mpMap(pM)
, mpHost(pH)
, mShowDefaultArea(true)
{
    setupUi(this);

#if defined(INCLUDE_3DMAPPER)
    glWidget = nullptr;
    QSurfaceFormat fmt;
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
        QString name = it.value();
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
    widget_2DControls->setVisible(true);
    spinBox_roomSize->setValue(mpHost->mRoomSize * 10);
    spinBox_exitSize->setValue(mpHost->mLineSize);

    checkBox_showRoomIds->setChecked(mpHost->mShowRoomID);
    mp2dMap->mShowRoomID = mpHost->mShowRoomID;

    checkBox_showRoomNames->setVisible(mpMap->getRoomNamesPresent());
    checkBox_showRoomNames->setChecked(mpMap->getRoomNamesShown());

    widget_panel->setVisible(mpHost->mShowPanel);
    connect(checkBox_roundRooms, &QAbstractButton::clicked, this, &dlgMapper::slot_toggleRoundRooms);
    connect(pushButton_shiftZup, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZup);
    connect(pushButton_shiftZdown, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftZdown);
    connect(pushButton_shiftLeft, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftLeft);
    connect(pushButton_shiftRight, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftRight);
    connect(pushButton_shiftUp, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftUp);
    connect(pushButton_shiftDown, &QAbstractButton::clicked, mp2dMap, &T2DMap::slot_shiftDown);
    connect(spinBox_exitSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgMapper::slot_exitSize);
    connect(spinBox_roomSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgMapper::slot_roomSize);
    connect(toolButton_togglePanel, &QAbstractButton::clicked, this, &dlgMapper::slot_togglePanel);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    connect(comboBox_showArea, qOverload<int>(&QComboBox::activated), this, &dlgMapper::slot_switchArea);
#else
    connect(comboBox_showArea, qOverload<const QString&>(&QComboBox::activated), mp2dMap, &T2DMap::slot_switchArea);
#endif
#if defined(INCLUDE_3DMAPPER)
    connect(pushButton_3D, &QAbstractButton::clicked, this, &dlgMapper::slot_toggle3DView);
#else
    pushButton_3D->hide();
#endif
    connect(checkBox_showRoomIds, &QCheckBox::stateChanged, this, &dlgMapper::slot_toggleShowRoomIDs);
    connect(checkBox_showRoomNames, &QCheckBox::stateChanged, this, &dlgMapper::slot_toggleShowRoomNames);

    // Explicitly set the font otherwise it changes between the Application and
    // the default System one as the mapper is docked and undocked!
    QFont mapperFont = QFont(mpHost->getDisplayFont().family());
    if (mpHost->mNoAntiAlias) {
        mapperFont.setStyleStrategy(QFont::NoAntialias);
    } else {
        mapperFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality));
    }
    setFont(mapperFont);
    mp2dMap->mFontHeight = QFontMetrics(mpHost->getDisplayFont()).height();
    mpMap->mCustomEnvColors[257] = mpHost->mRed_2;
    mpMap->mCustomEnvColors[258] = mpHost->mGreen_2;
    mpMap->mCustomEnvColors[259] = mpHost->mYellow_2;
    mpMap->mCustomEnvColors[260] = mpHost->mBlue_2;
    mpMap->mCustomEnvColors[261] = mpHost->mMagenta_2;
    mpMap->mCustomEnvColors[262] = mpHost->mCyan_2;
    mpMap->mCustomEnvColors[263] = mpHost->mWhite_2;
    mpMap->mCustomEnvColors[264] = mpHost->mBlack_2;
    mpMap->mCustomEnvColors[265] = mpHost->mLightRed_2;
    mpMap->mCustomEnvColors[266] = mpHost->mLightGreen_2;
    mpMap->mCustomEnvColors[267] = mpHost->mLightYellow_2;
    mpMap->mCustomEnvColors[268] = mpHost->mLightBlue_2;
    mpMap->mCustomEnvColors[269] = mpHost->mLightMagenta_2;
    mpMap->mCustomEnvColors[270] = mpHost->mLightCyan_2;
    mpMap->mCustomEnvColors[271] = mpHost->mLightWhite_2;
    mpMap->mCustomEnvColors[272] = mpHost->mLightBlack_2;
    auto menu = new QMenu(this);
    pushButton_info->setMenu(menu);

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

void dlgMapper::updateAreaComboBox()
{
    if (!mpMap) {
        // We do not have a valid TMap instance pointer so doing anything now
        // is pointless - just leave the widget empty and disabled
        comboBox_showArea->clear();
        comboBox_showArea->setEnabled(false);
        return;
    }

    QString oldValue = comboBox_showArea->currentText(); // Remember where we were
    QMapIterator<int, QString> itAreaNamesA(mpMap->mpRoomDB->getAreaNamesMap());
    //insert sort them alphabetically (case INsensitive)
    QMap<QString, QString> areaNames;
    while (itAreaNamesA.hasNext()) {
        itAreaNamesA.next();
        if (itAreaNamesA.key() == -1 && !mShowDefaultArea) {
            continue; // Skip the default area from the listing if so directed
        }

        uint deduplicate = 0;
        QString name;
        do {
            name = QStringLiteral("%1+%2").arg(itAreaNamesA.value().toLower(), QString::number(++deduplicate));
            // Use a different suffix separator to one that area names
            // deduplication uses ('_') - makes debugging easier?
        } while (areaNames.contains(name));
        areaNames.insert(name, itAreaNamesA.value());
    }

    comboBox_showArea->clear();

    if (areaNames.isEmpty() || (mpMap && areaNames.count() == 1 && (*areaNames.constBegin() == mpMap->getDefaultAreaName()) && !mShowDefaultArea)) {
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

    if ( areaNames.count() == ((areaNames.contains(mpMap->getDefaultAreaName()) && !mShowDefaultArea) ? 2 : 1)) {
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

void dlgMapper::slot_toggleShowRoomIDs(int s)
{
    if (s == Qt::Checked) {
        mp2dMap->mShowRoomID = true;
    } else {
        mp2dMap->mShowRoomID = false;
    }
    mp2dMap->mpHost->mShowRoomID = mp2dMap->mShowRoomID;
    mp2dMap->update();
}

void dlgMapper::slot_toggleShowRoomNames(int s)
{
    mpMap->setRoomNamesShown(s == Qt::Checked);
    mp2dMap->update();
}

void dlgMapper::slot_toggleStrongHighlight(int v)
{
    mpHost->mMapStrongHighlight = v == Qt::Checked ? true : false;
    mp2dMap->update();
}

void dlgMapper::slot_togglePanel()
{
    widget_panel->setVisible(!widget_panel->isVisible());
    mpHost->mShowPanel = widget_panel->isVisible();
}

void dlgMapper::slot_toggle3DView(const bool is3DMode)
{
#if defined(INCLUDE_3DMAPPER)
    if (mpHost->mpMap->mpM && mpHost->mpMap->mpMapper) {
        mpHost->mpMap->mpM->update();
    }
    if (!glWidget) {
        glWidget = new GLWidget(mpMap, mpHost, this);
        glWidget->setObjectName("glWidget");

        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy);
        verticalLayout_mapper->insertWidget(0, glWidget);
        mpMap->mpM = mpMap->mpMapper->glWidget;
        connect(pushButton_ortho, &QAbstractButton::clicked, glWidget, &GLWidget::slot_showAllLevels);
        connect(pushButton_singleLevel, &QAbstractButton::clicked, glWidget, &GLWidget::slot_singleLevelView);
        connect(pushButton_increaseTop, &QAbstractButton::clicked, glWidget, &GLWidget::slot_showMoreUpperLevels);
        connect(pushButton_increaseBottom, &QAbstractButton::clicked, glWidget, &GLWidget::slot_showMoreLowerLevels);
        connect(pushButton_reduceTop, &QAbstractButton::clicked, glWidget, &GLWidget::slot_showLessUpperLevels);
        connect(pushButton_reduceBottom, &QAbstractButton::clicked, glWidget, &GLWidget::slot_showLessLowerLevels);
        connect(pushButton_shiftZup, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftZup);
        connect(pushButton_shiftZdown, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftZdown);
        connect(pushButton_shiftLeft, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftLeft);
        connect(pushButton_shiftRight, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftRight);
        connect(pushButton_shiftUp, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftUp);
        connect(pushButton_shiftDown, &QAbstractButton::clicked, glWidget, &GLWidget::slot_shiftDown);
        connect(pushButton_defaultView, &QAbstractButton::clicked, glWidget, &GLWidget::slot_defaultView);
        connect(pushButton_sideView, &QAbstractButton::clicked, glWidget, &GLWidget::slot_sideView);
        connect(pushButton_topView, &QAbstractButton::clicked, glWidget, &GLWidget::slot_topView);
        connect(slider_scale, &QAbstractSlider::valueChanged, glWidget, &GLWidget::slot_setScale);
        connect(slider_xRot, &QAbstractSlider::valueChanged, glWidget, &GLWidget::slot_setCameraPositionX);
        connect(slider_yRot, &QAbstractSlider::valueChanged, glWidget, &GLWidget::slot_setCameraPositionY);
        connect(slider_zRot, &QAbstractSlider::valueChanged, glWidget, &GLWidget::slot_setCameraPositionZ);
    }


    mp2dMap->setVisible(!is3DMode);
    glWidget->setVisible(is3DMode);
    if (glWidget->isVisible()) {
        widget_3DControls->setVisible(true);
        widget_2DControls->setVisible(false);
    } else {
        // workaround for buttons reloading oddly
        QTimer::singleShot(100ms, [this]() {
            widget_3DControls->setVisible(false);
            widget_2DControls->setVisible(true);
        });
    }

#else
    mp2dMap->setVisible(true);
    widget_3DControls->setVisible(false);
    widget_2DControls->setVisible(true);
#endif
}

void dlgMapper::slot_roomSize(int d)
{
    float s = static_cast<float>(d / 10.0);
    mp2dMap->setRoomSize(s);
    mp2dMap->update();
}

void dlgMapper::slot_exitSize(int d)
{
    mp2dMap->setExitSize(d);
    mp2dMap->update();
}

void dlgMapper::slot_toggleRoundRooms(const bool state)
{
    if (checkBox_roundRooms->isChecked() != state) {
        checkBox_roundRooms->setChecked(state);
    }
    if (mp2dMap->mpHost->mBubbleMode != state) {
        mp2dMap->mpHost->mBubbleMode = state;
    }
    if (mp2dMap->mBubbleMode != state) {
        mp2dMap->mBubbleMode = state;
        mp2dMap->update();
    }
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

    TRoom* pR = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
    if (pR) {
        int playerRoomArea = pR->getArea();
        TArea* pA = mpMap->mpRoomDB->getArea(playerRoomArea);
        if (pA) {
            QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(playerRoomArea);
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

#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
// Only needed in newer Qt versions as the old SIGNAL overload that returned the
// QString of the activated QComboBox entry has been obsoleted:
void dlgMapper::slot_switchArea(const int index)
{
    const QString areaName{comboBox_showArea->itemText(index)};
    mp2dMap->switchArea(areaName);
}
#endif

void dlgMapper::slot_updateInfoContributors()
{
    pushButton_info->menu()->clear();
    auto* clearAction = new QAction(tr("None", "Don't show the map overlay, 'none' meaning no map overlay styled are enabled"), pushButton_info);
    pushButton_info->menu()->addAction(clearAction);
    connect(clearAction, &QAction::triggered, this, [=]() {
        for (auto action : pushButton_info->menu()->actions()) {
            action->setChecked(false);
        }
    });

    for (const auto& name : mpMap->mMapInfoContributorManager->getContributorKeys()) {
        auto* action = new QAction(name, pushButton_info);
        action->setCheckable(true);
        action->setChecked(mpHost->mMapInfoContributors.contains(name));
        connect(action, &QAction::toggled, this, [=](bool isToggled) {
            if (isToggled) {
                mpHost->mMapInfoContributors.insert(name);
            } else {
                mpHost->mMapInfoContributors.remove(name);
            }
            mp2dMap->update();
        });
        pushButton_info->menu()->addAction(action);
    }
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
