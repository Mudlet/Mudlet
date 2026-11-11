#ifndef MUDLET_HOSTDIALOGS_H
#define MUDLET_HOSTDIALOGS_H

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

#include <QObject>
#include <QPointer>
#include <QString>
#include <memory>

class Host;
class TConsoleModel;
class dlgModuleManager;
class dlgNotepad;
class dlgPackageManager;
class dlgProfilePreferences;
class dlgTriggerEditor;

// The dialogs the frontend has opened for one profile, at most one of each, and
// what the editor is to show once built. A child of the profile's Host, so it
// goes when the Host does.
class HostDialogs : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(HostDialogs)
    // Makes the profile's set on first use
    static HostDialogs& of(Host*);
    // Only looks: nullptr for a profile the frontend has not taken on
    static HostDialogs* find(const Host*);
    // Makes the frontend answer the profile's requests to close or destroy the
    // dialogs it opened for it. Call once per Host.
    static void connectTeardown(Host*);
    // Built on first use, with what it would have shown had it been built at profile load
    static dlgTriggerEditor* editor(Host*);
    // Held until the editor is built, only the latest as the editor shows one, and dropped by a
    // clean reset. Dropped too for a profile the frontend has not taken on.
    static void showEditorWarning(Host*, const QString&);

    QPointer<dlgTriggerEditor> mpEditorDialog;
    QPointer<dlgNotepad> mpNotePad;
    QPointer<dlgPackageManager> mpPackageManager;
    QPointer<dlgModuleManager> mpModuleManager;
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;

private:
    explicit HostDialogs(Host*);
    ~HostDialogs() override;

    // Handed to the editor when it is built
    std::unique_ptr<TConsoleModel> mpErrorConsoleModel;
    QString mPendingEditorWarning;
    bool mEditorResetQueued = false;
};

#endif // MUDLET_HOSTDIALOGS_H
