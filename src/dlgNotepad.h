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
#include <QPointer>

class Host;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QShortcut;
class QTimer;
class QTimerEvent;
class QToolButton;


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
    void setTabsStyleSheet(const QString& styleSheet);

signals:
    void notepadClosing(const QString& profileName);

public slots:
    int addTab(const QString& name = QString(), const QString& content = QString());
    void closeTab(int index);
    void renameTab(int index);

private slots:
    void slot_tabCloseRequested(int index);
    void slot_tabContextMenu(const QPoint& pos);
    void slot_addTabClicked();
    void slot_textChanged();
    void slot_sendAll();
    void slot_sendLine();
    void slot_sendSelection();
    void slot_sendNextLine();
    void slot_stopSending();
    void slot_toggleSendControls(bool checked);
    void slot_showFindBar();
    void slot_hideFindBar();
    void slot_findNext();
    void slot_findPrevious();
    void slot_findTextChanged(const QString& text);
    void slot_currentTabChanged(int index);

private:
    void timerEvent(QTimerEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    QPlainTextEdit* currentTextEdit() const;
    void setupAddTabButton();
    void setupFindBar();
    void highlightAllMatches();
    void clearSearchHighlights();
    bool migrateOldNotesFile();
    void startSendingLines(const QStringList& lines);

    QPointer<Host> mpHost;
    QToolButton* mpAddTabButton = nullptr;
    bool mNeedToSave = false;
    QAction* action_stop = nullptr;
    QAction* action_prependText = nullptr;
    QAction* action_prependTextLabel = nullptr;
    QLabel* label_prependText = nullptr;
    QLineEdit* lineEdit_prependText = nullptr;
    QStringList mLinesToSend;
    QTimer* mSendTimer = nullptr;
    int mCurrentLineIndex = 0;

    QWidget* mpFindBar = nullptr;
    QLineEdit* mpFindLineEdit = nullptr;
    QToolButton* mpFindPrevButton = nullptr;
    QToolButton* mpFindNextButton = nullptr;
    QToolButton* mpFindCloseButton = nullptr;
    QShortcut* mpFindShortcut = nullptr;
};

#endif // MUDLET_DLGNOTEPAD_H
