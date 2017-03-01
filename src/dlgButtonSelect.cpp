/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2017 by Ian Adkins - ieadkins@gmail.com                 *
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


#include "dlgButtonSelect.h"


#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QtUiTools>
#include <QGamepad>
#include "post_guard.h"


#define _DEBUG_

dlgButtonSelect::dlgButtonSelect(QWidget * parent)
: QDialog( parent )
{
    setupUi( this );

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonPressEvent, this, slot_gamepadButtonPress);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonReleaseEvent, this, slot_gamepadButtonRelease);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadAxisEvent, this, slot_gamepadAxisEvent);

    connect(pressRadio, SIGNAL(pressed()), this, SLOT(slot_pressRadioPressed()));
    connect(releaseRadio, SIGNAL(pressed()), this, SLOT(slot_releaseRadioPressed()));
    connect(repickButton, SIGNAL(clicked()), this, SLOT(slot_repickButtonClicked()));

    buttonSelected = false;
}

void dlgButtonSelect::slot_pressRadioPressed()
{
    pressThresholdSlider->setEnabled(true);
}

void dlgButtonSelect::slot_releaseRadioPressed()
{
    pressThresholdSlider->setDisabled(true);
}

void dlgButtonSelect::slot_repickButtonClicked()
{
    buttonSelected = false;
    tabWidget->setCurrentIndex(0);
    deviceEdit->setText("0");
    buttonEdit->setText("0");
    pressRadio->click();
    pressThresholdSlider->setValue(75);
    repickButton->setDisabled(true);
}

void dlgButtonSelect::slot_gamepadButtonPress(int deviceId, QGamepadManager::GamepadButton button, double value)
{
    lastEventLabel->setText(QString("Device %1: Button %2 was pressed to the %3 value").arg(deviceId).arg(button).arg(value));

    if ( !buttonSelected ){
        deviceEdit->setText(QString("%1").arg(deviceId));
        tabWidget->setCurrentIndex(0);
        buttonEdit->setText(QString("%1").arg(button));
        buttonSelected = true;
        repickButton->setEnabled(true);
        return;
    }

    if ( button == 12 ){// dpad U
        pressRadio->click();
    } else if ( button == 13 ){// dpad D
        releaseRadio->click();
    } else if ( button == 15 ){// dpad L
        if ( pressRadio->isChecked() ){
            if ( pressThresholdSlider->value() > 4 ){
                pressThresholdSlider->setValue(pressThresholdSlider->value()-5);
            } else {
                pressThresholdSlider->setValue(0);
            }
        }
    } else if ( button == 14 ){// dpad R
        if ( pressRadio->isChecked() ){
            if ( pressThresholdSlider->value() < 95)
            {
                pressThresholdSlider->setValue(pressThresholdSlider->value()+5);
            } else {
                pressThresholdSlider->setValue(99);
            }
        }
    } else if ( button == 0 ){// A
        accept();
    } else if ( button == 1 ){// B
        done(QDialog::Rejected);
    } else if ( button == 2 ){// X
        slot_repickButtonClicked();
        return;
    }

    buttonSelected = true;
}

void dlgButtonSelect::slot_gamepadButtonRelease(int deviceId, QGamepadManager::GamepadButton button)
{
    //lastEventLabel->setText(QString("Released: dev%1 btn%2").arg(deviceId).arg(button));
}

void dlgButtonSelect::slot_gamepadAxisEvent(int deviceId, QGamepadManager::GamepadAxis axis, double value)
{
    //lastEventLabel->setText(QString("Axis: dev%1 axis%2 val(%3)").arg(deviceId).arg(axis).arg(value));
    if ( axis == 0 ){
        axisLeftX = value;
    } else if ( axis == 1 ){
        axisLeftY = value;
    } else if ( axis == 2 ){
        axisRightX = value;
    } else if ( axis == 3 ){
        axisRightY = value;
    }

    double x, y;
    QString stick;

    if ( axis == 0 || axis == 1 ){//left stick-- 0:y, 1:x
        x = axisLeftX;
        y = axisLeftY;
        stick = "left";
    } else {//right stick-- 2:y, 3:x
    //} else if ( axis == 2 || axis == 3 ){//right stick-- 2:y, 3:x
        x = axisRightX;
        y = axisRightY;
        stick = "right";
    }

    double deadzone = .33;

    QString dir;

    if ( x < -deadzone && y < -deadzone){//nw
        dir = "nw";
    } else if ( x < deadzone && x > -deadzone && y < -deadzone ){//n
        dir = "n";
    } else if ( x > deadzone && y < -deadzone ){//ne
        dir = "ne";
    } else if ( x < -deadzone && y < deadzone && y > -deadzone ){//w
        dir = "w";
    } else if ( x > deadzone && y < deadzone && y > -deadzone ){//e
        dir = "e";
    } else if ( x < -deadzone && y > deadzone ){//sw
        dir = "sw";
    } else if ( x > -deadzone && x < deadzone && y > deadzone ){//s
        dir = "s";
    } else if ( x > deadzone && y > deadzone ){//se
        dir = "se";
    } else {//center
        dir = "none";
    }

    lastEventLabel->setText(QString("Device %1: %2 Stick was pressed %3 via x(%4) y(%5)").arg(deviceId).arg(stick).arg(dir).arg(x).arg(y));
    //lastEventLabel->setText(QString("%1 Stick: %2x %3y dir(%4)").arg(stick).arg(x).arg(y).arg(dir));

    if ( !buttonSelected && dir != "none" )
    {
        deviceEdit->setText(QString("%1").arg(deviceId));
        tabWidget->setCurrentIndex(1);
        if ( stick == "left" ){
            if ( dir == "nw" ){
                leftNWRadio->click();
            } else if ( dir == "n" ){
                leftNRadio->click();
            } else if ( dir == "ne" ){
                leftNERadio->click();
            } else if ( dir == "w" ){
                leftWRadio->click();
            } else if ( dir == "e" ){
                leftERadio->click();
            } else if ( dir == "sw" ){
                leftSWRadio->click();
            } else if ( dir == "s" ){
                leftSRadio->click();
            } else if ( dir == "se" ){
                leftSERadio->click();
            }
        } else if ( stick == "right" ){
            if ( dir == "nw" ){
                rightNWRadio->click();
            } else if ( dir == "n" ){
                rightNRadio->click();
            } else if ( dir == "ne" ){
                rightNERadio->click();
            } else if ( dir == "w" ){
                rightWRadio->click();
            } else if ( dir == "e" ){
                rightERadio->click();
            } else if ( dir == "sw" ){
                rightSWRadio->click();
            } else if ( dir == "s" ){
                rightSRadio->click();
            } else if ( dir == "se" ){
                rightSERadio->click();
            }
        }
        buttonSelected = true;
        repickButton->setEnabled(true);
        return;
    }
}
