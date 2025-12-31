#ifndef MUDLET_DLGNOTEPAD_H
#define MUDLET_DLGNOTEPAD_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2022, 2025 by Stephen Lyons                       *
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


#include "ui_notes_editor.h"
#include <QCheckBox>
#include <QCloseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QSettings>
#include <QTimer>
#include <QTabWidget>
#include <QPlainTextEdit>

class Host;
class NotesManager;


class dlgNotepad : public QMainWindow, public Ui::notes_editor
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgNotepad)
    explicit dlgNotepad(Host*);
    ~dlgNotepad();

    void save();
    void restore();
    void saveSettings();
    void restoreSettings();
    void setFont(const QFont &);

signals:
    void notepadClosing(const QString& profileName);

private slots:
    void slot_textWritten();
    void slot_sendAll();
    void slot_sendLine();
    void slot_sendSelection();
    void slot_sendNextLine();
    void slot_stopSending();
    void slot_toggleSendControls(bool checked);
    void slot_addTab();
    void slot_removeTab();
    void slot_renameTab();
    void slot_currentTabChanged(int index);
    void slot_managerTabAdded(const QString& tabId, const QString& tabName);
    void slot_managerTabRemoved(const QString& tabId);
    void slot_managerTabRenamed(const QString& tabId, const QString& newName);

private:
    void timerEvent(QTimerEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void startSendingLines(const QStringList& lines);
    QPlainTextEdit* getCurrentTextEdit() const;
    QString getCurrentTabId() const;
    void setupTabContextMenu();
    void createTabContent(const QString& tabId, const QString& tabName);
    void updateTabContent(const QString& tabId);

    QPointer<Host> mpHost;
    QPointer<NotesManager> mpNotesManager;
    QTabWidget* mTabWidget = nullptr;
    QMap<QString, int> mTabIdToIndex;
    QMap<int, QString> mIndexToTabId;

    bool mNeedToSave = false;
    QAction* action_stop = nullptr;
    QAction* action_prependText = nullptr;
    QAction* action_prependTextLabel = nullptr;
    QLabel* label_prependText = nullptr;
    QLineEdit* lineEdit_prependText = nullptr;
    QStringList mLinesToSend;
    QTimer* mSendTimer = nullptr;
    int mCurrentLineIndex = 0;

};

#endif // MUDLET_DLGNOTEPAD_H
