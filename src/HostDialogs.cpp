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
#include "KeyUnit.h"
#include "TConsoleModel.h"
#include "dlgIRC.h"
#include "dlgNotepad.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include <QTimer>
#include <utility>

using namespace std::chrono_literals;

namespace {

void closeDialogs(Host* pHost, HostDialogs* pDialogs)
{
    if (pDialogs && pDialogs->mpEditorDialog) {
        pDialogs->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
        pDialogs->mpEditorDialog->close();
        // close() only posts the deletion, so the dialog outlives this release.
        // Cutting the signals with the pointer is what keeps an emit from
        // reaching an editor the Host has already let go of:
        QObject::disconnect(pHost, nullptr, pDialogs->mpEditorDialog, nullptr);
        pDialogs->mpEditorDialog = nullptr;
    }

    if (pDialogs && pDialogs->mpNotePad) {
        pDialogs->mpNotePad->save();
        pDialogs->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
        pDialogs->mpNotePad->close();
        QObject::disconnect(pHost, nullptr, pDialogs->mpNotePad, nullptr);
        pDialogs->mpNotePad = nullptr;
    }

    if (pHost->mpDlgIRC) {
        pHost->mpDlgIRC->setAttribute(Qt::WA_DeleteOnClose);
        pHost->mpDlgIRC->deleteLater();
        pHost->mpDlgIRC = nullptr;
    }
}

void destroyDialogs(Host* pHost, HostDialogs* pDialogs)
{
    // The editor is a parentless top-level window, so delete it here while the
    // units it references are still alive. Null the QPointer first: it only
    // clears itself once ~QObject is reached, so anything looking at
    // mpEditorDialog mid-teardown would find a half-destroyed widget:
    if (auto* pEditor = pDialogs ? pDialogs->mpEditorDialog.data() : nullptr) {
        pDialogs->mpEditorDialog = nullptr;
        QObject::disconnect(pHost, nullptr, pEditor, nullptr);
        delete pEditor;
    }

    if (auto* pNotePad = pDialogs ? pDialogs->mpNotePad.data() : nullptr) {
        if (mudlet::self()) {
            pNotePad->save();
            pNotePad->close();
        }
        pDialogs->mpNotePad = nullptr;
        QObject::disconnect(pHost, nullptr, pNotePad, nullptr);
        delete pNotePad;
    }

    if (auto* pDlgIRC = pHost->mpDlgIRC.data()) {
        pHost->mpDlgIRC = nullptr;
        delete pDlgIRC;
    }
}

} // namespace

HostDialogs::HostDialogs(Host* pHost)
: QObject(pHost)
, mpErrorConsoleModel(dlgTriggerEditor::newErrorConsoleModel(pHost))
{
    // Until an editor takes the model, after which it prints to it itself
    connect(pHost, &Host::signal_errorConsolePrint, this, [this](const QString& text, const QColor& fgColor, const QColor& bgColor) {
        if (mpErrorConsoleModel) {
            mpErrorConsoleModel->print(text, fgColor, bgColor);
        }
    });
    // In the editor, not on the main screen: see mudlet::warnProfilesLosingBindingTo()
    connect(pHost, &Host::signal_keyBoundByScript, this, [pHost](const int id) {
        if (const QString warning = dlgTriggerEditor::takenKeyWarning(pHost, pHost->getKeyUnit()->getKey(id)); !warning.isEmpty()) {
            showEditorWarning(pHost, warning);
        }
    });
    // Cleared when the reset would have run, as the reset clears a built editor's notices
    connect(pHost, &Host::signal_editorCleanResetRequested, this, [this]() {
        if (mpEditorDialog || mEditorResetQueued) {
            return;
        }
        mEditorResetQueued = true;
        QTimer::singleShot(0ms, this, [this]() {
            mEditorResetQueued = false;
            if (!mpEditorDialog) {
                mPendingEditorWarning.clear();
            }
        });
    });
}

HostDialogs::~HostDialogs() = default;

HostDialogs& HostDialogs::of(Host* pHost)
{
    auto* pDialogs = find(pHost);
    if (!pDialogs) {
        pDialogs = new HostDialogs(pHost);
    }
    return *pDialogs;
}

HostDialogs* HostDialogs::find(const Host* pHost)
{
    return pHost->findChild<HostDialogs*>(QString(), Qt::FindDirectChildrenOnly);
}

void HostDialogs::connectTeardown(Host* pHost)
{
    // The Host is the context rather than the main window: ~Host() also runs
    // after the main window has started to come apart, and the dialogs must
    // still go then. The held error console goes with the editor, so what the
    // teardown prints reaches neither, nor --mirror.
    QObject::connect(
            pHost,
            &Host::signal_closeProfileDialogs,
            pHost,
            [pHost]() {
                HostDialogs* pDialogs = find(pHost);
                if (pDialogs) {
                    pDialogs->mpErrorConsoleModel.reset();
                }
                closeDialogs(pHost, pDialogs);
            },
            Qt::DirectConnection);
    QObject::connect(
            pHost,
            &Host::signal_destroyProfileDialogs,
            pHost,
            [pHost]() {
                HostDialogs* pDialogs = find(pHost);
                if (pDialogs) {
                    pDialogs->mpErrorConsoleModel.reset();
                }
                destroyDialogs(pHost, pDialogs);
            },
            Qt::DirectConnection);
}

dlgTriggerEditor* HostDialogs::editor(Host* pHost)
{
    HostDialogs& dialogs = of(pHost);
    if (dialogs.mpEditorDialog) {
        return dialogs.mpEditorDialog;
    }

    auto* pEditor = new dlgTriggerEditor(pHost, std::move(dialogs.mpErrorConsoleModel));
    dialogs.mpEditorDialog = pEditor;
    connect(pHost, &Host::profileSaveStarted, pEditor, &dlgTriggerEditor::slot_profileSaveStarted);
    connect(pHost, &Host::profileSaveFinished, pEditor, &dlgTriggerEditor::slot_profileSaveFinished);
    if (pHost->currentlySavingProfile()) {
        pEditor->slot_profileSaveStarted();
    }
    // The profile applies its stylesheet only as it changes, which may have been before now
    if (!pHost->mProfileStyleSheet.isEmpty()) {
        pEditor->setStyleSheet(pHost->mProfileStyleSheet);
    }
    if (!dialogs.mPendingEditorWarning.isEmpty()) {
        pEditor->showWarning(std::exchange(dialogs.mPendingEditorWarning, QString()), false);
    }
    if (dialogs.mEditorResetQueued) {
        pEditor->doCleanReset();
    }
    return pEditor;
}

void HostDialogs::showEditorWarning(Host* pHost, const QString& warning)
{
    HostDialogs* pDialogs = find(pHost);
    if (!pDialogs) {
        return;
    }
    if (!pDialogs->mpEditorDialog) {
        pDialogs->mPendingEditorWarning = warning;
        return;
    }
    // Read out only when it can be seen, or a script binding keys on connect
    // would have it read out at every connect
    pDialogs->mpEditorDialog->showWarning(warning, pDialogs->mpEditorDialog->isVisible());
}
