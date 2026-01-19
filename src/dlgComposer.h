#ifndef MUDLET_DLGCOMPOSER_H
#define MUDLET_DLGCOMPOSER_H

/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "ui_composer.h"
#include <QPointer>
#include <QPoint>

class Host;
class QMenu;
class QMouseEvent;
class QTextCursor;


class dlgComposer : public QMainWindow, public Ui::composer
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgComposer)
    explicit dlgComposer(Host*);

    void init(const QString &title, const QString &newText);

public slots:
    void slot_save();
    void slot_cancel();

private slots:
    void slot_spellCheck();
    void slot_contextMenu(const QPoint& pos);
    void slot_addWord();
    void slot_removeWord();
    void slot_popupMenu();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void spellCheckWord(QTextCursor& cursor);
    void recheckWholeLine();
    void fillSpellCheckList(QMouseEvent* event, QMenu* popup);

    QPointer<Host> mpHost;
    QString mSpellCheckedWord;
    bool mSpellChecking = false;
    int mSystemDictionarySuggestionsCount = 0;
    int mUserDictionarySuggestionsCount = 0;
    char** mpSystemSuggestionsList = nullptr;
    char** mpUserSuggestionsList = nullptr;
    QPoint mPopupPosition;
    int mLastWordStart = -1;
    int mLastWordEnd = -1;
};

#endif // MUDLET_DLGCOMPOSER_H
