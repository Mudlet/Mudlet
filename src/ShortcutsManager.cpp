/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2021 by Stephen Lyons - slysven@virginmdedia.com        *
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

#include "ShortcutsManager.h"

ShortcutsManager::~ShortcutsManager()
{
    QMutableMapIterator<QString, QKeySequence*> itShortcut(shortcuts);
    while (itShortcut.hasNext()) {
        itShortcut.next();
        auto pKeySequence = itShortcut.value();
        delete pKeySequence;
        itShortcut.remove();
    }
    QMutableMapIterator<QString, QKeySequence*> itDefault(defaults);
    while (itDefault.hasNext()) {
        itDefault.next();
        auto pKeySequence = itDefault.value();
        delete pKeySequence;
        itDefault.remove();
    }
}

void ShortcutsManager::registerShortcut(const QString& key, const QString& translation, QKeySequence* sequence)
{
    shortcutKeys << key;
    shortcuts.insert(key, sequence);
    translations.insert(key, translation);
    defaults.insert(key, new QKeySequence(*sequence));
}

void ShortcutsManager::setShortcut(const QString& key, QKeySequence* sequence)
{
    QKeySequence* newSequence = new QKeySequence(*sequence);
    shortcuts.value(key)->swap(*newSequence);
    delete newSequence;
}

QKeySequence* ShortcutsManager::getSequence(const QString& key)
{
    return shortcuts.value(key);
}

QKeySequence* ShortcutsManager::getDefault(const QString& key)
{
    return defaults.value(key);
}

QString ShortcutsManager::getLabel(const QString& key)
{
    return translations.value(key);
}

QStringListIterator ShortcutsManager::iterator()
{
    return QStringListIterator(shortcutKeys);
}
