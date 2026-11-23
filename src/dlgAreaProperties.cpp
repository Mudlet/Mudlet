/***************************************************************************
 *   Copyright (C) 2026 by Jens Wegener - code@jenswegener.net             *
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

#include "dlgAreaProperties.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"
#include "uiDesign.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

dlgAreaProperties::dlgAreaProperties(TMap* pMap, const int areaId, QWidget* parent)
: QDialog(parent)
, mpMap(pMap)
, mAreaId(areaId)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(qsl("areaPropertiesDialog"));
    //: Title of the dialog editing one area's properties, opened from Configure Areas; %1 is the area's name
    setWindowTitle(tr("Area properties: %1").arg(pMap->mpRoomDB->getAreaNamesMap().value(areaId)));

    auto* pOuter = new QVBoxLayout(this);
    pOuter->setContentsMargins(0, 0, 0, 0);
    // Sheets go on this, not the dialog: a profile's Lua stylesheet replaces the dialog's on every show
    mpShell = new QWidget(this);
    mpShell->setObjectName(qsl("areaPropertiesShell"));
    uiDesign::markAsShellSurface(mpShell);
    pOuter->addWidget(mpShell);
    auto* pLayout = new QVBoxLayout(mpShell);

    auto* pForm = new QWidget(mpShell);
    pForm->setObjectName(qsl("areaPropertiesForm"));
    pLayout->addWidget(pForm);
    buildRows(pForm);

    auto* pButtons = new QDialogButtonBox(mpShell);
    pButtons->setObjectName(qsl("areaPropertiesButtons"));
    if (mRows.isEmpty()) {
        auto* pNone = new QLabel(mpShell);
        pNone->setObjectName(qsl("areaPropertiesNoFields"));
        pNone->setWordWrap(true);
        //: Shown in place of the form when the map defines no area properties; setMapAreaFields is a Lua function name and stays as it is
        pNone->setText(tr("This map has no area properties yet. A mapper script defines them with setMapAreaFields()."));
        pLayout->insertWidget(0, pNone);
        pForm->hide();
        pButtons->setStandardButtons(QDialogButtonBox::Close);
        connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    } else {
        pButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        pButtons->button(QDialogButtonBox::Ok)->setProperty(uiDesign::scmProp_primaryButton, true);
        connect(pButtons, &QDialogButtonBox::accepted, this, &dlgAreaProperties::accept);
        connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    pLayout->addStretch(1);
    pLayout->addWidget(pButtons);

    applyAreaPropertiesStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgAreaProperties::applyAreaPropertiesStyle);
}

void dlgAreaProperties::buildRows(QWidget* pForm)
{
    auto* pFormLayout = new QFormLayout(pForm);
    pFormLayout->setContentsMargins(0, 0, 0, 0);
    const TArea* pArea = mpMap->mpRoomDB->getArea(mAreaId);
    for (const TAreaFields::Field& field : TAreaFields::fields(*mpMap)) {
        Row row;
        row.field = field;
        if (pArea && pArea->mUserData.contains(field.key)) {
            row.original = pArea->mUserData.value(field.key);
        }
        // What the area would take from a parent area while it has no value of its own, as the field's editor words it
        QString inheritedValue;
        QString inheritedFrom;
        if (field.inherit) {
            if (const std::optional<int> parentId = mpMap->mpRoomDB->getAreaParent(mAreaId)) {
                const TAreaFields::Lookup found = TAreaFields::valueFor(*mpMap, *parentId, field);
                if (found.value) {
                    //: An answer for a yes/no area property, here the one a parent area gives
                    inheritedValue = field.type == TAreaFields::Type::YesNo ? (found.value->toBool() ? tr("Yes") : tr("No")) : TAreaFields::formatValue(field, *found.value);
                    inheritedFrom = mpMap->mpRoomDB->getAreaNamesMap().value(found.areaId);
                }
            }
        }

        QWidget* pEditor = nullptr;
        if (field.type == TAreaFields::Type::YesNo || field.type == TAreaFields::Type::Choice) {
            row.pList = new QComboBox(pForm);
            // Without item data, which is how rowText() tells it apart from a value
            if (inheritedFrom.isEmpty()) {
                //: The entry of an area property's list that leaves it unset
                row.pList->addItem(tr("(not set)"));
            } else {
                //: The entry of an area property's list that leaves it unset, so that it takes a parent area's value; %1 is that area's name, %2 the value
                row.pList->addItem(tr("(from %1: %2)").arg(inheritedFrom, inheritedValue));
            }
            if (field.type == TAreaFields::Type::YesNo) {
                //: An answer for a yes/no area property
                row.pList->addItem(tr("Yes"), qsl("true"));
                //: An answer for a yes/no area property
                row.pList->addItem(tr("No"), qsl("false"));
            } else {
                for (const QString& choice : field.choices) {
                    row.pList->addItem(choice, choice);
                }
            }
            if (row.original) {
                if (const std::optional<QVariant> value = TAreaFields::parseValue(field, *row.original)) {
                    row.pList->setCurrentIndex(row.pList->findData(TAreaFields::formatValue(field, *value)));
                } else {
                    // Kept as it is, so that opening the dialog and pressing OK changes nothing
                    row.pList->addItem(*row.original, *row.original);
                    row.pList->setCurrentIndex(row.pList->count() - 1);
                }
            }
            pEditor = row.pList;
        } else {
            row.pLine = new QLineEdit(pForm);
            row.pLine->setText(row.original.value_or(QString()));
            if (!inheritedFrom.isEmpty()) {
                //: Shown in an empty area property that takes its value from a parent area; %1 is the value, %2 the parent area's name
                row.pLine->setPlaceholderText(tr("%1 (from %2)").arg(inheritedValue, inheritedFrom));
            } else if (field.type == TAreaFields::Type::Range) {
                //: Placeholder of an area property holding a range of numbers, showing how to write one
                row.pLine->setPlaceholderText(tr("e.g. 10-20"));
            }
            pEditor = row.pLine;
        }
        row.initialText = row.pLine ? row.pLine->text() : QString();
        row.initialIndex = row.pList ? row.pList->currentIndex() : -1;
        pEditor->setObjectName(qsl("areaPropertiesField"));
        pEditor->setProperty("areaField", field.key);
        pEditor->setAccessibleName(field.name);
        auto* pLabel = new QLabel(field.name, pForm);
        pLabel->setBuddy(pEditor);
        pFormLayout->addRow(pLabel, pEditor);
        mRows.append(row);
    }
}

std::optional<QString> dlgAreaProperties::rowText(const Row& row, QString& error) const
{
    // Untouched, it keeps what the area had to the byte, spaces and "yes" for "true" included
    if (row.pList ? row.pList->currentIndex() == row.initialIndex : row.pLine->text() == row.initialText) {
        return row.original;
    }
    QString text;
    if (row.pList) {
        if (!row.pList->currentData().isValid()) {
            return std::nullopt;
        }
        text = row.pList->currentData().toString();
    } else {
        text = row.pLine->text().trimmed();
        if (text.isEmpty()) {
            return std::nullopt;
        }
    }
    if (row.original && text == *row.original) {
        return text;
    }
    if (!TAreaFields::parseValue(row.field, text)) {
        switch (row.field.type) {
        case TAreaFields::Type::Number:
            //: Why an area property could not be kept; %1 is the property's name
            error = tr("%1 has to be a number.").arg(row.field.name);
            break;
        case TAreaFields::Type::Range:
            //: Why an area property could not be kept; %1 is the property's name
            error = tr("%1 has to be two numbers with a dash between them, the lower first, such as 10-20.").arg(row.field.name);
            break;
        default:
            //: Why an area property could not be kept; %1 is the property's name
            error = tr("%1 does not have a valid value.").arg(row.field.name);
        }
    }
    return text;
}

void dlgAreaProperties::accept()
{
    TArea* pArea = mpMap ? mpMap->mpRoomDB->getArea(mAreaId) : nullptr;
    if (!pArea) {
        QDialog::reject();
        return;
    }
    QStringList errors;
    QList<std::pair<QString, std::optional<QString>>> changes;
    for (const Row& row : std::as_const(mRows)) {
        QString error;
        const std::optional<QString> text = rowText(row, error);
        if (!error.isEmpty()) {
            errors << error;
            continue;
        }
        if (text != row.original) {
            changes.append({row.field.key, text});
        }
    }
    if (!errors.isEmpty()) {
        //: Title of the message listing area properties that could not be kept
        QMessageBox::warning(this, tr("Area properties"), errors.join(QLatin1Char('\n')));
        return;
    }
    for (const auto& [key, text] : std::as_const(changes)) {
        if (text) {
            pArea->mUserData.insert(key, *text);
        } else {
            pArea->mUserData.remove(key);
        }
    }
    if (!changes.isEmpty()) {
        mpMap->setUnsaved(__func__);
    }
    QDialog::accept();
}

void dlgAreaProperties::applyAreaPropertiesStyle()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QString shell = qsl("#areaPropertiesShell");
    mpShell->setStyleSheet(qsl("QWidget#areaPropertiesShell { background-color: %1; } %2 QLabel { color: %3; }").arg(tokens.page.name(), shell, tokens.mutedText.name())
                           + uiDesign::inputStyleSheet(tokens, shell) + uiDesign::buttonStyleSheet(tokens, shell));
    uiDesign::letPopupsTakeTheFieldsCorner(mpShell);
    uiDesign::keepClickFocusOffControls(mpShell);
}

// Under "follow the system" a system theme change emits no signal_appearanceChanged; the style or palette change Qt
// sends is the only notice
void dlgAreaProperties::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if (mpShell && (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)) {
        applyAreaPropertiesStyle();
    }
}
