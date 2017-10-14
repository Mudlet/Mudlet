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


#include "dlgAliasMainArea.h"

#include "mudlet.h"

dlgAliasMainArea::dlgAliasMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
    connect(lineEdit_alias_name, SIGNAL(editingFinished()), this, SLOT(slot_editing_name_finished()));
}

void dlgAliasMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_alias_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                    .arg(tr("<p>Choose a unique name for your alias; it will show in the tree and is needed for scripting.</p>")));
    lineEdit_alias_pattern->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>Enter a PERL regex pattern for your alias; alias are triggers on your input. Be careful not to use a <i>Command:</i> entry that would match what is entered here, Mudlet tries to prevent it but if it happens and it is not detected when you enter it this would cause the alias to trigger itself in an infinite loop - which is bad!</p>")));
    lineEdit_alias_command->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>Type in one or more commands you want the alias to send directly to the MUD if the keys entered match the pattern. (Optional)</p>"
                                               "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below.  Anything entered here is, literally, just sent to the MUD Server.</p>"
                                               "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <i>before</i> the script is run.</p>")));
}

void dlgAliasMainArea::trimName()
{
    lineEdit_alias_name->setText(lineEdit_alias_name->text().trimmed());
}

void dlgAliasMainArea::slot_editing_name_finished()
{
    trimName();
}
