/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2019, 2022 by Stephen Lyons                       *
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


#include "EAction.h"

#include "ActionUnit.h"
#include "Host.h"
#include "mudlet.h"
#include "TAction.h"


EAction::EAction(Host* pHost, const QIcon& icon, const QString& name, const int id)
: QAction(icon, name, mudlet::self())
, mID(id)
, mpHost(pHost)
{
    setText(name);
    setObjectName(name);
    setIcon(icon);
    connect(this, &QAction::triggered, this, &EAction::slot_execute);
}

void EAction::slot_execute(bool checked)
{
    mpHost->getActionUnit()->getAction(mID)->mButtonState = checked;
    mpHost->getActionUnit()->getAction(mID)->execute();
    // Deliberately no doCleanup() here: a menu item runs nested inside a bar's
    // slot_pressed() (TEasyButtonBar::showMenu() spins a modal event loop), which
    // dereferences its own button after this returns - flushing here could free
    // an action that ancestor frame still holds. The deferred deletes are cleared
    // by that ancestor bar dispatcher and by Host's catch-all doCleanup() calls.
}
