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


#include "dlgTimersMainArea.h"

#include "mudlet.h"


dlgTimersMainArea::dlgTimersMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
    connect(lineEdit_timer_name, SIGNAL(editingFinished()), this, SLOT(slot_editing_name_finished()));
}

void dlgTimersMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    lineEdit_timer_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                    .arg(tr("<p>Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.</p>")));
    lineEdit_timer_command->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>Type in one or more commands you want the timer to send directly to the MUD when the time has elapsed. (Optional)</p>"
                                               "<p>To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered <i>instead</i> in the editor area below. Anything entered here is, literally, just sent to the MUD Server.</p>"
                                               "<p>It is permissable to use both this <i>and</i> a Lua script - this will be sent <b>before</b> the script is run.</p>")));
    timeEdit_timer_hours->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>The <b>hour</b> part of the interval that the timer will go off at.</p>")));

    timeEdit_timer_minutes->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>The <b>minute</b> part of the interval that the timer will go off at.</p>")));

    timeEdit_timer_seconds->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>The <b>second</b> part of the interval that the timer will go off at.</p>")));

    timeEdit_timer_msecs->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>The <b>milli-second</b> part of the interval that the timer will go off at (1000 milliseconds = 1 second).</p>")));
}

void dlgTimersMainArea::trimName()
{
    lineEdit_timer_name->setText(lineEdit_timer_name->text().trimmed());
}

void dlgTimersMainArea::slot_editing_name_finished()
{
    trimName();
}
