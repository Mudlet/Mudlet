#ifndef MUDLET_DLGAREAPROPERTIES_H
#define MUDLET_DLGAREAPROPERTIES_H

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

#include "TAreaFields.h"

#include <QDialog>
#include <QList>
#include <QPointer>

#include <optional>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class TMap;

// Edits one area's values for the fields its map defines (TAreaFields): a line for text, a number or a range, a list
// for yes/no or a choice. What is kept is checked against the field's type on OK, and nothing is written until all of
// it fits.
class dlgAreaProperties : public QDialog
{
    Q_OBJECT

public:
    dlgAreaProperties(TMap* pMap, int areaId, QWidget* parent = nullptr);

    // The fields shown, one per row, in the order of their keys
    int fieldCount() const { return mRows.size(); }

public slots:
    void accept() override;

protected:
    void changeEvent(QEvent*) override;

private:
    struct Row
    {
        TAreaFields::Field field;
        // The area's own text for the field, which is left as it is while it is not changed, even where it does not fit
        std::optional<QString> original;
        QLineEdit* pLine = nullptr;
        QComboBox* pList = nullptr;
    };

    void buildRows(QWidget* pForm);
    void applyAreaPropertiesStyle();
    // What a row asks to keep: none to remove the value, or the text to store; an error where it does not fit
    std::optional<QString> rowText(const Row&, QString& error) const;

    QPointer<TMap> mpMap;
    int mAreaId = 0;
    QWidget* mpShell = nullptr;
    QList<Row> mRows;
};

#endif // MUDLET_DLGAREAPROPERTIES_H
