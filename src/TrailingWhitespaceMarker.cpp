/***************************************************************************
 *   Copyright (C) 2023-2023 by Adam Robinson - seldon1951@hotmail.com     *
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

#include <QString>
#include <QLineEdit>
#include <QDebug>
void unmarkQString(QString* text) {
    QChar middleDot(0x00B7);
    text->replace(middleDot, ' ');
}
void markQString(QString* text) {
    QChar middleDot(0x00B7);

    // Trim text, check first and last character for ^ or $
    QString trimmedText = text->trimmed();
    if (trimmedText.length() == 0) {
        return;
    }

    // Mark leading spaces before ^ with a middle dot
    if (trimmedText.at(0) == '^') {
        for (int i = 0; i < text->length(); i++){
            if (text->at(i) == ' '){
                text->replace(i, 1, middleDot);
            } else {
                break;
            }
        }
    }

    // Mark trailing spaces after $ with a middle dot
    if (trimmedText.at(trimmedText.length()-1) == '$') {
        for (int i = text->length() - 1; i > -1; i--){
            if (text->at(i) == ' '){
                text->replace(i, 1, middleDot);
            } else {
                break;
            }
        }
    }

}

void markQLineEdit(QLineEdit* lineEdit) {

    QString text = lineEdit->text();

    unmarkQString(&text);
    markQString(&text);

    lineEdit->blockSignals(true);
    int cursorPos = lineEdit->cursorPosition();
    lineEdit->setText(text);
    lineEdit->setCursorPosition(cursorPos);


    lineEdit->blockSignals(false);
}

void unmarkQLineEdit(QLineEdit* lineEdit) {

    QString text = lineEdit->text();

    unmarkQString(&text);

    lineEdit->blockSignals(true);
    int cursorPos = lineEdit->cursorPosition();
    lineEdit->setText(text);
    lineEdit->setCursorPosition(cursorPos);


    lineEdit->blockSignals(false);
}
