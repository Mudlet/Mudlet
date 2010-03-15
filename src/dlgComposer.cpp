/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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

#include "dlgComposer.h"
#include <QDir>
#include "TDebug.h"
#include "Host.h"


dlgComposer::dlgComposer( Host * pH )
: mpHost( pH )

{
    setupUi(this);
    QFont f = QFont("Bitstream Vera Sans Mono", 10, QFont::Courier );
    edit->setFont ( f );
    connect( saveButton, SIGNAL(pressed()), this, SLOT(save()));
    connect( cancelButton, SIGNAL(pressed()), this, SLOT(cancel()));
}

void dlgComposer::cancel()
{
    mpHost->mTelnet.atcpComposerCancel();
    this->hide();
}

void dlgComposer::save()
{
    mpHost->mTelnet.atcpComposerSave( edit->toPlainText() );
    this->hide();
}

void dlgComposer::init( QString t, QString txt )
{
    titel->setText( t );
    edit->setPlainText( txt );
}

