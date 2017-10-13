/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2017 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "mudlet.h"


dlgComposer::dlgComposer(Host* pH)
: mpHost(pH)
{
    setupUi(this);
    QFont f = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 10, QFont::Normal);
    edit->setFont(f);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...

    slot_guiLanguageChange();
    connect(saveButton, SIGNAL(pressed()), this, SLOT(save()));
    connect(cancelButton, SIGNAL(pressed()), this, SLOT(cancel()));
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
}

void dlgComposer::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    saveButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                           .arg(tr("<p>Save (Shift+Tab).</p>")));
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

void dlgComposer::init(QString t, QString txt)
{
    titel->setText(t);
    edit->setPlainText(txt);
}
