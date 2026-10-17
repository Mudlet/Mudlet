#ifndef MUDLET_MAPSEARCHPANEL_H
#define MUDLET_MAPSEARCHPANEL_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "MapSearch.h"

#include <QColor>
#include <QFrame>
#include <QList>
#include <QRectF>
#include <QString>

class QAction;
class QLabel;
class QLineEdit;
class QListWidget;
class QToolButton;

// A search box over the modern 3D map, with the places found listed under it; what is searched, and what a
// chosen place does, is left to the view
class MapSearchPanel : public QFrame
{
    Q_OBJECT
    friend class ModernMapperSceneTest;

public:
    // What each place listed carries for the list's delegate, beyond its title as the display text
    enum ItemRole { DetailRole = Qt::UserRole + 1, KindRole, ChipColorRole, MatchStartRole, MatchLengthRole, RoutableRole };

    struct Row
    {
        MapSearch::Kind kind = MapSearch::Kind::Room;
        QString title;
        QString detail;
        // Read out by a screen reader in place of the two lines drawn
        QString spoken;
        // A room's environment color; none for the others
        QColor chip;
        // Whether Ctrl+Enter can plan a route to it
        bool routable = false;
    };

    // The room around the card left for its shadow, which still belongs to the panel
    static constexpr int scmShadowMargin = 12;

    explicit MapSearchPanel(QWidget* parent);

    // Shows it with what was typed last selected, ready to be typed over
    void open();
    QString text() const;
    bool labelsIncluded() const;
    // total is how many places matched in all, of which rows are the best
    void setResults(const QList<Row>& rows, qsizetype total);
    int currentRow() const;
    int preferredWidth() const;

signals:
    void queryChanged();
    void resultChosen(int row, bool planRoute);
    void contextMenuRequested(int row, const QPoint& globalPosition);
    void closed();

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void dismiss();
    void escape();
    void step(int rows);
    void restyle();
    void showState(qsizetype shown, qsizetype total);
    void showKeys();
    QRectF card() const;

    QLineEdit* mpEdit = nullptr;
    QAction* mpSearchIcon = nullptr;
    QToolButton* mpLabels = nullptr;
    QToolButton* mpClose = nullptr;
    QListWidget* mpList = nullptr;
    QLabel* mpMessage = nullptr;
    QLabel* mpHint = nullptr;
    QLabel* mpCount = nullptr;
    QLabel* mpKeys = nullptr;
    bool mRestyling = false;
    // A press on the shadow around the card, whose drag and release go on to the map with it
    bool mPassingThrough = false;
};

#endif // MUDLET_MAPSEARCHPANEL_H
