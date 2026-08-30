#ifndef MUDLET_TWINDOWREGISTRY_H
#define MUDLET_TWINDOWREGISTRY_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
#include <QString>

class TLabelModel;

// The profile's index of its named windows, owned by Host. It answers "is there
// a window called X, what kind is it, and what is its model" - questions only
// the view's widget maps can answer today, and none of them can once the core
// builds without Qt Widgets (#8681).
//
// Entries are NON-OWNING. A named window's model is owned by the view object it
// belongs to (a TLabelModel by its TLabel), so it is the view that has to keep
// the registry in step: register where it creates the widget, deregister where
// it destroys it. TMainConsole's own QMap<QString, TLabel*> stays as the view's
// half of that pair, and remains its only way to reach a widget by name.
//
// Only labels are indexed here so far; the other named-window kinds still live
// solely in the view's maps.
class TWindowRegistry
{
public:
    void registerLabel(const QString& name, TLabelModel* pModel) { mLabels.insert(name, pModel); }

    // Identity-checked because a label outlives its removal from the view's map:
    // TMainConsole::deleteLabel() takes the entry and defers the delete, so the
    // same name can be registered again before the old TLabel's destructor runs.
    // Naming the model that is going stops it evicting its replacement.
    void deregisterLabel(const QString& name, const TLabelModel* pModel)
    {
        if (mLabels.value(name) == pModel) {
            mLabels.remove(name);
        }
    }

    bool hasLabel(const QString& name) const { return mLabels.contains(name); }
    TLabelModel* labelModel(const QString& name) const { return mLabels.value(name); }

private:
    QMap<QString, TLabelModel*> mLabels;
};

#endif // MUDLET_TWINDOWREGISTRY_H
