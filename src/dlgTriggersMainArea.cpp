/***************************************************************************
 *   Copyright (C) 2009 by Heiko Koehn - KoehnHeiko@googlemail.com         *
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


#include "dlgTriggersMainArea.h"

#include "mudlet.h"

dlgTriggersMainArea::dlgTriggersMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
}

void dlgTriggersMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_trigger_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                      .arg(tr("<p>Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.</p>")));
    lineEdit_trigger_command->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>Type in one or more commands you want the trigger to send directly to the MUD if it fires. (Optional)</p>"
                                                 "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below. Anything entered here is, literally, just sent to the MUD Server.</p>"
                                                 "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <b>before</b> the script is run.</p>")));
    checkBox_stayOpen->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>Keep firing the script for the specified number (<i>delta</i>) more lines, after the trigger or chain has matched.</p>")));
    spinBox_stayOpen->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>Within how many lines must <b>all</b> condition be true to fire the trigger?</p>")));
    soundTrigger->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                             .arg(tr("<p>Play a wave (or other) sound file if the trigger fires.</p>")));
    checkBox_multlinetrigger->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>The trigger will only fire if <b>all</b> conditions on the list have been met within the specified line <i>delta</i>, and captures will be saved in <i>multimatches</i> instead of <i>matches</i>.</p>"
                                                 "<p>If this option is not set the trigger will fire if any conditon on the list has been met.</p>")));
    spinBox_linemargin->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(tr("Within how many lines must all condition be true to fire the trigger?</p>")));
    filterTrigger->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                              .arg(tr("<p>Act as filter, i.e. only pass matches to its children to trigger on, instead of the <i>original line</i> from the MUD Server.</p>")));
    label_15->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                         .arg(tr("<p>Only the filtered content (i.e. the <i>capture groups</i>) will be passed on to child triggers, not the initial line (see the manual section on <i>filters</i>).</p>")));
    perlSlashGOption->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>Choose this option if you want to include all possible matches of the pattern in the line.</p>"
                                         "<p>Without this option, the pattern matching will stop after the first successful match.</p>")));
    colorizerTrigger->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>Highlights the capture groups if there are any, otherwise highlight the entire match.</p>")));
}
