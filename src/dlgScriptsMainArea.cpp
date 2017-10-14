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


#include "dlgScriptsMainArea.h"

#include "mudlet.h"


dlgScriptsMainArea::dlgScriptsMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
    connect(lineEdit_script_name, SIGNAL(editingFinished()), this, SLOT(slot_editing_name_finished()));
    connect(lineEdit_script_event_handler_entry, SIGNAL(editingFinished()), this, SLOT(slot_editing_event_name_finished()));

}

void dlgScriptsMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_script_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.</p>"
                                             "<p>If a function within the script is to be used to handle events entered in the list below <b><u>it must have the same name as is entered here.</u></b></p>")));

    toolButton_script_remove_event_handler->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                       .arg(tr("<p>Remove (selected) event handler from list.</p>")));

    toolButton_script_add_event_handler->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                    .arg(tr("<p>Add entered event handler name to list.</p>")));
}

void dlgScriptsMainArea::trimName()
{
    lineEdit_script_name->setText(lineEdit_script_name->text().trimmed());
}

void dlgScriptsMainArea::trimEventHandlerName()
{
    lineEdit_script_event_handler_entry->setText(lineEdit_script_event_handler_entry->text().trimmed());
}

void dlgScriptsMainArea::slot_editing_name_finished()
{
    trimName();
}

void dlgScriptsMainArea::slot_editing_event_name_finished()
{
    trimEventHandlerName();
}
