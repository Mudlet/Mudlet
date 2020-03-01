/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "Host.h"


dlgComposer::dlgComposer(Host* pH) : mpHost(pH)

{
    setupUi(this);
    QFont f = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 10, QFont::Normal);
    edit->setFont(f);
    connect(saveButton, &QAbstractButton::pressed, this, &dlgComposer::save);
    connect(cancelButton, &QAbstractButton::pressed, this, &dlgComposer::cancel);
}

void dlgComposer::cancel()
{
    mpHost->mTelnet.atcpComposerCancel();
    this->hide();
}

void dlgComposer::save()
{
    mpHost->mTelnet.atcpComposerSave(edit->toPlainText());
    this->hide();
}

void dlgComposer::init(const QString &newTitle, const QString &newText)
{
    title->setText(newTitle);
    edit->setPlainText(newText);
}
