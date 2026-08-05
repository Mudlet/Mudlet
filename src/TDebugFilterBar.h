#ifndef MUDLET_TDEBUGFILTERBAR_H
#define MUDLET_TDEBUGFILTERBAR_H

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

#include "TDebug.h"

#include <QComboBox>
#include <QMap>
#include <QToolBar>

#include <functional>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QTimer;
class QToolButton;

// A combo box that rebuilds its list as it is opened, so it always offers the
// triggers, aliases and the rest that the profile has right now rather than
// whatever it had when the console was first shown.
class TRefreshingComboBox : public QComboBox
{
public:
    explicit TRefreshingComboBox(QWidget* parent = nullptr)
    : QComboBox(parent)
    {
    }
    void showPopup() override
    {
        if (mRefresh) {
            mRefresh();
        }
        QComboBox::showPopup();
    }

    std::function<void()> mRefresh;
};

// The controls along the bottom of the Central Debug Console. Everything it
// changes is static filter state on TDebug, which is applied to messages as
// they arrive - so switching a filter never disturbs what is already on screen.
class TDebugFilterBar : public QToolBar
{
    Q_OBJECT

public:
    explicit TDebugFilterBar(QWidget* parent = nullptr);

    // Called when a profile is loaded or closed, so the profile menu keeps up:
    void refreshProfiles();
    // Called when something outside the bar changes the text filter, such as
    // the console's own right-click menu:
    void refreshTextFilter();

private slots:
    void slot_togglePause(const bool);
    void slot_clear();
    void slot_textFilterChanged(const QString&);
    void slot_caseSensitivityChanged(const bool);
    void slot_updatePausedCount();

private:
    void addCategoryMenu();
    void addProfileMenu();
    void addItemFilter();
    void addTextFilter();
    void applyCategoryFromMenu();
    void refreshCategoryLabel();
    void refreshItemList();
    static QString csmAllItems();

    QAction* mpActionPause = nullptr;
    QMenu* mpCategoryMenu = nullptr;
    QToolButton* mpProfileButton = nullptr;
    // Hiding the button alone is not enough - a toolbar shows its widgets
    // through the action that wraps them:
    QAction* mpActionProfiles = nullptr;
    QMenu* mpProfileMenu = nullptr;
    QLineEdit* mpTextFilter = nullptr;
    TRefreshingComboBox* mpItemFilter = nullptr;
    QToolButton* mpCategoryButton = nullptr;
    QLabel* mpPausedLabel = nullptr;
    QAction* mpActionPausedLabel = nullptr;
    QTimer* mpPausedLabelTimer = nullptr;
    QMap<TDebug::Category, QAction*> mCategoryActions;
};

#endif // MUDLET_TDEBUGFILTERBAR_H
