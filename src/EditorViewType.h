#ifndef MUDLET_EDITORVIEWTYPE_H
#define MUDLET_EDITORVIEWTYPE_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org   *
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

// Editor view type enum - used by both dlgTriggerEditor and EditorUndoSystem
enum class EditorViewType {
    cmUnknownView = 0,
    cmTriggerView = 0x01,
    cmTimerView = 0x02,
    cmAliasView = 0x03,
    cmScriptView = 0x04,
    cmActionView = 0x05,
    cmKeysView = 0x06,
    cmVarsView = 0x07
};

#endif // MUDLET_EDITORVIEWTYPE_H
