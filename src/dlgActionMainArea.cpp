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


#include "dlgActionMainArea.h"

#include "mudlet.h"

dlgActionMainArea::dlgActionMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
}

void dlgActionMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_action_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>Choose a good, (ideally, though it need not be, unique) name for your button, menu or toolbar. This will be displayed in the buttons tree.</p>")));

    lineEdit_action_button_command_down->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                    .arg(tr("<p>Type in one or more commands you want the button to send directly to the MUD if it is pressed. (Optional)</p>"
                                                            "<p>If this is a <i>push-down</i> button then this is sent only when the button goes from the <i>up</i> to <i>down</i> state.</p>"
                                                            "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below.  "
                                                            "Anything entered here is, literally, just sent to the MUD Server.</p>"
                                                            "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <b>before</b> the script is run.</p>")));

    lineEdit_action_button_command_up->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                    .arg(tr("<p>Type in one or more commands you want the button to send directly to the MUD when this button goes from the <i>down</i> to <i>up</I> state.</p>"
                                                            "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below.  "
                                                            "Anything entered here is, literally, just sent to the MUD Server.</p>"
                                                            "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <b>before</b> the script is run.</p>")));
}
