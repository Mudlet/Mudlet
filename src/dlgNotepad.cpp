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

#include <QCloseEvent>
#include <QDir>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QShortcut>
#include <QStringConverter>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>

using namespace std::chrono;

const QString jsonNotesFileName{qsl("notes.json")};
const QString local8BitEncodedNotesFileName{qsl("notes.txt")};
const QString utf8EncodedNotesFileName{qsl("notes_utf8.txt")};

dlgNotepad::dlgNotepad(Host* pH)
: mpHost(pH)
{
    setupUi(this);

    setupAddTabButton();

    tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabWidget->tabBar(), &QWidget::customContextMenuRequested, this, &dlgNotepad::slot_tabContextMenu);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &dlgNotepad::slot_tabCloseRequested);

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
        restoreSettings();
    }

    setupFindBar();
    connect(tabWidget, &QTabWidget::currentChanged, this, &dlgNotepad::slot_currentTabChanged);

    startTimer(2min);
}

void dlgNotepad::setupAddTabButton()
{
    mpAddTabButton = new QToolButton(this);
    mpAddTabButton->setText(qsl("+"));
    mpAddTabButton->setToolTip(tr("Add new note tab (Ctrl+T)"));
    connect(mpAddTabButton, &QToolButton::clicked, this, &dlgNotepad::slot_addTabClicked);
    tabWidget->setCornerWidget(mpAddTabButton, Qt::TopRightCorner);

    auto* newTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(newTabShortcut, &QShortcut::activated, this, &dlgNotepad::slot_addTabClicked);
}

void dlgNotepad::setupFindBar()
{
    mpFindBar = new QWidget(this);
    auto* layout = new QHBoxLayout(mpFindBar);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    mpFindLineEdit = new QLineEdit(mpFindBar);
    //: Placeholder text for the search field in notepad
    mpFindLineEdit->setPlaceholderText(tr("Find"));
    mpFindLineEdit->setClearButtonEnabled(true);
    mpFindLineEdit->installEventFilter(this);

    mpFindPrevButton = new QToolButton(mpFindBar);
    mpFindPrevButton->setIcon(QIcon(qsl(":/icons/export.png")));
    mpFindPrevButton->setToolTip(tr("Find previous"));
    mpFindPrevButton->setAutoRaise(true);
    mpFindPrevButton->setMaximumSize(24, 24);

    mpFindNextButton = new QToolButton(mpFindBar);
    mpFindNextButton->setIcon(QIcon(qsl(":/icons/import.png")));
    mpFindNextButton->setToolTip(tr("Find next"));
    mpFindNextButton->setAutoRaise(true);
    mpFindNextButton->setMaximumSize(24, 24);

    mpFindCloseButton = new QToolButton(mpFindBar);
    mpFindCloseButton->setIcon(QIcon(qsl(":/icons/dialog-close.png")));
    mpFindCloseButton->setToolTip(tr("Close find bar"));
    mpFindCloseButton->setAutoRaise(true);
    mpFindCloseButton->setMaximumSize(24, 24);

    layout->addWidget(mpFindLineEdit, 1);
    layout->addWidget(mpFindPrevButton);
    layout->addWidget(mpFindNextButton);
    layout->addWidget(mpFindCloseButton);

    verticalLayout->addWidget(mpFindBar);
    mpFindBar->hide();

    connect(mpFindLineEdit, &QLineEdit::textChanged, this, &dlgNotepad::slot_findTextChanged);
    connect(mpFindPrevButton, &QToolButton::clicked, this, &dlgNotepad::slot_findPrevious);
    connect(mpFindNextButton, &QToolButton::clicked, this, &dlgNotepad::slot_findNext);
    connect(mpFindCloseButton, &QToolButton::clicked, this, &dlgNotepad::slot_hideFindBar);

    mpFindShortcut = new QShortcut(QKeySequence::Find, this);
    connect(mpFindShortcut, &QShortcut::activated, this, &dlgNotepad::slot_showFindBar);
}

void dlgNotepad::slot_addTabClicked()
{
    addTab();
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void dlgNotepad::setFont(const QFont& font)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            textEdit->setFont(font);
        }
    }
}

void dlgNotepad::setTabsStyleSheet(const QString& styleSheet)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            textEdit->setStyleSheet(styleSheet);
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

int dlgNotepad::addTab(const QString& name, const QString& content)
{
    auto* textEdit = new QPlainTextEdit(this);
    if (mpHost) {
        textEdit->setFont(mpHost->getDisplayFont());
        textEdit->setStyleSheet(mpHost->mProfileStyleSheet);
    }
    if (!content.isEmpty()) {
        textEdit->setPlainText(content);
    }

    connect(textEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::slot_textChanged);

    //: Default name for a new note tab
    const QString tabName = name.isEmpty() ? tr("New Note") : name;
    const int index = tabWidget->addTab(textEdit, tabName);
    return index;
}

void dlgNotepad::closeTab(int index)
{
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }

    if (tabWidget->count() <= 1) {
        return;
    }

    save();

    QWidget* widget = tabWidget->widget(index);
    tabWidget->removeTab(index);
    delete widget;

    mNeedToSave = true;
}

void dlgNotepad::renameTab(int index)
{
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }

    const QString currentName = tabWidget->tabText(index);
    bool ok = false;
    //: Dialog title for renaming a note tab
    const QString newName = QInputDialog::getText(this, tr("Rename Note Tab"),
                                                   //: Label for the input field when renaming a note tab
                                                   tr("New name:"), QLineEdit::Normal,
                                                   currentName, &ok);
    if (ok && !newName.isEmpty() && newName != currentName) {
        tabWidget->setTabText(index, newName);
        mNeedToSave = true;
    }
}

void dlgNotepad::slot_tabCloseRequested(int index)
{
    closeTab(index);
}

void dlgNotepad::slot_tabContextMenu(const QPoint& pos)
{
    const int tabIndex = tabWidget->tabBar()->tabAt(pos);

    QMenu menu(this);

    //: Context menu action to create a new note tab
    QAction* newTabAction = menu.addAction(tr("New Tab"));
    connect(newTabAction, &QAction::triggered, this, [this]() {
        addTab();
        tabWidget->setCurrentIndex(tabWidget->count() - 1);
    });

    if (tabIndex >= 0) {
        menu.addSeparator();

        //: Context menu action to rename a note tab
        QAction* renameAction = menu.addAction(tr("Rename Tab"));
        connect(renameAction, &QAction::triggered, this, [this, tabIndex]() {
            renameTab(tabIndex);
        });

        if (tabWidget->count() > 1) {
            //: Context menu action to close a note tab
            QAction* closeAction = menu.addAction(tr("Close Tab"));
            connect(closeAction, &QAction::triggered, this, [this, tabIndex]() {
                closeTab(tabIndex);
            });

            menu.addSeparator();

            //: Context menu action to close all note tabs except the clicked one
            QAction* closeOthersAction = menu.addAction(tr("Close Other Tabs"));
            connect(closeOthersAction, &QAction::triggered, this, [this, tabIndex]() {
                save();
                for (int i = tabWidget->count() - 1; i >= 0; --i) {
                    if (i != tabIndex) {
                        QWidget* widget = tabWidget->widget(i);
                        tabWidget->removeTab(i);
                        delete widget;
                    }
                }
                mNeedToSave = true;
            });
        }
    }

    menu.exec(tabWidget->tabBar()->mapToGlobal(pos));
}

QPlainTextEdit* dlgNotepad::currentTextEdit() const
{
    return qobject_cast<QPlainTextEdit*>(tabWidget->currentWidget());
}

void dlgNotepad::save()
{
    const QString directoryPath = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName());
    const QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonNotesFileName);

    const QDir dir;
    if (!dir.exists(directoryPath)) {
        dir.mkpath(directoryPath);
    }

    QJsonObject root;
    root.insert(qsl("version"), 1);

    QJsonArray tabsArray;
    for (int i = 0; i < tabWidget->count(); ++i) {
        QJsonObject tabObj;
        tabObj.insert(qsl("name"), tabWidget->tabText(i));

        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            tabObj.insert(qsl("content"), textEdit->toPlainText());
        }

        tabsArray.append(tabObj);
    }

    root.insert(qsl("tabs"), tabsArray);
    root.insert(qsl("activeTab"), tabWidget->currentIndex());

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "dlgNotepad::save: failed to open file for writing:" << file.errorString();
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qDebug() << "dlgNotepad::save: error saving notepad contents:" << file.errorString();
    }

    mNeedToSave = false;
}

bool dlgNotepad::migrateOldNotesFile()
{
    QString oldFileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    bool useUtf8 = true;

    if (!QFile::exists(oldFileName)) {
        oldFileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), local8BitEncodedNotesFileName);
        useUtf8 = false;

        if (!QFile::exists(oldFileName)) {
            return false;
        }
    }

    QFile file(oldFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "dlgNotepad::migrateOldNotesFile: failed to open file for reading:" << file.errorString();
        return false;
    }

    QTextStream fileStream(&file);
    if (!useUtf8) {
        fileStream.setEncoding(QStringEncoder::Encoding::System);
    }

    const QString content = fileStream.readAll();
    file.close();

    //: Name for the migrated notes tab when upgrading from single-note to tabbed notepad
    addTab(tr("Notes"), content);

    return true;
}

void dlgNotepad::restore()
{
    const QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonNotesFileName);

    if (QFile::exists(fileName)) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "dlgNotepad::restore: failed to open file for reading:" << file.errorString();
            //: Default name for the first note tab
            addTab(tr("Notes"));
            return;
        }

        const QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "dlgNotepad::restore: JSON parse error:" << parseError.errorString();
            addTab(tr("Notes"));
            return;
        }

        if (!doc.isObject()) {
            qDebug() << "dlgNotepad::restore: JSON root is not an object";
            addTab(tr("Notes"));
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonArray tabsArray = root.value(qsl("tabs")).toArray();

        if (tabsArray.isEmpty()) {
            addTab(tr("Notes"));
        } else {
            for (const QJsonValue& tabValue : tabsArray) {
                const QJsonObject tabObj = tabValue.toObject();
                const QString name = tabObj.value(qsl("name")).toString();
                const QString content = tabObj.value(qsl("content")).toString();
                addTab(name, content);
            }

            const int activeTab = root.value(qsl("activeTab")).toInt(0);
            if (activeTab >= 0 && activeTab < tabWidget->count()) {
                tabWidget->setCurrentIndex(activeTab);
            }
        }
    } else if (!migrateOldNotesFile()) {
        addTab(tr("Notes"));
    }
}

void dlgNotepad::slot_textChanged()
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
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    const QString allText = textEdit->toPlainText();
    const QStringList lines = allText.split('\n');
    startSendingLines(lines);
}

void dlgNotepad::slot_sendLine()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    const QString line = cursor.selectedText();

    if (!line.isEmpty()) {
        startSendingLines(QStringList{line});
    }
}

void dlgNotepad::slot_sendSelection()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QString selectedText = textEdit->textCursor().selectedText();

    if (!selectedText.isEmpty()) {
        const QStringList lines = selectedText.replace(QChar(0x2029), qsl("\n")).split('\n');
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

    const QString line = mLinesToSend[mCurrentLineIndex++];
    if (!line.isEmpty()) {
        const QString prepend = lineEdit_prependText->text().isEmpty() ? QString() : lineEdit_prependText->text();
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

    mpHost->writeProfileIniData(qsl("Notepad/SendControlsVisible"),
                                 action_toggleSendControls->isChecked() ? qsl("true") : qsl("false"));
    mpHost->writeProfileIniData(qsl("Notepad/WindowState"),
                                 QString::fromLatin1(saveState().toBase64()));
}

void dlgNotepad::restoreSettings()
{
    if (!mpHost) {
        return;
    }

    const QString sendControlsVisibleStr = mpHost->readProfileIniData(qsl("Notepad/SendControlsVisible"));
    const bool sendControlsVisible = (sendControlsVisibleStr.compare(qsl("true"), Qt::CaseInsensitive) == 0);

    const bool wasBlocked = action_toggleSendControls->signalsBlocked();
    action_toggleSendControls->blockSignals(true);
    action_toggleSendControls->setChecked(sendControlsVisible);
    action_toggleSendControls->blockSignals(wasBlocked);

    slot_toggleSendControls(sendControlsVisible);

    const QString windowStateStr = mpHost->readProfileIniData(qsl("Notepad/WindowState"));
    if (!windowStateStr.isEmpty()) {
        restoreState(QByteArray::fromBase64(windowStateStr.toLatin1()));
    }
}

void dlgNotepad::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

bool dlgNotepad::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == mpFindLineEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                slot_findPrevious();
            } else {
                slot_findNext();
            }
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            slot_hideFindBar();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void dlgNotepad::slot_showFindBar()
{
    mpFindBar->show();
    mpFindLineEdit->setFocus();
    mpFindLineEdit->selectAll();
    highlightAllMatches();
}

void dlgNotepad::slot_hideFindBar()
{
    mpFindBar->hide();
    clearSearchHighlights();
    if (auto* textEdit = currentTextEdit()) {
        textEdit->setFocus();
    }
}

void dlgNotepad::slot_findNext()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit || mpFindLineEdit->text().isEmpty()) {
        return;
    }

    if (!textEdit->find(mpFindLineEdit->text())) {
        textEdit->moveCursor(QTextCursor::Start);
        textEdit->find(mpFindLineEdit->text());
    }
    highlightAllMatches();
}

void dlgNotepad::slot_findPrevious()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit || mpFindLineEdit->text().isEmpty()) {
        return;
    }

    if (!textEdit->find(mpFindLineEdit->text(), QTextDocument::FindBackward)) {
        textEdit->moveCursor(QTextCursor::End);
        textEdit->find(mpFindLineEdit->text(), QTextDocument::FindBackward);
    }
    highlightAllMatches();
}

void dlgNotepad::slot_findTextChanged(const QString& text)
{
    Q_UNUSED(text)
    highlightAllMatches();

    auto* textEdit = currentTextEdit();
    if (textEdit && !mpFindLineEdit->text().isEmpty()) {
        QTextCursor cursor = textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        textEdit->setTextCursor(cursor);
        textEdit->find(mpFindLineEdit->text());
    }
}

void dlgNotepad::slot_currentTabChanged(int index)
{
    Q_UNUSED(index)
    if (mpFindBar->isVisible()) {
        highlightAllMatches();
    }
}

void dlgNotepad::highlightAllMatches()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;
    const QString searchText = mpFindLineEdit->text();

    if (searchText.isEmpty()) {
        textEdit->setExtraSelections(extraSelections);
        return;
    }

    QTextDocument* doc = textEdit->document();
    QTextCursor cursor(doc);
    QTextCursor currentCursor = textEdit->textCursor();

    QColor highlightColor(Qt::yellow);
    highlightColor.setAlpha(100);
    QColor currentMatchColor(255, 165, 0);
    currentMatchColor.setAlpha(150);

    while (!cursor.isNull() && !cursor.atEnd()) {
        cursor = doc->find(searchText, cursor);
        if (!cursor.isNull()) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;

            if (cursor.selectionStart() == currentCursor.selectionStart() &&
                cursor.selectionEnd() == currentCursor.selectionEnd()) {
                selection.format.setBackground(currentMatchColor);
            } else {
                selection.format.setBackground(highlightColor);
            }

            extraSelections.append(selection);
        }
    }

    textEdit->setExtraSelections(extraSelections);
}

void dlgNotepad::clearSearchHighlights()
{
    auto* textEdit = currentTextEdit();
    if (textEdit) {
        textEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    }
}
