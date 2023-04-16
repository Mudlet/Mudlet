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
#include <QTextCodec>
#include "post_guard.h"

using namespace std::chrono;

// Used before we spotted a problem with not specifying an encoding:
const QString local8BitEncodedNotesFileName{qsl("notes.txt")};
// Used afterwards:
const QString utf8EncodedNotesFileName{qsl("notes_utf8.txt")};

dlgNotepad::dlgNotepad(Host* pH)
: mpHost(pH)
{
    setupUi(this);

    if (mpHost) {
        restore();
    }

    connect(notesEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::slot_textWritten);

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
    QString fileName = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    QDir dirFile;
    if (!dirFile.exists(directoryFile)) {
        dirFile.mkpath(directoryFile);
    }
    QFile file;
    file.setFileName(fileName);
    file.open(QIODevice::WriteOnly);
    QTextStream fileStream;
    fileStream.setDevice(&file);
    // fileStream.setCodec is removed in Qt6 and UTF-8 is the default
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    fileStream.setCodec(QTextCodec::codecForName("UTF-8"));
#endif
    fileStream << notesEdit->toPlainText();
    file.close();

    mNeedToSave = false;
}

void dlgNotepad::restoreFile(const QString& fn, const bool useUtf8Encoding)
{
    QFile file(fn);
    file.open(QIODevice::ReadOnly);
    QTextStream fileStream;
    fileStream.setDevice(&file);
    // In Qt6 the default encoding is UTF-8 instead of the system default
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (useUtf8Encoding) {
        fileStream.setCodec(QTextCodec::codecForName("UTF-8"));
    }
#else
    if (!useUtf8Encoding) {
        fileStream.setEncoding(QStringEncoder::Encoding::System);
    }
#endif
    const QString txt = fileStream.readAll();
    notesEdit->blockSignals(true);
    notesEdit->setPlainText(txt);
    notesEdit->blockSignals(false);
    file.close();
}

void dlgNotepad::restore()
{
    QString fileName = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    if (QFile::exists(fileName)) {
        restoreFile(fileName, true);
        return;
    }

    // A utf8 encoded (new style) file was not found, so look for an older one
    // where we did not enforce an encoding (and, at least on Windows, it
    // defaulted to the local8Bit one) and it would break if characters were
    // used {e.g. emojis} that that encoding did not handle:
    fileName = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), local8BitEncodedNotesFileName);
    restoreFile(fileName, false);
}

void dlgNotepad::slot_textWritten()
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
