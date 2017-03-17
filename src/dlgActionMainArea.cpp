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


dlgActionMainArea::dlgActionMainArea(QWidget * pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // I had previously enabled these in the actions_main_area.ui file but when
    // that file is written out by a recent QtDesigner (or the one built into
    // QtCreator) it inserts an XML entry for each that is not understood by
    // older versions of the Qt Libraries and causes compilation to fail even
    // if the option is THEN switched off in the same QtDesigner/QtCreator. This
    // is because it does NOT then remove the entry from the file and just
    // leaves it as "false" which cannot be understood by the older versions.
    // To correct it one has to edit the .ui file with a text editor to take the
    // entry out manually.
    // The only safe way to use the feature is to do a conditional compile of
    // the relevant item:
#if QT_VERSION >= 0x050200
    lineEdit_action_command_up->setClearButtonEnabled( true );
    lineEdit_action_command_down->setClearButtonEnabled( true );
#endif
}
