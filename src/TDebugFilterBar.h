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
class TRefreshingComboBox;

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

public slots:
    // Public so the console's right-click menu can empty it through the bar,
    // which keeps the "N messages held" label in step:
    void slot_clear();

private slots:
    void slot_togglePause(const bool);
    void slot_textFilterChanged(const QString&);
    void slot_caseSensitivityChanged(const bool);
    void slot_updatePausedCount();

private:
    void addCategoryMenu();
    void addProfileMenu();
    void addItemFilter();
    void addTextFilter();
    void applyCategoryFromMenu();
    void refreshItemList();
    QStringList itemNames() const;
    void applyTypedItemFilter();
    static QString allItemsLabel();

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
