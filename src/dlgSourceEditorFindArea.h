#ifndef MUDLET_DLGSOURCEEDITORFINDAREA_H
#define MUDLET_DLGSOURCEEDITORFINDAREA_H

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


#include "pre_guard.h"
#include "ui_source_editor_find_area.h"
#include <QTextEdit>
#include <QKeyEvent>
#include "post_guard.h"

/*namespace Ui {
class dlgSourceEditorFindArea;
}*/

class dlgSourceEditorFindArea : public QWidget, public Ui::source_editor_find_area
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgSourceEditorFindArea)
    explicit dlgSourceEditorFindArea(QWidget*);

    bool eventFilter(QObject*, QEvent* event) override;

signals:
    void signal_sourceEditorFindPrevious();
    void signal_sourceEditorFindNext();
    void signal_sourceEditorMovementNecessary();
};

#endif // MUDLET_DLGSOURCEEDITORFINDAREA_H
