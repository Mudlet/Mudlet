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

#include <QFrame>
#include <QStringList>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;

// A search box over the modern 3D map, with the places found listed under it; what is searched, and what a
// chosen place does, is left to the view
class MapSearchPanel : public QFrame
{
    Q_OBJECT
    friend class ModernMapperSceneTest;

public:
    explicit MapSearchPanel(QWidget* parent);

    // Shows it with what was typed last selected, ready to be typed over
    void open();
    QString text() const;
    bool labelsIncluded() const;
    void setResults(const QStringList& lines, const QString& summary);
    int currentRow() const;

signals:
    void queryChanged();
    void resultChosen(int row, bool planRoute);
    void contextMenuRequested(int row, const QPoint& globalPosition);
    void closed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void dismiss();

    QLineEdit* mpEdit = nullptr;
    QCheckBox* mpLabels = nullptr;
    QListWidget* mpList = nullptr;
    QLabel* mpSummary = nullptr;
};

#endif // MUDLET_MAPSEARCHPANEL_H
