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

// Host's index of named windows (kind and model by name), so the core needs no Qt Widgets to ask.
// Entries are NON-OWNING: each model belongs to its view object (TLabel, TConsole), which must register
// where it creates the widget and deregister where it destroys it. TMainConsole's widget maps stay the
// view's only way to reach a widget by name.
class TWindowRegistry
{
public:
    // TConsole::ConsoleType is widget-side, so the kind is recorded here. Other leaves it to
    // Host::windowType(), not registration, to decide what an impossible type means.
    enum class SubConsoleKind { MiniConsole, UserWindow, Buffer, Other };

    void registerLabel(const QString& name, TLabelModel* pModel) { mLabels.insert(name, pModel); }

    // Identity-checked: TMainConsole::deleteLabel() defers the delete, so the name can be registered again
    // before the old TLabel's destructor runs, which must not evict the replacement.
    void deregisterLabel(const QString& name, const TLabelModel* pModel)
    {
        if (mLabels.value(name) == pModel) {
            mLabels.remove(name);
        }
    }

    bool hasLabel(const QString& name) const { return mLabels.contains(name); }
    TLabelModel* labelModel(const QString& name) { return mLabels.value(name); }
    const TLabelModel* labelModel(const QString& name) const { return mLabels.value(name); }

    void registerSubConsole(const QString& name, TConsoleModel* pModel, const SubConsoleKind kind) { mSubConsoles.insert(name, {pModel, kind}); }

    // Identity-checked like deregisterLabel(): sub-console deletes are deferred too.
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

    // A copy, so callers such as Host::closeChildren() can remove entries while walking it.
    QStringList subConsoleNames() const { return QStringList(mSubConsoles.keys()); }

    // Name only: core just asks whether a name has a dock (a user window, not a miniconsole).
    void registerDockWidget(const QString& name) { mDockWidgets.insert(name); }

    // Not identity-checked: nothing deregisters a dock from its destructor, so removal happens as the view
    // drops it from its own map and a stale deregistration can't evict a replacement.
    void deregisterDockWidget(const QString& name) { mDockWidgets.remove(name); }

    bool hasDockWidget(const QString& name) const { return mDockWidgets.contains(name); }

    // Name only, like docks: core asks just whether a name exists and its kind.
    // Three sets as the name spaces are independent; a name in several resolves in declaration order.
    void registerScrollBox(const QString& name) { mScrollBoxes.insert(name); }
    void deregisterScrollBox(const QString& name) { mScrollBoxes.remove(name); }
    bool hasScrollBox(const QString& name) const { return mScrollBoxes.contains(name); }

    void registerCommandLine(const QString& name) { mCommandLines.insert(name); }

    // Not identity-checked, like docks. The view's destroyed() handlers drop these by widget identity,
    // so they only name a name the dying widget still holds.
    void deregisterCommandLine(const QString& name) { mCommandLines.remove(name); }
    bool hasCommandLine(const QString& name) const { return mCommandLines.contains(name); }

    void registerTextBox(const QString& name) { mTextBoxes.insert(name); }
    void deregisterTextBox(const QString& name) { mTextBoxes.remove(name); }
    bool hasTextBox(const QString& name) const { return mTextBoxes.contains(name); }

    bool hasPlainWindow(const QString& name) const { return hasScrollBox(name) || hasCommandLine(name) || hasTextBox(name); }

private:
    struct SubConsoleEntry
    {
        TConsoleModel* pModel = nullptr;
        SubConsoleKind kind = SubConsoleKind::Other;
    };

    QMap<QString, TLabelModel*> mLabels;
    QMap<QString, SubConsoleEntry> mSubConsoles;
    QSet<QString> mDockWidgets;
    QSet<QString> mScrollBoxes;
    QSet<QString> mCommandLines;
    QSet<QString> mTextBoxes;
};

#endif // MUDLET_TWINDOWREGISTRY_H
