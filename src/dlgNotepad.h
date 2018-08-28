#ifndef MUDLET_DLGNOTEPAD_H
#define MUDLET_DLGNOTEPAD_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "ui_notes_editor.h"
#include <QPointer>
#include "post_guard.h"

class Host;


class dlgNotepad : public QMainWindow, public Ui::notes_editor
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgNotepad)
    dlgNotepad(Host*);
    ~dlgNotepad();

    void save();
    void restore();

private:
    QPointer<Host> mpHost;
};

#endif // MUDLET_DLGNOTEPAD_H
