/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2018-2019, 2021 by Stephen Lyons                  *
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


#include "dlgColorTrigger.h"


#include "dlgTriggerEditor.h"
#include "Host.h"
#include "TTextEdit.h"
#include "TTrigger.h"
#include "mudlet.h"


dlgColorTrigger::dlgColorTrigger(QWidget* pParentWidget, TTrigger* pT, const bool isBackGround, const QString& title)
: QDialog(pParentWidget)
, mpTrigger(pT)
, mIsBackground(isBackGround)
{
    // init generated dialog
    setupUi(this);

    mSignalMapper = new QSignalMapper(this);

    // The buttonBox's Cancel button has been connect up to this class's
    // rejected() signal to close it without changing anything in the caller!
    // The Ignore button means do not consider this fore or back ground (as per
    // mIsBackground) for the colour trigger - only use the other one (which
    // must be set)
    // The Apply button unhides the display of the colors codes 16 - 255 - it
    // is set up as a one-shot operation, and is effectively "fired" if the
    // existing color is in that range of numbers:
    buttonBox->button(QDialogButtonBox::Apply)->setText(tr("More colors"));
    buttonBox->button(QDialogButtonBox::Apply)->setCheckable(true);
    buttonBox->button(QDialogButtonBox::Apply)->setToolTip(utils::richText(tr("Click to access all 256 ANSI colors.")));
    connect(buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &dlgColorTrigger::slot_moreColorsClicked);

    connect(buttonBox->button(QDialogButtonBox::Ignore), &QAbstractButton::clicked, this, &dlgColorTrigger::slot_resetColorClicked);
    buttonBox->button(QDialogButtonBox::Ignore)->setToolTip(utils::richText(mIsBackground
                                                                            ? tr("Click to make the color trigger ignore the text's background color - however choosing this for both foreground and background is an error.")
                                                                            : tr("Click to make the color trigger ignore the text's foreground color - however choosing this for both foreground and background is an error.")));
    // The Reset button means trigger only if the text is not set to anything
    // so is in the "default" colour - what ever THAT is:
    connect(buttonBox->button(QDialogButtonBox::Reset), &QAbstractButton::clicked, this, &dlgColorTrigger::slot_defaultColorClicked);
    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Default"));
    buttonBox->button(QDialogButtonBox::Reset)->setToolTip(utils::richText(mIsBackground
                                                                                ? tr("Click to make the color trigger when the text's background color has not been modified from its normal value.")
                                                                                : tr("Click to make the color trigger when the text's foreground color has not been modified from its normal value.")));
    connect(mSignalMapper, SIGNAL(mapped(int)), this, SLOT(slot_basicColorClicked(int)));

    groupBox_basicColors->setToolTip(utils::richText(mIsBackground
                                                     ? tr("Click a color to make the trigger fire only when the text's background color matches the color number indicated.")
                                                     : tr("Click a color to make the trigger fire only when the text's foreground color matches the color number indicated.")));

    connect(pushButton_setUsingRgbValue, &QAbstractButton::clicked, this, &dlgColorTrigger::slot_rgbColorClicked);
    connect(pushButton_setUsingGrayValue, &QAbstractButton::clicked, this, &dlgColorTrigger::slot_grayColorClicked);

    setupBasicButton(pushButton_black, 0, mpTrigger->mpHost->mBlack, tr("Black"));
    setupBasicButton(pushButton_red, 1, mpTrigger->mpHost->mRed, tr("Red"));
    setupBasicButton(pushButton_green, 2, mpTrigger->mpHost->mGreen, tr("Green"));
    setupBasicButton(pushButton_yellow, 3, mpTrigger->mpHost->mYellow, tr("Yellow"));
    setupBasicButton(pushButton_blue, 4, mpTrigger->mpHost->mBlue, tr("Blue"));
    setupBasicButton(pushButton_magenta, 5, mpTrigger->mpHost->mMagenta, tr("Magenta"));
    setupBasicButton(pushButton_cyan, 6, mpTrigger->mpHost->mCyan, tr("Cyan"));
    setupBasicButton(pushButton_white, 7, mpTrigger->mpHost->mWhite, tr("White (Light gray)"));

    setupBasicButton(pushButton_Lblack, 8, mpTrigger->mpHost->mLightBlack, tr("Light black (Dark gray)"));
    setupBasicButton(pushButton_Lred, 9, mpTrigger->mpHost->mLightRed, tr("Light red"));
    setupBasicButton(pushButton_Lgreen, 10, mpTrigger->mpHost->mLightGreen, tr("Light green"));
    setupBasicButton(pushButton_Lyellow, 11, mpTrigger->mpHost->mLightYellow, tr("Light yellow"));
    setupBasicButton(pushButton_Lblue, 12, mpTrigger->mpHost->mLightBlue, tr("Light blue"));
    setupBasicButton(pushButton_Lmagenta, 13, mpTrigger->mpHost->mLightMagenta, tr("Light magenta"));
    setupBasicButton(pushButton_Lcyan, 14, mpTrigger->mpHost->mLightCyan, tr("Light cyan"));
    setupBasicButton(pushButton_Lwhite, 15, mpTrigger->mpHost->mLightWhite, tr("Light white"));

    // These will correctly set the colours on the value label in their area:
    if (mIsBackground) {
        if (mpTrigger->mColorTriggerBgAnsi >= 16 && mpTrigger->mColorTriggerBgAnsi <= 231) {
            slot_moreColorsClicked();
            horizontalSlider_red->setSliderPosition((mpTrigger->mColorTriggerBgAnsi - 16) / 36);
            horizontalSlider_green->setSliderPosition((mpTrigger->mColorTriggerBgAnsi - 16 - horizontalSlider_red->value() * 36) / 6);
            horizontalSlider_blue->setSliderPosition((mpTrigger->mColorTriggerBgAnsi - 16 - horizontalSlider_red->value() * 36) - horizontalSlider_green->value() * 6);
            slot_rgbColorChanged();
            slot_grayColorChanged(0);
            slot_setRBGButtonFocus();

        } else if (mpTrigger->mColorTriggerBgAnsi >= 232 && mpTrigger->mColorTriggerBgAnsi <= 255) {
            slot_moreColorsClicked();
            // The RGB controls will be at zeros so this will set the things up
            // to those
            slot_rgbColorChanged();
            // We can set the gray directly
            slot_grayColorChanged(mpTrigger->mColorTriggerBgAnsi - 232);
            slot_setGreyButtonFocus();

        } else {
            // In the case of default / ignore or one of the basic 16 - colors
            // we will hide the extra ones:
            groupBox_rgbScale->setVisible(false);
            groupBox_grayScale->setVisible(false);

            // The color is either not set (is reset) or a basic colour - we do
            // not provide a means to set a colour trigger based on a 16M value
            // (yet) from the GUI?

            // The RGB controls will be at zeros so this will set the things up
            // to those
            slot_rgbColorChanged();
            // We can set the gray directly
            slot_grayColorChanged(0);
        }

    } else {
        if (mpTrigger->mColorTriggerFgAnsi >= 16 && mpTrigger->mColorTriggerFgAnsi <= 231) {
            slot_moreColorsClicked();
            // Current background for this trigger is a RGB one so set that
            // control to the matching value:

            horizontalSlider_red->setSliderPosition((mpTrigger->mColorTriggerFgAnsi - 16) / 36);
            horizontalSlider_green->setSliderPosition((mpTrigger->mColorTriggerFgAnsi - 16 - horizontalSlider_red->value() * 36) / 6);
            horizontalSlider_blue->setSliderPosition((mpTrigger->mColorTriggerFgAnsi - 16 - horizontalSlider_red->value() * 36) - horizontalSlider_green->value() * 6);
            slot_rgbColorChanged();
            slot_grayColorChanged(0);
            slot_setRBGButtonFocus();

        } else if (mpTrigger->mColorTriggerFgAnsi >= 232 && mpTrigger->mColorTriggerFgAnsi <= 255) {
            slot_moreColorsClicked();
            // Current background for this trigger is a Gray one so set that
            // control to the matching value:

            // The RGB controls will be at zeros so this will set the things up
            // to those
            slot_rgbColorChanged();
            // We can set the gray directly
            slot_grayColorChanged(mpTrigger->mColorTriggerFgAnsi - 232);
            slot_setGreyButtonFocus();

        } else {
            // In the case of default / ignore or one of the basic 16 - colors
            // we will hide the extra ones:
            groupBox_rgbScale->setVisible(false);
            groupBox_grayScale->setVisible(false);

            // The color is either not set (is reset) or a basic colour - we do
            // not provide a means to set a colour trigger based on a 16M value
            // (yet) from the GUI?

            // The RGB controls will be at zeros so this will set the things up
            // to those
            slot_rgbColorChanged();
            // We can set the gray directly
            slot_grayColorChanged(0);
        }
    }

    if (!title.isNull()) {
        setWindowTitle(title);
    }

    label_rgbValue->setEnabled(false);
    label_grayValue->setEnabled(false);

    // We set up the controls into the default state before wiring up the slots
    // that get run on changing the controls - the "set" button in each of these
    // groupboxes gets the focus once any of the sliders are adjusted:
    connect(horizontalSlider_red, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_rgbColorChanged);
    connect(horizontalSlider_red, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_setRBGButtonFocus);
    connect(horizontalSlider_green, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_rgbColorChanged);
    connect(horizontalSlider_green, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_setRBGButtonFocus);
    connect(horizontalSlider_blue, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_rgbColorChanged);
    connect(horizontalSlider_blue, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_setRBGButtonFocus);
    connect(horizontalSlider_gray, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_grayColorChanged);
    connect(horizontalSlider_gray, &QAbstractSlider::valueChanged, this, &dlgColorTrigger::slot_setGreyButtonFocus);

    // If the current is ignored or default set the focus onto them:
    if ((mIsBackground && mpTrigger->mColorTriggerBgAnsi == TTrigger::scmDefault) || (!mIsBackground && mpTrigger->mColorTriggerFgAnsi == TTrigger::scmDefault)) {

        buttonBox->button(QDialogButtonBox::Reset)->setFocus();

    } else if ((mIsBackground && mpTrigger->mColorTriggerBgAnsi == TTrigger::scmIgnored) || (!mIsBackground && mpTrigger->mColorTriggerFgAnsi == TTrigger::scmIgnored)) {

        buttonBox->button(QDialogButtonBox::Ignore)->setFocus();

    }
}

void dlgColorTrigger::setupBasicButton(QPushButton* pButton, const int ansiColor, const QColor& color, const QString& colorText)
{
    // TODO: Eliminate use of QSignalMapper and use a lambda function
    connect(pButton, SIGNAL(clicked()), mSignalMapper, SLOT(map()));
    mSignalMapper->setMapping(pButton, ansiColor);

    if ((mIsBackground && (mpTrigger->mColorTriggerBgAnsi == ansiColor))
     || (!mIsBackground && (mpTrigger->mColorTriggerFgAnsi == ansiColor))) {

        pButton->setFocus();
    }

    //: Color Trigger dialog button in basic 16-color set, the first value is the name of the color, the second is the ANSI color number - for most languages modification is not likely to be needed - this text is used in two places
    pButton->setText(tr("%1 [%2]").arg(colorText, QString::number(ansiColor)));
    pButton->setStyleSheet(dlgTriggerEditor::generateButtonStyleSheet(color));
}

void dlgColorTrigger::slot_rgbColorChanged()
{
    mRgbAnsiColorNumber = 16 + 36 * horizontalSlider_red->value() + 6 * horizontalSlider_green->value() + horizontalSlider_blue->value();
    mRgbAnsiColor = QColor(horizontalSlider_red->value() * 51, horizontalSlider_green->value() * 51, horizontalSlider_blue->value() * 51);
    label_rgbValue->setText(qsl("[%1]").arg(QString::number(mRgbAnsiColorNumber)));
    // Use the same stylesheet code as in the main editor and for the basic 16
    // color buttons but because this is a QLabel we need to replace one word in
    // the generated stylesheet:
    label_rgbValue->setStyleSheet(dlgTriggerEditor::generateButtonStyleSheet(mRgbAnsiColor)
                                  .replace(QLatin1String("QPushButton"), QLatin1String("QLabel")));
}

void dlgColorTrigger::slot_setRBGButtonFocus()
{
    pushButton_setUsingRgbValue->setFocus();
}

void dlgColorTrigger::slot_grayColorChanged(int sliderValue)
{
    mGrayAnsiColorNumber = 232 + sliderValue;
    const int value = sliderValue * 10 + 8;

    mGrayAnsiColor = QColor(value, value, value);
    label_grayValue->setText(qsl("[%1]").arg(QString::number(mGrayAnsiColorNumber)));
    // Use the same stylesheet code as in the main editor and for the basic 16
    // color buttons but because this is a QLabel we need to replace one word in
    // the generated stylesheet:
    label_grayValue->setStyleSheet(dlgTriggerEditor::generateButtonStyleSheet(mGrayAnsiColor)
                                   .replace(QLatin1String("QPushButton"), QLatin1String("QLabel")));

    if (horizontalSlider_gray->value() != sliderValue) {
        // This slot is also used to set the value so it may need to reposition
        // the slider as well - but we do not want to get into a loop so only
        // do this if the values differ:
        horizontalSlider_gray->setValue(sliderValue);
    }
}

void dlgColorTrigger::slot_setGreyButtonFocus()
{
    pushButton_setUsingGrayValue->setFocus();
}

void dlgColorTrigger::slot_basicColorClicked(int ansiColor)
{
    if (!mpTrigger) {
        return;
    }

    QColor choosenColor;
    // clang-format off
    switch (ansiColor) {
    case 0:     choosenColor = mpTrigger->mpHost->mBlack;           break;
    case 1:     choosenColor = mpTrigger->mpHost->mRed;             break;
    case 2:     choosenColor = mpTrigger->mpHost->mGreen;           break;
    case 3:     choosenColor = mpTrigger->mpHost->mYellow;          break;
    case 4:     choosenColor = mpTrigger->mpHost->mBlue;            break;
    case 5:     choosenColor = mpTrigger->mpHost->mMagenta;         break;
    case 6:     choosenColor = mpTrigger->mpHost->mCyan;            break;
    case 7:     choosenColor = mpTrigger->mpHost->mWhite;           break;
    case 8:     choosenColor = mpTrigger->mpHost->mLightBlack;      break;
    case 9:     choosenColor = mpTrigger->mpHost->mLightRed;        break;
    case 10:    choosenColor = mpTrigger->mpHost->mLightGreen;      break;
    case 11:    choosenColor = mpTrigger->mpHost->mLightYellow;     break;
    case 12:    choosenColor = mpTrigger->mpHost->mLightBlue;       break;
    case 13:    choosenColor = mpTrigger->mpHost->mLightMagenta;    break;
    case 14:    choosenColor = mpTrigger->mpHost->mLightCyan;       break;
    case 15:    choosenColor = mpTrigger->mpHost->mLightWhite;      break;
    default:
        Q_UNREACHABLE();
    }
    // clang-format on

    mpTrigger->mColorTrigger = true;
    if (mIsBackground) {
        mpTrigger->mColorTriggerBgAnsi = ansiColor;
        mpTrigger->mColorTriggerBgColor = choosenColor;
    } else {
        mpTrigger->mColorTriggerFgAnsi = ansiColor;
        mpTrigger->mColorTriggerFgColor = choosenColor;
    }

    close();
}

void dlgColorTrigger::slot_rgbColorClicked()
{
    mpTrigger->mColorTrigger = true;
    if (mIsBackground) {
        mpTrigger->mColorTriggerBgAnsi = mRgbAnsiColorNumber;
        mpTrigger->mColorTriggerBgColor = mRgbAnsiColor;
    } else {
        mpTrigger->mColorTriggerFgAnsi = mRgbAnsiColorNumber;
        mpTrigger->mColorTriggerFgColor = mRgbAnsiColor;
    }

    close();
}

void dlgColorTrigger::slot_grayColorClicked()
{
    mpTrigger->mColorTrigger = true;
    if (mIsBackground) {
        mpTrigger->mColorTriggerBgAnsi = mGrayAnsiColorNumber;
        mpTrigger->mColorTriggerBgColor = mGrayAnsiColor;
    } else {
        mpTrigger->mColorTriggerFgAnsi = mGrayAnsiColorNumber;
        mpTrigger->mColorTriggerFgColor = mGrayAnsiColor;
    }

    close();
}

void dlgColorTrigger::slot_resetColorClicked()
{
    if (mIsBackground) {
        mpTrigger->mColorTriggerBgAnsi = TTrigger::scmIgnored;
        mpTrigger->mColorTriggerBgColor = QColor();
    } else {
        mpTrigger->mColorTriggerFgAnsi = TTrigger::scmIgnored;
        mpTrigger->mColorTriggerFgColor = QColor();
    }

    // Reset mColorTrigger if NEITHER this one OR the other are set
    mpTrigger->mColorTrigger = (mpTrigger->mColorTriggerFgAnsi != TTrigger::scmIgnored || mpTrigger->mColorTriggerBgAnsi != TTrigger::scmIgnored);

    close();
}

void dlgColorTrigger::slot_defaultColorClicked()
{
    if (mIsBackground) {
        mpTrigger->mColorTriggerBgAnsi = TTrigger::scmDefault;
        mpTrigger->mColorTriggerBgColor = QColor();
    } else {
        mpTrigger->mColorTriggerFgAnsi = TTrigger::scmDefault;
        mpTrigger->mColorTriggerFgColor = QColor();
    }

    close();
}

void dlgColorTrigger::slot_moreColorsClicked()
{
    // Used to pull the button down if this slot is called from the constructor:
    buttonBox->button(QDialogButtonBox::Apply)->setChecked(true);

    // Implement a one-shot action by disabling it once it is pressed:
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    buttonBox->button(QDialogButtonBox::Apply)->setToolTip(utils::richText(tr("All color options are showing.")));

    if (groupBox_rgbScale->isHidden()) {
        groupBox_rgbScale->show();
    }

    if (groupBox_grayScale->isHidden()) {
        groupBox_grayScale->show();
    }
}
