/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "dlgNotepad.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QDir>
#include "post_guard.h"

using namespace std::chrono;

dlgNotepad::dlgNotepad(Host* pH)
: mpHost(pH)
{
    setupUi(this);

    if (mpHost) {
        restore();
    }

    connect(notesEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::slot_text_written);

    startTimer(2min);
}

dlgNotepad::~dlgNotepad()
{
    // Safety step, just in case:
    if (mpHost && mpHost->mpNotePad) {
        save();
        mpHost->mpNotePad = nullptr;
    }
}

void dlgNotepad::save()
{
    QString directoryFile = mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName());
    QString fileName = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), QStringLiteral("notes.txt"));
    QDir dirFile;
    if (!dirFile.exists(directoryFile)) {
        dirFile.mkpath(directoryFile);
    }
    QFile file;
    file.setFileName(fileName);
    file.open(QIODevice::WriteOnly);
    QTextStream fileStream;
    fileStream.setDevice(&file);
    fileStream << notesEdit->toPlainText();
    file.close();

    mNeedToSave = false;
}

void dlgNotepad::restore()
{
    QString fileName = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), QStringLiteral("notes.txt"));
    QFile file;
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    QTextStream fileStream;
    fileStream.setDevice(&file);
    QString txt = fileStream.readAll();
    notesEdit->blockSignals(true);
    notesEdit->setPlainText(txt);
    notesEdit->blockSignals(false);
    file.close();
}

void dlgNotepad::slot_text_written()
{
    mNeedToSave = true;
}

void dlgNotepad::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event);

    if (!mNeedToSave) {
        return;
    }

    save();
}
