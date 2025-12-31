/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2025 by Mudlet Development Team                         *
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

#include "NotesTabContainer.h"
#include "dlgNotepad.h"
#include "Host.h"
#include <QVBoxLayout>

NotesTabContainer::NotesTabContainer(Host* pH, QWidget* parent)
    : QWidget(parent)
    , mpHost(pH)
{
    setupUi();
    initConnections();
}

NotesTabContainer::~NotesTabContainer()
{
    if (mpNotesWidget) {
        disconnect(mpNotesWidget, nullptr, this, nullptr);
    }
}

void NotesTabContainer::setupUi()
{
    mpLayout = new QVBoxLayout(this);
    mpLayout->setContentsMargins(0, 0, 0, 0);
    mpLayout->setSpacing(0);
    
    if (mpHost) {
        mpNotesWidget = new dlgNotepad(mpHost);
        mpLayout->addWidget(mpNotesWidget);
    }
}

void NotesTabContainer::initConnections()
{
    if (mpNotesWidget) {
        connect(mpNotesWidget, &dlgNotepad::notepadClosing, this, [this](const QString& profileName) {
            Q_UNUSED(profileName);
            emit notesTabHidden();
        });
    }
}

void NotesTabContainer::showNotesTab()
{
    if (mpNotesWidget) {
        mpNotesWidget->show();
        mpNotesWidget->restore();
        emit notesTabFocused();
    }
}

void NotesTabContainer::hideNotesTab()
{
    if (mpNotesWidget) {
        mpNotesWidget->save();
        hide();
        emit notesTabHidden();
    }
}

void NotesTabContainer::setFont(const QFont& font)
{
    if (mpNotesWidget) {
        mpNotesWidget->setFont(font);
    }
}

void NotesTabContainer::saveState()
{
    if (mpNotesWidget) {
        mpNotesWidget->saveSettings();
    }
}

void NotesTabContainer::restoreState()
{
    if (mpNotesWidget) {
        mpNotesWidget->restoreSettings();
    }
}
