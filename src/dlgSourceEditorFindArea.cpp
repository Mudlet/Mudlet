/***************************************************************************
 *   Copyright (C) 2020 by Ian Adkins - ieadkins@gmail.com                 *
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


#include "dlgSourceEditorFindArea.h"
#include "ui_dlgPackageExporter.h"

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

dlgSourceEditorFindArea::dlgSourceEditorFindArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);
    lineEdit_findText->installEventFilter(this);
}

bool dlgSourceEditorFindArea::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == lineEdit_findText) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Enter or keyEvent->key() == Qt::Key_Return) {
                if (keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                    emit signal_sourceEditorFindPrevious();
                } else {
                    emit signal_sourceEditorFindNext();
                }
                return true;
            }
        }
    } else {
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
            emit signal_sourceEditorMovementNecessary();
        }
    }
    return QObject::eventFilter(obj, event);
}
