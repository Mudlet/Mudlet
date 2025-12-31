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
#include "NotesManager.h"

#include "mudlet.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

using namespace std::chrono;

dlgNotepad::dlgNotepad(Host* pH)
: mpHost(pH)
{
    setupUi(this);

    mpNotesManager = pH->mpNotesManager;

    mTabWidget = new QTabWidget(this);
    mTabWidget->setTabsClosable(true);
    mTabWidget->setMovable(true);
    mTabWidget->setDocumentMode(true);

    QLayout* layout = centralwidget->layout();
    layout->replaceWidget(notesEdit, mTabWidget);
    delete notesEdit;
    notesEdit = nullptr;

    setupTabContextMenu();

    label_prependText = new QLabel(tr("Prepend"), this);
    action_prependTextLabel = toolBar->addWidget(label_prependText);
    lineEdit_prependText = new QLineEdit(this);
    lineEdit_prependText->setPlaceholderText(tr("Text to prepend to lines"));
    lineEdit_prependText->setClearButtonEnabled(true);
    action_prependText = toolBar->addWidget(lineEdit_prependText);

    action_stop = new QAction(tr("Stop"), this);
    toolBar->addAction(action_stop);
    action_stop->setEnabled(false);

    connect(action_stop, &QAction::triggered, this, &dlgNotepad::slot_stopSending);
    connect(action_addTab, &QAction::triggered, this, &dlgNotepad::slot_addTab);
    connect(action_sendAll, &QAction::triggered, this, &dlgNotepad::slot_sendAll);
    connect(action_sendLine, &QAction::triggered, this, &dlgNotepad::slot_sendLine);
    connect(action_sendSelection, &QAction::triggered, this, &dlgNotepad::slot_sendSelection);
    connect(action_toggleSendControls, &QAction::triggered, this, &dlgNotepad::slot_toggleSendControls);
    connect(action_toggleSendControls, &QAction::triggered, this, &dlgNotepad::saveSettings);

    connect(mTabWidget, &QTabWidget::tabCloseRequested, this, &dlgNotepad::slot_removeTab);
    connect(mTabWidget, &QTabWidget::currentChanged, this, &dlgNotepad::slot_currentTabChanged);

    connect(mpNotesManager, &NotesManager::tabAdded, this, &dlgNotepad::slot_managerTabAdded);
    connect(mpNotesManager, &NotesManager::tabRemoved, this, &dlgNotepad::slot_managerTabRemoved);
    connect(mpNotesManager, &NotesManager::tabRenamed, this, &dlgNotepad::slot_managerTabRenamed);

    if (mpHost) {
        restore();
        restoreSettings();
    }

    const auto& tabsMap = mpNotesManager->getTabsMap();
    for (auto it = tabsMap.constBegin(); it != tabsMap.constEnd(); ++it) {
        createTabContent(it.key(), it.value().name);
    }

    if (mTabWidget->count() == 0) {
        const QString tabId = mpNotesManager->addTab(tr("General Notes"));
        createTabContent(tabId, tr("General Notes"));
    }

    startTimer(2min);
}

void dlgNotepad::setFont(const QFont& font)
{
    for (int i = 0; i < mTabWidget->count(); ++i) {
        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(mTabWidget->widget(i))) {
            textEdit->setFont(font);
        }
    }
}

dlgNotepad::~dlgNotepad()
{
    if (mpHost && mpHost->mpNotePad) {
        save();
        mpHost->mpNotePad = nullptr;
    }
}

void dlgNotepad::save()
{
    updateTabContent(getCurrentTabId());

    if (mpNotesManager) {
        mpNotesManager->save();
    }

    mNeedToSave = false;
}

void dlgNotepad::restore()
{
    if (mpNotesManager) {
        mpNotesManager->restore();
    }
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

void dlgNotepad::slot_sendAll()
{
    auto* textEdit = getCurrentTextEdit();
    if (!textEdit) {
        return;
    }

    QString allText = textEdit->toPlainText();
    QStringList lines = allText.split('\n');
    startSendingLines(lines);
}

void dlgNotepad::slot_sendLine()
{
    auto* textEdit = getCurrentTextEdit();
    if (!textEdit) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString line = cursor.selectedText();

    if (!line.isEmpty()) {
        startSendingLines(QStringList{line});
    }
}

void dlgNotepad::slot_sendSelection()
{
    auto* textEdit = getCurrentTextEdit();
    if (!textEdit) {
        return;
    }

    QString selectedText = textEdit->textCursor().selectedText();

    if (!selectedText.isEmpty()) {
        QStringList lines = selectedText.replace(QChar(0x2029), "\n").split('\n');
        startSendingLines(lines);
    }
}

void dlgNotepad::startSendingLines(const QStringList& lines)
{
    mLinesToSend = lines;
    mCurrentLineIndex = 0;

    if (!mSendTimer) {
        mSendTimer = new QTimer(this);
        connect(mSendTimer, &QTimer::timeout, this, &dlgNotepad::slot_sendNextLine);
    }

    action_stop->setEnabled(true);
    mSendTimer->start(300);
}

void dlgNotepad::slot_sendNextLine()
{
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

void dlgNotepad::slot_stopSending()
{
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

    const bool wasBlocked = action_toggleSendControls->signalsBlocked();
    action_toggleSendControls->blockSignals(true);
    action_toggleSendControls->setChecked(sendControlsVisible);
    action_toggleSendControls->blockSignals(wasBlocked);

    slot_toggleSendControls(sendControlsVisible);
}

QPlainTextEdit* dlgNotepad::getCurrentTextEdit() const
{
    QWidget* currentWidget = mTabWidget->currentWidget();
    return qobject_cast<QPlainTextEdit*>(currentWidget);
}

QString dlgNotepad::getCurrentTabId() const
{
    int currentIndex = mTabWidget->currentIndex();
    if (currentIndex < 0) {
        return QString();
    }

    return mIndexToTabId.value(currentIndex, QString());
}

void dlgNotepad::setupTabContextMenu()
{
    mTabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mTabWidget, &QTabWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        int tabIndex = mTabWidget->tabBar()->tabAt(pos);
        if (tabIndex < 0) {
            return;
        }

        QMenu menu(this);
        QAction* renameAction = menu.addAction(tr("Rename"));
        QAction* deleteAction = menu.addAction(tr("Delete"));

        QAction* selectedAction = menu.exec(mTabWidget->tabBar()->mapToGlobal(pos));
        if (selectedAction == renameAction) {
            slot_renameTab();
        } else if (selectedAction == deleteAction) {
            slot_removeTab();
        }
    });
}

void dlgNotepad::slot_addTab()
{
    const QString tabId = mpNotesManager->addTab();
}

void dlgNotepad::slot_removeTab()
{
    const QString tabId = getCurrentTabId();
    if (!tabId.isEmpty() && mTabWidget->count() > 1) {
        mpNotesManager->removeTab(tabId);
    }
}

void dlgNotepad::slot_renameTab()
{
    const QString tabId = getCurrentTabId();
    if (tabId.isEmpty()) {
        return;
    }

    int currentIndex = mTabWidget->currentIndex();
    if (currentIndex < 0) {
        return;
    }

    const QString currentName = mTabWidget->tabText(currentIndex);
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename Tab"), tr("New tab name:"), QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        mpNotesManager->renameTab(tabId, newName);
    }
}

void dlgNotepad::slot_currentTabChanged(int index)
{
    if (index >= 0 && index < mTabWidget->count()) {
        const QString tabId = mIndexToTabId.value(index, QString());
        if (!tabId.isEmpty() && mpNotesManager) {
            const QString content = mpNotesManager->getTabContent(tabId);
            auto* textEdit = qobject_cast<QPlainTextEdit*>(mTabWidget->widget(index));
            if (textEdit) {
                textEdit->blockSignals(true);
                textEdit->setPlainText(content);
                textEdit->blockSignals(false);
            }
        }
    }
}

void dlgNotepad::slot_managerTabAdded(const QString& tabId, const QString& tabName)
{
    createTabContent(tabId, tabName);
}

void dlgNotepad::slot_managerTabRemoved(const QString& tabId)
{
    int index = mTabIdToIndex.value(tabId, -1);
    if (index >= 0) {
        mIndexToTabId.remove(index);
        mTabIdToIndex.remove(tabId);
        mTabWidget->removeTab(index);

        for (int i = index; i < mTabWidget->count(); ++i) {
            const QString id = mIndexToTabId.value(i, QString());
            if (!id.isEmpty()) {
                mIndexToTabId[i] = id;
                mTabIdToIndex[id] = i;
            }
        }
    }
}

void dlgNotepad::slot_managerTabRenamed(const QString& tabId, const QString& newName)
{
    int index = mTabIdToIndex.value(tabId, -1);
    if (index >= 0) {
        mTabWidget->setTabText(index, newName);
    }
}

void dlgNotepad::createTabContent(const QString& tabId, const QString& tabName)
{
    auto* textEdit = new QPlainTextEdit(mTabWidget);
    textEdit->setObjectName(qsl("tab_textedit_%1").arg(tabId));

    if (mpHost) {
        textEdit->setFont(mpHost->getDisplayFont());
    }

    if (mpNotesManager) {
        const QString content = mpNotesManager->getTabContent(tabId);
        textEdit->setPlainText(content);
    }

    connect(textEdit, &QPlainTextEdit::textChanged, this, [this, tabId]() {
        mNeedToSave = true;
        updateTabContent(tabId);
    });

    const int index = mTabWidget->addTab(textEdit, tabName);
    mTabIdToIndex[tabId] = index;
    mIndexToTabId[index] = tabId;
}

void dlgNotepad::updateTabContent(const QString& tabId)
{
    if (!mpNotesManager || tabId.isEmpty()) {
        return;
    }

    int index = mTabIdToIndex.value(tabId, -1);
    if (index >= 0) {
        auto* textEdit = qobject_cast<QPlainTextEdit*>(mTabWidget->widget(index));
        if (textEdit) {
            mpNotesManager->setTabContent(tabId, textEdit->toPlainText());
        }
    }
}

void dlgNotepad::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

