/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "Host.h"
#include "mudlet.h"
#include "TAction.h"


EAction::EAction( QIcon & icon, QString & name, mudlet * parent )
: QAction( icon, name, parent )
{
    setText( name );
    setObjectName( name );
    setIcon( icon );
    connect(this, SIGNAL(triggered(bool)), this, SLOT(slot_execute(bool)));
}

void EAction::slot_execute(bool isChecked)
{
    qDebug() << "\nEAction::slot_execute("
             << isChecked
             << ") called for: "
             << mpHost->getActionUnit()->getAction( mID )->getName()
             << "it will now set the related TAction's mButtonState to:"
             << (isChecked ? "2" : "1")
             << "and then call TAction::execute().";

    if( isChecked )
    {
        mpHost->getActionUnit()->getAction( mID )->mButtonState = 2;
    }
    else
    {
        mpHost->getActionUnit()->getAction( mID )->mButtonState = 1;
    }

    mpHost->getActionUnit()->getAction( mID )->execute();
}
