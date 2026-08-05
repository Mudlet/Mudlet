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

#include <QMap>
#include <QToolBar>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QTimer;
class QToolButton;

// The controls along the top of the Central Debug Console. Everything it
// changes is static filter state on TDebug, which is applied to messages as
// they arrive - so switching a filter never disturbs what is already on screen.
class TDebugFilterBar : public QToolBar
{
    Q_OBJECT

public:
    explicit TDebugFilterBar(QWidget* parent = nullptr);

    // Called when a profile is loaded or closed, so the profile menu keeps up:
    void refreshProfiles();

private slots:
    void slot_togglePause(const bool);
    void slot_clear();
    void slot_textFilterChanged(const QString&);
    void slot_caseSensitivityChanged(const bool);
    void slot_updatePausedCount();

private:
    void addCategoryMenu();
    void addProfileMenu();
    void addTextFilter();
    void applyCategoryFromMenu();

    QAction* mpActionPause = nullptr;
    QMenu* mpCategoryMenu = nullptr;
    QToolButton* mpProfileButton = nullptr;
    // Hiding the button alone is not enough - a toolbar shows its widgets
    // through the action that wraps them:
    QAction* mpActionProfiles = nullptr;
    QMenu* mpProfileMenu = nullptr;
    QLineEdit* mpTextFilter = nullptr;
    QLabel* mpPausedLabel = nullptr;
    QAction* mpActionPausedLabel = nullptr;
    QTimer* mpPausedLabelTimer = nullptr;
    QMap<TDebug::Category, QAction*> mCategoryActions;
};

#endif // MUDLET_TDEBUGFILTERBAR_H
