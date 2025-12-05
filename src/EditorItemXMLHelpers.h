#ifndef MUDLET_EDITORITEMXMLHELPERS_H
#define MUDLET_EDITORITEMXMLHELPERS_H

/***************************************************************************
 *   Copyright (C) 2025 by Excellencedev - ademiluyisuccessandexcellence@gmail.com *
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

#include "EditorCommand.h"

#include <QString>

class TTrigger;
class TAlias;
class TTimer;
class TScript;
class TKey;
class TAction;
class Host;

// XML Export/Import functions
QString exportTriggerToXML(TTrigger* trigger);
QString exportAliasToXML(TAlias* alias);
QString exportTimerToXML(TTimer* timer);
QString exportScriptToXML(TScript* script);
QString exportKeyToXML(TKey* key);
QString exportActionToXML(TAction* action);

// Helper to get item name safely
QString getItemName(EditorViewType viewType, int itemID, Host* host);
QString getViewTypeName(EditorViewType viewType);

// Import functions - return the newly created item
TTrigger* importTriggerFromXML(const QString& xmlSnapshot, TTrigger* pParent, Host* host, int position = -1);
TAlias* importAliasFromXML(const QString& xmlSnapshot, TAlias* pParent, Host* host, int position = -1);
TTimer* importTimerFromXML(const QString& xmlSnapshot, TTimer* pParent, Host* host, int position = -1);
TScript* importScriptFromXML(const QString& xmlSnapshot, TScript* pParent, Host* host, int position = -1);
TKey* importKeyFromXML(const QString& xmlSnapshot, TKey* pParent, Host* host, int position = -1);
TAction* importActionFromXML(const QString& xmlSnapshot, TAction* pParent, Host* host, int position = -1);

// Update functions - update existing item from XML
bool updateTriggerFromXML(TTrigger* pT, const QString& xmlSnapshot);
bool updateAliasFromXML(TAlias* pA, const QString& xmlSnapshot);
bool updateTimerFromXML(TTimer* pT, const QString& xmlSnapshot);
bool updateScriptFromXML(TScript* pS, const QString& xmlSnapshot);
bool updateKeyFromXML(TKey* pK, const QString& xmlSnapshot);
bool updateActionFromXML(TAction* pA, const QString& xmlSnapshot);

#endif // MUDLET_EDITORITEMXMLHELPERS_H
