#ifndef MUDLET_EDITORITEMXMLHELPERS_H
#define MUDLET_EDITORITEMXMLHELPERS_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QString>

// Forward declarations
class TTrigger;
class TAlias;
class TTimer;
class TScript;
class TKey;
class TAction;
class Host;

// XML Helper Functions for item export/import
// These functions are used by the Qt undo framework commands
// to preserve item state during undo/redo operations

QString exportTriggerToXML(TTrigger* trigger);
TTrigger* importTriggerFromXML(const QString& xmlSnapshot, TTrigger* pParent, Host* host, int position = -1);
bool updateTriggerFromXML(TTrigger* trigger, const QString& xmlSnapshot);

QString exportAliasToXML(TAlias* alias);
TAlias* importAliasFromXML(const QString& xmlSnapshot, TAlias* pParent, Host* host, int position = -1);
bool updateAliasFromXML(TAlias* alias, const QString& xmlSnapshot);

QString exportTimerToXML(TTimer* timer);
TTimer* importTimerFromXML(const QString& xmlSnapshot, TTimer* pParent, Host* host, int position = -1);
bool updateTimerFromXML(TTimer* timer, const QString& xmlSnapshot);

QString exportScriptToXML(TScript* script);
TScript* importScriptFromXML(const QString& xmlSnapshot, TScript* pParent, Host* host, int position = -1);
bool updateScriptFromXML(TScript* script, const QString& xmlSnapshot);

QString exportKeyToXML(TKey* key);
TKey* importKeyFromXML(const QString& xmlSnapshot, TKey* pParent, Host* host, int position = -1);
bool updateKeyFromXML(TKey* key, const QString& xmlSnapshot);

QString exportActionToXML(TAction* action);
TAction* importActionFromXML(const QString& xmlSnapshot, TAction* pParent, Host* host, int position = -1);
bool updateActionFromXML(TAction* action, const QString& xmlSnapshot);

#endif // MUDLET_EDITORITEMXMLHELPERS_H
