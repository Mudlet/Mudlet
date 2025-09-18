/***************************************************************************
 *   Copyright (C) 2023-2023 by Adam Robinson - seldon1951@hotmail.com     *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "pre_guard.h"
#include <QLineEdit>
#include <QString>
#include "post_guard.h"

#include <SingleLineTextEdit.h>

void unmarkQString(QString* text)
{
    QChar middleDot(0x00B7);
    text->replace(middleDot, QChar::Space);
}
void markQString(QString* text)
{
    QChar middleDot(0x00B7);

    // Trim text, check first and last character for ^ or $
    QString trimmedText = text->trimmed();
    if (trimmedText.isEmpty()) {
        return;
    }

    // Mark leading spaces before ^ with a middle dot
    if (trimmedText.front() == '^') {
        auto firstNonSpace = std::find_if_not(text->begin(), text->end(), [](QChar c) {return c == QChar(' '); });
        std::replace(text->begin(), firstNonSpace, QChar(' '), middleDot);
    }

    // Mark trailing spaces after $ with a middle dot
    if (trimmedText.back() == '$') {
        auto lastNonSpace = std::find_if_not(text->rbegin(), text->rend(), [](QChar c) {return c == QChar(' '); });
        std::replace(lastNonSpace, text->rend(), QChar(' '), middleDot);
    }
}

void markQLineEdit(QLineEdit* lineEdit)
{

    QString text = lineEdit->text();

    unmarkQString(&text);
    markQString(&text);

    lineEdit->blockSignals(true);
    int cursorPos = lineEdit->cursorPosition();
    lineEdit->setText(text);
    lineEdit->setCursorPosition(cursorPos);

    lineEdit->blockSignals(false);
}

void unmarkQLineEdit(QLineEdit* lineEdit)
{

    QString text = lineEdit->text();

    unmarkQString(&text);

    lineEdit->blockSignals(true);
    int cursorPos = lineEdit->cursorPosition();
    lineEdit->setText(text);
    lineEdit->setCursorPosition(cursorPos);

    lineEdit->blockSignals(false);
}

void markQTextEdit(QPlainTextEdit* textEdit)
{
    QString text = textEdit->toPlainText();

    unmarkQString(&text);
    markQString(&text);

    textEdit->blockSignals(true);
    int cursorPos = textEdit->textCursor().position();
    textEdit->setPlainText(text);

    QTextCursor cursor = textEdit->textCursor();
    cursor.setPosition(cursorPos);
    textEdit->setTextCursor(cursor);
    textEdit->blockSignals(false);
}

void unmarkQTextEdit(QPlainTextEdit* textEdit)
{
    QString text = textEdit->toPlainText();

    unmarkQString(&text);

    textEdit->blockSignals(true);
    int cursorPos = textEdit->textCursor().position();
    textEdit->setPlainText(text);

    QTextCursor cursor = textEdit->textCursor();
    cursor.setPosition(cursorPos);
    textEdit->setTextCursor(cursor);
    textEdit->blockSignals(false);
}
