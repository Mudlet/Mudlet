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
#include <QSet>
#include <QString>
#include <QStringList>

#include <optional>

class TConsoleModel;
class TLabelModel;

// The profile's index of its named windows, owned by Host. It answers "is there
// a window called X, what kind is it, and what is its model" - questions only
// the view's widget maps can answer today, and none of them can once the core
// builds without Qt Widgets (#8681).
//
// Entries are NON-OWNING. A named window's model is owned by the view object it
// belongs to (a TLabelModel by its TLabel, a TConsoleModel by its TConsole), so
// it is the view that has to keep the registry in step: register where it
// creates the widget, deregister where it destroys it. TMainConsole's own
// QMap<QString, TLabel*> and QMap<QString, TConsole*> stay as the view's half of
// that pair, and remain its only way to reach a widget by name.
//
// Labels, sub-consoles and user-window docks are indexed here so far; command
// lines, scroll boxes and text boxes still live solely in the view's maps.
class TWindowRegistry
{
public:
    // What a sub-console name was created as. The view's TConsole::ConsoleType
    // is a widget-side flag set, so the kind is recorded here at registration
    // instead. Other covers a console that is none of the three kinds a
    // sub-console can be, which leaves Host::windowType() - rather than
    // registration - to decide what an impossible type means.
    enum class SubConsoleKind { MiniConsole, UserWindow, Buffer, Other };

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

    void registerSubConsole(const QString& name, TConsoleModel* pModel, const SubConsoleKind kind) { mSubConsoles.insert(name, {pModel, kind}); }

    // Identity-checked for the same reason deregisterLabel() is: every path that
    // destroys a sub-console defers the delete, so ~TConsole can run after the
    // name has been given to a replacement.
    void deregisterSubConsole(const QString& name, const TConsoleModel* pModel)
    {
        if (mSubConsoles.value(name).pModel == pModel) {
            mSubConsoles.remove(name);
        }
    }

    bool hasSubConsole(const QString& name) const { return mSubConsoles.contains(name); }
    TConsoleModel* subConsoleModel(const QString& name) const { return mSubConsoles.value(name).pModel; }

    std::optional<SubConsoleKind> subConsoleKind(const QString& name) const
    {
        const auto it = mSubConsoles.constFind(name);
        if (it == mSubConsoles.constEnd()) {
            return {};
        }
        return {it->kind};
    }

    // A detached snapshot, which is what makes it safe to act on each name in
    // turn even when acting on a name removes its entry mid-walk -
    // Host::closeChildren() closes every sub-console.
    QStringList subConsoleNames() const { return QStringList(mSubConsoles.keys()); }

    // Dock widgets are indexed by name alone. Everything core asks of one is
    // whether the name has a dock, which is what tells a user window apart from
    // a miniconsole; the rest - geometry, floating state, stylesheet, the
    // layoutChanged property - is widget state with no core-side meaning, so
    // there is nothing for a model to hold and no widget pointer to keep here.
    void registerDockWidget(const QString& name) { mDockWidgets.insert(name); }

    // Not identity-checked, unlike the two above: nothing deregisters a dock
    // from its own destructor, so every removal is the view taking the entry out
    // of its map at the same moment, and there is no window in which a stale
    // deregistration could evict a replacement.
    void deregisterDockWidget(const QString& name) { mDockWidgets.remove(name); }

    bool hasDockWidget(const QString& name) const { return mDockWidgets.contains(name); }

private:
    struct SubConsoleEntry
    {
        TConsoleModel* pModel = nullptr;
        SubConsoleKind kind = SubConsoleKind::Other;
    };

    QMap<QString, TLabelModel*> mLabels;
    QMap<QString, SubConsoleEntry> mSubConsoles;
    QSet<QString> mDockWidgets;
};

#endif // MUDLET_TWINDOWREGISTRY_H
