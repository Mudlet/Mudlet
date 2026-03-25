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
#ifndef MUDLET_TRAILINGWHITESPACEMARKER_H
#define MUDLET_TRAILINGWHITESPACEMARKER_H

#include <QString>
#include <QLineEdit>
#include <SingleLineTextEdit.h>

void markQString(QString* input);
void unmarkQString(QString* input);
void markQLineEdit(QLineEdit* lineEdit);
void unmarkQLineEdit(QLineEdit* lineEdit);
void markQTextEdit(QPlainTextEdit* textEdit);
void unmarkQTextEdit(QPlainTextEdit* textEdit);
#endif // MUDLET_TRAILINGWHITESPACEMARKER_H
