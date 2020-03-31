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
    //setWindowFlag(Qt::Popup);
    //setAttribute(QT_CORE_LIB::WA) WA_StyledBackground)
    //setAutoFillBackground(true);
    //setAttribute(Qt::WA_StyledBackground);
    //setWindowFlag(Qt::WA_StyledBackground, true);
    //installEventFilter(this);
    //installEventFilter(this);
    lineEdit_findText->installEventFilter(this);
    //connect(pushButton_findPrevious, &QPushButton::click, this, &dlgSourceEditorFindArea::signal_sourceEditorFindPrevious);
    //connect(pushButton_findNext, &QPushButton::click, this, &dlgSourceEditorFindArea::signal_sourceEditorFindNext);
}

bool dlgSourceEditorFindArea::eventFilter(QObject* obj, QEvent* event)
{
    /*if (obj->objectName() == "trigger_editor") {
        if (event->type() == QEvent::Resize) {
            if (!isHidden()) {

            }
        }
    }

    if (obj == qApp) {
        qDebug() << "qApp Event: " << event->type();
        if (event->type() == QEvent::ActivationChange) {
            qDebug() << "Activation change on app!";
        }
        return QObject::eventFilter(obj, event);
    }*/

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

        /*if (event->type() == QEvent::FocusAboutToChange) {
            qDebug() << "Focus changing! Obj: " << obj << " Event: " << event;
            mpHost->postMessage("Focus changing! Obj: " + obj->objectName() + " Event: " + QString("").number(event->type()) + "\n");
        } else if (event->type() != QEvent::Paint) {
            qDebug() << "Misc Event! Obj: " << obj << " Event: " << event;
            mpHost->postMessage("Misc Event! Obj: " + obj->objectName() + " Event: " + QString("").number(event->type()) + "\n");
        }*/

    }

    /*if (obj == parentWidget()->parentWidget()) {
        qDebug() << "Parent of parent of dlgSourceEditorFindArea!";
    }*/

//    qDebug() << "obj: " << obj << " (" << obj->objectName() << "), event: " << event << " (" << event->type() << ")";

//    if (obj == this || obj->parent() == this) {
//        /*QObject* oParent = parent();

//        while(oParent != nullptr){
//            oParent = oParent->parent();
//        }

//        qDebug() << obj->objectName() << " parent -> " << oParent << " (" << oParent->objectName() << ")";*/
//        if (QFocusEvent* focusEvent = static_cast<QFocusEvent*>(event)) {
//            if (focusEvent->lostFocus()) {
//                qDebug() << "FocusOut in eventFilter!";
//                hide();
//                return QObject::eventFilter(obj, event);
//            }
//        }
//        if (event->type() == QEvent::Resize) {
//            qDebug() << "Resize event!";
//            hide();
//            return QObject::eventFilter(obj, event);
//        }
//        qDebug() << "dlgSourceEditorFindArea event: " << event->type();
//    }

    return QObject::eventFilter(obj, event);
}
