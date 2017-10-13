/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgKeysMainArea.h"

#include "mudlet.h"

dlgKeysMainArea::dlgKeysMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
}

void dlgKeysMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_key_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Choose a good, (ideally, though it need not be, unique) name for your key or key group. This will be displayed in the key tree.</p>")));
    lineEdit_key_command->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>Type in one or more commands you want the key to send directly to the MUD when pressed. (Optional)</p>"
                                             "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below.  Anything entered here is, literally, just sent to the MUD Server.</p>"
                                             "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <i>before</i> the script is run.</p>")));
    // TODO:
//    lineEdit_key_binding->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
//                                     .arg(tr("<p></p>")));

//    pushButton_key_grabKey->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
//                                       .arg(tr("<p></p>")));
}
