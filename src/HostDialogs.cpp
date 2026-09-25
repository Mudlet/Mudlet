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

#include "HostDialogs.h"

#include "Host.h"
#include "dlgIRC.h"
#include "dlgNotepad.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

namespace {

void closeDialogs(Host* pHost)
{
    if (pHost->mpEditorDialog) {
        pHost->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
        pHost->mpEditorDialog->close();
        // close() only posts the deletion, so the dialog outlives this release.
        // Cutting the signals with the pointer is what keeps an emit from
        // reaching an editor the Host has already let go of:
        QObject::disconnect(pHost, nullptr, pHost->mpEditorDialog, nullptr);
        pHost->mpEditorDialog = nullptr;
    }

    if (pHost->mpNotePad) {
        pHost->mpNotePad->save();
        pHost->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
        pHost->mpNotePad->close();
        QObject::disconnect(pHost, nullptr, pHost->mpNotePad, nullptr);
        pHost->mpNotePad = nullptr;
    }

    if (pHost->mpDlgIRC) {
        pHost->mpDlgIRC->setAttribute(Qt::WA_DeleteOnClose);
        pHost->mpDlgIRC->deleteLater();
        pHost->mpDlgIRC = nullptr;
    }
}

void destroyDialogs(Host* pHost)
{
    // The editor is a parentless top-level window, so delete it here while the
    // units it references are still alive. Null the QPointer first: it only
    // clears itself once ~QObject is reached, so anything looking at
    // mpEditorDialog mid-teardown would find a half-destroyed widget:
    if (auto* pEditor = pHost->mpEditorDialog.data()) {
        pHost->mpEditorDialog = nullptr;
        QObject::disconnect(pHost, nullptr, pEditor, nullptr);
        delete pEditor;
    }

    if (auto* pNotePad = pHost->mpNotePad.data()) {
        if (mudlet::self()) {
            pNotePad->save();
            pNotePad->close();
        }
        pHost->mpNotePad = nullptr;
        QObject::disconnect(pHost, nullptr, pNotePad, nullptr);
        delete pNotePad;
    }

    if (auto* pDlgIRC = pHost->mpDlgIRC.data()) {
        pHost->mpDlgIRC = nullptr;
        delete pDlgIRC;
    }
}

} // namespace

void HostDialogs::connectTeardown(Host* pHost)
{
    // The Host is the context rather than the main window: ~Host() also runs
    // after the main window has started to come apart, and the dialogs must
    // still go then.
    QObject::connect(
            pHost,
            &Host::signal_closeProfileDialogs,
            pHost,
            [pHost]() {
                closeDialogs(pHost);
            },
            Qt::DirectConnection);
    QObject::connect(
            pHost,
            &Host::signal_destroyProfileDialogs,
            pHost,
            [pHost]() {
                destroyDialogs(pHost);
            },
            Qt::DirectConnection);
}
