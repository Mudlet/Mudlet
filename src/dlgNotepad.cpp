/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018, 2025 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

    //: label for prepended text entry box in notepad
    label_prependText = new QLabel(tr("Prepend"), this);
    action_prependTextLabel = toolBar->addWidget(label_prependText);
    lineEdit_prependText = new QLineEdit(this);
    //: placeholder text for text entry box in notepad - text which gets added before sending a line
    lineEdit_prependText->setPlaceholderText(tr("Text to prepend to lines"));
    lineEdit_prependText->setClearButtonEnabled(true);
    action_prependText = toolBar->addWidget(lineEdit_prependText);

    action_stop = new QAction(tr("Stop"), this);
    toolBar->addAction(action_stop);
    action_stop->setEnabled(false);

    connect(action_stop, &QAction::triggered, this, &dlgNotepad::slot_stopSending);
    connect(action_sendAll, &QAction::triggered, this, &dlgNotepad::slot_sendAll);
    connect(action_sendLine, &QAction::triggered, this, &dlgNotepad::slot_sendLine);
    connect(action_sendSelection, &QAction::triggered, this, &dlgNotepad::slot_sendSelection);
    connect(action_toggleSendControls, &QAction::triggered, this, &dlgNotepad::slot_toggleSendControls);
    connect(action_toggleSendControls, &QAction::triggered, this, &dlgNotepad::saveSettings);

    if (mpHost) {
        restore();
        notesEdit->setFont(mpHost->getDisplayFont());
        restoreSettings();
    }

    connect(notesEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::slot_textWritten);

    startTimer(2min);
}

void dlgNotepad::setFont(const QFont& font)
{
    notesEdit->setFont(font);
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
    const QString directoryFile = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName());
    const QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    const QDir dirFile;
    if (!dirFile.exists(directoryFile)) {
        dirFile.mkpath(directoryFile);
    }
    QSaveFile file;
    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "dlgNotepad::save: failed to open file for writing:" << file.errorString();
        return;
    }
    QTextStream fileStream;
    fileStream.setDevice(&file);
    fileStream << notesEdit->toPlainText();
    if (!file.commit()) {
        qDebug() << "dlgNotepad::save: error saving notepad contents: " << file.errorString();
    }

    mNeedToSave = false;
}

void dlgNotepad::restoreFile(const QString& fn, const bool useUtf8Encoding)
{
    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "dlgNotepad::restoreFile: failed to open file for reading:" << file.errorString();
        return;
    }
    QTextStream fileStream;
    fileStream.setDevice(&file);
    if (!useUtf8Encoding) {
        fileStream.setEncoding(QStringEncoder::Encoding::System);
    }
    const QString txt = fileStream.readAll();
    notesEdit->blockSignals(true);
    notesEdit->setPlainText(txt);
    notesEdit->blockSignals(false);
    file.close();
}

void dlgNotepad::restore()
{
    QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    if (QFile::exists(fileName)) {
        restoreFile(fileName, true);
        return;
    }

    // A utf8 encoded (new style) file was not found, so look for an older one
    // where we did not enforce an encoding (and, at least on Windows, it
    // defaulted to the local8Bit one) and it would break if characters were
    // used {e.g. emojis} that that encoding did not handle:
    fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), local8BitEncodedNotesFileName);
    restoreFile(fileName, false);
}

void dlgNotepad::slot_textWritten()
{
    mNeedToSave = true;
}

void dlgNotepad::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    if (!mNeedToSave) {
        return;
    }

    save();
}

void dlgNotepad::slot_sendAll() {
    QString allText = notesEdit->toPlainText();
    QStringList lines = allText.split('\n');
    startSendingLines(lines);
}

void dlgNotepad::slot_sendLine() {
    QTextCursor cursor = notesEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString line = cursor.selectedText();

    if (!line.isEmpty()) {
        startSendingLines(QStringList{line});
    }
}

void dlgNotepad::slot_sendSelection() {
    QString selectedText = notesEdit->textCursor().selectedText();

    if (!selectedText.isEmpty()) {
        QStringList lines = selectedText.replace(QChar(0x2029), "\n").split('\n');
        startSendingLines(lines);
    }
}

void dlgNotepad::startSendingLines(const QStringList& lines) {
    mLinesToSend = lines;
    mCurrentLineIndex = 0;

    if (!mSendTimer) {
        mSendTimer = new QTimer(this);
        connect(mSendTimer, &QTimer::timeout, this, &dlgNotepad::slot_sendNextLine);
    }

    action_stop->setEnabled(true);
    mSendTimer->start(300);
}

void dlgNotepad::slot_sendNextLine() {
    if (mCurrentLineIndex >= mLinesToSend.size()) {
        mSendTimer->stop();
        action_stop->setEnabled(false);
        return;
    }

    QString line = mLinesToSend[mCurrentLineIndex++];
    if (!line.isEmpty()) {
        QString prepend = lineEdit_prependText->text().isEmpty() ? QString() : lineEdit_prependText->text();
        mpHost->send(prepend + line);
    }
}

void dlgNotepad::slot_stopSending() {
    if (mSendTimer && mSendTimer->isActive()) {
        mSendTimer->stop();
    }

    action_stop->setEnabled(false);
    mLinesToSend.clear();
    mCurrentLineIndex = 0;
}

void dlgNotepad::slot_toggleSendControls(bool checked)
{
    action_sendAll->setVisible(checked);
    action_sendLine->setVisible(checked);
    action_sendSelection->setVisible(checked);

    if (action_prependTextLabel) {
        action_prependTextLabel->setVisible(checked);
    }

    if (action_prependText) {
        action_prependText->setVisible(checked);
    }

    if (action_stop) {
        action_stop->setVisible(checked);
    }

    if (action_toggleSendControls->isChecked() != checked) {
        action_toggleSendControls->setChecked(checked);
    }
}

void dlgNotepad::saveSettings()
{
    if (!mpHost) {
        return;
    }

    QSettings* pQSettings = mudlet::getQSettings();
    if (!pQSettings) {
        return;
    }

    const QString settingsKey = qsl("notepad/%1/sendControlsVisible").arg(mpHost->getName());
    pQSettings->setValue(settingsKey, action_toggleSendControls->isChecked());
}

void dlgNotepad::restoreSettings()
{
    if (!mpHost) {
        return;
    }

    QSettings* pQSettings = mudlet::getQSettings();
    if (!pQSettings) {
        return;
    }

    const QString settingsKey = qsl("notepad/%1/sendControlsVisible").arg(mpHost->getName());
    const bool sendControlsVisible = pQSettings->value(settingsKey, false).toBool();

    // Block signals to avoid triggering saveSettings during restoration
    const bool wasBlocked = action_toggleSendControls->signalsBlocked();
    action_toggleSendControls->blockSignals(true);
    action_toggleSendControls->setChecked(sendControlsVisible);
    action_toggleSendControls->blockSignals(wasBlocked);

    slot_toggleSendControls(sendControlsVisible);
}

void dlgNotepad::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}
