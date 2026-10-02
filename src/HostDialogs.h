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

class Host;
class dlgModuleManager;
class dlgNotepad;
class dlgPackageManager;
class dlgProfilePreferences;
class dlgTriggerEditor;

// The dialogs the frontend has opened for one profile, at most one of each. It
// is a child of the profile's Host, so it goes when the Host does.
class HostDialogs : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(HostDialogs)
    // Makes the profile's set on first use
    static HostDialogs& of(Host*);
    // Only looks: nullptr when the frontend has opened nothing for the profile yet
    static HostDialogs* find(const Host*);
    // Makes the frontend answer the profile's requests to close or destroy the
    // dialogs it opened for it. Call once per Host.
    static void connectTeardown(Host*);

    QPointer<dlgTriggerEditor> mpEditorDialog;
    QPointer<dlgNotepad> mpNotePad;
    QPointer<dlgPackageManager> mpPackageManager;
    QPointer<dlgModuleManager> mpModuleManager;
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;

private:
    explicit HostDialogs(Host*);
};

#endif // MUDLET_HOSTDIALOGS_H
