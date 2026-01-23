#ifndef MUDLET_SHORTCUTSMANAGER_H
#define MUDLET_SHORTCUTSMANAGER_H

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

#include <QMap>
#include <QObject>
#include <QString>
#include <QShortcut>

class ShortcutsManager : public QObject
{

    Q_OBJECT

public:
    explicit ShortcutsManager(QObject* parent = nullptr) : QObject(parent) {}
    ShortcutsManager(ShortcutsManager const&) = delete;
    ShortcutsManager& operator=(ShortcutsManager const&) = delete;
    ShortcutsManager(ShortcutsManager&&) = delete;
    ShortcutsManager& operator=(ShortcutsManager&&) = delete;
    ~ShortcutsManager();

    void registerShortcut(const QString&, const QString&, QKeySequence*);
    QStringListIterator iterator();
    void setShortcut(const QString&, QKeySequence*);
    QKeySequence* getSequence(const QString&);
    QKeySequence* getDefault(const QString&);
    QString getLabel(const QString& key);

private:
    QList<QString> shortcutKeys;
    QMap<QString, QKeySequence*> shortcuts; //shortcut key : sequence in use pointer
    QMap<QString, QKeySequence*> defaults; //shortcut key : default sequence
    QMap<QString, QString> translations; //shortcut key : translation for shortcut label

};

#endif //MUDLET_SHORTCUTSMANAGER_H
